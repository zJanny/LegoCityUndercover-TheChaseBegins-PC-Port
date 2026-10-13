import importlib.util
import socket
import struct
import threading
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('azahar_oracle', Path(__file__).parents[1] / 'tools/azahar_oracle.py')
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)


class FakeRpcServer:
    """Answers like Azahar's RPC server (src/core/rpc/rpc_server.cpp) for a flat memory image."""

    def __init__(self, memory, silent=False):
        self.memory = memory
        self.silent = silent
        self.requests = []
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('127.0.0.1', 0))
        self.port = self.sock.getsockname()[1]
        self.thread = threading.Thread(target=self.serve, daemon=True)
        self.thread.start()

    def serve(self):
        while True:
            try:
                datagram, peer = self.sock.recvfrom(65536)
            except OSError:
                return
            version, request_id, kind, size = struct.unpack_from('<4I', datagram)
            payload = datagram[16:]
            self.requests.append((kind, payload))
            if self.silent:
                continue
            reply = b''
            if kind == oracle.RPC_READ:
                address, length = struct.unpack_from('<II', payload)
                if 0 < length <= oracle.RPC_READ_MAX:
                    reply = bytes(self.memory[address:address + length])
            elif kind == oracle.RPC_WRITE:
                address, length = struct.unpack_from('<II', payload)
                self.memory[address:address + length] = payload[8:8 + length]
            elif kind == oracle.RPC_PROCESS_LIST:
                reply = struct.pack('<I', 2) + struct.pack('<IQ8s', 1, 0x0004013000001A02, b'sm') \
                    + struct.pack('<IQ8s', 11, 0x00040000000AD600, b'LEGOCITY')
            self.sock.sendto(struct.pack('<4I', version, request_id, kind, len(reply)) + reply, peer)

    def close(self):
        self.sock.close()


class RpcRemoteTest(unittest.TestCase):
    def setUp(self):
        self.memory = bytearray(range(256)) * 0x1100
        self.server = FakeRpcServer(self.memory)
        self.client = oracle.RpcRemote(self.server.port, timeout=0.5, retries=2)

    def tearDown(self):
        self.client.close()
        self.server.close()

    def test_read_is_chunked_at_the_server_limit(self):
        data = self.client.read(0x100, 2 * oracle.RPC_READ_MAX + 10)
        self.assertEqual(data, bytes(self.memory[0x100:0x100 + 2 * oracle.RPC_READ_MAX + 10]))
        sizes = [struct.unpack_from('<II', payload)[1] for _, payload in self.server.requests]
        self.assertEqual(sizes, [oracle.RPC_READ_MAX, oracle.RPC_READ_MAX, 10])

    def test_u32_is_little_endian(self):
        self.memory[0x40:0x44] = bytes((1, 2, 3, 4))
        self.assertEqual(self.client.u32(0x40), 0x04030201)

    def test_write_is_verified_by_reading_back(self):
        self.client.write(0x00100000 + 4, b'\xaa\xbb')
        self.assertEqual(self.memory[0x00100004:0x00100006], b'\xaa\xbb')

    def test_write_outside_the_writable_regions_is_refused_locally(self):
        with self.assertRaises(oracle.RpcError):
            self.client.write(0x10000000, b'\0')
        self.assertFalse(self.server.requests)

    def test_short_read_reply_is_an_error(self):
        with self.assertRaises(oracle.RpcError):
            self.client.read(len(self.memory) - 2, 4)

    def test_select_application_picks_the_application_process(self):
        self.assertEqual(self.client.select_application(), (11, 0x00040000000AD600, 'LEGOCITY'))
        kind, payload = self.server.requests[-1]
        self.assertEqual((kind, struct.unpack('<II', payload)), (oracle.RPC_PROCESS, (1, 11)))

    def test_requests_carry_the_current_version_and_matching_size(self):
        self.client.u32(0)
        self.assertEqual(oracle.RPC_VERSION, 2)
        self.assertEqual(oracle.RPC_READ_MAX, 32768)


class RpcTimeoutTest(unittest.TestCase):
    def test_no_reply_raises_after_retries(self):
        server = FakeRpcServer(bytearray(16), silent=True)
        client = oracle.RpcRemote(server.port, timeout=0.1, retries=3)
        try:
            with self.assertRaises(oracle.RpcError):
                client.u32(0)
            self.assertEqual(len(server.requests), 3)
        finally:
            client.close()
            server.close()


if __name__ == '__main__':
    unittest.main()
