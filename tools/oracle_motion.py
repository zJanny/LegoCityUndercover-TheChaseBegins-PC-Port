#!/usr/bin/env python3
"""Virtual motion source for the Azahar oracle: a Cemuhook DSU (protocol 1001) UDP server.

usage: oracle_motion.py serve --port PORT --state FILE

Sends a steady motion signal for pad slot 0 to every client that registered for pad data. The signal comes from
the JSON file FILE ({"accel": [x, y, z], "gyro": [pitch, yaw, roll]}, accel in g, gyro in degrees per second),
which is re-read when it changes; the oracle script commands 'tilt' and 'touch' write it. The optional key "touch"
([x, y] in bottom-screen pixels, or null) is sent as the first touch point of the pad. Protocol and axis findings:
docs/re/oracle-setup.md.
"""
import argparse
import json
import os
import re
import select
import socket
import struct
import sys
import time
import zlib
from pathlib import Path

PROTOCOL = 1001
HEADER_SIZE = 16
MAGIC_SERVER = b'DSUS'
MAGIC_CLIENT = b'DSUC'
TYPE_VERSION = 0x100000
TYPE_PORTS = 0x100001
TYPE_PAD_DATA = 0x100002
PAD_DATA_SIZE = 100
SEND_INTERVAL = 0.01
CLIENT_TIMEOUT = 30.0
SERVER_ID = 0x4f524143
SLOT_COUNT = 4
MAC = bytes([0x02, 0x00, 0x00, 0x00, 0x00, 0x01])
LEVEL_ACCEL = (0.0, 0.0, 0.0)
LEVEL_GYRO = (0.0, 0.0, 0.0)
GRAVITY_AXIS = 2
MAX_ACCEL = 8.0
MAX_GYRO = 2000.0
SCREEN_WIDTH = 320
SCREEN_HEIGHT = 240
TOUCH_PAD_SCALE = 10
TOUCH_PAD_WIDTH = SCREEN_WIDTH * TOUCH_PAD_SCALE
TOUCH_PAD_HEIGHT = SCREEN_HEIGHT * TOUCH_PAD_SCALE
TOUCH_ID = 1
MAX_TOUCH_UPDATES = 600
DEFAULT_TAP_UPDATES = 6
DEFAULT_DRAG_UPDATES = 10
MIN_DRAG_UPDATES = 2


class MotionError(ValueError):
    pass


def level_state():
    accel = [0.0, 0.0, 0.0]
    accel[GRAVITY_AXIS] = -1.0
    return {'accel': accel, 'gyro': list(LEVEL_GYRO)}


def parse_vector(words, limit, name):
    if len(words) != 3:
        raise MotionError('%s needs 3 numbers, got %d' % (name, len(words)))
    values = []
    for word in words:
        try:
            value = float(word)
        except ValueError:
            raise MotionError('%s: %r is not a number' % (name, word)) from None
        if value != value or abs(value) > limit:
            raise MotionError('%s: %r is outside -%g..%g' % (name, word, limit, limit))
        values.append(value)
    return values


def parse_tilt(words):
    """Script arguments 'AX AY AZ [GP GY GR]' to a signal state; the gyro defaults to zero."""
    if len(words) not in (3, 6):
        raise MotionError('tilt needs 3 accel values (g) and optionally 3 gyro values (deg/s), got %d' % len(words))
    return {'accel': parse_vector(words[:3], MAX_ACCEL, 'accel'),
            'gyro': parse_vector(words[3:], MAX_GYRO, 'gyro') if words[3:] else list(LEVEL_GYRO)}


def parse_integer(word, name, low, high):
    if not re.fullmatch(r'-?[0-9]+', word):
        raise MotionError('%s: %r is not an integer' % (name, word))
    value = int(word)
    if not low <= value <= high:
        raise MotionError('%s: %r is outside %d..%d' % (name, word, low, high))
    return value


def parse_point(words, name):
    return (parse_integer(words[0], name + ' x', 0, SCREEN_WIDTH - 1),
            parse_integer(words[1], name + ' y', 0, SCREEN_HEIGHT - 1))


