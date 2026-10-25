import importlib.util
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

TOOLS = Path(__file__).parents[1] / 'tools'
spec = importlib.util.spec_from_file_location('azahar_oracle', TOOLS / 'azahar_oracle.py')
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)


def run_tool(scenario_text, *options):
    directory = Path(tempfile.mkdtemp())
    scenario = directory / 'scenario.txt'
    scenario.write_text(scenario_text)
    states = directory / 'states'
    states.mkdir()
    result = subprocess.run([sys.executable, str(TOOLS / 'azahar_oracle.py'), str(scenario), '--states', str(states),
                             '--output', str(directory / 'out'), *options],
                            capture_output=True, text=True, timeout=60)
    return result, states


class ParseStateParamsTest(unittest.TestCase):
    def test_roles_map_to_names(self):
        self.assertEqual(oracle.parse_state_params(['PATROL_CAR=my-state', 'A1=x.y_z']),
                         {'PATROL_CAR': 'my-state', 'A1': 'x.y_z'})
        self.assertEqual(oracle.parse_state_params(None), {})

    def test_bad_items_are_rejected_with_the_text(self):
        for item in ('PATROL_CAR', 'patrol=a', '=a', 'A=', 'A=../b', 'A=b/c', 'A=-b', '1A=b', 'A B=c', 'A=b c'):
            with self.assertRaises(ValueError, msg=item) as caught:
                oracle.parse_state_params([item])
            self.assertIn("'%s'" % item, str(caught.exception))

    def test_a_role_given_twice_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "role given twice: 'A=c'"):
            oracle.parse_state_params(['A=b', 'A=c'])


class ResolveStateTest(unittest.TestCase):
    def test_plain_names_stay_and_roles_resolve(self):
        self.assertEqual(oracle.resolve_state('gameplay-start', {}), 'gameplay-start')
        self.assertEqual(oracle.resolve_state('@PATROL_CAR', {'PATROL_CAR': 'mine'}), 'mine')

    def test_unknown_and_malformed_roles_are_rejected_with_the_text(self):
        with self.assertRaisesRegex(ValueError, "no --state-param PATROL_CAR=NAME was given for the role: '@PATROL_CAR'"):
            oracle.resolve_state('@PATROL_CAR', {'OTHER': 'x'})
        for text in ('@', '@patrol', '@1A', '@A-B', '@A@B'):
            with self.assertRaisesRegex(ValueError, "malformed state role.*'%s'" % re.escape(text)):
                oracle.resolve_state(text, {'A': 'x'})


class ScenarioStateErrorTest(unittest.TestCase):
    def test_valid_steps_pass(self):
        lines = ['loadstate gameplay-start', 'loadstate @A  # comment', '', '# loadstate @B', 'savestate made-here']
        self.assertIsNone(oracle.scenario_state_error(lines, {'A': 'x'}))

    def test_a_missing_role_names_the_line_and_the_text(self):
        self.assertEqual(oracle.scenario_state_error(['frames 1', 'loadstate @A'], {}),
                         "scenario line 2: loadstate: no --state-param A=NAME was given for the role: '@A'")

    def test_malformed_steps_are_rejected(self):
        self.assertIn("expected one state name, got 0 word(s): 'loadstate'", oracle.scenario_state_error(['loadstate'], {}))
        self.assertIn("got 2 word(s): 'loadstate a b'", oracle.scenario_state_error(['loadstate a b'], {}))
        self.assertIn("got 2 word(s): 'savestate a b'", oracle.scenario_state_error(['savestate a b'], {}))
        self.assertIn("malformed state role", oracle.scenario_state_error(['loadstate @a'], {'a': 'x'}))
        self.assertIn("not a plain state name (a role is only for loadstate): '@A'",
                      oracle.scenario_state_error(['savestate @A'], {'A': 'x'}))

    def test_param_states_skip_names_the_scenario_saves(self):
        lines = ['loadstate @A', 'savestate made', 'loadstate @B', 'loadstate @A', 'loadstate plain']
        self.assertEqual(oracle.scenario_param_states(lines, {'A': 'a1', 'B': 'made'}), ['a1'])

    def test_missing_state_files_are_reported(self):
        states = Path(tempfile.mkdtemp())
        (states / 'there.json').write_text('{}')
        self.assertIn("(missing there.cst): 'there'", oracle.state_files_error(states, ['there']))
        (states / 'there.cst').write_bytes(b'')
        self.assertIsNone(oracle.state_files_error(states, ['there']))
        self.assertIn("(missing gone.json): 'gone'", oracle.state_files_error(states, ['there', 'gone']))


