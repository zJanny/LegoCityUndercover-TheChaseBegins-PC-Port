#!/usr/bin/env python3
"""Run the original game in Azahar as a runtime oracle and observe it through the GDB stub.

The emulator runs headless with the software renderer by default (see --renderer), in an isolated
home directory under artifacts/oracle/ so the user's own Azahar settings and saves are
never touched. The image is opened read-only by Azahar. Fidelity rules and the fast workflow are in
docs/re/oracle.md.

A scenario file drives the run, one command per line ('#' starts a comment):

  frames N              advance N calls of the scheduler 0x100f44 (one game update each)
  wait SECONDS          let the emulator run freely for a wall-clock time (no frame count)
  hold BUTTONS          hold buttons from now on, e.g. 'hold A' or 'hold A+UP'; 'hold -' releases all
  stick X Y             raw circle-pad values (about -156..156); 'stick 0 0' centres it
  screenshot NAME [gdb] write NAME_top.png and NAME_bottom.png (software renderer) or NAME.png (OpenGL with --rpc);
                        'gdb' forces the slow memory-read path
  report                print scheduler and frame-pacing values
  channels              print the pad device's 20-byte channel records (value, type, deadzone, threshold, edges)
  player                print the player entity (*0x3772b8): world position, facing, intent heading, flags
  trace N NAME [mem]    advance N frames, writing the player state of every frame to NAME.csv; 'mem' also writes
                        NAME.mem: per frame u32 frame, then entity (0x100 bytes), controller (0x340) and node (0xb0)
  memtrace N NAME SPEC...
                        advance N frames, writing per frame u32 frame, f32 dt and each memory region to NAME.mem.
                        SPEC is ADDR:LEN, or *PTR+OFF:LEN (read the word at PTR, add OFF)
  watch ADDR LEN N NAME [KIND]
                        advance N frames with a GDB watchpoint (KIND 2 write, 3 read, 4 access; default 2) and write
                        the registers and 32 stack words of every stop to NAME.txt
  trap ADDR[,ADDR...] N NAME
                        advance N frames with execution breakpoints at the addresses and write the registers and
                        32 stack words of every stop to NAME.txt
  teleport X Y Z        write the world and local translation of the player node (a measuring aid: places the player
                        at a ledge without walking there; the node's parent has an identity transform at gameplay-start)
  dt MIN MAX            write the scheduler's dt limits (floats at C+0x10 and C+0x14); MIN == MAX fixes the step
  read ADDR LEN         print LEN bytes at ADDR as hex words
  poke ADDR HEXBYTES    write the bytes (hex digits, e.g. 0200) at ADDR through the stub (a measuring aid, for example to
                        send an object a request that its script would send; the write clears the instruction cache)
  sample SECONDS INTERVAL NAME
                        free-run for SECONDS (no breakpoints) and sample the game clock, vblank counters and the
                        player position every INTERVAL seconds through RPC into NAME.csv (needs --rpc)
  perf                  print the emulator's own speed statistics (needs --rpc)
  savestate NAME        save the emulator state as NAME in --states (needs --display xvfb)
  loadstate NAME        restore the state NAME (needs --display xvfb)

Input is injected inside the game's pad reader 0x10a4dc: the held mask in r0 at 0x10a57c
and the raw circle-pad words at r4+12/+14 before 0x10a540. It is therefore exact per game
frame and independent of host input.

Fast iteration (a cold boot to the language screen takes about 4 minutes):

  --display xvfb        run Azahar on a private Xvfb display; the tool presses the emulator's Quick Save / Quick Load
                        hotkeys (F1 / F12, written to the generated config) with XTEST. Needed for savestate,
                        loadstate and --state.
  --state NAME          load the saved state NAME right after boot instead of playing through the menus.
  --keep                leave the emulator running after the scenario (the GDB client detaches); the PID and port
                        are printed and recorded in <home>/session.json.
  --attach              connect to the emulator kept on --port instead of launching one (no image needed). The
                        emulator keeps running afterwards. The frame counter of every new attach starts at 0.
  --stop                kill the kept emulator (and its Xvfb) recorded in <home>/session.json.

RPC channel (--rpc): Azahar's RPC server is a second, non-halting channel next to the GDB stub. It listens on the
fixed UDP port 45987 (not configurable), so only one emulator at a time can have it; the tool refuses to start
with --rpc while the port is taken. Reads (up to 32 KB per datagram) and writes go straight to guest memory
while the game runs, so measurements can be taken without stopping it. Memory reads for report, player, channels,
trace, read and the software-renderer screenshot use RPC when it is connected and fall back to the stub
otherwise. RPC writes are accepted only inside the process image, heap, linear heap and New 3DS extra RAM and
invalidate the JIT cache. Details are in docs/re/oracle.md.

  --rpc                 enable the RPC server in the generated config and connect to it (with --attach: connect to
                        the kept emulator's RPC server).
  --rpc-input           breakpoint-free pad input: patch the pad reader once (0x10a578 reads the held mask from the
                        a mailbox word in the code cave 0x356580) and let 'hold' write that word over RPC. Implies --rpc. Circle-pad
                        'stick' values still use the per-frame breakpoint at 0x10a540.

After loadstate the frame counter is set to the value stored with the state. A load is refused while the
game is still starting (Azahar shows a 'Savestates' dialog); the tool dismisses it and retries for up to
two minutes. Reaching the language screen is much faster free-running ('wait 25') than stepping 600 frames.
"""
import argparse
import csv
import ctypes
import json
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
TRACE_ENTITY_BYTES = 0x100
TRACE_CTRL_BYTES = 0x340
TRACE_NODE_BYTES = 0xb0
CAMERA_YAW = 0x376894
GAME_CLOCK = 0x376b0c
VBLANKS = 0x593d14
PAD_LOAD = 0x10a578
PAD_LOAD_ORIGINAL = 0xe5940000
PAD_RESUME = 0x10a57c
PAD_CAVE = 0x356580
PAD_MAILBOX = PAD_CAVE + 8


def arm_branch(source, target):
    return 0xea000000 | (((target - (source + 8)) >> 2) & 0xffffff)


PAD_CAVE_CODE = struct.pack('<II', 0xe59f0000, arm_branch(PAD_CAVE + 4, PAD_RESUME))
PAD_LOAD_PATCHED = arm_branch(PAD_LOAD, PAD_CAVE)

