#include "gl_text.h"
#include <rphii/utf8.h>

static bool text_initialized;

static inline size_t static_hash_unsigned_long(unsigned long val) {
    return val * 0x61c88647UL;
}

static inline int static_cmp_unsigned_long(unsigned long a, unsigned long b) {
    return !(a == b);
}

LUT_IMPLEMENT(TCharacter, tcharacter, unsigned long, BY_VAL, Character, BY_VAL,
        static_hash_unsigned_long, static_cmp_unsigned_long, 0, 0);

static FT_Library ft;
static bool initialized;
static unsigned int VAO;
static unsigned int VBO;
static mat4 transforms[TEXT_INSTANCE_LIMIT];
static unsigned int maps[TEXT_INSTANCE_LIMIT];

void text_init(void) {
    /* initialize freetype */
    if(FT_Init_FreeType(&ft)) {
        fprintf(stderr, "[FREETYPE] Error! Could not initialize library\n");
        assert(0);
    }

    float vertex_data[4 * 2] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
    };

    /* initialize VAO */
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    initialized = true;
}

Font font_init(const Str *path, int height, float hspace, float vspace, unsigned int glyphs) {
    if(!text_initialized) {
        text_init();
        text_initialized = true;
    }
    FT_Face face = {0};
    int width = 0;
    char cpath[PATH_MAX];
    str_as_cstr(*path, cpath, PATH_MAX);
    if(FT_New_Face(ft, cpath, 0, &face)) {
        fprintf(stderr, "[FREETYPE] Error! Could not load font: '%s'\n", cpath);
        //assert(0);
        return (Font){0};
    }
    //if(FT_Set_Pixel_Sizes(face, width, height)) {
    if(FT_Set_Pixel_Sizes(face, height, height)) {
        fprintf(stderr, "[FREETYPE] Error! Could not set pixel sizes\n");
        //assert(0);
        return (Font){0};
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); /* disable byte-alignment restriction */
    Font font = {
        .face = face,
        .height = height,
        .width = width,
        .hspace = hspace,
        .vspace = vspace,
        .glyphs = glyphs,
    };
    /* set up 3D texture */
    //int w = font.height < 16 ? 16 : font.height; /* width has to be at least 16 */
    //int h = font.height < 256 ? 256 : font.height; /* height has to be at least 256 */
    font.gl.buf_w = font.height < 256 ? 256 : font.height; /* width has to be at least 16 */
    font.gl.buf_h = font.height < 256 ? 256 : font.height; /* height has to be at least 256 */
    //font.gl.buf_w = font.height;
    //font.gl.buf_h = font.height;
    glGenTextures(1, &font.texture_array.id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, font.texture_array.id);
    glActiveTexture(GL_TEXTURE0);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, font.gl.buf_w, font.gl.buf_h, glyphs, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    //glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, height + 128, height + 128, glyphs, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    /* enable unicode */
    FT_Select_Charmap(font.face , ft_encoding_unicode);
    return font;
}