class RunScenarioTest(unittest.TestCase):
    def test_loadstate_loads_the_state_the_role_names(self):
        fake = mock.Mock()
        oracle.run_scenario(fake, ['loadstate @A', 'loadstate plain'], state_params={'A': 'mine'})
        self.assertEqual([call.args for call in fake.loadstate.call_args_list], [('mine',), ('plain',)])


class CommandLineTest(unittest.TestCase):
    def assert_rejected(self, result, *fragments):
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        for fragment in fragments:
            self.assertIn(fragment, result.stderr)

    def test_a_script_role_without_a_parameter_stops_before_the_emulator(self):
        result, _ = run_tool('frames 1\nloadstate @PATROL_CAR\n')
        self.assert_rejected(result, "scenario line 2: loadstate: no --state-param PATROL_CAR=NAME was given for the "
                                     "role: '@PATROL_CAR'")

    def test_a_malformed_parameter_names_the_option_and_the_text(self):
        for item in ('PATROL_CAR', 'patrol=x', 'A=../x'):
            result, _ = run_tool('frames 1\n', '--state-param', item)
            self.assert_rejected(result, '--state-param: ', "'%s'" % item)

    def test_a_duplicate_parameter_is_rejected(self):
        result, _ = run_tool('frames 1\n', '--state-param', 'A=x', '--state-param', 'A=y')
        self.assert_rejected(result, "--state-param: role given twice: 'A=y'")

    def test_the_state_option_takes_roles(self):
        result, _ = run_tool('frames 1\n', '--state', '@PATROL_CAR')
        self.assert_rejected(result, "--state: no --state-param PATROL_CAR=NAME was given for the role: '@PATROL_CAR'")
        result, _ = run_tool('frames 1\n', '--state', '@bad')
        self.assert_rejected(result, "--state: malformed state role", "'@bad'")

    def test_a_malformed_state_step_is_rejected(self):
        result, _ = run_tool('loadstate a b\n')
        self.assert_rejected(result, "scenario line 1: loadstate: expected one state name, got 2 word(s): 'loadstate a b'")
        result, _ = run_tool('savestate @A\n', '--state-param', 'A=x')
        self.assert_rejected(result, "not a plain state name (a role is only for loadstate): '@A'")

    def test_a_missing_state_file_stops_before_the_emulator(self):
        result, states = run_tool('loadstate @A\n', '--state-param', 'A=absent')
        self.assert_rejected(result, '--state-param: no saved state in ', '(missing absent.json): \'absent\'')
        result, _ = run_tool('frames 1\n', '--state', '@A', '--state-param', 'A=absent2')
        self.assert_rejected(result, "(missing absent2.json): 'absent2'")


class CommittedScriptsTest(unittest.TestCase):
    scripts = sorted((TOOLS / 'oracle').glob('*.txt'))

    def roles(self):
        found = set()
        for path in self.scripts:
            found.update(re.findall(r'@([A-Z][A-Z0-9_]*)', path.read_text()))
        return found

    def test_the_scripts_exist_and_use_roles(self):
        self.assertGreater(len(self.scripts), 30)
        self.assertGreaterEqual(len(self.roles()), 10)

    def test_every_script_is_valid_with_all_roles_given(self):
        params = {role: 'x-' + role.lower() for role in self.roles()}
        for path in self.scripts:
            self.assertIsNone(oracle.scenario_state_error(path.read_text().splitlines(), params), path.name)

    def test_a_script_with_a_role_needs_its_parameter(self):
        for path in self.scripts:
            lines = path.read_text().splitlines()
            wanted = {w[1][1:] for w in (l.split('#', 1)[0].split() for l in lines)
                      if len(w) == 2 and w[0] == 'loadstate' and w[1].startswith('@')}
            if not wanted:
                continue
            self.assertIn('no --state-param ', oracle.scenario_state_error(lines, {}), path.name)


if __name__ == '__main__':
    unittest.main()
