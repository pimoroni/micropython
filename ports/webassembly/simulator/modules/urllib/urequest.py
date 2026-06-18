# Fetch-backed urllib.urequest.urlopen for the webassembly simulator.
#
# A drop-in for micropython-lib's urllib.urequest, but the request goes through
# the browser's fetch() (see simulator/jsfetch.c) instead of a raw socket. The
# returned object exposes the usual read()/readinto()/readline()/close() stream
# interface over the response body, plus a `.status` attribute.
import _jsfetch


class _Response:
    def __init__(self, status, content):
        self.status = status
        self._buf = content
        self._pos = 0

    def read(self, size=-1):
        if size is None or size < 0:
            chunk = self._buf[self._pos:]
            self._pos = len(self._buf)
        else:
            chunk = self._buf[self._pos:self._pos + size]
            self._pos += len(chunk)
        return chunk

    def readinto(self, buf):
        chunk = self.read(len(buf))
        buf[:len(chunk)] = chunk
        return len(chunk)

    def readline(self):
        nl = self._buf.find(b"\n", self._pos)
        if nl < 0:
            return self.read()
        line = self._buf[self._pos:nl + 1]
        self._pos = nl + 1
        return line

    def close(self):
        self._buf = b""
        self._pos = 0


def urlopen(url, data=None, method="GET"):
    if data is not None and method == "GET":
        method = "POST"
    body = data
    if isinstance(body, str):
        body = body.encode("utf-8")
    status, reason, headers, content = _jsfetch.request(method, url, "", body)
    return _Response(status, content)
