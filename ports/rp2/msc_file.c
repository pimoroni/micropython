/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Christopher Parrott for Pimoroni Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// A read-only file on the USB mass storage media, read straight from flash through extents the
// caller found in the filesystem's tables. It reads on while the host owns the media, and every
// read raises once the host has written anything since the file was opened.

#include <string.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/smallint.h"
#include "modrp2.h"

#if MICROPY_HW_USB_MSC

extern uint32_t rp2_tud_msc_write_count(void);
extern const uint8_t *rp2_tud_msc_media(size_t *bytes);

typedef struct _msc_extent_t {
    uint32_t position;      // Where it starts in the file
    uint32_t offset;        // Where it starts on the media
    uint32_t length;
} msc_extent_t;

typedef struct _rp2_msc_file_obj_t {
    mp_obj_base_t base;
    mp_obj_t message;       // The OSError's text once the host has written, or None for none
    uint32_t writes;        // The host's write count at the open, as msc_write_count() gives it
    uint32_t size;
    uint32_t position;
    bool closed;
    size_t extent;
    msc_extent_t extents[];
} rp2_msc_file_obj_t;

// Matches msc_write_count(), which wraps within a small int
static uint32_t msc_file_writes(void) {
    return rp2_tud_msc_write_count() & MP_SMALL_INT_POSITIVE_MASK;
}

static MP_NORETURN void msc_file_raise_changed(rp2_msc_file_obj_t *self) {
    self->closed = true;
    if (self->message == mp_const_none) {
        mp_raise_OSError(MP_EIO);
    }
    mp_obj_t args[2] = { MP_OBJ_NEW_SMALL_INT(MP_EIO), self->message };
    nlr_raise(mp_obj_new_exception_args(&mp_type_OSError, 2, args));
}

static mp_uint_t msc_file_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    rp2_msc_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->closed) {
        *errcode = MP_EBADF;
        return MP_STREAM_ERROR;
    }

    size_t media_bytes;
    const uint8_t *media = rp2_tud_msc_media(&media_bytes);
    uint32_t wanted = self->position < self->size ? MIN(size, self->size - self->position) : 0;
    uint8_t *out = buf;
    uint32_t done = 0;
    while (done < wanted) {
        // Reads run forwards, so the extent last read from, or the next, almost always holds it
        const msc_extent_t *extent = &self->extents[self->extent];
        if (self->position < extent->position) {
            self->extent = 0;
            continue;
        }
        if (self->position >= extent->position + extent->length) {
            self->extent++;
            continue;
        }
        uint32_t into = self->position - extent->position;
        uint32_t count = MIN(extent->length - into, wanted - done);
        memcpy(out + done, media + extent->offset + into, count);
        done += count;
        self->position += count;
    }

    // After the copy, since the count moves before a write reaches the flash
    if (msc_file_writes() != self->writes) {
        msc_file_raise_changed(self);
    }
    return done;
}

static mp_uint_t msc_file_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    rp2_msc_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (request == MP_STREAM_SEEK) {
        if (self->closed) {
            *errcode = MP_EBADF;
            return MP_STREAM_ERROR;
        }
        struct mp_stream_seek_t *seek = (struct mp_stream_seek_t *)arg;
        mp_off_t base = 0;
        if (seek->whence == MP_SEEK_CUR) {
            base = self->position;
        } else if (seek->whence == MP_SEEK_END) {
            base = self->size;
        }
        mp_off_t target = base + seek->offset;
        self->position = target < 0 ? 0 : target;
        seek->offset = self->position;
        return 0;
    } else if (request == MP_STREAM_CLOSE) {
        self->closed = true;
        return 0;
    } else if (request == MP_STREAM_FLUSH) {
        return 0;
    }
    *errcode = MP_EINVAL;
    return MP_STREAM_ERROR;
}

static const mp_rom_map_elem_t rp2_msc_file_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&mp_stream___exit___obj) },
};
static MP_DEFINE_CONST_DICT(rp2_msc_file_locals_dict, rp2_msc_file_locals_dict_table);

static const mp_stream_p_t rp2_msc_textfile_stream_p = {
    .read = msc_file_read,
    .ioctl = msc_file_ioctl,
    .is_text = true,
};

// What a text-mode open returns, built only by rp2.MSCFile itself
static MP_DEFINE_CONST_OBJ_TYPE(
    rp2_msc_textfile_type,
    MP_QSTR_MSCFile,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    protocol, &rp2_msc_textfile_stream_p,
    locals_dict, &rp2_msc_file_locals_dict
    );

// MSCFile(extents, size, writes, *, text=False, message=None)
//
// extents is a sequence of (position in the file, offset on the media, length), in file order and
// together covering exactly size bytes. writes is msc_write_count() taken before the extents were
// read from the tables, so a write while finding them fails the first read too.
static mp_obj_t rp2_msc_file_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_extents, ARG_size, ARG_writes, ARG_text, ARG_message };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_extents, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_size, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_writes, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_text, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_message, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    size_t count;
    mp_obj_t *items;
    mp_obj_get_array(args[ARG_extents].u_obj, &count, &items);

    const mp_obj_type_t *made = args[ARG_text].u_bool ? &rp2_msc_textfile_type : type;
    rp2_msc_file_obj_t *self = mp_obj_malloc_var(rp2_msc_file_obj_t, extents, msc_extent_t, count, made);

    // Checked once here, so a read never strays outside the media whatever the tables said
    size_t media_bytes;
    rp2_tud_msc_media(&media_bytes);
    uint32_t covered = 0;
    for (size_t i = 0; i < count; i++) {
        mp_obj_t *fields;
        mp_obj_get_array_fixed_n(items[i], 3, &fields);
        msc_extent_t *extent = &self->extents[i];
        extent->position = mp_obj_get_int(fields[0]);
        extent->offset = mp_obj_get_int(fields[1]);
        extent->length = mp_obj_get_int(fields[2]);
        if (extent->position != covered || extent->length == 0
            || extent->offset >= media_bytes || extent->length > media_bytes - extent->offset) {
            mp_raise_ValueError(MP_ERROR_TEXT("bad extent"));
        }
        covered += extent->length;
    }
    if (args[ARG_size].u_int < 0 || covered != (uint32_t)args[ARG_size].u_int) {
        mp_raise_ValueError(MP_ERROR_TEXT("extents don't cover size"));
    }

    self->message = args[ARG_message].u_obj;
    self->writes = args[ARG_writes].u_int;
    self->size = covered;
    self->position = 0;
    self->closed = false;
    self->extent = 0;
    return MP_OBJ_FROM_PTR(self);
}

static const mp_stream_p_t rp2_msc_file_stream_p = {
    .read = msc_file_read,
    .ioctl = msc_file_ioctl,
};

MP_DEFINE_CONST_OBJ_TYPE(
    rp2_msc_file_type,
    MP_QSTR_MSCFile,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    make_new, rp2_msc_file_make_new,
    protocol, &rp2_msc_file_stream_p,
    locals_dict, &rp2_msc_file_locals_dict
    );

#endif // MICROPY_HW_USB_MSC