BUTTONS = {'A': 0x1, 'B': 0x2, 'SELECT': 0x4, 'START': 0x8, 'RIGHT': 0x10, 'LEFT': 0x20,
           'UP': 0x40, 'DOWN': 0x80, 'R': 0x100, 'L': 0x200, 'X': 0x400, 'Y': 0x800}

CONFIG = """[Audio]
volume=0
volume\\default=false
[Debugging]
use_gdbstub=true
use_gdbstub\\default=false
enable_rpc_server={rpc}
enable_rpc_server\\default=false
[Renderer]
graphics_api={renderer}
graphics_api\\default=false
frame_limit={frame_limit}
frame_limit\\default=false
[System]
is_new_3ds=false
is_new_3ds\\default=false
init_clock=1
init_clock\\default=false
[UI]
confirmClose=false
confirmClose\\default=false
singleWindowMode={single_window}
singleWindowMode\\default={single_default}
{shortcuts}"""

QUICK_SAVE_KEY = 'F1'
QUICK_LOAD_KEY = 'F12'
STATE_SETTLE_SECONDS = 3.0
STATE_RETRY_SECONDS = 5.0
STATE_LOAD_TIMEOUT = 120.0


LAUNCH_ATTEMPTS = 3
RENDER_WINDOW_SUFFIX = '| Primary Window'
RENDER_WINDOW_TIMEOUT = 60.0
CRASH_SIGNATURE = ('SoftwareRenderWidget::Present', 'ArchiveManager')


class EmulatorExited(RuntimeError):
    pass


def parse_stop_reply(reply):
    """Split a stop reply 'T05watch:08568bb0;...' into its signal and its name:value fields."""
    fields = {}
    for item in reply[3:].split(';'):
        name, separator, value = item.partition(':')
        if separator:
            fields[name] = value
    return int(reply[1:3], 16), fields


class GdbRemote:
    def __init__(self, port, timeout=60.0, alive=None):
        self.port = port
        deadline = time.monotonic() + timeout
        while True:
            if alive:
                alive()
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
        self.watchpoints = set()
        self.last_stop = None
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

    def add_watchpoint(self, address, length, kind=2):
        if self.command('Z%d,%x,%x' % (kind, address, length)) != 'OK':
            raise IOError('cannot set watchpoint at 0x%x' % address)
        self.watchpoints.add((address, kind))

    def remove_watchpoints(self):
        for address, kind in sorted(self.watchpoints):
            self.command('z%d,%x,1' % (kind, address))
        self.watchpoints.clear()

    def run_to_breakpoint(self):
        pc = self.register(15)
        if pc in self.breakpoints:
            self.command('z0,%x,4' % pc)
            self.command('s')
            self.command('Z0,%x,4' % pc)
        reply = self.command('c')
        if not reply.startswith(('T', 'S')):
            raise ConnectionError('unexpected stop reply %r' % reply)
        self.last_stop = parse_stop_reply(reply)
        return self.register(15)


RPC_PORT = 45987
RPC_VERSION = 2
RPC_READ_MAX = 32 * 1024
RPC_WRITE_MAX = RPC_READ_MAX - 8
RPC_READ, RPC_WRITE, RPC_PROCESS_LIST, RPC_PROCESS, RPC_SCREENSHOT, RPC_READ_SCREENSHOT, RPC_PERF = range(1, 8)
RPC_PERF_FIELDS = ('system_fps', 'game_fps', 'time_vblank_interval', 'time_hle_svc', 'time_hle_ipc', 'time_gpu',
                   'time_swap', 'time_remaining', 'emulation_speed', 'artic_transmitted')
RPC_SCREENSHOT_RESULTS = {1: 'is unsupported by the active renderer (the software renderer has none)', 2: 'is busy',
                          3: 'timed out waiting for a frame', 4: 'PNG encoding failed', 5: 'got an invalid argument'}
RPC_WRITABLE = ((0x00100000, 0x04000000), (0x08000000, 0x10000000), (0x14000000, 0x1c000000),
                (0x1e800000, 0x1ec00000))
APPLICATION_TITLE_TYPE = 0x00040000


def rpc_port_free():
    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        probe.bind(('127.0.0.1', RPC_PORT))
        return True
    except OSError:
        return False
    finally:
        probe.close()


class RpcError(IOError):
    pass


