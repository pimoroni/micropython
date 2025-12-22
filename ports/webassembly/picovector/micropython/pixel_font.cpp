#include "mp_tracked_allocator.hpp"

#include "mp_helpers.hpp"
#include "picovector.hpp"

extern "C" {
  #include <inttypes.h>
  #include "py/stream.h"
  #include "py/reader.h"
  #include "py/runtime.h"
  #include "extmod/vfs.h"

  MPY_BIND_DEL(pixel_font, {
    self(self_in, pixel_font_obj_t);
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
    m_free(self->glyph_buffer, self->glyph_buffer_size);
    m_free(self->glyph_data_buffer, self->glyph_data_buffer_size);
#else
    m_free(self->glyph_buffer);
    m_free(self->glyph_data_buffer);
#endif
    return mp_const_none;
  })

  MPY_BIND_STATICMETHOD_ARGS1(load, path, {
    pixel_font_obj_t *result = mp_obj_malloc_with_finaliser(pixel_font_obj_t, &type_pixel_font);

    // open the file for binary reading
    //mp_obj_t args[2] = {path, MP_ROM_QSTR(MP_QSTR_r)} // Brace enclosed initialiser lists don't work in the binding macros :(
    mp_obj_t args[2];
    args[0] = path;
    args[1] = MP_ROM_QSTR(MP_QSTR_r);
    mp_obj_t file = mp_vfs_open(MP_ARRAY_SIZE(args), args, (mp_map_t *)&mp_const_empty_map);

    int error;

    //debug_printf("load pixel font\n");

    // check for ppf file header
    char marker[4];
    mp_stream_read_exactly(file, &marker, sizeof(marker), &error);
    if(memcmp(marker, "ppf!", 4) != 0) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("failed to load font, missing PPF header"));
    }
    //debug_printf("- valid header\n");

    uint16_t flags        = ru16(file);
    uint32_t glyph_count  = ru32(file);
    uint16_t glyph_width  = ru16(file);
    uint16_t glyph_height = ru16(file);
    //debug_printf("- glyph width = %d, height = %d, count = %" PRIu32 "\n", glyph_width, glyph_height, glyph_count);

    char name[32];
    mp_stream_read_exactly(file, name, sizeof(name), &error);
    //debug_printf("- font name '%s'\n", name);

    // calculate how much data needed to store each glyphs pixel data
    uint32_t bpr = floor((glyph_width + 7) / 8);
    uint32_t glyph_data_size = bpr * glyph_height;
    //debug_printf("- glyph data size = %" PRIu32 " (%" PRIu32 " byes per row)\n", glyph_data_size, bpr);

    // allocate buffers to store glyph and pixel data
    result->glyph_buffer_size = sizeof(pixel_font_glyph_t) * glyph_count;
    result->glyph_buffer = (uint8_t*)m_malloc(result->glyph_buffer_size);

    result->glyph_data_buffer_size = glyph_data_size * glyph_count;
    result->glyph_data_buffer = (uint8_t*)m_malloc(result->glyph_data_buffer_size);

    //debug_printf("- glyph buffer at %p (%" PRIu32 " bytes)\n", result->glyph_buffer, result->glyph_buffer_size);
    //debug_printf("- glyph data buffer at %p (%" PRIu32 " bytes)\n", result->glyph_data_buffer, result->glyph_data_buffer_size);

    if(!result->glyph_buffer || !result->glyph_data_buffer) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("couldn't allocate buffer for font data"));
    }

    // read codepoint list
    pixel_font_glyph_t *glyphs = (pixel_font_glyph_t*)result->glyph_buffer;
    for(uint32_t i = 0; i < glyph_count; i++) {
      glyphs[i].codepoint = ru32(file);
      glyphs[i].width = ru16(file);
    }
    //debug_printf("- read codepoint list\n");

    // read glyph data into buffer
    //debug_printf("- writing into glyph data buffer\n");
    mp_stream_read_exactly(file, result->glyph_data_buffer, result->glyph_data_buffer_size, &error);
    //debug_printf("- read pixel data\n");

    result->font = m_new_class(pixel_font_t);
    result->font->glyph_count     = glyph_count;
    result->font->width           = glyph_width;
    result->font->height          = glyph_height;
    result->font->glyph_data_size = glyph_data_size;
    result->font->glyphs          = glyphs;
    result->font->glyph_data      = result->glyph_data_buffer;
    strcpy(result->font->name, name);

    mp_stream_close(file);

    return MP_OBJ_FROM_PTR(result);
  })

  static void pixel_font_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    self(self_in, pixel_font_obj_t);

    action_t action = m_attr_action(dest);

    switch(attr) {
      case MP_QSTR_height: {
        if(action == GET) {
          dest[0] = mp_obj_new_int(self->font->height);
          return;
        }
      };

      case MP_QSTR_name: {
        if(action == GET) {
          dest[0] = mp_obj_new_str(self->font->name, strlen(self->font->name));
          return;
        }
      };
    }

    // we didn't handle this, fall back to alternative methods
    dest[1] = MP_OBJ_SENTINEL;
  }

  MPY_BIND_LOCALS_DICT(pixel_font,
      MPY_BIND_ROM_PTR_DEL(pixel_font),
      MPY_BIND_ROM_PTR_STATIC(load),
  )

  MP_DEFINE_CONST_OBJ_TYPE(
      type_pixel_font,
      MP_QSTR_pixel_font,
      MP_TYPE_FLAG_NONE,
      attr, (const void *)pixel_font_attr,
      locals_dict, &pixel_font_locals_dict
  );

}