#!/usr/bin/env python3
"""Run the original game in Azahar as a runtime oracle and observe it through the GDB stub.

The emulator runs headless (Qt offscreen) with the software renderer by default (see --renderer), in an isolated
home directory under artifacts/oracle/ so the user's own Azahar settings and saves are
never touched. The image is opened read-only by Azahar.

A scenario file drives the run, one command per line ('#' starts a comment):

  frames N              advance N calls of the scheduler 0x100f44 (one game update each)
  wait SECONDS          let the emulator run freely for a wall-clock time (no frame count)
  hold BUTTONS          hold buttons from now on, e.g. 'hold A' or 'hold A+UP'; 'hold -' releases all
  stick X Y             raw circle-pad values (about -156..156); 'stick 0 0' centres it
  screenshot NAME       write NAME_top.png and NAME_bottom.png to the output directory
  report                print scheduler and frame-pacing values
  channels              print the pad device's 20-byte channel records (value, type, deadzone, threshold, edges)
  player                print the player entity (*0x3772b8): world position, facing, intent heading, flags
  trace N NAME          advance N frames, writing the player state of every frame to NAME.csv
  read ADDR LEN         print LEN bytes at ADDR as hex words

Input is injected inside the game's pad reader 0x10a4dc: the held mask in r0 at 0x10a57c
and the raw circle-pad words at r4+12/+14 before 0x10a540. It is therefore exact per game
frame and independent of host input.
"""
import argparse
import os
import shutil
import signal
import socket
import struct
import subprocess
import sys
import time
import zlib
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

SCHEDULER = 0x100f44
PAD_HELD = 0x10a57c
PAD_STICK = 0x10a540
GSP_SHARED = 0x10000000
PLAYER = 0x3772b8
PAD_DEVICE = 0x3769e4
CAMERA_YAW = 0x376894

BUTTONS = {'A': 0x1, 'B': 0x2, 'SELECT': 0x4, 'START': 0x8, 'RIGHT': 0x10, 'LEFT': 0x20,
           'UP': 0x40, 'DOWN': 0x80, 'R': 0x100, 'L': 0x200, 'X': 0x400, 'Y': 0x800}

CONFIG = """[Audio]
volume=0
volume\\default=false
[Debugging]
use_gdbstub=true
use_gdbstub\\default=false
[Renderer]
graphics_api={renderer}
graphics_api\\default=false
frame_limit=100
frame_limit\\default=false
[System]
is_new_3ds=false
is_new_3ds\\default=false
init_clock=1
init_clock\\default=false
[UI]
confirmClose=false
confirmClose\\default=false
"""


class GdbRemote:
    def __init__(self, port, timeout=60.0):
        deadline = time.monotonic() + timeout
        while True:
            try:
                self.sock = socket.create_connection(('127.0.0.1', port), timeout=5)
                break
            except OSError:
                if time.monotonic() > deadline:
                    raise
                time.sleep(0.2)
        self.sock.settimeout(None)
        self.buffer = b''
        self.breakpoints = set()
        self.command('?')

    def _recv(self):
        chunk = self.sock.recv(65536)
        if not chunk:
            raise ConnectionError('gdb stub closed the connection')
        self.buffer += chunk

    def _packet(self):
        while True:
            start = self.buffer.find(b'$')
            if start >= 0:
                end = self.buffer.find(b'#', start)
                if end >= 0 and len(self.buffer) >= end + 3:
                    body = self.buffer[start + 1:end]
                    self.buffer = self.buffer[end + 3:]
                    self.sock.sendall(b'+')
                    return body.decode('latin-1')
            self._recv()

    def send(self, body):
        data = body.encode('latin-1')
        checksum = sum(data) & 0xff
        self.sock.sendall(b'$' + data + b'#%02x' % checksum)
        while not self.buffer:
            self._recv()
        ack, self.buffer = self.buffer[:1], self.buffer[1:]
        if ack != b'+':
            raise ConnectionError('gdb stub rejected packet %r' % body[:40])

    def command(self, body):
        self.send(body)
        return self._packet()

    def read(self, address, length):
        out = bytearray()
        while length:
            size = min(length, 0x800)
            reply = self.command('m%x,%x' % (address, size))
            if reply.startswith('E') or len(reply) != size * 2:
                raise IOError('memory read failed at 0x%x: %s' % (address, reply[:16]))
            out += bytes.fromhex(reply)
            address += size
            length -= size
        return bytes(out)

    def write(self, address, data):
        reply = self.command('M%x,%x:%s' % (address, len(data), data.hex()))
        if reply != 'OK':
            raise IOError('memory write failed at 0x%x: %s' % (address, reply))

    def u32(self, address):
        return struct.unpack('<I', self.read(address, 4))[0]

    def register(self, index):
        return struct.unpack('<I', bytes.fromhex(self.command('p%x' % index)[:8]))[0]

    def set_register(self, index, value):
        reply = self.command('P%x=%s' % (index, struct.pack('<I', value & 0xffffffff).hex()))
        if reply != 'OK':
            raise IOError('register write failed: %s' % reply)

    def add_breakpoint(self, address):
        if self.command('Z0,%x,4' % address) != 'OK':
            raise IOError('cannot set breakpoint at 0x%x' % address)
        self.breakpoints.add(address)

    def run_to_breakpoint(self):
        pc = self.register(15)
        if pc in self.breakpoints:
            self.command('z0,%x,4' % pc)
            self.command('s')
            self.command('Z0,%x,4' % pc)
        reply = self.command('c')
        if not reply.startswith(('T', 'S')):
            raise ConnectionError('unexpected stop reply %r' % reply)
        return self.register(15)