class RpcRemote:
    """Client for Azahar's RPC server (src/core/rpc in the Azahar source): UDP datagrams, header of four
    little-endian u32 (version, id, type, size) followed by size bytes; the reply echoes the header with its own
    size. A request the server rejects is answered with size 0, which is also the normal reply of a write."""

    def __init__(self, port=RPC_PORT, timeout=2.0, retries=3):
        self.timeout = timeout
        self.retries = retries
        self.next_id = 1
        self.requests = 0
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.connect(('127.0.0.1', port))

    def close(self):
        self.sock.close()

    def request(self, kind, payload, timeout=None):
        if len(payload) > RPC_READ_MAX:
            raise RpcError('RPC payload of %d bytes exceeds %d' % (len(payload), RPC_READ_MAX))
        request_id = self.next_id
        self.next_id = (self.next_id + 1) & 0xffffffff
        datagram = struct.pack('<4I', RPC_VERSION, request_id, kind, len(payload)) + payload
        self.sock.settimeout(timeout or self.timeout)
        for _ in range(1 if timeout else self.retries):
            self.sock.send(datagram)
            self.requests += 1
            try:
                while True:
                    reply = self.sock.recv(RPC_READ_MAX + 16)
                    if len(reply) >= 16 and struct.unpack_from('<I', reply, 4)[0] == request_id:
                        break
            except (socket.timeout, ConnectionRefusedError):
                continue
            size = struct.unpack_from('<I', reply, 12)[0]
            if len(reply) != 16 + size:
                raise RpcError('malformed RPC reply: %d bytes for size field %d' % (len(reply), size))
            return reply[16:]
        raise RpcError('no RPC reply on UDP port %d (is the emulator running with --rpc?)' % self.sock.getpeername()[1])

    def read(self, address, length):
        out = bytearray()
        while length:
            size = min(length, RPC_READ_MAX)
            data = self.request(RPC_READ, struct.pack('<II', address, size))
            if len(data) != size:
                raise RpcError('RPC read of %d bytes at 0x%x returned %d bytes (selected process gone?)'
                               % (size, address, len(data)))
            out += data
            address += size
            length -= size
        return bytes(out)

    def write(self, address, data, verify=True):
        if not any(low <= address < high for low, high in RPC_WRITABLE):
            raise RpcError('the RPC server ignores writes at 0x%08x (allowed: process image, heap, linear heap, '
                           'New 3DS extra RAM)' % address)
        offset = 0
        while offset < len(data):
            chunk = data[offset:offset + RPC_WRITE_MAX]
            self.request(RPC_WRITE, struct.pack('<II', address + offset, len(chunk)) + chunk)
            offset += len(chunk)
        if verify and self.read(address, len(data)) != bytes(data):
            raise RpcError('RPC write at 0x%x did not take effect' % address)

    def u32(self, address):
        return struct.unpack('<I', self.read(address, 4))[0]

    def processes(self):
        data = self.request(RPC_PROCESS_LIST, struct.pack('<II', 0, 64))
        found = []
        for index in range(struct.unpack_from('<I', data)[0]):
            pid, title, name = struct.unpack_from('<IQ8s', data, 4 + index * 0x14)
            found.append((pid, title, name.rstrip(b'\0').decode('latin-1')))
        return found

    def select_process(self, pid):
        self.request(RPC_PROCESS, struct.pack('<II', 1, pid))

    def select_application(self):
        applications = [p for p in self.processes() if p[1] >> 32 == APPLICATION_TITLE_TYPE]
        if not applications:
            raise RpcError('no application process in the RPC process list')
        self.select_process(applications[0][0])
        return applications[0]

    def perf(self, reset=False):
        data = self.request(RPC_PERF, struct.pack('<II', 1 if reset else 0, 0))
        if len(data) != 0x54:
            raise RpcError('unexpected RPC perf reply of %d bytes' % len(data))
        return dict(zip(RPC_PERF_FIELDS, struct.unpack_from('<10d', data)))

    def take_screenshot(self, flags=0, scale=1):
        info = self.request(RPC_SCREENSHOT, struct.pack('<II', scale, flags), timeout=15.0)
        result, width, height, size = struct.unpack('<4I', info)
        if result:
            raise RpcError('RPC screenshot %s' % RPC_SCREENSHOT_RESULTS.get(result, 'failed (%d)' % result))
        out = bytearray()
        while len(out) < size:
            chunk = self.request(RPC_READ_SCREENSHOT,
                                 struct.pack('<II', len(out), min(size - len(out), RPC_READ_MAX)))
            if not chunk:
                raise RpcError('RPC screenshot data ended at %d of %d bytes' % (len(out), size))
            out += chunk
        return width, height, bytes(out)


class XKeys:
    def __init__(self, display):
        self.x11 = ctypes.CDLL('libX11.so.6')
        self.xtst = ctypes.CDLL('libXtst.so.6')
        self.x11.XOpenDisplay.restype = ctypes.c_void_p
        self.x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
        self.x11.XStringToKeysym.restype = ctypes.c_ulong
        self.x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
        self.x11.XKeysymToKeycode.restype = ctypes.c_ubyte
        self.x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        self.x11.XFlush.argtypes = [ctypes.c_void_p]
        self.x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
        self.xtst.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
        self.x11.XDefaultRootWindow.restype = ctypes.c_ulong
        self.x11.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
        self.x11.XQueryTree.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong),
                                        ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)),
                                        ctypes.POINTER(ctypes.c_uint)]
        self.x11.XFetchName.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_char_p)]
        self.x11.XFree.argtypes = [ctypes.c_void_p]
        self.x11.XUnmapWindow.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        self.x11.XSetInputFocus.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
        self.x11.XSync.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.display = self.x11.XOpenDisplay(display.encode())
        if not self.display:
            raise OSError('cannot open X display %s' % display)

    def windows(self):
        root, parent = ctypes.c_ulong(), ctypes.c_ulong()
        children, count = ctypes.POINTER(ctypes.c_ulong)(), ctypes.c_uint()
        self.x11.XQueryTree(self.display, self.x11.XDefaultRootWindow(self.display), ctypes.byref(root),
                            ctypes.byref(parent), ctypes.byref(children), ctypes.byref(count))
        found = []
        for index in range(count.value):
            name = ctypes.c_char_p()
            if self.x11.XFetchName(self.display, children[index], ctypes.byref(name)) and name.value:
                found.append((children[index], name.value.decode('latin-1')))
        if count.value:
            self.x11.XFree(children)
        return found

    def window_names(self):
        return [name for _, name in self.windows()]

    def find_window(self, suffix):
        return next((window for window, name in self.windows() if name.endswith(suffix)), None)

    def unmap(self, window):
        self.x11.XUnmapWindow(self.display, window)
        self.x11.XSync(self.display, 0)

    def focus(self, window):
        self.x11.XSetInputFocus(self.display, window, 1, 0)
        self.x11.XSync(self.display, 0)

    def focus_main_window(self):
        main = next((window for window, name in self.windows()
                     if name.startswith('Azahar ') and not name.endswith(RENDER_WINDOW_SUFFIX)), None)
        if main is not None:
            self.focus(main)

    def press(self, key_name):
        keycode = self.x11.XKeysymToKeycode(self.display, self.x11.XStringToKeysym(key_name.encode()))
        if not keycode:
            raise ValueError('unknown X key %r' % key_name)
        for down in (1, 0):
            self.xtst.XTestFakeKeyEvent(self.display, keycode, down, 0)
            self.x11.XFlush(self.display)
            time.sleep(0.15)

    def close(self):
        self.x11.XCloseDisplay(self.display)


def session_file(home):
    return home / 'session.json'


def load_session(home):
    try:
        return json.loads(session_file(home).read_text())
    except (OSError, ValueError):
        return None


def state_frame(saved_states, name):
    return json.loads((saved_states / ('%s.json' % name)).read_text())['frame']


def place_state(saved_states, name, emulator_states):
    meta = json.loads((saved_states / ('%s.json' % name)).read_text())
    emulator_states.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(saved_states / ('%s.cst' % name), emulator_states / meta['file'])


def kill_group(pid):
    try:
        os.killpg(pid, signal.SIGKILL)
    except ProcessLookupError:
        return
    except PermissionError:
        os.kill(pid, signal.SIGKILL)


