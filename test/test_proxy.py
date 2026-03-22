import concurrent.futures
import hashlib
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from socketserver import ThreadingMixIn
from urllib.parse import urlparse

import requests
from requests.exceptions import ConnectionError, RequestException, Timeout

# ==============================================================================
# CONFIGURATION
# ==============================================================================
PROXY_HOST = "127.0.0.1"
PROXY_PORT = 1080
PROXIES = {
    "http": f"socks5h://{PROXY_HOST}:{PROXY_PORT}",
    "https": f"socks5h://{PROXY_HOST}:{PROXY_PORT}",
}

FILE_SERVER_PORT = 8080
BIND_ADDRESS = "127.0.0.1"
NULL_ROUTE_IP = "192.0.2.1"
DEAD_PORT = 65432

TEST_FILES = {"test_1mb.bin": 1, "test_10mb.bin": 10, "test_100mb.bin": 100}

CONCURRENCY_REQUESTS = 200
CONCURRENCY_WORKERS = 20
CHUNK_SIZE = 64 * 1024


class UI:
    HEADER_WIDTH = 80

    @staticmethod
    def print_header(title):
        print(f"\n{'-' * UI.HEADER_WIDTH}")
        print(f" {title.upper()}")
        print(f"{'-' * UI.HEADER_WIDTH}")

    @staticmethod
    def log(level, message):
        colors = {
            "INFO": "\033[94m",
            "PASS": "\033[92m",
            "FAIL": "\033[91m",
            "WARN": "\033[93m",
            "RESET": "\033[0m",
        }
        color = colors.get(level, colors["RESET"])
        tag = level.center(6)
        print(f"[{color}{tag}{colors['RESET']}] {message}")

    @staticmethod
    def progress_bar(iteration, total, prefix="", length=40):
        percent = ("{0:.1f}").format(100 * (iteration / float(total)))
        filled_length = int(length * iteration // total)
        bar = "█" * filled_length + "-" * (length - filled_length)
        print(f"\r{prefix} |{bar}| {percent}% Complete", end="\r")
        if iteration == total:
            print()

    @staticmethod
    def draw_histogram(data, bins=10):
        if not data:
            return
        min_val, max_val = min(data), max(data)
        bin_width = (max_val - min_val) / bins if max_val > min_val else 1
        histogram = [0] * bins
        for val in data:
            index = min(int((val - min_val) / bin_width), bins - 1)
            histogram[index] += 1

        max_count = max(histogram)
        print("\nLatency Distribution Histogram (ms):")
        for i in range(bins):
            range_start = min_val + i * bin_width
            range_end = range_start + bin_width
            bar_len = int((histogram[i] / max_count) * 40) if max_count > 0 else 0
            print(
                f" {range_start:7.1f} - {range_end:7.1f} | {'█' * bar_len} ({histogram[i]})"
            )
        print()


# ==============================================================================
# DIAGNOSTIC HTTP SERVER
# ==============================================================================
# ThreadingMixIn allows the server to handle the Full-Duplex test without blocking
class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True


class DiagnosticRequestHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass

    def handle_methods(self):
        self.send_response(200)
        self.send_header("Content-Length", str(len(self.command)))
        self.end_headers()
        self.wfile.write(self.command.encode())

    do_PUT = handle_methods
    do_DELETE = handle_methods
    do_PATCH = handle_methods
    do_OPTIONS = handle_methods

    def do_GET(self):
        try:
            parsed_path = urlparse(self.path)

            if parsed_path.path.endswith(".bin"):
                filename = parsed_path.path.lstrip("/")
                if not os.path.exists(filename):
                    self.send_error(404)
                    return

                file_size = os.path.getsize(filename)
                self.send_response(200)
                self.send_header("Content-type", "application/octet-stream")
                self.send_header("Content-Length", str(file_size))
                self.end_headers()

                with open(filename, "rb") as f:
                    while True:
                        buf = f.read(CHUNK_SIZE)
                        if not buf:
                            break
                        self.wfile.write(buf)
                        time.sleep(0.002)

            elif parsed_path.path == "/chunked":
                self.send_response(200)
                self.send_header("Transfer-Encoding", "chunked")
                self.end_headers()
                chunks = [b"Diagnostic", b" ", b"Chunked", b" ", b"Payload"]
                for c in chunks:
                    self.wfile.write(f"{len(c):X}\r\n".encode())
                    self.wfile.write(c + b"\r\n")
                    time.sleep(0.01)
                self.wfile.write(b"0\r\n\r\n")

            elif parsed_path.path == "/huge_headers":
                self.send_response(200)
                self.send_header("X-Huge-Header", "A" * 32000)
                self.send_header("Content-Length", "2")
                self.end_headers()
                self.wfile.write(b"OK")

            elif parsed_path.path == "/drop":
                self.send_response(200)
                self.send_header("Content-Length", "1000000")
                self.end_headers()
                self.wfile.write(b"A" * 50000)
                self.connection.close()

            elif parsed_path.path == "/garbage":
                self.request.sendall(
                    b"HTTP/1.1 200 OK\r\nContent-Length: X\r\nInvalid-Header\r\n\r\n"
                    + os.urandom(1024)
                )
                self.connection.close()

            elif parsed_path.path == "/zero":
                self.send_response(200)
                self.send_header("Content-Length", "0")
                self.end_headers()

            else:
                self.send_response(200)
                self.send_header("Content-Length", "2")
                self.end_headers()
                self.wfile.write(b"OK")

        except (BrokenPipeError, ConnectionResetError):
            pass

    def do_POST(self):
        try:
            if self.path == "/upload":
                content_length = int(self.headers.get("Content-Length", 0))
                sha256_hash = hashlib.sha256()
                bytes_read = 0

                while bytes_read < content_length:
                    chunk = self.rfile.read(
                        min(CHUNK_SIZE, content_length - bytes_read)
                    )
                    if not chunk:
                        break
                    sha256_hash.update(chunk)
                    bytes_read += len(chunk)

                response_data = sha256_hash.hexdigest().encode("utf-8")
                self.send_response(200)
                self.send_header("Content-Length", str(len(response_data)))
                self.end_headers()
                self.wfile.write(response_data)
        except (BrokenPipeError, ConnectionResetError):
            pass


# ==============================================================================
# TEST SUITE
# ==============================================================================
class ProxyTestSuite:
    def __init__(self):
        self.results = []

    def run_all(self):
        self._generate_files()

        server_thread = threading.Thread(target=self._run_server)
        server_thread.daemon = True
        server_thread.start()
        time.sleep(1.0)

        UI.print_header("Initialization & Environment")
        self.test_routing()
        self.test_dns_leak()

        UI.print_header("Layer 4 (Transport) Resilience")
        self.test_connection_refused()
        self.test_blackhole_timeout()

        UI.print_header("Layer 7 (Application) Edge Cases")
        self.test_http_methods()
        self.test_huge_headers()
        self.test_chunked_transfer()
        self.test_zero_byte()
        self.test_premature_disconnect()
        self.test_malformed_response()

        UI.print_header("Throughput & Integrity: Unidirectional")
        for file, size in TEST_FILES.items():
            self.test_download(file, size)

        self.test_upload(10)
        self.test_upload(100)

        UI.print_header("Throughput & Integrity: Full-Duplex")
        self.test_bidirectional_stress(10)

        UI.print_header("Concurrency & Multiplexing Stress Test")
        self.test_multiplexing()

        self._print_summary()
        self._cleanup()

    def _generate_files(self):
        UI.log("INFO", "Preparing cryptographic test vectors...")
        for filename, size_mb in TEST_FILES.items():
            if not os.path.exists(filename):
                with open(filename, "wb") as f:
                    f.write(os.urandom(size_mb * 1024 * 1024))

    def _run_server(self):
        httpd = ThreadedHTTPServer(
            (BIND_ADDRESS, FILE_SERVER_PORT), DiagnosticRequestHandler
        )
        httpd.serve_forever()

    def _get_local_sha256(self, filename):
        h = hashlib.sha256()
        with open(filename, "rb") as f:
            for block in iter(lambda: f.read(65536), b""):
                h.update(block)
        return h.hexdigest()

    def _record(self, name, status, details=""):
        self.results.append({"name": name, "status": status, "details": details})
        UI.log(status, f"{name:.<42} [{status}] {details}")

    def test_routing(self):
        try:
            r_direct = requests.get("https://api.ipify.org", timeout=5).text
            r_proxy = requests.get(
                "https://api.ipify.org", proxies=PROXIES, timeout=10
            ).text
            if r_direct != r_proxy:
                self._record("External IP Routing", "PASS", f"Masked ({r_proxy})")
            else:
                self._record(
                    "External IP Routing", "WARN", "Unchanged (Expected in Loopback)"
                )
        except Exception as e:
            self._record("External IP Routing", "FAIL", str(e))

    def test_dns_leak(self):
        try:
            requests.get(
                "http://invalid-domain.internal.test", proxies=PROXIES, timeout=5
            )
            self._record("Remote DNS Resolution", "FAIL", "Resolved unexpectedly")
        except ConnectionError:
            self._record("Remote DNS Resolution", "PASS", "SOCKS5 CMD 0x03 evaluated")
        except Exception as e:
            self._record(
                "Remote DNS Resolution", "WARN", f"Exception: {type(e).__name__}"
            )

    def test_connection_refused(self):
        try:
            requests.get(
                f"http://{BIND_ADDRESS}:{DEAD_PORT}", proxies=PROXIES, timeout=5
            )
            self._record("TCP Connection Refused", "FAIL", "Connected to dead port")
        except ConnectionError:
            self._record(
                "TCP Connection Refused", "PASS", "Proxy rejected connection gracefully"
            )
        except Exception as e:
            self._record(
                "TCP Connection Refused",
                "FAIL",
                f"Unhandled exception: {type(e).__name__}",
            )

    def test_blackhole_timeout(self):
        try:
            requests.get(f"http://{NULL_ROUTE_IP}", proxies=PROXIES, timeout=3)
            self._record(
                "TCP Blackhole Timeout", "FAIL", "Request succeeded unexpectedly"
            )
        except (Timeout, ConnectionError):
            self._record(
                "TCP Blackhole Timeout", "PASS", "Handled unroutable destination"
            )

    def test_http_methods(self):
        methods = ["PUT", "DELETE", "PATCH", "OPTIONS"]
        success = True
        try:
            for method in methods:
                r = requests.request(
                    method,
                    f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/",
                    proxies=PROXIES,
                    timeout=5,
                )
                if r.text != method:
                    success = False
            if success:
                self._record("HTTP Methods Forwarding", "PASS", "All methods preserved")
            else:
                self._record(
                    "HTTP Methods Forwarding", "FAIL", "Method mutation detected"
                )
        except Exception as e:
            self._record("HTTP Methods Forwarding", "FAIL", str(e))

    def test_huge_headers(self):
        try:
            r = requests.get(
                f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/huge_headers",
                proxies=PROXIES,
                timeout=5,
            )
            if len(r.headers.get("X-Huge-Header", "")) == 32000:
                self._record(
                    "Huge HTTP Header Handling", "PASS", "Buffer limits nominal"
                )
            else:
                self._record("Huge HTTP Header Handling", "FAIL", "Header truncated")
        except Exception as e:
            self._record("Huge HTTP Header Handling", "FAIL", str(e))

    def test_chunked_transfer(self):
        try:
            r = requests.get(
                f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/chunked",
                proxies=PROXIES,
                timeout=5,
            )
            if r.text == "Diagnostic Chunked Payload":
                self._record(
                    "Chunked Transfer Encoding", "PASS", "Reassembled successfully"
                )
            else:
                self._record("Chunked Transfer Encoding", "FAIL", "Reassembly failed")
        except Exception as e:
            self._record("Chunked Transfer Encoding", "FAIL", str(e))

    def test_zero_byte(self):
        try:
            r = requests.get(
                f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/zero",
                proxies=PROXIES,
                timeout=5,
            )
            if r.status_code == 200 and len(r.content) == 0:
                self._record(
                    "Zero-Byte Payload Transfer", "PASS", "Processed accurately"
                )
            else:
                self._record(
                    "Zero-Byte Payload Transfer", "FAIL", "Invalid length received"
                )
        except Exception as e:
            self._record("Zero-Byte Payload Transfer", "FAIL", str(e))

    def test_premature_disconnect(self):
        try:
            requests.get(
                f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/drop",
                proxies=PROXIES,
                timeout=5,
            )
            self._record("Abrupt TCP RST Handling", "FAIL", "Succeeded unexpectedly")
        except (requests.exceptions.ChunkedEncodingError, ConnectionError):
            self._record(
                "Abrupt TCP RST Handling", "PASS", "Caught RST segment cleanly"
            )
        except Exception as e:
            self._record(
                "Abrupt TCP RST Handling",
                "WARN",
                f"Caught generic drop: {type(e).__name__}",
            )

    def test_malformed_response(self):
        try:
            r = requests.get(
                f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/garbage",
                proxies=PROXIES,
                timeout=5,
            )
            if len(r.content) > 500:
                self._record(
                    "Malformed HTTP Forwarding",
                    "PASS",
                    "Transparent L4 delivery maintained",
                )
            else:
                self._record(
                    "Malformed HTTP Forwarding",
                    "FAIL",
                    f"Payload truncated: {len(r.content)} bytes",
                )
        except RequestException:
            self._record(
                "Malformed HTTP Forwarding",
                "PASS",
                "Client rejected L7 layer correctly",
            )
        except Exception as e:
            self._record(
                "Malformed HTTP Forwarding", "WARN", f"Aborted: {type(e).__name__}"
            )

    def test_download(self, filename, size_mb):
        target_url = f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/{filename}"
        expected_hash = self._get_local_sha256(filename)
        dl_filename = f"dl_{filename}"

        start_t = time.time()
        try:
            with requests.get(
                target_url, proxies=PROXIES, stream=True, timeout=30
            ) as r:
                r.raise_for_status()
                downloaded = 0
                with open(dl_filename, "wb") as f:
                    for chunk in r.iter_content(chunk_size=CHUNK_SIZE):
                        if chunk:
                            f.write(chunk)
                            downloaded += len(chunk)
                            UI.progress_bar(
                                downloaded,
                                size_mb * 1024 * 1024,
                                prefix=f"DL {size_mb:>3}MB",
                            )

            duration = time.time() - start_t
            mbps = (size_mb * 8) / duration
            actual_hash = self._get_local_sha256(dl_filename)

            if expected_hash == actual_hash:
                self._record(
                    f"DL {size_mb}MB Integrity Validation", "PASS", f"{mbps:.2f} Mbps"
                )
            else:
                self._record(
                    f"DL {size_mb}MB Integrity Validation", "FAIL", "SHA256 mismatch"
                )
        except Exception as e:
            self._record(
                f"DL {size_mb}MB Integrity Validation",
                "FAIL",
                f"Exception: {type(e).__name__}",
            )
        finally:
            if os.path.exists(dl_filename):
                os.remove(dl_filename)

    def test_upload(self, size_mb):
        target_url = f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/upload"
        payload = os.urandom(size_mb * 1024 * 1024)
        expected_hash = hashlib.sha256(payload).hexdigest()

        start_t = time.time()
        try:
            r = requests.post(target_url, data=payload, proxies=PROXIES, timeout=120)
            duration = time.time() - start_t
            mbps = (size_mb * 8) / duration

            if r.status_code == 200 and r.text == expected_hash:
                self._record(
                    f"UP {size_mb}MB Integrity Validation", "PASS", f"{mbps:.2f} Mbps"
                )
            else:
                self._record(
                    f"UP {size_mb}MB Integrity Validation",
                    "FAIL",
                    "SHA256 mismatch on target",
                )
        except Exception as e:
            self._record(
                f"UP {size_mb}MB Integrity Validation",
                "FAIL",
                f"Exception: {type(e).__name__}",
            )

    def test_bidirectional_stress(self, size_mb):
        def up_task():
            payload = os.urandom(size_mb * 1024 * 1024)
            h = hashlib.sha256(payload).hexdigest()
            r = requests.post(
                f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/upload",
                data=payload,
                proxies=PROXIES,
                timeout=120,
            )
            return r.status_code == 200 and r.text == h

        def down_task():
            target_url = (
                f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/test_{size_mb}mb.bin"
            )
            h = hashlib.sha256()
            with requests.get(
                target_url, proxies=PROXIES, stream=True, timeout=120
            ) as r:
                for chunk in r.iter_content(chunk_size=CHUNK_SIZE):
                    if chunk:
                        h.update(chunk)
            return h.hexdigest() == self._get_local_sha256(f"test_{size_mb}mb.bin")

        start_t = time.time()
        try:
            with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
                f_up = executor.submit(up_task)
                f_down = executor.submit(down_task)

                up_pass = f_up.result()
                down_pass = f_down.result()

            duration = time.time() - start_t
            mbps = ((size_mb * 2) * 8) / duration

            if up_pass and down_pass:
                self._record(
                    f"Full-Duplex {size_mb}MB I/O",
                    "PASS",
                    f"Agg. Throughput: {mbps:.2f} Mbps",
                )
            else:
                self._record(
                    f"Full-Duplex {size_mb}MB I/O",
                    "FAIL",
                    f"UP: {up_pass}, DOWN: {down_pass}",
                )
        except Exception as e:
            self._record(f"Full-Duplex {size_mb}MB I/O", "FAIL", str(e))

    def test_multiplexing(self):
        latencies = []
        success_count = 0

        def task():
            start = time.perf_counter()
            try:
                r = requests.get(
                    f"http://{BIND_ADDRESS}:{FILE_SERVER_PORT}/",
                    proxies=PROXIES,
                    timeout=10,
                )
                if r.status_code == 200:
                    return True, (time.perf_counter() - start) * 1000
            except:
                pass
            return False, 0

        with concurrent.futures.ThreadPoolExecutor(
            max_workers=CONCURRENCY_WORKERS
        ) as executor:
            futures = [executor.submit(task) for _ in range(CONCURRENCY_REQUESTS)]
            for i, future in enumerate(concurrent.futures.as_completed(futures)):
                success, lat = future.result()
                if success:
                    success_count += 1
                    latencies.append(lat)
                UI.progress_bar(i + 1, CONCURRENCY_REQUESTS, prefix="Reqs    ")

        if success_count == CONCURRENCY_REQUESTS:
            self._record(
                "Concurrent Multiplexing Payload",
                "PASS",
                f"100% Success ({success_count}/{CONCURRENCY_REQUESTS})",
            )
        else:
            self._record(
                "Concurrent Multiplexing Payload",
                "FAIL",
                f"Dropped {CONCURRENCY_REQUESTS - success_count} requests",
            )

        UI.draw_histogram(latencies)

    def _print_summary(self):
        UI.print_header("Diagnostic Execution Summary")
        passed = sum(1 for r in self.results if r["status"] == "PASS")
        warns = sum(1 for r in self.results if r["status"] == "WARN")
        fails = sum(1 for r in self.results if r["status"] == "FAIL")
        total = len(self.results)

        for r in self.results:
            print(f" * {r['name']:.<45} [{r['status']}]")

        print("\nExecution Metrics:")
        print(
            f" TOTAL: {total} | \033[92mPASS: {passed}\033[0m | \033[93mWARN: {warns}\033[0m | \033[91mFAIL: {fails}\033[0m"
        )

        if fails == 0:
            print("\n>> ALL DIAGNOSTIC TESTS COMPLETED SUCCESSFULLY <<\n")
        else:
            print("\n>> ERRORS DETECTED DURING DIAGNOSTIC EXECUTION <<\n")

    def _cleanup(self):
        UI.log("INFO", "Purging cryptographic test vectors from local storage...")
        for f in TEST_FILES.keys():
            if os.path.exists(f):
                os.remove(f)
        for f in ["dl_test_1mb.bin", "dl_test_10mb.bin", "dl_test_100mb.bin"]:
            if os.path.exists(f):
                os.remove(f)


if __name__ == "__main__":
    suite = ProxyTestSuite()
    try:
        suite.run_all()
    except KeyboardInterrupt:
        print("\n\n[FAIL] Diagnostics aborted by operator intervention.")
