#!/usr/bin/env python3
"""Virtual motion source for the Azahar oracle: a Cemuhook DSU (protocol 1001) UDP server.

usage: oracle_motion.py serve --port PORT --state FILE

Sends a steady motion signal for pad slot 0 to every client that registered for pad data. The signal comes from
the JSON file FILE ({"accel": [x, y, z], "gyro": [pitch, yaw, roll]}, accel in g, gyro in degrees per second),
which is re-read when it changes; the oracle script command 'tilt' writes it. Protocol and axis findings: docs/re/oracle-setup.md.
"""
import argparse
import json
import os
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


def load_state(text):
    try:
        data = json.loads(text)
        accel = [float(v) for v in data['accel']]
        gyro = [float(v) for v in data['gyro']]
    except (ValueError, KeyError, TypeError) as error:
        raise MotionError('bad motion state: %s' % error) from None
    if len(accel) != 3 or len(gyro) != 3:
        raise MotionError('bad motion state: accel and gyro need 3 numbers each')
    return {'accel': accel, 'gyro': gyro}


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
    body += bytes(12)
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