def stop_process(process):
    """Kill a process group this tool started (the child is its own session leader) and reap the child."""
    kill_group(process.pid)
    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


class Launched:
    """The emulator process and the Xvfb the tool started for it; stop() kills and reaps exactly these."""

    def __init__(self, process, xvfb, display, keys):
        self.process = process
        self.xvfb = xvfb
        self.display = display
        self.keys = keys

    def check(self):
        code = self.process.poll()
        if code is not None:
            raise EmulatorExited('Azahar exited with %s' % describe_exit(code))

    def silence_render_window(self):
        """Unmap Azahar's render window so that Qt stops painting it, and move the keyboard focus to the main
        window (the hotkeys are application shortcuts). A paint of the software render widget that overlaps
        a state load dereferences the freed GPU and aborts Azahar."""
        deadline = time.monotonic() + RENDER_WINDOW_TIMEOUT
        while True:
            self.check()
            windows = self.keys.windows()
            render = [w for w, name in windows if name.endswith(RENDER_WINDOW_SUFFIX)]
            main = [w for w, name in windows if name.startswith('Azahar ') and not name.endswith(RENDER_WINDOW_SUFFIX)]
            if render and main:
                break
            if time.monotonic() > deadline:
                raise RuntimeError('Azahar did not open its render and main windows')
            time.sleep(0.2)
        self.keys.unmap(render[0])
        self.keys.focus(main[0])

    def exited(self):
        try:
            self.process.wait(timeout=1.0)
        except subprocess.TimeoutExpired:
            return False
        return True

    def stop(self):
        if self.keys:
            self.keys.close()
            self.keys = None
        stop_process(self.process)
        if self.xvfb:
            stop_process(self.xvfb)


def describe_exit(code):
    if code < 0:
        try:
            return 'signal %s' % signal.Signals(-code).name
        except ValueError:
            return 'signal %d' % -code
    return 'status %d' % code


def crash_description(home, code):
    texts = []
    for path in (home / 'data/azahar-emu/log/azahar_log.txt', home / 'azahar-stdout.log'):
        try:
            texts.append(path.read_text(errors='replace'))
        except OSError:
            pass
    text = '\n'.join(texts)
    if all(frame in text for frame in CRASH_SIGNATURE):
        return ('Azahar crashed with %s in %s -> %s (null system pointer, paint event before the system '
                'is initialised)' % ((describe_exit(code),) + CRASH_SIGNATURE))
    return 'Azahar exited with %s; no %s frames in its logs' % (describe_exit(code), ' / '.join(CRASH_SIGNATURE))


def save_crash_logs(home, attempt):
    for path in (home / 'data/azahar-emu/log/azahar_log.txt', home / 'azahar-stdout.log'):
        if path.exists():
            shutil.copyfile(path, path.with_name('crash%d-%s' % (attempt, path.name)))


def stop_session(home):
    session = load_session(home)
    if not session:
        raise SystemExit('no kept emulator recorded in %s' % session_file(home))
    for key in ('pid', 'xvfb_pid'):
        if session.get(key):
            kill_group(session[key])
    session_file(home).unlink()
    print('stopped emulator pid %d port %d' % (session['pid'], session['port']))


def lock_owner_alive(lock):
    try:
        os.kill(int(lock.read_text().strip()), 0)
    except (ValueError, ProcessLookupError):
        return False
    except PermissionError:
        return True
    return True


def free_display(root=Path('/tmp')):
    for number in range(90, 200):
        lock = root / ('.X%d-lock' % number)
        if not lock.exists():
            return ':%d' % number
        if not lock_owner_alive(lock):
            lock.unlink(missing_ok=True)
            (root / '.X11-unix' / ('X%d' % number)).unlink(missing_ok=True)
            return ':%d' % number
    raise SystemExit('no free X display number')


