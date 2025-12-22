#include "mp_helpers.hpp"
#include "picovector.hpp"

extern "C" {

  #include "py/stream.h"
  #include "py/reader.h"
  #include "py/runtime.h"
  #include "extmod/vfs.h"

  MPY_BIND_DEL(font, {
    self(self_in, font_obj_t);
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
    m_free(self->buffer, self->buffer_size);
#else
    m_free(self->buffer);
#endif
    return mp_const_none;
  })

  MPY_BIND_STATICMETHOD_ARGS1(load, path, {
    //const char *s = mp_obj_str_get_str(path);
    font_obj_t *result = mp_obj_malloc_with_finaliser(font_obj_t, &type_font);

    // PNG *png = new(m_malloc(sizeof(PNG))) PNG();
    // int status = png->open(mp_obj_str_get_str(path), pngdec_open_callback, pngdec_close_callback, pngdec_read_callback, pngdec_seek_callback, pngdec_decode_callback);
    // result->image = new(m_malloc(sizeof(image))) image(png->getWidth(), png->getHeight());
    // png->decode((void *)result->image, 0);
    // m_free(png, sizeof(png));
    //mp_obj_t fn = mp_obj_new_str(s, (mp_uint_t)strlen(s));

    // get file size (maybe not needed?)
    // mp_obj_t stat = mp_vfs_stat(path);
    // mp_obj_tuple_t *tuple = (mp_obj_tuple_t*)MP_OBJ_TO_PTR(stat);
    // mp_int_t size = mp_obj_get_int(tuple->items[6]);

    // open the file for binary reading
    //mp_obj_t args[2] = {path, MP_ROM_QSTR(MP_QSTR_r)} // Brace enclosed initialiser lists don't work in the binding macros :(
    mp_obj_t args[2];
    args[0] = path;
    args[1] = MP_ROM_QSTR(MP_QSTR_r);;
    mp_obj_t file = mp_vfs_open(MP_ARRAY_SIZE(args), args, (mp_map_t *)&mp_const_empty_map);

    int error;

    char marker[4];
    mp_stream_read_exactly(file, &marker, sizeof(marker), &error);

    if(memcmp(marker, "af!?", 4) != 0) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("failed to load font, missing AF header"));
    }

    uint16_t flags       = ru16(file);
    uint16_t glyph_count = ru16(file);
    uint16_t path_count  = ru16(file);
    uint16_t point_count = ru16(file);

    size_t glyph_buffer_size = sizeof(glyph_t) * glyph_count;
    size_t path_buffer_size = sizeof(glyph_path_t) * path_count;
    size_t point_buffer_size = sizeof(glyph_path_point_t) * point_count;

    // allocate buffer to store font glyph, path, and point data
    result->buffer_size = glyph_buffer_size + path_buffer_size + point_buffer_size;
    result->buffer = (uint8_t*)m_malloc(result->buffer_size);

    if(!result->buffer) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("couldn't allocate buffer for font data"));
    }

    glyph_t *glyphs = (glyph_t*)result->buffer;
    glyph_path_t *paths = (glyph_path_t*)(result->buffer + glyph_buffer_size);
    glyph_path_point_t *points = (glyph_path_point_t*)(result->buffer + glyph_buffer_size + path_buffer_size);

    // load glyph dictionary
    result->font.glyph_count = glyph_count;
    result->font.glyphs      = glyphs;
    for(int i = 0; i < glyph_count; i++) {
      glyph_t *glyph = &result->font.glyphs[i];
      glyph->codepoint  = ru16(file);
      glyph->x          =  rs8(file);
      glyph->y          =  rs8(file);
      glyph->w          =  ru8(file);
      glyph->h          =  ru8(file);
      glyph->advance    =  ru8(file);
      glyph->path_count =  ru8(file);
      glyph->paths      =      paths;
      paths += glyph->path_count;
    }

    // load the glyph paths
    for(int i = 0; i < glyph_count; i++) {
      glyph_t *glyph = &result->font.glyphs[i];
      for(int j = 0; j < glyph->path_count; j++) {
        glyph_path_t *path = &glyph->paths[j];
        path->point_count = flags & 0b1 ? ru16(file) : ru8(file);
        path->points = points;
        points += path->point_count;
      }
    }

    // load the glyph points
    for(int i = 0; i < glyph_count; i++) {
      glyph_t *glyph = &result->font.glyphs[i];
      for(int j = 0; j < glyph->path_count; j++) {
        glyph_path_t *path = &glyph->paths[j];
        for(int k = 0; k < path->point_count; k++) {
          glyph_path_point_t *point = &path->points[k];
          point->x = ru8(file);
          point->y = ru8(file);
        }
      }
    }

    mp_stream_close(file);

    return MP_OBJ_FROM_PTR(result);
  })

  MPY_BIND_LOCALS_DICT(font,
    MPY_BIND_ROM_PTR_DEL(font),
    MPY_BIND_ROM_PTR_STATIC(load),
  )

  MP_DEFINE_CONST_OBJ_TYPE(
    type_font,
    MP_QSTR_font,
    MP_TYPE_FLAG_NONE,
    locals_dict, &font_locals_dict
  );

}