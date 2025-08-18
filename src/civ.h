#ifndef CIV_H

#include <rlc/vec.h>
#include <rlarg.h>
#include <rlpw.h>

#include "glad.h"
#include "timer.h"
#include "gl_text.h"

#include <cglm/cglm.h>
#include <stdbool.h>
#include <pthread.h>
#include <GLFW/glfw3.h>
#include "civ-config.h"

typedef enum {
    FILTER_NONE,
    FILTER_LINEAR,
    FILTER_NEAREST,
    /* above */
    FILTER__COUNT,
} FilterList;

typedef enum {
    FIT_XY,
    FIT_FILL,
    FIT_PAN,
    /* above */
    FIT__COUNT,
    /* misc */
    FIT__X,
    FIT__Y,
} FitList;

typedef struct Image {
    So filename;
    unsigned char *data;
    bool freed_from_cpu;
    int width;
    int height;
    int channels;
    unsigned int texture;
    FilterList sent;
    size_t index_pre_loading;
} Image;

int image_cmp(Image *a, Image *b);
void image_free(Image *image);

VEC_INCLUDE(VImage, vimage, Image, BY_REF, BASE);
VEC_INCLUDE(VImage, vimage, Image, BY_REF, SORT);

typedef struct VImage VImage;

typedef struct GlContext {
    GLFWwindow *window;
    pthread_mutex_t mutex;
} GlContext;

typedef enum {
    POPUP_NONE,
    POPUP_SELECT,
    POPUP_FIT,
    POPUP_DESCRIPTION,
    POPUP_ZOOM,
    POPUP_FILTER,
    POPUP_PAN,
    POPUP_PRINT_STDOUT,
    /* above */
    POPUP__COUNT
} PopupList;

typedef struct QueueState QueueState;

typedef struct ActionMap {
    bool gl_update;
    bool fit_next;
    bool resized;
    bool filter_next;
    bool toggle_fullscreen;
    bool toggle_description;
    bool quit;
    bool select_random;
    bool print_stdout;
    int select_image;
    double zoom;
    double pan_x;
    double pan_y;
} ActionMap;

typedef struct StateMap {
    int wwidth;
    int wheight;
    int twidth;
    int theight;
    mat4 text_projection;
    mat4 image_projection;
    mat4 image_transform;
    mat4 image_view;
    double mouse_down_x;
    double mouse_down_y;
    double mouse_x;
    double mouse_y;
    Timer t_global;
} StateMap;

typedef struct Civ {
    ActionMap action_map;
    StateMap state_map;
    VImage images;
    VImage images_discover;
    size_t images_loaded;
    pthread_mutex_t images_mtx;
    bool *gl_update;
    Font *font;
    pthread_mutex_t font_mtx;
    Image *active;
    FilterList filter;
    size_t selected;
    struct {
        float initial;
        float current;
    } zoom;
    struct {
        FitList initial;
        FitList current;
    } fit;
    vec2 pan;
    Pw pw;
    Pw pipe_observer;
    struct {
        Timer timer;
        PopupList active;
    } popup;
    struct CivConfig config;
    struct CivConfig defaults;
    struct Arg *arg;
    VSo filenames;
    QueueState *qstate;
    bool pending_pipe;
} Civ;

void glcontext_acquire(GlContext *context);
void glcontext_release(GlContext *context);

const char *fit_cstr(FitList id);
const char *filter_cstr(FilterList id);

void send_texture_to_gpu(Image *image, FilterList filter, bool *render);
void civ_free(Civ *state);
#define ERR_civ_defaults(...) "error while loading defaults"
int civ_defaults(Civ *civ);
void civ_arg(Civ *civ, const char *name);

void civ_popup_set(Civ *state, PopupList id);

void civ_cmd_random(Civ *civ, bool random);
void civ_cmd_print_stdout(Civ *civ, bool print_stdout);
void civ_cmd_select(Civ *civ, int change);
void civ_cmd_fit(Civ *civ, bool next);
void civ_cmd_description(Civ *civ, bool toggle);
void civ_cmd_zoom(Civ *civ, double zoom);
void civ_cmd_filter(Civ *civ, bool next);
void civ_cmd_pan(Civ *civ, vec2 pan);

#define CIV_H
#endif

