# Fetch-backed `requests` for the webassembly simulator.
#
# Mirrors enough of the micropython-lib `requests` API for typical use, but
# performs the request via the browser's fetch() (see simulator/jsfetch.c)
# rather than a raw socket. HTTPS works without `ssl`. Subject to the browser's
# CORS policy, so cross-origin endpoints must send permissive CORS headers.
import _jsfetch


class Response:
    def __init__(self, status, reason, headers, content):
        self.status_code = status
        self.reason = reason
        self.content = content
        self.encoding = "utf-8"
        self.headers = {}
        for line in headers.split("\r\n"):
            if ":" in line:
                key, _, value = line.partition(":")
                self.headers[key.strip()] = value.strip()

    @property
    def text(self):
        return str(self.content, self.encoding)

    def json(self):
        import json
        return json.loads(self.content)

    def close(self):
        self.content = b""


def request(method, url, data=None, json=None, headers=None, auth=None, timeout=None):
    hdr = {}
    if headers:
        hdr.update(headers)

    body = None
    if json is not None:
        import json as _json
        body = _json.dumps(json).encode("utf-8")
        hdr.setdefault("Content-Type", "application/json")
    elif data is not None:
        body = data.encode("utf-8") if isinstance(data, str) else data

    if auth is not None:
        import binascii
        token = binascii.b2a_base64(("%s:%s" % (auth[0], auth[1])).encode()).strip().decode()
        hdr["Authorization"] = "Basic " + token

    header_str = "\n".join("%s: %s" % (k, v) for k, v in hdr.items())
    status, reason, resp_headers, content = _jsfetch.request(method, url, header_str, body)
    return Response(status, reason, resp_headers, content)


def head(url, **kw):
    return request("HEAD", url, **kw)


def get(url, **kw):
    return request("GET", url, **kw)


def post(url, **kw):
    return request("POST", url, **kw)


def put(url, **kw):
    return request("PUT", url, **kw)


def patch(url, **kw):
    return request("PATCH", url, **kw)


def delete(url, **kw):
    return request("DELETE", url, **kw)
