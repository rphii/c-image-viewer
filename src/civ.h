#include <rlc/vec.h>
#include <rlarg.h>
#include <rlpw.h>

#include "glad.h"
#include "timer.h"

#include <cglm/cglm.h>
#include <stdbool.h>
#include <pthread.h>
#include <GLFW/glfw3.h>
#include "civ_config.h"

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
} Image;

void image_free(Image *image);

VEC_INCLUDE(VImage, vimage, Image, BY_REF, BASE);

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

typedef struct Civ {
    VImage images;
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
    //ImageLoadArgs loader;
    struct {
        Timer timer;
        PopupList active;
    } popup;
    struct CivConfig config;
    struct CivConfig defaults;
    struct Arg *arg;
    VSo filenames;
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