void font_load(Font *font, unsigned long i0, unsigned long iE) {
    for(unsigned long i = i0; i < iE; ++i) {
#if 1
        if(tcharacter_get(&font->characters, i)) {
            continue;
        }
#endif

        /* load glyph */
        unsigned long c = FT_Get_Char_Index(font->face, i);
        if(c == 0 && i >= 128) {
            //fprintf(stderr, "[FREETYPE] Error! Character not found for codepoint '%#lx'\n", i);
            continue;
        }
        if(FT_Load_Glyph(font->face, c, FT_LOAD_RENDER)) {
        //if (FT_Load_Char(font->face, i, FT_LOAD_RENDER)) {
            //fprintf(stderr, "[FREETYPE] Error! Could not load character '%#lx'\n", i);
            continue;
            //assert(0);
        }
#if 1
        /* fix buf */
        uint8_t *buf = malloc((size_t)font->gl.buf_h * (size_t)font->gl.buf_w);
        if(!buf) {
            fprintf(stderr, "[MALLOC] failed allocating requested size: %zu", (size_t)font->gl.buf_h * (size_t)font->gl.buf_w);
            return;
        }
        memset(buf, 0x00, font->gl.buf_h * font->gl.buf_w);
        for(size_t i = 0; i < font->face->glyph->bitmap.rows; ++i) {
            memcpy(&buf[i * font->gl.buf_w], &font->face->glyph->bitmap.buffer[i * (font->face->glyph->bitmap.width)], font->face->glyph->bitmap.width);
        }
#endif
#if 1
#if 1
        //printff("BITMAP %u x %u", font->face->glyph->bitmap.width, font->face->glyph->bitmap.rows);
        /* generate texture */
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, font->characters.used, 
                font->gl.buf_w, font->gl.buf_h,
                //font->face->glyph->bitmap.width,
                //font->face->glyph->bitmap.rows,
                //1, GL_RED, GL_UNSIGNED_BYTE, font->face->glyph->bitmap.buffer
                1, GL_RED, GL_UNSIGNED_BYTE, buf
                );
#endif
        /* set texture options */
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        free(buf);
#endif
        /* now store character for lager use */
        //printf("width %u rows %u\n", font->face->glyph->bitmap.width, font->face->glyph->bitmap.rows);
        Character character = {
            .map = font->characters.used,
            .size = {font->face->glyph->bitmap.width, font->face->glyph->bitmap.rows},
            .bearing = {font->face->glyph->bitmap_left, font->face->glyph->bitmap_top},
            .advance = font->face->glyph->advance.x,
        };
        //fprintf(stderr, "[FREETYPE] Add '%#lx' to %#x \n", i, character.map);
        if(font->characters.used + 1 >= font->glyphs) {
            fprintf(stderr, "[FONT] Error! Not enough glyphs initialized! '%#lx'\n", font->characters.used + 1);
            assert(0);
        }
        if(!tcharacter_set(&font->characters, i, character)) {
            fprintf(stderr, "[FREETYPE] Error! Could not set texture '%#lx'\n", i);
            assert(0);
        }
    }
    //fprintf(stderr, "[FONT] Internal Lookup Length: %zu, Alloced %zu\n", font->characters.used, LUT_CAP(font->characters.width));
}

void font_load_single(Font *font, unsigned long i) {
    font_load(font, i, i + 1);
}

void font_shader(Font *font, Shader shader) {
    font->shader.loc_transforms = glGetUniformLocation(shader.id, "transforms");
    font->shader.loc_projection = glGetUniformLocation(shader.id, "projection");
    font->shader.loc_map = glGetUniformLocation(shader.id, "letter_map");
    font->shader.loc_color = glGetUniformLocation(shader.id, "text_color");
    font->shader.id = shader;
    assert(font->shader.loc_map >= 0);
    assert(font->shader.loc_color >= 0);
    assert(font->shader.loc_transforms >= 0);
    assert(font->shader.loc_projection >= 0);
}