class Oracle:
    def __init__(self, remote, output):
        self.remote = remote
        self.output = output
        self.held = 0
        self.stick = (0, 0)
        self.frame = 0
        self.frame_object = None
        remote.add_breakpoint(SCHEDULER)

    def inject_input(self):
        if PAD_HELD not in self.remote.breakpoints:
            self.remote.add_breakpoint(PAD_STICK)
            self.remote.add_breakpoint(PAD_HELD)

    def advance(self, count, on_frame=None):
        target = self.frame + count
        while self.frame < target:
            pc = self.remote.run_to_breakpoint()
            if pc == SCHEDULER:
                self.frame += 1
                self.frame_object = self.remote.register(0)
                if on_frame:
                    on_frame()
            elif pc == PAD_HELD:
                self.remote.set_register(0, self.held)
            elif pc == PAD_STICK:
                base = self.remote.register(4)
                self.remote.write(base + 12, struct.pack('<hh', *self.stick))

    def wait(self, seconds):
        r = self.remote
        installed = sorted(r.breakpoints)
        for address in installed:
            r.command('z0,%x,4' % address)
        r.send('c')
        time.sleep(seconds)
        r.sock.sendall(b'\x03')
        reply = r._packet()
        if not reply.startswith(('T', 'S')):
            raise ConnectionError('unexpected stop reply %r' % reply)
        for address in installed:
            r.command('Z0,%x,4' % address)
        self.frame_object = None

    def report(self):
        r = self.remote
        print('frame %d' % self.frame)
        if self.frame_object is not None:
            p = self.frame_object
            c = r.u32(p + 0xc)
            dt_min, dt_max, dt_fixed, scale = struct.unpack('<4f', r.read(c + 0x10, 16))
            flags = r.read(c + 0x20, 3)
            cap, last = struct.unpack('<II', r.read(p + 0x70, 8))
            dt = struct.unpack('<f', r.read(p + 0x78, 4))[0]
            print('  scheduler P=0x%08x C=0x%08x vtable=0x%08x' % (p, c, r.u32(c)))
            print('  dt min %.6f max %.6f fixed %.6f scale %.3f flags %s cap %d dt %.6f'
                  % (dt_min, dt_max, dt_fixed, scale, flags.hex(), cap, dt))
            print('  last=%d' % last)
        print('  clock 0x376b0c=%d' % r.u32(0x376b0c))
        mode, eye = r.read(0x52a1f4, 1)[0], r.read(0x52a1f8, 1)[0]
        print('  present mode 0x52a1f4=%d 0x52a1f8=%d interval 0x529de8=%d'
              % (mode, eye, r.u32(0x529de8)))
        print('  vblank 0x593d14=%d 0x593d18=%d' % (r.u32(0x593d14), r.u32(0x593d18)))

    def player(self):
        r = self.remote
        entity = r.u32(PLAYER)
        if not entity:
            return None
        node = r.u32(entity + 0x3c)
        ctrl = r.u32(entity + 0x7c)
        state = {
            'entity': entity, 'node': node, 'ctrl': ctrl,
            'dirty': bool(r.u32(node) & 0x1000),
            'world': struct.unpack('<3f', r.read(node + 0x8c, 12)),
            'forward': struct.unpack('<3f', r.read(node + 0x7c, 12)),
            'local': struct.unpack('<3f', r.read(node + 0x4c, 12)),
            'camera_yaw': struct.unpack('<f', r.read(CAMERA_YAW, 4))[0],
        }
        if ctrl:
            state['intent'] = struct.unpack('<h', r.read(ctrl + 6, 2))[0]
            state['held_flags'], state['press_flags'] = struct.unpack('<HH', r.read(ctrl + 0xc, 4))
            state['ctrl_state'] = r.read(ctrl + 0x14, 1)[0]
        return state

    def print_player(self):
        state = self.player()
        if state is None:
            print('  no player entity (*0x%x == 0)' % PLAYER)
            return
        print('  entity 0x%08x node 0x%08x ctrl 0x%08x dirty %d' % (state['entity'], state['node'], state['ctrl'], state['dirty']))
        print('  world %.3f %.3f %.3f  local %.3f %.3f %.3f' % (state['world'] + state['local']))
        print('  forward %.3f %.3f %.3f  camera yaw %.4f' % (state['forward'] + (state['camera_yaw'],)))
        if 'intent' in state:
            print('  intent %d held 0x%04x press 0x%04x state %d'
                  % (state['intent'], state['held_flags'], state['press_flags'], state['ctrl_state']))

    def channels(self, count=42):
        device = self.remote.u32(PAD_DEVICE)
        if not device:
            print('  no pad device (*0x%x == 0)' % PAD_DEVICE)
            return
        base = self.remote.u32(device + 0x14)
        data = self.remote.read(base, count * 0x14)
        print('  device 0x%08x channels 0x%08x' % (device, base))
        for index in range(count):
            value, kind, deadzone, threshold, press, release = struct.unpack_from('<fB3xffhh', data, index * 0x14)
            print('  ch%-2d value %8.4f type %3d deadzone %.4f threshold %.4f press %d release %d'
                  % (index, value, kind, deadzone, threshold, press, release))

    def trace(self, count, name):
        path = self.output / ('%s.csv' % name)
        with open(path, 'w') as out:
            out.write('frame,dt,held,stick_x,stick_y,x,y,z,fx,fz,intent,flags,state,camera_yaw\n')

            def sample():
                state = self.player()
                dt = struct.unpack('<f', self.remote.read(self.frame_object + 0x78, 4))[0]
                if state is None:
                    return
                out.write('%d,%.6f,%d,%d,%d,%.5f,%.5f,%.5f,%.5f,%.5f,%d,%d,%d,%.5f\n' % (
                    self.frame, dt, self.held, self.stick[0], self.stick[1], *state['world'],
                    state['forward'][0], state['forward'][2], state.get('intent', 0),
                    state.get('held_flags', 0), state.get('ctrl_state', 0), state['camera_yaw']))

            self.advance(count, sample)
        print('  %s' % path)

    def screenshot(self, name):
        for screen, base, width in (('top', GSP_SHARED + 0x200, 400), ('bottom', GSP_SHARED + 0x240, 320)):
            index = self.remote.read(base, 1)[0] & 1
            info = struct.unpack('<7I', self.remote.read(base + 4 + index * 0x1c, 0x1c))
            address, stride, fmt = info[1], info[3], info[4] & 7
            if 0x18000000 <= address < 0x18600000:
                address += 0x07000000
            elif address >= 0x20000000:
                address = address - 0x20000000 + 0x14000000
            pixels = self.remote.read(address, stride * width)
            path = self.output / ('%s_%s.png' % (name, screen))
            write_png(path, decode_framebuffer(pixels, width, stride, fmt))
            print('  %s: 0x%08x format %d -> %s' % (screen, address, fmt, path))


