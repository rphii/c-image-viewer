#include "text.h"
#include "utf8.h"

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
}

Font font_init(const char *path, int height, float hspace, float vspace, unsigned int glyphs) {
    if(!text_initialized) {
        text_init();
        text_initialized = true;
    }
    FT_Face face = {0};
    int width = 0;
    if(FT_New_Face(ft, path, 0, &face)) {
        fprintf(stderr, "[FREETYPE] Error! Could not load font\n");
        assert(0);
    }
    //if(FT_Set_Pixel_Sizes(face, width, height)) {
    if(FT_Set_Pixel_Sizes(face, height, height)) {
        fprintf(stderr, "[FREETYPE] Error! Could not set pixel sizes\n");
        assert(0);
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
    glGenTextures(1, &font.texture_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, font.texture_array);
    glActiveTexture(GL_TEXTURE0);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, font.gl.buf_w, font.gl.buf_h, glyphs, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    //glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, height + 128, height + 128, glyphs, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    /* enable unicode */
    FT_Select_Charmap(font.face , ft_encoding_unicode);
    return font;
}

void font_load(Font *font, unsigned long i0, unsigned long iE) {
    for(unsigned long i = i0; i < iE; ++i) {
        /* load glyph */
        unsigned long c = FT_Get_Char_Index(font->face, i);
        if(c == 0 && i >= 128) {
            //fprintf(stderr, "[FREETYPE] Error! Character not found for codepoint '%#lx'\n", i);
            continue;
        }
        if(FT_Load_Glyph(font->face, c, FT_LOAD_RENDER)) {
        //if (FT_Load_Char(font->face, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "[FREETYPE] Error! Could not load character '%#lx'\n", i);
            continue;
            //assert(0);
        }
#if 1
        /* generate texture */
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, font->characters.used, 
                font->face->glyph->bitmap.width,
                font->face->glyph->bitmap.rows,
                1, GL_RED, GL_UNSIGNED_BYTE, font->face->glyph->bitmap.buffer
                );
        /* set texture options */
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
        if(tcharacter_set(&font->characters, i, character)) {
            fprintf(stderr, "[FREETYPE] Error! Could not set texture '%#lx'\n", i);
            assert(0);
        }
    }
    fprintf(stderr, "[FONT] Internal Lookup Length: %zu, Alloced %zu\n", font->characters.used, LUT_CAP(font->characters.width));
}

void font_render(Font font, Shader shader, const char *text, float x, float y, float scale, float expand, vec3 color, vec4 dimensions, bool render) {
    glUseProgram(shader);
    //scale = scale * font.height / 256.0f; /* re-adjust scale to original size of font */
    int loc_transforms = glGetUniformLocation(shader, "transforms");
    int loc_map = glGetUniformLocation(shader, "letter_map");
    int loc_color = glGetUniformLocation(shader, "text_color");
    assert(loc_color >= 0);
    assert(loc_transforms >= 0);
    
    if(render) {
        glUniform3f(loc_color, color[0], color[1], color[2]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, font.texture_array);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
    }
    
    float x0 = x;
    mat4 mat_scale;
    unsigned int i_instance = 0;
    float min_x = 0, min_y = 0, max_x = 0, max_y = 0;
    /* iterate through characters */
    unsigned long len = strlen(text);
    for(unsigned long i = 0; i < len; ++i) {
        U8Point u8p = {0};
        if(str_to_u8_point((char *)&text[i], &u8p)) {
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
        float ymax = y + (float)(font.height - ch->bearing[1]) * scale;
        float xmin = x + ch->bearing[0] * scale;
        float ymin = y - (float)(font.height - ch->bearing[1]) * scale;

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

        if(i_instance >= TEXT_INSTANCE_LIMIT && render) {
            glUniformMatrix4fv(loc_transforms, i_instance, GL_FALSE, (float *)transforms);
            glUniform1iv(loc_map, i_instance, (int *)maps);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, i_instance);
            i_instance = 0;
        }

    }

    if(i_instance && render) {
        glUniformMatrix4fv(loc_transforms, i_instance, GL_FALSE, (float *)transforms);
        glUniform1iv(loc_map, i_instance, (int *)maps);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, i_instance);
    }

    /* done */
    if(render) {
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
}

void font_free(Font *font) {
    FT_Done_Face(font->face);
    //memset(font, 0, sizeof(*font));
    glDeleteTextures(1, &font->texture_array);
}

void text_free(void) {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    FT_Done_FreeType(ft);
}
