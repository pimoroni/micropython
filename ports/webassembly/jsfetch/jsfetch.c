// _jsfetch: a blocking HTTP primitive backed by the browser's fetch(). A
// browser can't open raw sockets, so the socket-based `requests` /
// `urllib.urequest` from micropython-lib don't work; this gives Python code a
// fetch-backed alternative to build on.
//
// request() kicks off the fetch then suspends the WASM stack (via
// emscripten_sleep) until it settles, giving Python an ordinary blocking call.
// It therefore requires a suspend-capable build (a JSPI or Asyncify variant);
// enable it by including jsfetch/jsfetch.mk, which sets MICROPY_PY_JSFETCH.

#include "py/runtime.h"
#include "py/objstr.h"

#if MICROPY_PY_JSFETCH

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>

// Implemented in jsfetch/jsfetch.js.
extern void mp_js_fetch_start(const char *method, const char *url, const char *headers, const char *body, int body_len);
extern int mp_js_fetch_poll(void);     // 0 = pending, 1 = done, 2 = error
extern int mp_js_fetch_status(void);
extern int mp_js_fetch_body_len(void);
extern void mp_js_fetch_body_copy(void *dest);
extern char *mp_js_fetch_str(int which);  // 0 reason, 1 headers, 2 error (malloc'd)

// Streaming variant (simulator/jsfetch.js).
extern void mp_js_sfetch_start(const char *method, const char *url, const char *headers, const char *body, int body_len);
extern int mp_js_sfetch_phase(void);   // 0 = pending, 1 = response received, 3 = error
extern int mp_js_sfetch_status(void);
extern char *mp_js_sfetch_str(int which);  // 0 reason, 1 headers, 2 error (malloc'd)
extern int mp_js_sfetch_done(void);
extern int mp_js_sfetch_read(void *dest, int maxlen);

// request(method, url, headers="", body=None) -> (status, reason, headers, body)
//   headers: "Key: Value" lines separated by "\n"
//   body:    a bytes-like object, or None
//   returns: status (int), reason (str), response headers (str), body (bytes)
static mp_obj_t jsfetch_request(size_t n_args, const mp_obj_t *args) {
    const char *method = mp_obj_str_get_str(args[0]);
    const char *url = mp_obj_str_get_str(args[1]);
    const char *headers = (n_args > 2 && args[2] != mp_const_none) ? mp_obj_str_get_str(args[2]) : "";

    const char *body = "";
    int body_len = 0;
    if (n_args > 3 && args[3] != mp_const_none) {
        mp_buffer_info_t bi;
        mp_get_buffer_raise(args[3], &bi, MP_BUFFER_READ);
        body = (const char *)bi.buf;
        body_len = (int)bi.len;
    }

    mp_js_fetch_start(method, url, headers, body, body_len);
    while (mp_js_fetch_poll() == 0) {
        emscripten_sleep(5);  // JSPI-suspend; the fetch resolves on the event loop
    }

    if (mp_js_fetch_poll() == 2) {
        char *err = mp_js_fetch_str(2);
        mp_obj_t exc = mp_obj_new_exception_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("fetch failed: %s"), err);
        free(err);
        nlr_raise(exc);
    }

    int status = mp_js_fetch_status();
    char *reason = mp_js_fetch_str(0);
    char *hdrs = mp_js_fetch_str(1);

    int blen = mp_js_fetch_body_len();
    vstr_t vstr;
    vstr_init_len(&vstr, blen);
    if (blen > 0) {
        mp_js_fetch_body_copy((void *)vstr.buf);
    }

    mp_obj_t tuple[4];
    tuple[0] = mp_obj_new_int(status);
    tuple[1] = mp_obj_new_str(reason, strlen(reason));
    tuple[2] = mp_obj_new_str(hdrs, strlen(hdrs));
    tuple[3] = mp_obj_new_bytes_from_vstr(&vstr);
    free(reason);
    free(hdrs);
    return mp_obj_new_tuple(4, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jsfetch_request_obj, 2, 4, jsfetch_request);

// ---- streaming API, for a cooperative AsyncFetch-style client --------------