def start_xvfb(log):
    display = free_display()
    process = subprocess.Popen(['Xvfb', display, '-screen', '0', '800x600x24', '-nolisten', 'tcp'],
                               stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    deadline = time.monotonic() + 15
    while not Path('/tmp/.X11-unix/X%s' % display[1:]).exists():
        if process.poll() is not None or time.monotonic() > deadline:
            stop_process(process)
            raise SystemExit('Xvfb did not start on %s' % display)
        time.sleep(0.1)
    return process, display


def shortcut_lines():
    lines = []
    for name, key in (('Quick Save', QUICK_SAVE_KEY), ('Quick Load', QUICK_LOAD_KEY)):
        prefix = 'Shortcuts\\Main%20Window\\' + name.replace(' ', '%20')
        lines += ['%s\\Context=2' % prefix, '%s\\Context\\default=false' % prefix,
                  '%s\\KeySeq=%s' % (prefix, key), '%s\\KeySeq\\default=false' % prefix]
    return '\n'.join(lines) + '\n'


class Oracle:
    def __init__(self, remote, output, keys=None, emulator_states=None, saved_states=None, rpc=None,
                 renderer='software', rpc_input=False, alive=None):
        self.remote = remote
        self.alive = alive or (lambda: None)
        self.rpc = rpc
        self.renderer = renderer
        self.rpc_input = rpc_input
        self.pad_patched = False
        self.output = output
        self.keys = keys
        self.emulator_states = emulator_states
        self.saved_states = saved_states
        self.held = 0
        self.stick = (0, 0)
        self.frame = 0
        self.frame_object = None
        self.watch_hits = None
        remote.add_breakpoint(SCHEDULER)

    @property
    def mem(self):
        return self.rpc or self.remote

    def patch_pad_reader(self):
        current = self.rpc.u32(PAD_LOAD)
        if current not in (PAD_LOAD_ORIGINAL, PAD_LOAD_PATCHED):
            raise RuntimeError('pad reader at 0x%x holds 0x%08x, expected 0x%08x' % (PAD_LOAD, current, PAD_LOAD_ORIGINAL))
        cave = self.rpc.read(PAD_CAVE, 12)
        if cave[:8] != PAD_CAVE_CODE and any(cave):
            raise RuntimeError('code cave at 0x%x is not empty: %s' % (PAD_CAVE, cave.hex()))
        self.rpc.write(PAD_CAVE, PAD_CAVE_CODE + struct.pack('<I', self.held))
        self.rpc.write(PAD_LOAD, struct.pack('<I', PAD_LOAD_PATCHED))
        self.pad_patched = True

    def unpatch_pad_reader(self):
        if self.pad_patched:
            self.rpc.write(PAD_LOAD, struct.pack('<I', PAD_LOAD_ORIGINAL))
            self.rpc.write(PAD_CAVE, bytes(12))
            self.pad_patched = False

    def inject_input(self):
        if self.rpc_input:
            if not self.pad_patched:
                self.patch_pad_reader()
            self.rpc.write(PAD_MAILBOX, struct.pack('<I', self.held))
            if self.stick != (0, 0) and PAD_STICK not in self.remote.breakpoints:
                self.remote.add_breakpoint(PAD_STICK)
        elif PAD_HELD not in self.remote.breakpoints:
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
            elif self.watch_hits is not None:
                self.record_watch_hit(pc)

    def set_dt(self, low, high):
        if self.frame_object is None:
            self.advance(1)
        controller = self.mem.u32(self.frame_object + 0xc)
        self.mem.write(controller + 0x10, struct.pack('<ff', low, high))
        print('  dt limits at 0x%08x set to %.6f %.6f' % (controller + 0x10, low, high))

    def record_watch_hit(self, pc):
        r = self.remote
        fields = r.last_stop[1] if r.last_stop else {}
        registers = [r.register(index) for index in range(16)]
        stack = r.read(registers[13], 0x80)
        pointed = {}
        for index in range(4):
            value = registers[index]
            if 0x00100000 <= value < 0x10000000 or 0x14000000 <= value < 0x1c000000:
                pointed[index] = r.read(value, 0x40)
        hit = {'frame': self.frame, 'pc': pc, 'registers': registers, 'stack': struct.unpack('<32I', stack),
               'fields': fields, 'pointed': pointed}
        self.watch_hits.append(hit)
        print('  watch hit frame %d pc 0x%08x lr 0x%08x %s' % (self.frame, pc, registers[14], fields))

    def watch(self, address, length, count, name, kind=2):
        self.record_stops(count, name, watchpoint=(address, length, kind))

    def trap(self, addresses, count, name):
        self.record_stops(count, name, breakpoints=addresses)

    def record_stops(self, count, name, watchpoint=None, breakpoints=()):
        path = self.output / ('%s.txt' % name)
        self.watch_hits = []
        added = [a for a in breakpoints if a not in self.remote.breakpoints]
        for address in added:
            self.remote.add_breakpoint(address)
        if watchpoint:
            self.remote.add_watchpoint(*watchpoint)
        try:
            self.advance(count)
        finally:
            self.remote.remove_watchpoints()
            for address in added:
                self.remote.command('z0,%x,4' % address)
                self.remote.breakpoints.discard(address)
            hits, self.watch_hits = self.watch_hits, None
        with open(path, 'w') as out:
            for hit in hits:
                out.write('frame %d pc 0x%08x fields %s\n' % (hit['frame'], hit['pc'], hit['fields']))
                out.write('  r0-r15 %s\n' % ' '.join('%08x' % v for v in hit['registers']))
                out.write('  stack %s\n' % ' '.join('%08x' % v for v in hit['stack']))
                for index, data in sorted(hit['pointed'].items()):
                    out.write('  [r%d] %s\n' % (index, data.hex(' ', 4)))
        print('  %d watchpoint stops -> %s' % (len(hits), path))

    def run_free(self, during, seconds):
        r = self.remote
        installed = sorted(r.breakpoints)
        for address in installed:
            r.command('z0,%x,4' % address)
        r.send('c')
        try:
            during(seconds)
        finally:
            r.sock.sendall(b'\x03')
            reply = r._packet()
            if not reply.startswith(('T', 'S')):
                raise ConnectionError('unexpected stop reply %r' % reply)
            for address in installed:
                r.command('Z0,%x,4' % address)
        self.frame_object = None

    def wait(self, seconds):
        self.run_free(time.sleep, seconds)

    def detach(self):
        r = self.remote
        installed = sorted(r.breakpoints)
        for address in installed:
            r.command('z0,%x,4' % address)
        r.breakpoints.clear()
        r.command('D')
        r.sock.close()
        return installed

    def reattach(self, installed):
        self.remote = GdbRemote(self.remote.port, alive=self.alive)
        for address in installed:
            self.remote.add_breakpoint(address)
        self.frame_object = None

    def snapshot_states(self):
        return {path: path.stat().st_mtime_ns for path in self.emulator_states.glob('*.cst')}

    def require_keys(self):
        if self.keys is None:
            raise SystemExit('save states need --display xvfb')

    def savestate(self, name):
        self.require_keys()
        self.saved_states.mkdir(parents=True, exist_ok=True)
        before = self.snapshot_states()
        written = []

        def press_and_wait(seconds):
            self.keys.focus_main_window()
            self.keys.press(QUICK_SAVE_KEY)
            deadline = time.monotonic() + seconds
            size = -1
            while time.monotonic() < deadline:
                time.sleep(0.5)
                now = {p: t for p, t in self.snapshot_states().items() if before.get(p) != t}
                if now:
                    path = max(now, key=now.get)
                    if path.stat().st_size == size:
                        written.append(path)
                        return
                    size = path.stat().st_size

        self.run_free(press_and_wait, 60)
        if not written:
            if 'Savestates' in self.keys.window_names():
                self.keys.press('Return')
            raise RuntimeError('quick save did not write a state file (the game may still be starting)')
        shutil.copyfile(written[0], self.saved_states / ('%s.cst' % name))
        (self.saved_states / ('%s.json' % name)).write_text(json.dumps({'file': written[0].name, 'frame': self.frame}))
        print('  saved %s (%d bytes, frame %d)' % (name, written[0].stat().st_size, self.frame))

    def loadstate(self, name):
        self.require_keys()
        place_state(self.saved_states, name, self.emulator_states)
        installed = self.detach()
        deadline = time.monotonic() + STATE_LOAD_TIMEOUT
        while True:
            self.alive()
            self.keys.focus_main_window()
            self.keys.press(QUICK_LOAD_KEY)
            time.sleep(STATE_SETTLE_SECONDS)
            self.alive()
            if 'Savestates' not in self.keys.window_names():
                break
            if time.monotonic() > deadline:
                raise RuntimeError('Azahar refused to load the state for %d s' % STATE_LOAD_TIMEOUT)
            self.keys.press('Return')
            time.sleep(STATE_RETRY_SECONDS)
        self.reattach(installed)
        if self.rpc_input:
            self.pad_patched = False
            self.inject_input()
        frame = state_frame(self.saved_states, name)
        self.advance(1)
        self.frame = frame
        print('  loaded %s (frame %d)' % (name, frame))

    def report(self):
        r = self.mem
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
        print('  clock 0x%x=%d' % (GAME_CLOCK, r.u32(GAME_CLOCK)))
        mode, eye = r.read(0x52a1f4, 1)[0], r.read(0x52a1f8, 1)[0]
        print('  present mode 0x52a1f4=%d 0x52a1f8=%d interval 0x529de8=%d'
              % (mode, eye, r.u32(0x529de8)))
        print('  vblank 0x%x=%d 0x%x=%d' % (VBLANKS, r.u32(VBLANKS), VBLANKS + 4, r.u32(VBLANKS + 4)))

    def player(self):
        r = self.mem
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

    def teleport(self, x, y, z):
        state = self.player()
        if state is None:
            raise RuntimeError('no player entity (*0x%x == 0)' % PLAYER)
        if state['world'] != state['local']:
            raise RuntimeError('player node world %s differs from local %s: not a pure root node' % (state['world'], state['local']))
        position = struct.pack('<3f', x, y, z)
        self.mem.write(state['node'] + 0x8c, position)
        self.mem.write(state['node'] + 0x4c, position)

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
        device = self.mem.u32(PAD_DEVICE)
        if not device:
            print('  no pad device (*0x%x == 0)' % PAD_DEVICE)
            return
        base = self.mem.u32(device + 0x14)
        data = self.mem.read(base, count * 0x14)
        print('  device 0x%08x channels 0x%08x' % (device, base))
        for index in range(count):
            value, kind, deadzone, threshold, press, release = struct.unpack_from('<fB3xffhh', data, index * 0x14)
            print('  ch%-2d value %8.4f type %3d deadzone %.4f threshold %.4f press %d release %d'
                  % (index, value, kind, deadzone, threshold, press, release))

    def trace(self, count, name, with_memory=False):
        path = self.output / ('%s.csv' % name)
        memory = open(self.output / ('%s.mem' % name), 'wb') if with_memory else None
        with open(path, 'w') as out:
            out.write('frame,dt,held,stick_x,stick_y,x,y,z,fx,fz,intent,flags,state,camera_yaw\n')

            def sample():
                state = self.player()
                dt = struct.unpack('<f', self.mem.read(self.frame_object + 0x78, 4))[0]
                if state is None:
                    return
                if memory:
                    memory.write(struct.pack('<I', self.frame))
                    for base, size in ((state['entity'], TRACE_ENTITY_BYTES), (state['ctrl'], TRACE_CTRL_BYTES),
                                       (state['node'], TRACE_NODE_BYTES)):
                        memory.write(self.mem.read(base, size))
                out.write('%d,%.6f,%d,%d,%d,%.5f,%.5f,%.5f,%.5f,%.5f,%d,%d,%d,%.5f\n' % (
                    self.frame, dt, self.held, self.stick[0], self.stick[1], *state['world'],
                    state['forward'][0], state['forward'][2], state.get('intent', 0),
                    state.get('held_flags', 0), state.get('ctrl_state', 0), state['camera_yaw']))

            self.advance(count, sample)
        if memory:
            memory.close()
        print('  %s' % path)

    def memtrace(self, count, name, specs):
        regions = []
        for spec in specs:
            location, length = spec.rsplit(':', 1)
            pointer = location.startswith('*')
            base, _, offset = location.lstrip('*').partition('+')
            regions.append((pointer, int(base, 0), int(offset, 0) if offset else 0, int(length, 0)))
        with open(self.output / ('%s.mem' % name), 'wb') as memory:
            def sample():
                dt = struct.unpack('<f', self.mem.read(self.frame_object + 0x78, 4))[0]
                memory.write(struct.pack('<If', self.frame, dt))
                for pointer, base, offset, length in regions:
                    address = (self.mem.u32(base) if pointer else base) + offset
                    memory.write(self.mem.read(address, length))

            self.advance(count, sample)
        print('  %s (per frame: u32 frame, f32 dt, then %s)' % (
            self.output / ('%s.mem' % name), ', '.join('%d bytes' % region[3] for region in regions)))

    def screenshot(self, name, via_gdb=False):
        started = time.monotonic()
        if self.rpc and not via_gdb and self.renderer == 'opengl':
            captured = []
            self.run_free(lambda _: captured.append(self.rpc.take_screenshot()), 0)
            width, height, png = captured[0]
            path = self.output / ('%s.png' % name)
            path.write_bytes(png)
            print('  rendered frame %dx%d -> %s' % (width, height, path))
            route = 'RPC screenshot request (the game ran while the frame was captured)'
        else:
            reader = self.remote if via_gdb or not self.rpc else self.rpc
            for screen, base, width in (('top', GSP_SHARED + 0x200, 400), ('bottom', GSP_SHARED + 0x240, 320)):
                index = reader.read(base, 1)[0] & 1
                info = struct.unpack('<7I', reader.read(base + 4 + index * 0x1c, 0x1c))
                address, stride, fmt = info[1], info[3], info[4] & 7
                if 0x18000000 <= address < 0x18600000:
                    address += 0x07000000
                elif address >= 0x20000000:
                    address = address - 0x20000000 + 0x14000000
                pixels = reader.read(address, stride * width)
                path = self.output / ('%s_%s.png' % (name, screen))
                write_png(path, decode_framebuffer(pixels, width, stride, fmt))
                print('  %s: 0x%08x format %d -> %s' % (screen, address, fmt, path))
            route = 'RPC memory reads' if reader is self.rpc else 'GDB stub memory reads'
        print('  screenshot took %.2f s via %s' % (time.monotonic() - started, route))

    def perf(self):
        for key, value in self.rpc.perf().items():
            print('  %-22s %.6f' % (key, value))

    def sample(self, seconds, interval, name):
        if not self.rpc:
            raise SystemExit('sample needs the RPC channel (--rpc)')
        path = self.output / ('%s.csv' % name)
        rows = []
        reader = self.rpc

        def collect(duration):
            start = time.monotonic()
            epoch = time.time()
            due = start
            while time.monotonic() - start < duration:
                before = time.monotonic()
                clock = reader.u32(GAME_CLOCK)
                vblank_a, vblank_b = struct.unpack('<II', reader.read(VBLANKS, 8))
                entity = reader.u32(PLAYER)
                position = (float('nan'),) * 3
                if entity:
                    position = struct.unpack('<3f', reader.read(reader.u32(entity + 0x3c) + 0x8c, 12))
                after = time.monotonic()
                rows.append(((before + after) / 2 - start, epoch + (before + after) / 2 - start, after - before,
                             clock, vblank_a, vblank_b, entity, *position))
                due += interval
                time.sleep(max(0.0, due - time.monotonic()))

        self.run_free(collect, seconds)
        with open(path, 'w', newline='') as out:
            writer = csv.writer(out)
            writer.writerow(('t', 'epoch', 'rpc_seconds', 'clock', 'vblank_a', 'vblank_b', 'entity', 'x', 'y', 'z'))
            for row in rows:
                writer.writerow(('%.4f' % row[0], '%.4f' % row[1], '%.5f' % row[2]) + row[3:7] + tuple('%.5f' % v for v in row[7:]))
        print('  %d samples -> %s' % (len(rows), path))
        if len(rows) > 1:
            first, last = rows[0], rows[-1]
            span = last[0] - first[0]
            for label, column in (('0x593d14', 4), ('0x593d18', 5)):
                vblanks = (last[column] - first[column]) & 0xffffffff
                ticks = (last[3] - first[3]) & 0xffffffff
                if vblanks:
                    print('  clock ticks per vblank (%s): %.2f over %.2f s (%d vblanks, %.2f vblanks/s, %d ticks)'
                          % (label, ticks / vblanks, span, vblanks, vblanks / span, ticks))
            print('  mean RPC time per sample %.2f ms' % (1000 * sum(r[2] for r in rows) / len(rows)))


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
            oracle.screenshot(args[0], via_gdb=args[1:] == ['gdb'])
        elif op == 'sample':
            oracle.sample(float(args[0]), float(args[1]), args[2])
        elif op == 'perf':
            oracle.perf()
        elif op == 'savestate':
            oracle.savestate(args[0])
        elif op == 'loadstate':
            oracle.loadstate(args[0])
        elif op == 'wait':
            oracle.wait(float(args[0]))
        elif op == 'channels':
            oracle.channels()
        elif op == 'player':
            oracle.print_player()
        elif op == 'trace':
            oracle.trace(int(args[0]), args[1], args[2:] == ['mem'])
        elif op == 'memtrace':
            oracle.memtrace(int(args[0]), args[1], args[2:])
        elif op == 'watch':
            oracle.watch(int(args[0], 0), int(args[1], 0), int(args[2]), args[3], int(args[4]) if args[4:] else 2)
        elif op == 'trap':
            oracle.trap([int(a, 0) for a in args[0].split(',')], int(args[1]), args[2])
        elif op == 'dt':
            oracle.set_dt(float(args[0]), float(args[1]))
        elif op == 'teleport':
            oracle.teleport(float(args[0]), float(args[1]), float(args[2]))
        elif op == 'report':
            oracle.report()
        elif op == 'poke':
            oracle.mem.write(int(args[0], 0), bytes.fromhex(args[1]))
        elif op == 'read':
            data = oracle.mem.read(int(args[0], 0), int(args[1], 0))
            for i in range(0, len(data), 16):
                print('  0x%08x: %s' % (int(args[0], 0) + i, data[i:i + 16].hex(' ', 4)))
        else:
            raise ValueError('line %d: unknown command %r' % (number, op))


def find_image():
    images = sorted(REPO.glob('*.cci'))
    if len(images) != 1:
        raise SystemExit('pass --image: found %d .cci files in the repository root' % len(images))
    return images[0]


def connect_rpc(timeout=60.0, alive=None):
    rpc = RpcRemote()
    deadline = time.monotonic() + timeout
    while True:
        if alive:
            alive()
        try:
            pid, title, name = rpc.select_application()
            print('RPC connected: process %d title %016x %s' % (pid, title, name))
            return rpc
        except RpcError:
            if time.monotonic() > deadline:
                raise
            time.sleep(0.5)


def terminate(signum, frame):
    raise SystemExit(128 + signum)


def install_termination_handlers():
    for number in (signal.SIGTERM, signal.SIGHUP):
        signal.signal(number, terminate)


def launch_emulator(args, home, azahar, image):
    env = dict(os.environ, XDG_CONFIG_HOME=str(home / 'config'), XDG_DATA_HOME=str(home / 'data'),
               XDG_CACHE_HOME=str(home / 'cache'))
    log = open(home / 'azahar-stdout.log', 'wb')
    xvfb = keys = process = None
    display = None
    try:
        if args.display == 'xvfb':
            xvfb, display = start_xvfb(log)
            env.update(QT_QPA_PLATFORM='xcb', DISPLAY=display)
            keys = XKeys(display)
        else:
            env['QT_QPA_PLATFORM'] = 'offscreen'
        process = subprocess.Popen([azahar, '-g', str(args.port), str(image)], env=env,
                                   stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    except BaseException:
        if keys:
            keys.close()
        for started in (process, xvfb):
            if started:
                stop_process(started)
        raise
    finally:
        log.close()
    session_file(home).write_text(json.dumps({
        'pid': process.pid, 'port': args.port, 'xvfb_pid': xvfb.pid if xvfb else None,
        'display': display, 'rpc': args.rpc, 'renderer': args.renderer}))
    return Launched(process, xvfb, display, keys)


def start_emulator(args, home, azahar, image, output, emulator_states, saved_states):
    """Launch Azahar and bring it to the point where the scenario can start (gdb stub, RPC and the optional state
    load). Azahar 2126.2 sometimes crashes in this phase; the attempt is then cleaned up and repeated. Nothing
    is retried once this function has returned, so a scenario never runs twice."""
    for attempt in range(1, LAUNCH_ATTEMPTS + 1):
        launched = launch_emulator(args, home, azahar, image)
        try:
            if launched.keys:
                launched.silence_render_window()
            remote = GdbRemote(args.port, alive=launched.check)
            rpc = connect_rpc(alive=launched.check) if args.rpc else None
            oracle = Oracle(remote, output, launched.keys, emulator_states, saved_states, rpc, args.renderer,
                            args.rpc_input, alive=launched.check)
            if args.state:
                oracle.loadstate(args.state)
            return launched, oracle
        except Exception as error:
            crashed = isinstance(error, EmulatorExited) or launched.exited()
            code = launched.process.poll()
            if crashed and code is not None:
                save_crash_logs(home, attempt)
            launched.stop()
            session_file(home).unlink(missing_ok=True)
            if not crashed:
                raise
            print('startup attempt %d/%d failed: %s' % (attempt, LAUNCH_ATTEMPTS, crash_description(home, code)),
                  flush=True)
            if attempt == LAUNCH_ATTEMPTS:
                raise SystemExit('Azahar crashed during startup in all %d attempts (logs: %s/crash*-*)'
                                 % (LAUNCH_ATTEMPTS, home)) from error
        except BaseException:
            launched.stop()
            session_file(home).unlink(missing_ok=True)
            raise


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('scenario', type=Path, nargs='?')
    parser.add_argument('--image', type=Path)
    parser.add_argument('--home', type=Path, default=REPO / 'artifacts/oracle/home',
                        help='isolated Azahar home (created on first use)')
    parser.add_argument('--output', type=Path, default=REPO / 'artifacts/oracle/out')
    parser.add_argument('--states', type=Path, default=REPO / 'artifacts/oracle/states',
                        help='where savestate/loadstate keep their named state files')
    parser.add_argument('--port', type=int, default=24690)
    parser.add_argument('--renderer', choices=('software', 'opengl'), default='software',
                        help='software writes frames to emulated VRAM (screenshots work) but delivers vblanks late; '
                             'opengl keeps timing faithful but screenshots show stale VRAM')
    parser.add_argument('--display', choices=('offscreen', 'xvfb'), default='offscreen',
                        help='xvfb runs Azahar on a private X server so that savestate/loadstate/--state can press '
                             'the emulator hotkeys')
    parser.add_argument('--frame-limit', type=int, default=100,
                        help='Azahar speed limit in percent; 0 is unthrottled and distorts the game clock')
    parser.add_argument('--state', help='load this saved state right after boot (needs --display xvfb)')
    parser.add_argument('--rpc', action='store_true',
                        help="enable Azahar's RPC server (UDP port 45987, one emulator at a time) and use it for memory "
                             'reads, screenshots and the sample command')
    parser.add_argument('--rpc-input', action='store_true',
                        help='inject pad buttons by patching the pad reader once instead of a breakpoint per frame '
                             '(implies --rpc)')
    parser.add_argument('--keep', action='store_true', help='leave the emulator running after the scenario')
    parser.add_argument('--attach', action='store_true',
                        help='connect to the emulator already running on --port instead of launching one; '
                             'it is left running afterwards. The frame counter starts at 0')
    parser.add_argument('--stop', action='store_true', help='kill the kept emulator recorded in --home and exit')
    parser.add_argument('--fresh', action='store_true', help='delete the isolated home first (resets the emulated save)')
    args = parser.parse_args()
    args.rpc = args.rpc or args.rpc_input

    home = args.home.resolve()
    if args.stop:
        return stop_session(home)
    if args.scenario is None and not args.keep:
        parser.error('a scenario file is required')
    scenario = args.scenario.read_text().splitlines() if args.scenario else []
    args.output.mkdir(parents=True, exist_ok=True)
    emulator_states = home / 'data/azahar-emu/states'
    saved_states = args.states.resolve()
    started = time.monotonic()

    launched = keys = rpc = oracle = None
    renderer = args.renderer
    output = args.output.resolve()
    keep = args.keep or args.attach
    install_termination_handlers()
    try:
        if args.attach:
            session = load_session(home) or {}
            renderer = session.get('renderer', renderer)
            args.rpc = args.rpc or bool(session.get('rpc'))
            remote = GdbRemote(args.port)
            if session.get('display'):
                keys = XKeys(session['display'])
            if args.rpc:
                rpc = connect_rpc()
            oracle = Oracle(remote, output, keys, emulator_states, saved_states, rpc, renderer, args.rpc_input)
        else:
            image = (args.image or find_image()).resolve()
            azahar = shutil.which('azahar')
            if not azahar:
                raise SystemExit('azahar not found in PATH')
            if load_session(home):
                raise SystemExit('a kept emulator is already recorded in %s; run --stop first' % home)
            if args.rpc and not rpc_port_free():
                raise SystemExit('UDP port %d (the RPC server port, not configurable) is already in use by another '
                                 'emulator' % RPC_PORT)
            if args.fresh and home.exists():
                if not (home / 'config/azahar-emu').is_dir():
                    raise SystemExit('%s does not look like an oracle home; refusing to delete it' % home)
                shutil.rmtree(home)
            config = home / 'config/azahar-emu/qt-config.ini'
            config.parent.mkdir(parents=True, exist_ok=True)
            config.write_text(CONFIG.replace('{renderer}', '0' if args.renderer == 'software' else '1')
                              .replace('{frame_limit}', str(args.frame_limit))
                              .replace('{rpc}', 'true' if args.rpc else 'false')
                              .replace('{single_window}', 'false' if args.display == 'xvfb' else 'true')
                              .replace('{single_default}', 'false' if args.display == 'xvfb' else 'true')
                              .replace('{shortcuts}', shortcut_lines()))
            if args.state:
                place_state(saved_states, args.state, emulator_states)
            launched, oracle = start_emulator(args, home, azahar, image, output, emulator_states, saved_states)
            rpc = oracle.rpc
        run_scenario(oracle, scenario)
    finally:
        if keep and oracle is not None:
            try:
                oracle.unpatch_pad_reader()
                oracle.detach()
            except OSError:
                pass
        if launched is not None and not keep:
            launched.stop()
            session_file(home).unlink(missing_ok=True)
    if keep:
        session = load_session(home) or {}
        print('emulator left running: pid %s port %d display %s (stop with --stop --home %s)'
              % (session.get('pid', '?'), args.port, session.get('display') or 'offscreen', home))
    print('done in %.1f s' % (time.monotonic() - started))


if __name__ == '__main__':
    sys.exit(main())
