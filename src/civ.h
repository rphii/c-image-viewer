#include "vec.h"
#include "glad.h"
#include "timer.h"
#include <cglm/cglm.h>
#include <stdbool.h>
#include <pthread.h>
#include <GLFW/glfw3.h>
#include "arg.h"

typedef enum {
    FILTER_NONE,
    FILTER_LINEAR,
    FILTER_NEAREST,
    /* above */
    FILTER__COUNT,
} FilterList;

typedef enum {
    FIT_XY,
    FIT_FILL_XY,
    FIT_X,
    FIT_Y,
    FIT_PAN,
    /* above */
    FIT__COUNT
} FitList;

typedef struct Image {
    const char *filename;
    unsigned char *data;
    int width;
    int height;
    int channels;
    unsigned int texture;
    FilterList sent;
} Image;

void image_free(Image *image);

VEC_INCLUDE(VImage, vimage, Image, BY_REF, BASE);

#define ThreadQueue(X) \
    typedef struct X##ThreadQueue { \
        pthread_t id; \
        pthread_attr_t attr; \
        pthread_mutex_t mutex; \
        size_t i0; \
        long len; \
        struct X **q; \
        long jobs; \
        long *done; \
    } X##ThreadQueue;

typedef struct VImage VImage;

ThreadQueue(ImageLoad);
typedef struct ImageLoad {
    size_t index;
    VImage *images;
    pthread_mutex_t *mutex;
  ImageLoadThreadQueue *queue;
} ImageLoad;

typedef struct ImageLoadArgs {
    VImage *images;
    //const char **files;
    VrStr *files;
    pthread_mutex_t *mutex;
    bool *cancel;
    pthread_t thread;
    long jobs;
    long done;
} ImageLoadArgs;

typedef enum {
    POPUP_NONE,
    POPUP_SELECT,
    POPUP_FIT,
    POPUP_DESCRIPTION,
    POPUP_ZOOM,
    POPUP_FILTER,
    POPUP_PAN,
    /* above */
    POPUP__COUNT
} PopupList;

typedef struct CivConfig {
    char *font_path;
    unsigned int font_size;
} CivConfig;

typedef struct Civ {
    VImage images;
    Image *active;
    FilterList filter;
    size_t selected;
    float zoom; struct {
        FitList initial;
        FitList current;
    } fit;
    vec2 pan;
    ImageLoadArgs loader;
    bool show_description;
    bool show_loaded;
    struct {
        Timer timer;
        PopupList active;
    } popup;
    CivConfig config;
    CivConfig defaults;
    Arg arg;
} Civ;

void send_texture_to_gpu(Image *image, FilterList filter, bool *render);
const char *fit_cstr(FitList id);
const char *filter_cstr(FilterList id);
void images_load_async(ImageLoadArgs *args);
void civ_free(Civ *state);
void civ_defaults(Civ *civ);
void civ_arg(Civ *civ, const char *name);

void civ_popup_set(Civ *state, PopupList id);

void civ_cmd_select(Civ *civ, int change);
void civ_cmd_fit(Civ *civ, bool next);
void civ_cmd_description(Civ *civ, bool toggle);
void civ_cmd_zoom(Civ *civ, double zoom);
void civ_cmd_filter(Civ *civ, bool next);
void civ_cmd_pan(Civ *civ, vec2 pan);
