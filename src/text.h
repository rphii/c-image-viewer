#ifndef TEXT_H

#define TEXT_INSTANCE_LIMIT     125

#include <assert.h>
#include <cglm/cglm.h>
#include "glad.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include "shader.h"
#include "lut.h"
#include "utf8.h"

typedef struct Character {
    //unsigned int texture;
    unsigned int map;
    vec2 size;
    vec2 bearing;
    unsigned int advance;
} Character;

LUT_INCLUDE(TCharacter, tcharacter, unsigned long, BY_VAL, Character, BY_VAL);

typedef struct Font {
    const FT_Face face;
    const int height;
    const int width;
    struct {
        int loc_projection;
        int loc_transforms;
        int loc_map;
        int loc_color;
        Shader id;
    } shader;
    float hspace;
    float vspace;
    TCharacter characters;
    unsigned int texture_array;
    unsigned int glyphs;
    struct {
        int buf_w;
        int buf_h;
    } gl;
} Font;

void text_init(void);
[[nodiscard("memory leak")]] Font font_init(const char *path, int height, float hspace, float vspace, unsigned int glyphs);
void font_load(Font *font, unsigned long i0, unsigned long iE);
void font_shader(Font *font, Shader shader);
void font_render(Font font, const char *text, mat4 projection, float x, float y, float scale, float expand, vec3 color, vec4 dimensions, bool render);
void font_free(Font *font);
void text_free(void);

#define TEXT_H
#endif


