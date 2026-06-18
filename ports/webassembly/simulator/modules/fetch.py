import io
import time
import json
import _jsfetch


class HTTPException(Exception):
    def __init__(self, fetch):
        self.fetch = fetch
        super().__init__(f"HTTP error {fetch.http_status}")


class AsyncFetch:
    """Cooperative, streaming HTTP client for the simulator.

    A shim with the same interface as the raw-socket AsyncFetch, but backed by
    the browser's Fetch API via the _jsfetch C module. The response body is read
    incrementally (a chunk per update() call) and streamed to a file or an
    in-memory buffer, so it plays nicely with a frame loop and never blocks the
    whole download in one step.
    """

    FETCH_BLOCK_SIZE = 1024
    buffer = bytearray(FETCH_BLOCK_SIZE)

    STATUS_TEXT = ["Idle", "Fetching", "Done", "Error"]
    IDLE = 0
    FETCHING = 1
    DONE = 2
    ERROR = 3

    # _jsfetch.stream_phase() values.
    _PHASE_PENDING = 0
    _PHASE_READY = 1
    _PHASE_ERROR = 3

    def __init__(self, host, port=None, use_tls=True, debug=False):
        self._debug = debug

        self._fetch = None  # hold the pending fetch generator

        self._last_update = None  # keep track of the last updated time

        self._buffer = None  # buffer for BytesIO
        self._buffer_len = 0  # amount of data actually in our buffer

        self._interval = 0  # fetch interval in seconds
        self._path = None
        self._file = None  # Target file, or none to fetch to a stream

        self._on_complete = None
        self._on_error = None

        self._data = None
        self._method = "GET"

        self._headers = {}
        self._response_headers = {}
        self._status_code = None
        self._content_length = 0

        self._use_tls = use_tls

        self._host = host
        self._port = port or (443 if use_tls else 80)

        self._status = AsyncFetch.IDLE

    def fetch(self, path, file=None, interval=None, headers=None, data=None, method=None, blocking=False):
        if self._fetch:
            raise RuntimeError("Cannot interrupt a running fetch...")

        if path.startswith(("http://", "https://")):
            raise ValueError("fetch requires a relative path, not a full URL.")

        self._buffer_len = 0

        if interval is not None:
            self._interval = interval

        # Make sure we re-trigger if the interval is 0 (no-repeat)
        if self._interval == 0:
            self._last_update = None

        if headers is not None:
            self._headers = headers

        self._path = path[1:] if path.startswith("/") else path
        self._file = file
        self._data = data

        if method is not None:
            self._method = method

        if "User-Agent" not in self._headers:
            self._headers["User-Agent"] = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.4 Safari/605.1.15"

        if blocking:
            self.finish()
            return self.stream

    def _url(self):
        scheme = "https" if self._use_tls else "http"
        netloc = self._host if self._port in (80, 443) else "%s:%d" % (self._host, self._port)
        return "%s://%s/%s" % (scheme, netloc, self._path)

    def _http_fetch(self):
        self._status_code = None
        self._response_headers = {}

        if self._data is not None and self._method == "GET":
            self._method = "POST"

        header_str = "\n".join("%s: %s" % (k, v) for k, v in self._headers.items())
        _jsfetch.stream_start(self._method, self._url(), header_str, self._data)

        # Wait (cooperatively) for the response headers to arrive.
        while _jsfetch.stream_phase() == AsyncFetch._PHASE_PENDING:
            yield

        if _jsfetch.stream_phase() == AsyncFetch._PHASE_ERROR:
            raise OSError(_jsfetch.stream_error())

        self._status_code, _reason, header_str = _jsfetch.stream_response()
        for line in header_str.split("\r\n"):
            if ": " in line:
                k, _, v = line.partition(": ")
                self._response_headers[k] = v

        self._content_length = int(self._response_headers.get("Content-Length", 0))

        if self._debug:
            print(f"Got status {self._status_code}, {self._content_length} bytes")

    def _fetch_to_stream(self):
        # Grab the headers
        yield from self._http_fetch()

        self._buffer_len = 0

        if self._file:
            stream = open(self._file, "wb")
            if self._debug:
                print(f"Streaming to {self._file}")
        else:
            if self._buffer is None:
                self._buffer = io.BytesIO()
            stream = self._buffer
            stream.seek(0)
            if self._debug:
                print("Streaming to buffer")

        # Pull the body a chunk at a time, until the reader is exhausted.
        while not _jsfetch.stream_done():
            yield
            length = _jsfetch.stream_readinto(AsyncFetch.buffer)
            if length in (0, None):
                continue
            self._buffer_len += length
            if self._content_length:
                self._content_length = max(0, self._content_length - length)
            if self._debug:
                print(f"Fetched {self._buffer_len} bytes")
            stream.write(AsyncFetch.buffer[:length])

        # Leave the BytesIO stream open
        if self._file:
            stream.close()

    def update(self):
        self._status = AsyncFetch.IDLE

        if self._path is not None:
            if self._last_update is None or (self._interval > 0 and self.duration > int(self._interval * 1000)):
                # Don't overwrite an existing fetch operation if the interval comes up...
                if self._fetch is None and self._debug:
                    print("Fetch started")
                self._fetch = self._fetch or self._fetch_to_stream()

            if self._fetch:
                try:
                    next(self._fetch)
                    self._status = AsyncFetch.FETCHING

                except StopIteration:
                    if self._debug:
                        print("Fetch done")
                    self._last_update = time.ticks_ms()
                    self._fetch = None

                    if self._status_code == 200:
                        self._status = AsyncFetch.DONE
                        if callable(self._on_complete):
                            self._on_complete(self)

                    else:
                        self._status = AsyncFetch.ERROR
                        if not callable(self._on_error) or not self._on_error(self):
                            raise HTTPException(self)

        return self._status

    @property
    def duration(self):
        return time.ticks_diff(time.ticks_ms(), self._last_update)

    @property
    def status(self):
        return self._status

    @property
    def source(self):
        return self._path

    @property
    def destination(self):
        return self._file

    @property
    def http_status(self):
        return self._status_code

    @property
    def http_response_headers(self):
        return self._response_headers

    @property
    def stream(self):
        # Return a FileIO for a saved file or a StringIO containing the buffer value
        return open(self._file, "r") if self._file else io.StringIO(memoryview(self._buffer.getvalue())[: self._buffer_len], encoding="UTF-8")

    def to_json(self):
        return json.load(self.stream)

    def finish(self):
        # Force a blocking finish of the fetch command. The tight loop yields to
        # the event loop via the simulator's cooperative VM hook, so the fetch
        # makes progress between iterations.
        while self.update() != AsyncFetch.DONE:
            pass

    def on_complete(self, handler):
        self._on_complete = handler

    def on_error(self, handler):
        self._on_error = handler


if __name__ == "__main__":
    api = AsyncFetch("pimoroni.github.io", 443, use_tls=True, debug=True)

    # Run update once before we have a valid fetch
    assert api.update() == AsyncFetch.IDLE

    # Handler for the completed fetch
    @api.on_complete
    def handle_complete(fetch):
        print(fetch.to_json())

    # Handler for an HTTP error
    # .update() will additionally raise an Exception
    @api.on_error
    def handle_error(fetch):
        if fetch.http_status == 404:
            fetch.fetch("/feed2image/jokeapi-0.json")
            return True  # Don't raise an exception at the "update" call site

        if fetch.http_status == 302:
            if "location" in fetch.http_response_headers:
                print(f"302 redirect to: {fetch.http_response_headers['location']}")

        return False  # We can't handle this, raise

    # Start a fetch
    api.fetch("/feed2image/jokeapi-0.json")

    while True:
        try:
            api.update()
        except HTTPException as e:
            print("Exception was raised!")
            if e.fetch.http_status != 404:
                raise e

        if api.status == AsyncFetch.IDLE:
            break