def decode_framebuffer(data, width, stride, fmt):
    height = 240
    size = {0: 4, 1: 3, 2: 2, 3: 2, 4: 2}[fmt]
    rows = []
    for y in range(height):
        row = bytearray()
        for x in range(width):
            offset = x * stride + (height - 1 - y) * size
            if fmt == 0:
                a, b, g, r = data[offset:offset + 4]
            elif fmt == 1:
                b, g, r = data[offset:offset + 3]
            else:
                v = data[offset] | data[offset + 1] << 8
                if fmt == 2:
                    r, g, b = (v >> 11) << 3, ((v >> 5) & 0x3f) << 2, (v & 0x1f) << 3
                elif fmt == 3:
                    r, g, b = (v >> 11) << 3, ((v >> 6) & 0x1f) << 3, ((v >> 1) & 0x1f) << 3
                else:
                    r, g, b = (v >> 12) << 4, ((v >> 8) & 0xf) << 4, ((v >> 4) & 0xf) << 4
            row += bytes((r, g, b))
        rows.append(bytes(row))
    return rows


def write_png(path, rows):
    width, height = len(rows[0]) // 3, len(rows)

    def chunk(kind, payload):
        return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', zlib.crc32(kind + payload))

    raw = b''.join(b'\0' + row for row in rows)
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