void font_render(Font font, const char *text, mat4 projection, vec2 pos, float scale, float expand, vec3 color, vec4 dimensions, TextAlignList align) {
    glUseProgram(font.shader.id.id);
    //scale = scale * font.height / 256.0f; /* re-adjust scale to original size of font */
    
    if(align == TEXT_ALIGN_RENDER) {
        glUniformMatrix4fv(font.shader.loc_projection, 1, GL_FALSE, (float *)projection);
        glUniform3f(font.shader.loc_color, color[0], color[1], color[2]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, font.texture_array.id);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
    }
    
    float x = pos[0];
    float y = pos[1];
    float x0 = x;
    mat4 mat_scale;
    unsigned int i_instance = 0;
    float min_x = 0, min_y = 0, max_x = 0, max_y = 0;
    /* iterate through characters */
    unsigned long len = strlen(text);
    for(unsigned long i = 0; i < len; ++i) {
        U8Point u8p = {0};
        if(cstr_to_u8_point((char *)&text[i], &u8p)) {
            u8p.val = 0;
            u8p.bytes = 1;
        }
        //char c = text[i];
        unsigned long c = u8p.val;
        Character *ch = tcharacter_get(&font.characters, c);
        if(!ch) {
            i += (u8p.bytes - 1);
            continue; /* TODO: what do? */
        }
        if(c == '\n') {
            y -= ch->size[1] * font.vspace * scale;
            x = x0;
            continue;
        }
        if(c == ' ') {
            x += (ch->advance >> 6) * scale * font.hspace;
            continue;
        }

        /* CORRECT: */
        float xpos = x + ch->bearing[0] * scale;
        float ypos = y - (font.gl.buf_h - ch->bearing[1]) * scale;

        float w = font.height * scale / ((float)font.height / font.gl.buf_w);
        float h = font.height * scale / ((float)font.height / font.gl.buf_h);

        float xmax = x + (float)(ch->size[0] + ch->bearing[0]) * scale;
        float xmin = x + ch->bearing[0] * scale;
        //float ymin = y - (float)(font.height - ch->bearing[1]) * scale;
        //float ymax = ymin + (float)(ch->size[1]);

        float ymax = y + (float)(ch->bearing[1]) * scale;
        float ymin = ymax - (float)(ch->size[1]);

        if(i == 0 || xpos < min_x) min_x = xmin;
        if(i == 0 || ypos < min_y) min_y = ymin;
        if(i == 0 || xmax > max_x) max_x = xmax;
        if(i == 0 || ymax > max_y) max_y = ymax;

        // float ex = expand;
        // float ey = (h * expand - h) - (float)font.height / 2; // / 2;

        glm_mat4_identity(transforms[i_instance]);
        //glm_translate(transforms[i_instance], (vec3){xpos - ex, ypos - ey, 0});
        glm_translate(transforms[i_instance], (vec3){xpos, ypos});
        glm_mat4_identity(mat_scale);
        glm_scale(mat_scale, (vec3){w, h, 0});
        glm_mat4_mul(transforms[i_instance], mat_scale, transforms[i_instance]);

        maps[i_instance] = ch->map;

        /* advance cursors for next glyph (advance is 1/64 pixels) */
        x += (ch->advance >> 6) * scale * font.hspace;
        ++i_instance;
        i += (u8p.bytes - 1);

        if(i_instance >= TEXT_INSTANCE_LIMIT && align == TEXT_ALIGN_RENDER) {
            glUniformMatrix4fv(font.shader.loc_transforms, i_instance, GL_FALSE, (float *)transforms);
            glUniform1iv(font.shader.loc_map, i_instance, (int *)maps);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, i_instance);
            i_instance = 0;
        }

    }

    /* done */
    if(align == TEXT_ALIGN_RENDER) {
        if(i_instance) {
            glUniformMatrix4fv(font.shader.loc_transforms, i_instance, GL_FALSE, (float *)transforms);
            glUniform1iv(font.shader.loc_map, i_instance, (int *)maps);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, i_instance);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    if(dimensions) {
        dimensions[0] = min_x;
        dimensions[1] = min_y;
        dimensions[2] = max_x;
        dimensions[3] = max_y;
    }

    switch(align) {
        case TEXT_ALIGN_CENTER: {
            float dx = (dimensions[2] - dimensions[0]) / 2.0f;
            float dy = (dimensions[3] - dimensions[1]) / 2.0f;
            pos[0] -= dx;
            pos[1] -= dy;
            dimensions[0] -= dx;
            dimensions[2] -= dx;
            dimensions[1] -= dy;
            dimensions[3] -= dy;
        } break;
        case TEXT_ALIGN_RIGHT: {
            float dx = (dimensions[2] - dimensions[0]);
            pos[0] -= dx;
            dimensions[0] -= dx;
            dimensions[2] -= dx;
        } break;
        default: break;
    }

}

void font_free(Font *font) {
    FT_Done_Face(font->face);
    tcharacter_free(&font->characters);
    //memset(font, 0, sizeof(*font));
    if(font->texture_array.loaded) {
        glDeleteTextures(1, &font->texture_array.id);
    }
}

void text_free(void) {
    if(!initialized) return;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    FT_Done_FreeType(ft);
}