// stream_start(method, url, headers="", body=None): kick off a streaming fetch.
static mp_obj_t jsfetch_stream_start(size_t n_args, const mp_obj_t *args) {
    const char *method = mp_obj_str_get_str(args[0]);
    const char *url = mp_obj_str_get_str(args[1]);
    const char *headers = (n_args > 2 && args[2] != mp_const_none) ? mp_obj_str_get_str(args[2]) : "";

    const char *body = "";
    int body_len = 0;
    if (n_args > 3 && args[3] != mp_const_none) {
        mp_buffer_info_t bi;
        mp_get_buffer_raise(args[3], &bi, MP_BUFFER_READ);
        body = (const char *)bi.buf;
        body_len = (int)bi.len;
    }
    mp_js_sfetch_start(method, url, headers, body, body_len);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jsfetch_stream_start_obj, 2, 4, jsfetch_stream_start);

// stream_phase() -> 0 pending, 1 response received (streaming), 3 error.
static mp_obj_t jsfetch_stream_phase(void) {
    return MP_OBJ_NEW_SMALL_INT(mp_js_sfetch_phase());
}
static MP_DEFINE_CONST_FUN_OBJ_0(jsfetch_stream_phase_obj, jsfetch_stream_phase);

// stream_response() -> (status:int, reason:str, headers:str)
static mp_obj_t jsfetch_stream_response(void) {
    char *reason = mp_js_sfetch_str(0);
    char *hdrs = mp_js_sfetch_str(1);
    mp_obj_t tuple[3];
    tuple[0] = MP_OBJ_NEW_SMALL_INT(mp_js_sfetch_status());
    tuple[1] = mp_obj_new_str(reason, strlen(reason));
    tuple[2] = mp_obj_new_str(hdrs, strlen(hdrs));
    free(reason);
    free(hdrs);
    return mp_obj_new_tuple(3, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jsfetch_stream_response_obj, jsfetch_stream_response);

// stream_error() -> str
static mp_obj_t jsfetch_stream_error(void) {
    char *err = mp_js_sfetch_str(2);
    mp_obj_t s = mp_obj_new_str(err, strlen(err));
    free(err);
    return s;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jsfetch_stream_error_obj, jsfetch_stream_error);

// stream_done() -> bool (body fully read and all buffered bytes consumed)
static mp_obj_t jsfetch_stream_done(void) {
    return mp_obj_new_bool(mp_js_sfetch_done());
}
static MP_DEFINE_CONST_FUN_OBJ_0(jsfetch_stream_done_obj, jsfetch_stream_done);

// stream_readinto(buf) -> int bytes copied (0 if none buffered yet)
static mp_obj_t jsfetch_stream_readinto(mp_obj_t buf_in) {
    mp_buffer_info_t bi;
    mp_get_buffer_raise(buf_in, &bi, MP_BUFFER_WRITE);
    return MP_OBJ_NEW_SMALL_INT(mp_js_sfetch_read(bi.buf, (int)bi.len));
}
static MP_DEFINE_CONST_FUN_OBJ_1(jsfetch_stream_readinto_obj, jsfetch_stream_readinto);

static const mp_rom_map_elem_t jsfetch_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__jsfetch) },
    { MP_ROM_QSTR(MP_QSTR_request), MP_ROM_PTR(&jsfetch_request_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_start), MP_ROM_PTR(&jsfetch_stream_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_phase), MP_ROM_PTR(&jsfetch_stream_phase_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_response), MP_ROM_PTR(&jsfetch_stream_response_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_error), MP_ROM_PTR(&jsfetch_stream_error_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_done), MP_ROM_PTR(&jsfetch_stream_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_readinto), MP_ROM_PTR(&jsfetch_stream_readinto_obj) },
};
static MP_DEFINE_CONST_DICT(jsfetch_globals, jsfetch_globals_table);

const mp_obj_module_t mp_module__jsfetch = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&jsfetch_globals,
};

MP_REGISTER_MODULE(MP_QSTR__jsfetch, mp_module__jsfetch);

#endif // MICROPY_PY_JSFETCH