def parse_touch(words):
    """Script arguments 'X Y [UPDATES]' (a tap) or 'release' to a touch plan: a list of (point or None) per update."""
    if words[:1] == ['release']:
        if len(words) != 1:
            raise MotionError('touch release takes no arguments, got %r' % ' '.join(words[1:]))
        return []
    if len(words) not in (2, 3):
        raise MotionError('touch needs X Y [UPDATES] or release, got %d words' % len(words))
    point = parse_point(words[:2], 'touch')
    updates = parse_integer(words[2], 'touch updates', 1, MAX_TOUCH_UPDATES) if words[2:] else DEFAULT_TAP_UPDATES
    return [point] * updates


def parse_drag(words):
    """Script arguments 'X1 Y1 X2 Y2 [UPDATES]' to a touch plan: the points of the drag, one per update."""
    if len(words) not in (4, 5):
        raise MotionError('drag needs X1 Y1 X2 Y2 [UPDATES], got %d words' % len(words))
    start = parse_point(words[:2], 'drag start')
    end = parse_point(words[2:4], 'drag end')
    updates = (parse_integer(words[4], 'drag updates', MIN_DRAG_UPDATES, MAX_TOUCH_UPDATES) if words[4:]
               else DEFAULT_DRAG_UPDATES)
    steps = updates - 1
    return [tuple(a + (2 * (b - a) * index + steps) // (2 * steps) for a, b in zip(start, end))
            for index in range(updates)]


def touch_pad_point(pixel_point):
    """The pad position that Azahar maps back onto the middle of the bottom-screen pixel."""
    return tuple(pixel * TOUCH_PAD_SCALE + TOUCH_PAD_SCALE // 2 for pixel in pixel_point)


def load_state(text):
    try:
        data = json.loads(text)
        accel = [float(v) for v in data['accel']]
        gyro = [float(v) for v in data['gyro']]
        touch = data.get('touch')
        if touch is not None:
            touch = [int(touch[0]), int(touch[1])]
    except (ValueError, KeyError, TypeError, IndexError, AttributeError) as error:
        raise MotionError('bad motion state: %s' % error) from None
    if len(accel) != 3 or len(gyro) != 3:
        raise MotionError('bad motion state: accel and gyro need 3 numbers each')
    if touch is not None and not (0 <= touch[0] < SCREEN_WIDTH and 0 <= touch[1] < SCREEN_HEIGHT):
        raise MotionError('bad motion state: touch %s is outside the %dx%d screen' % (touch, SCREEN_WIDTH, SCREEN_HEIGHT))
    state = {'accel': accel, 'gyro': gyro}
    if touch is not None:
        state['touch'] = touch
    return state


def write_state(path, state):
    path = Path(path)
    temporary = path.with_name(path.name + '.new')
    temporary.write_text(json.dumps(state))
    os.replace(temporary, path)


def update_state(path, **changes):
    try:
        state = load_state(Path(path).read_text())
    except (OSError, MotionError):
        state = level_state()
    state.update(changes)
    if state.get('touch') is None:
        state.pop('touch', None)
    write_state(path, state)
    return state


def finish_packet(magic, body):
    packet = bytearray(magic + struct.pack('<HHII', PROTOCOL, len(body), 0, SERVER_ID) + body)
    struct.pack_into('<I', packet, 8, zlib.crc32(bytes(packet)))
    return bytes(packet)


def parse_request(datagram):
    """Validate a client datagram and return (message type, body after the type)."""
    if len(datagram) < HEADER_SIZE + 4:
        raise MotionError('datagram of %d bytes is shorter than a request' % len(datagram))
    magic, version, length, crc, _ = struct.unpack_from('<4sHHII', datagram)
    if magic != MAGIC_CLIENT:
        raise MotionError('bad magic %r' % magic)
    if version != PROTOCOL:
        raise MotionError('protocol version %d, expected %d' % (version, PROTOCOL))
    if length != len(datagram) - HEADER_SIZE:
        raise MotionError('length field %d does not match the %d payload bytes' % (length, len(datagram) - HEADER_SIZE))
    zeroed = datagram[:8] + b'\0\0\0\0' + datagram[12:]
    if zlib.crc32(zeroed) != crc:
        raise MotionError('CRC mismatch')
    return struct.unpack_from('<I', datagram, HEADER_SIZE)[0], datagram[HEADER_SIZE + 4:]


def build_request(message_type, body=b''):
    payload = struct.pack('<I', message_type) + body
    packet = bytearray(MAGIC_CLIENT + struct.pack('<HHII', PROTOCOL, len(payload), 0, 0x12345678) + payload)
    struct.pack_into('<I', packet, 8, zlib.crc32(bytes(packet)))
    return bytes(packet)


def shared_response(slot):
    connected = slot == 0
    return struct.pack('<BBBB6sB', slot, 2 if connected else 0, 2 if connected else 0, 2 if connected else 0,
                       MAC if connected else bytes(6), 5 if connected else 0)


def version_packet():
    return finish_packet(MAGIC_SERVER, struct.pack('<IH', TYPE_VERSION, PROTOCOL))


def ports_packet(slot):
    return finish_packet(MAGIC_SERVER, struct.pack('<I', TYPE_PORTS) + shared_response(slot) + b'\0')


def pad_data_packet(counter, microseconds, state):
    body = struct.pack('<I', TYPE_PAD_DATA) + shared_response(0)
    body += struct.pack('<BI', 1, counter)
    body += bytes(2) + bytes(1) + bytes(1)
    body += bytes([128, 128, 128, 128])
    body += bytes(4) + bytes(8)
    touch = state.get('touch')
    if touch:
        x, y = touch_pad_point(touch)
        body += struct.pack('<BBHH', 1, TOUCH_ID, x, y)
    else:
        body += bytes(6)
    body += bytes(6)
    body += struct.pack('<Q', microseconds)
    body += struct.pack('<3f', *state['accel']) + struct.pack('<3f', *state['gyro'])
    packet = finish_packet(MAGIC_SERVER, body)
    assert len(packet) == PAD_DATA_SIZE
    return packet


def answer(datagram):
    """The immediate reply packets for one client datagram, and whether it registers for pad data."""
    message_type, body = parse_request(datagram)
    if message_type == TYPE_VERSION:
        return [version_packet()], False
    if message_type == TYPE_PORTS:
        if len(body) < 4:
            raise MotionError('ports request without a slot count')
        count = struct.unpack_from('<i', body)[0]
        if not 0 <= count <= SLOT_COUNT or len(body) not in (4 + count, 4 + SLOT_COUNT):
            raise MotionError('ports request with slot count %d and %d slot bytes' % (count, len(body) - 4))
        return [ports_packet(slot) for slot in body[4:4 + count] if slot < SLOT_COUNT], False
    if message_type == TYPE_PAD_DATA:
        return [], True
    raise MotionError('unknown message type 0x%x' % message_type)


def read_state(path, previous):
    try:
        return load_state(Path(path).read_text())
    except (OSError, MotionError) as error:
        print('motion state not read: %s' % error, file=sys.stderr, flush=True)
        return previous


def serve(port, state_path):
    state = read_state(state_path, level_state())
    modified = os.stat(state_path).st_mtime_ns
    server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 0)
    server.bind(('127.0.0.1', port))
    clients = {}
    counter = 0
    started = time.monotonic()
    next_send = started
    print('motion source on 127.0.0.1:%d' % port, flush=True)
    while True:
        now = time.monotonic()
        ready, _, _ = select.select([server], [], [], max(0.0, next_send - now))
        if ready:
            datagram, address = server.recvfrom(1024)
            try:
                replies, registers = answer(datagram)
            except MotionError as error:
                print('rejected datagram from %s: %s' % (address, error), file=sys.stderr, flush=True)
                continue
            for reply in replies:
                server.sendto(reply, address)
            if registers:
                if address not in clients:
                    print('pad data client %s registered' % (address,), flush=True)
                clients[address] = time.monotonic()
        now = time.monotonic()
        if now < next_send:
            continue
        next_send = now + SEND_INTERVAL
        try:
            changed = os.stat(state_path).st_mtime_ns
        except OSError:
            changed = modified
        if changed != modified:
            modified = changed
            state = read_state(state_path, state)
        for address in [a for a, seen in clients.items() if now - seen > CLIENT_TIMEOUT]:
            del clients[address]
        if clients:
            counter += 1
            packet = pad_data_packet(counter, int((now - started) * 1e6), state)
            for address in clients:
                server.sendto(packet, address)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    commands = parser.add_subparsers(dest='command', required=True)
    serve_parser = commands.add_parser('serve')
    serve_parser.add_argument('--port', type=int, required=True)
    serve_parser.add_argument('--state', type=Path, required=True)
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        parser.error('--port %d is outside 1024..65535' % args.port)
    if not args.state.is_file():
        parser.error('--state %s is not a file' % args.state)
    serve(args.port, args.state)


if __name__ == '__main__':
    sys.exit(main())