def parse_buttons(text):
    if text == '-':
        return 0
    mask = 0
    for name in text.upper().split('+'):
        if name not in BUTTONS:
            raise ValueError('unknown button %r (known: %s)' % (name, ', '.join(BUTTONS)))
        mask |= BUTTONS[name]
    return mask


def run_scenario(oracle, lines):
    for number, line in enumerate(lines, 1):
        words = line.split('#', 1)[0].split()
        if not words:
            continue
        op, args = words[0], words[1:]
        print('> %s' % ' '.join(words), flush=True)
        if op == 'frames':
            oracle.advance(int(args[0]))
        elif op == 'hold':
            oracle.held = parse_buttons(args[0])
            oracle.inject_input()
        elif op == 'stick':
            oracle.stick = (int(args[0]), int(args[1]))
            oracle.inject_input()
        elif op == 'screenshot':
            oracle.screenshot(args[0])
        elif op == 'wait':
            oracle.wait(float(args[0]))
        elif op == 'channels':
            oracle.channels()
        elif op == 'player':
            oracle.print_player()
        elif op == 'trace':
            oracle.trace(int(args[0]), args[1])
        elif op == 'report':
            oracle.report()
        elif op == 'read':
            data = oracle.remote.read(int(args[0], 0), int(args[1], 0))
            for i in range(0, len(data), 16):
                print('  0x%08x: %s' % (int(args[0], 0) + i, data[i:i + 16].hex(' ', 4)))
        else:
            raise ValueError('line %d: unknown command %r' % (number, op))


def find_image():
    images = sorted(REPO.glob('*.cci'))
    if len(images) != 1:
        raise SystemExit('pass --image: found %d .cci files in the repository root' % len(images))
    return images[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('scenario', type=Path)
    parser.add_argument('--image', type=Path)
    parser.add_argument('--home', type=Path, default=REPO / 'artifacts/oracle/home',
                        help='isolated Azahar home (created on first use)')
    parser.add_argument('--output', type=Path, default=REPO / 'artifacts/oracle/out')
    parser.add_argument('--port', type=int, default=24690)
    parser.add_argument('--renderer', choices=('software', 'opengl'), default='software',
                        help='software writes frames to emulated VRAM (screenshots work) but delivers vblanks late; '
                             'opengl keeps timing faithful but screenshots show stale VRAM')
    parser.add_argument('--fresh', action='store_true', help='delete the isolated home first (resets the emulated save)')
    args = parser.parse_args()

    image = (args.image or find_image()).resolve()
    azahar = shutil.which('azahar')
    if not azahar:
        raise SystemExit('azahar not found in PATH')
    home = args.home.resolve()
    if args.fresh and home.exists():
        if not (home / 'config/azahar-emu').is_dir():
            raise SystemExit('%s does not look like an oracle home; refusing to delete it' % home)
        shutil.rmtree(home)
    config = home / 'config/azahar-emu/qt-config.ini'
    config.parent.mkdir(parents=True, exist_ok=True)
    config.write_text(CONFIG.replace('{renderer}', '0' if args.renderer == 'software' else '1'))
    args.output.mkdir(parents=True, exist_ok=True)

    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', XDG_CONFIG_HOME=str(home / 'config'),
               XDG_DATA_HOME=str(home / 'data'), XDG_CACHE_HOME=str(home / 'cache'))
    log = open(home / 'azahar-stdout.log', 'wb')
    process = subprocess.Popen([azahar, '-g', str(args.port), str(image)], env=env,
                               stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    started = time.monotonic()
    try:
        remote = GdbRemote(args.port)
        oracle = Oracle(remote, args.output.resolve())
        run_scenario(oracle, args.scenario.read_text().splitlines())
    finally:
        os.killpg(process.pid, signal.SIGKILL)
        process.wait()
        log.close()
    print('done in %.1f s' % (time.monotonic() - started))


if __name__ == '__main__':
    sys.exit(main())
