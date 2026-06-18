// JavaScript half of the fetch-backed HTTP primitive (see jsfetch/jsfetch.c).
// Lives in a --js-library so it can use full JS (fetch, promises, object
// literals) without the EM_ASM comma/macro pitfalls.
//
// A single in-flight request is tracked in __mpFetch. The C side kicks it off
// with mp_js_fetch_start(), then polls mp_js_fetch_poll() while suspending the
// WASM stack until the promise settles, giving Python a blocking call.

mergeInto(LibraryManager.library, {
    mp_js_fetch_start__postset: "var __mpFetch = null;",

    // method/url/headers are NUL-terminated UTF-8; headers are "K: V" lines
    // separated by "\n". body is a pointer/length (length 0 means no body).
    mp_js_fetch_start: function (methodPtr, urlPtr, headersPtr, bodyPtr, bodyLen) {
        var method = UTF8ToString(methodPtr);
        var url = UTF8ToString(urlPtr);
        var headerStr = UTF8ToString(headersPtr);

        var state = { done: 0, status: 0, reason: "", headers: "", body: new Uint8Array(0), error: "" };
        __mpFetch = state;

        var opts = { method: method };
        if (headerStr) {
            var h = {};
            headerStr.split("\n").forEach(function (line) {
                var i = line.indexOf(":");
                if (i > 0) { h[line.slice(0, i).trim()] = line.slice(i + 1).trim(); }
            });
            opts.headers = h;
        }
        if (bodyLen > 0) {
            // Copy out of the WASM heap; the C buffer may be gone by the time
            // fetch reads it.
            opts.body = HEAPU8.slice(bodyPtr, bodyPtr + bodyLen);
        }

        fetch(url, opts).then(function (r) {
            state.status = r.status;
            state.reason = r.statusText || "";
            var hs = [];
            r.headers.forEach(function (v, k) { hs.push(k + ": " + v); });
            state.headers = hs.join("\r\n");
            return r.arrayBuffer();
        }).then(function (buf) {
            state.body = new Uint8Array(buf);
            state.done = 1;
        }).catch(function (e) {
            state.error = (e && e.message) ? e.message : ("" + e);
            state.done = 2;
        });
    },

    // 0 = pending, 1 = done, 2 = error.
    mp_js_fetch_poll: function () { return __mpFetch ? __mpFetch.done : 2; },

    mp_js_fetch_status: function () { return __mpFetch ? __mpFetch.status : 0; },

    mp_js_fetch_body_len: function () { return __mpFetch ? __mpFetch.body.length : 0; },

    mp_js_fetch_body_copy: function (dest) { if (__mpFetch) { HEAPU8.set(__mpFetch.body, dest); } },

    // which: 0 = reason, 1 = headers, 2 = error. Returns a malloc'd UTF-8 string
    // the caller must free().
    mp_js_fetch_str: function (which) {
        var s = "";
        if (__mpFetch) { s = which === 0 ? __mpFetch.reason : which === 1 ? __mpFetch.headers : __mpFetch.error; }
        return stringToNewUTF8(s || "");
    },

    // ---- streaming variant, for modules/fetch.py (AsyncFetch) ----------------
    // The body is read incrementally via a ReadableStream reader and queued so
    // the Python side can pull chunks across successive update() calls.
    mp_js_sfetch_start__postset: "var __mpSFetch = null;",

    mp_js_sfetch_start: function (methodPtr, urlPtr, headersPtr, bodyPtr, bodyLen) {
        var method = UTF8ToString(methodPtr);
        var url = UTF8ToString(urlPtr);
        var headerStr = UTF8ToString(headersPtr);

        // phase: 0 pending, 1 response received (streaming), 3 error.
        var st = { phase: 0, status: 0, reason: "", headers: "", chunks: [], avail: 0, readDone: false, error: "" };
        __mpSFetch = st;

        var opts = { method: method };
        if (headerStr) {
            var h = {};
            headerStr.split("\n").forEach(function (line) {
                var i = line.indexOf(":");
                if (i > 0) { h[line.slice(0, i).trim()] = line.slice(i + 1).trim(); }
            });
            opts.headers = h;
        }
        if (bodyLen > 0) { opts.body = HEAPU8.slice(bodyPtr, bodyPtr + bodyLen); }

        fetch(url, opts).then(function (r) {
            st.status = r.status;
            st.reason = r.statusText || "";
            var hs = [];
            r.headers.forEach(function (v, k) { hs.push(k + ": " + v); });
            st.headers = hs.join("\r\n");
            st.phase = 1;
            if (!r.body) { st.readDone = true; return; }
            var reader = r.body.getReader();
            var pump = function () {
                reader.read().then(function (res) {
                    if (res.done) { st.readDone = true; return; }
                    st.chunks.push(res.value);
                    st.avail += res.value.length;
                    pump();
                }).catch(function (e) {
                    st.error = (e && e.message) ? e.message : ("" + e);
                    st.readDone = true;
                });
            };
            pump();
        }).catch(function (e) {
            st.error = (e && e.message) ? e.message : ("" + e);
            st.phase = 3;
        });
    },

    mp_js_sfetch_phase: function () { return __mpSFetch ? __mpSFetch.phase : 3; },
    mp_js_sfetch_status: function () { return __mpSFetch ? __mpSFetch.status : 0; },

    // 0 = reason, 1 = headers, 2 = error. Returns a malloc'd UTF-8 string.
    mp_js_sfetch_str: function (which) {
        var s = "";
        if (__mpSFetch) { s = which === 0 ? __mpSFetch.reason : which === 1 ? __mpSFetch.headers : __mpSFetch.error; }
        return stringToNewUTF8(s || "");
    },

    // Done = body fully read AND everything buffered has been consumed.
    mp_js_sfetch_done: function () {
        if (!__mpSFetch) { return 1; }
        return (__mpSFetch.readDone && __mpSFetch.avail === 0) ? 1 : 0;
    },

    // Copy up to maxlen buffered bytes into dest; returns the count (0 if none
    // are available yet).
    mp_js_sfetch_read: function (dest, maxlen) {
        var st = __mpSFetch;
        if (!st || st.avail === 0) { return 0; }
        var written = 0;
        while (written < maxlen && st.chunks.length > 0) {
            var head = st.chunks[0];
            var take = Math.min(head.length, maxlen - written);
            HEAPU8.set(head.subarray(0, take), dest + written);
            written += take;
            if (take === head.length) { st.chunks.shift(); }
            else { st.chunks[0] = head.subarray(take); }
        }
        st.avail -= written;
        return written;
    },
});
