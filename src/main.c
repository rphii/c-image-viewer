#define STB_IMAGE_IMPLEMENTATION

//#include "shader.h"

/* DEPENDENCIES 
 * opengl
 * pthread (optional?)
 * c stdlib
 * cglm
 * freetype
 * (glad)
 * (glfw)
 */

#include "stb_image.h"
#include "text.h"
#include "vec.h"

#include "uniform.h"
#include "box.h"
#include "glad.h"
#include <pthread.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#include <unistd.h>
//#include <magic.h>

typedef enum {
    FILTER_NONE,
    FILTER_LINEAR,
    FILTER_NEAREST,
    /* above */
    FILTER__COUNT,
} FilterList;

typedef struct ActionMap {
    bool gl_update;
    bool select_next;
    bool select_previous;
    bool stretch_next;
    bool resized;
    bool filter_next;
    bool toggle_fullscreen;
    bool toggle_description;
    bool quit;
    double scroll_y;
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
    double t_now;
    double t_prev;
    double t_delta;
} StateMap;

typedef enum {
    FIT_XY,
    FIT_X,
    FIT_Y,
    FIT_PAN,
    /* above */
    FIT__COUNT
} FitList;

static const char *fit_cstr(FitList id) {
    switch(id) {
        case FIT_XY: return "XY";
        case FIT_X: return "X";
        case FIT_Y: return "Y";
        case FIT_PAN: return "PAN";
        default: return "";
    }
}

static ActionMap s_action;
static StateMap s_state;

typedef struct Image {
    const char *filename;
    unsigned char *data;
    int width;
    int height;
    int channels;
    unsigned int texture;
    FilterList sent;
} Image;

#ifndef PROC_COUNT
#define PROC_COUNT  16
#endif

#define ThreadQueue(X) \
    typedef struct X##ThreadQueue { \
        pthread_t id; \
        pthread_attr_t attr; \
        pthread_mutex_t mutex; \
        size_t i0; \
        size_t len; \
        struct X *q[PROC_COUNT]; \
    } X##ThreadQueue;

typedef struct VImage VImage;

ThreadQueue(ImageLoad);
typedef struct ImageLoad {
    size_t index;
    VImage *images;
    pthread_mutex_t *mutex;
  ImageLoadThreadQueue *queue;
} ImageLoad;

void image_free(Image *image);

VEC_INCLUDE(VImage, vimage, Image, BY_REF, BASE);
VEC_IMPLEMENT(VImage, vimage, Image, BY_REF, BASE, image_free);


void image_free(Image *image) {
    if(image->data) {
        glDeleteTextures(1, &image->texture);
        stbi_image_free(image->data);
    }
    memset(image, 0, sizeof(*image));
}

void send_texture_to_gpu(Image *image, FilterList filter) {
    if(!image) return;
    if(!image->data) return;

    if(filter == FILTER_NONE) return;
    if(filter == FILTER__COUNT) return;
    if(image->sent == filter) return;

    if(!image->sent) {
        GLenum format;
        if(image->channels == 1) format = GL_RED;
        else if(image->channels == 2) format = GL_RG;
        else if(image->channels == 3) format = GL_RGB;
        else if(image->channels == 4) format = GL_RGBA;
        else {
            fprintf(stderr, "[TEXTURE] Error: number of channels %u\n", image->channels);
            assert(0 && "unsupported image channels");
        }

        glGenTextures(1, &image->texture);
        glBindTexture(GL_TEXTURE_2D, image->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, image->width, image->height, 0, format, GL_UNSIGNED_BYTE, image->data);
    }

    switch(filter) {
        case FILTER_LINEAR: {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } break;
        case FILTER_NEAREST: {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } break;
        default: break;
    }
    image->sent = filter;
    s_action.gl_update = true;
}

void *image_load_thread(void *args) {
    ImageLoad *image_load = args;

    pthread_mutex_lock(image_load->mutex);
    Image *image = vimage_get_at(image_load->images, image_load->index);
    pthread_mutex_unlock(image_load->mutex);

    if(image->data) return 0; /* already loaded */

    image->data = stbi_load(image->filename, &image->width, &image->height, &image->channels, 0);
    glfwPostEmptyEvent();

    /* finished this thread .. make space for next thread */
    pthread_mutex_lock(&image_load->queue->mutex);
    image_load->queue->q[(image_load->queue->i0 + image_load->queue->len) % PROC_COUNT] = image_load;
    ++image_load->queue->len;
    pthread_mutex_unlock(&image_load->queue->mutex);

    return 0;
}

typedef struct ImageLoadArgs {
    VImage *images;
    const char **files;
    size_t n;
    pthread_mutex_t *mutex;
    bool *cancel;
    pthread_t thread;
    long jobs;
} ImageLoadArgs;

void images_load(VImage *images, const char **files, size_t n, pthread_mutex_t *mutex, bool *cancel, long jobs) {

    ImageLoad *image_load = malloc(sizeof(ImageLoad) * jobs);
    if(!image_load) return;
    ImageLoadThreadQueue queue = {0};

    /* push images to load */
    pthread_mutex_lock(mutex);
    for(size_t i = 0; i < n; ++i) {
        Image push = { .filename = files[i] };
        vimage_push_back(images, &push);
    }
    pthread_mutex_unlock(mutex);

    pthread_mutex_init(&queue.mutex, 0);
    pthread_attr_init(&queue.attr);
    pthread_attr_setdetachstate(&queue.attr, PTHREAD_CREATE_DETACHED);

    for(int i = 0; i < jobs; ++i) {
        image_load[i].mutex = mutex;
        image_load[i].queue = &queue;
        image_load[i].images = images;
        /* add to queue */
        queue.q[i] = &image_load[i];
        ++queue.len;
    }

    assert(queue.len <= jobs);

    /* load images */
    for(int i = 0; i < n && !*cancel; ++i) {
        /* this section is responsible for starting the thread */
        pthread_mutex_lock(&queue.mutex);
        if(queue.len) {
            /* load job */
            ImageLoad *job = queue.q[queue.i0];
            job->index = i;
            /* create thread */
            pthread_create(&job->queue->id, &queue.attr, image_load_thread, job);
            queue.i0 = (queue.i0 + 1) % jobs;
            --queue.len;
        } else {
            --i;
        }
        pthread_mutex_unlock(&queue.mutex);
    }

    /* wait until all threads finished */
    for(;;) {
        pthread_mutex_lock(&queue.mutex);
        if(queue.len == jobs) {
            pthread_mutex_unlock(&queue.mutex);
            break;
        }
        pthread_mutex_unlock(&queue.mutex);
    }
    /* now free since we *know* all threads finished, we can ignore usage of the lock */
    for(size_t i = 0; i < jobs; ++i) {
        if(image_load[i].queue->id) {
            //str_free(&thr_search[i].content);
            //str_free(&thr_search[i].cmd);
        }
    }
    free(image_load);
}

void *images_load_voidptr(void *argp) {
    ImageLoadArgs *args = argp;
    images_load(args->images, args->files, args->n, args->mutex, args->cancel, args->jobs);
    return 0;
}

void images_load_async(ImageLoadArgs *args) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&args->thread, &attr, images_load_voidptr, args);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    s_action.resized = true;
    s_state.wwidth = width;
    s_state.wheight = height;
    glViewport(0, 0, width, height);
    glm_perspective(glm_rad(45), (float)width / (float)height, 0.1, 100, s_state.image_projection);

#if 0
    /* update text projection */
    double ratio_want = 16.0 / 9.0;
    double ratio_have = (double)width / (double)height;
    double ratio2 = ratio_have / ratio_want;

    s_state.twidth = 2560; // get monitor dimensions here ...
    s_state.theight = 1920;
    if(ratio_have > ratio_want) {
        s_state.twidth = round(s_state.twidth * ratio2);
    } else if(ratio_have < ratio_want) {
        s_state.theight = round(s_state.theight / ratio2);
    }
#endif

    s_state.twidth = s_state.wwidth;
    s_state.theight = s_state.wheight;

    glm_ortho(0.0f, s_state.twidth, 0.0f, s_state.theight, -1.0f, 1.0f, s_state.text_projection);

    s_action.gl_update = true;
}

void process_input(GLFWwindow *window, bool *run_timer) {
    bool t_run = false;
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        s_action.quit = true;
    }
    if((glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS /*&& glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS*/) ||
       (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.pan_x += s_state.t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.pan_y -= s_state.t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.pan_y += s_state.t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS /*&& glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS*/) ||
       (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.pan_x -= s_state.t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.zoom += s_state.t_delta;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.zoom -= s_state.t_delta;
        t_run = true;
    };
    *run_timer = t_run;
}

void key_callback(GLFWwindow* window, int key, int scancode, int act, int mods)
{
    if(act == GLFW_PRESS || act == GLFW_REPEAT) {
        switch(key) {
            case GLFW_KEY_LEFT: {
                //if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                //    s_action.select_previous = true;
                //}
            } break;
            case GLFW_KEY_RIGHT: {
                //if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                //    s_action.select_next = true;
                //}
            } break;
            case GLFW_KEY_K: {
                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) {
                    s_action.select_previous = true;
                }
            } break;
            case GLFW_KEY_J: {
                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) {
                    s_action.select_next = true;
                }
            } break;
        }
    }
    if(act == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_S: {
                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    s_action.filter_next = true;
                } else {
                    s_action.stretch_next = true;
                }
            } break;
            case GLFW_KEY_F: { s_action.toggle_fullscreen = true; } break;
            case GLFW_KEY_Q: { s_action.quit = true; } break;
            case GLFW_KEY_D: { s_action.toggle_description = true; } break;
        }
    }
}

typedef struct Civ {
    VImage images;
    Image *active;
    FilterList filter;
    size_t selected;
    float zoom;
    FitList stretch;
    vec2 pan;
    ImageLoadArgs loader;
    bool show_description;
} Civ;

void civ_free(Civ *state) {
    pthread_mutex_lock(state->loader.mutex);
    vimage_free(&state->images);
    pthread_mutex_unlock(state->loader.mutex);
    pthread_join(state->loader.thread, 0);
    memset(state, 0, sizeof(*state));
}

void process_action_map(GLFWwindow *window, Civ *state) {
    if(s_action.select_next) {
        s_action.select_next = false;
        s_action.gl_update = true;
        glm_vec2_zero(state->pan);
        state->zoom = 1.0f;
        ++state->selected;
        if(vimage_length(state->images)) {
            state->selected %= vimage_length(state->images);
        }
    }
    if(s_action.select_previous) {
        s_action.select_previous = false;
        s_action.gl_update = true;
        glm_vec2_zero(state->pan);
        state->zoom = 1.0f;
        if(state->selected) {
            --state->selected;
        } else if(vimage_length(state->images)) {
            state->selected = vimage_length(state->images) - 1;
        }
    }

    if(s_action.quit) {
        glfwSetWindowShouldClose(window, true);
    }

    if(s_action.resized) {
        s_action.resized = false;
        //if(state->zoom != 1.0) {
        //    state->stretch = FIT_PAN;
        //}
    }

    if(s_action.stretch_next) {
        s_action.stretch_next = false;
        ++state->stretch;
        state->stretch %= FIT__COUNT;
        state->zoom = 1.0;
        glm_vec2_zero(state->pan);
        s_action.gl_update = true;
    }

    if(s_action.scroll_y) {
        double scroll = s_action.scroll_y;
        s_action.scroll_y = 0;
        if(scroll < 0) {
            state->zoom /= (+1.1);
        } else if(scroll > 0) {
            state->zoom *= (+1.1);
        }
        //printf("SCROLLED: %f zoom %f\n", scroll, state->zoom);
        state->stretch = FIT_PAN;
        s_action.gl_update = true;
    }

    if(s_action.zoom) {
        if(s_action.zoom < 0) {
            state->zoom *= (1.0 + s_action.zoom);
        } else if(s_action.zoom > 0) {
            state->zoom /= (1.0 - s_action.zoom);
        }
        //printf("ZOOM : %f\n", state->zoom);
        s_action.zoom = 0;
        state->stretch = FIT_PAN;
        s_action.gl_update = true;
    }

    if(s_action.filter_next) {
        s_action.filter_next = false;
        ++state->filter;
        state->filter %= FILTER__COUNT;
        if(state->filter == 0) ++state->filter;
        s_action.gl_update = true;
    }

    if(s_action.toggle_description) {
        s_action.toggle_description = false;
        state->show_description = !state->show_description;
        s_action.gl_update = true;
    }

    if(s_action.toggle_fullscreen) {
        s_action.toggle_fullscreen = false;
        //GLFWmonitor *monitor = glfwGetWindowMonitor(window);
        //const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        //glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, 0);
    }

    if(s_action.pan_x || s_action.pan_y) {
        state->pan[0] += s_action.pan_x / state->zoom; //s_action.pan_x / s_state.wwidth * state->zoom;
        state->pan[1] -= s_action.pan_y / state->zoom; //s_action.pan_y / s_state.wheight * state->zoom;
        /* done */
        s_action.pan_x = 0;
        s_action.pan_y = 0;
        s_action.gl_update = true;
    }

}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    s_state.mouse_x = xpos;
    s_state.mouse_y = ypos;
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if(state == GLFW_PRESS) {
        float dx = xpos - s_state.mouse_down_x;
        float dy = ypos - s_state.mouse_down_y;
        //printf("delta %f %f @ %f %f\n", dx, dy, xpos, ypos);
        s_state.mouse_down_x = xpos;
        s_state.mouse_down_y = ypos;
        s_action.pan_x += dx;
        s_action.pan_y += dy;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &s_state.mouse_down_x, &s_state.mouse_down_y);
    }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    s_action.scroll_y = yoffset;
}

int main(const int argc, const char **argv) {

    if(argc < 1) return -1;

    /* get directory */
    const char *program = argv[0];
    unsigned int n = strlen(program);
    while(n > 0) {
        if(program[n] == '/') {
            break;
        }
        --n;
    }
    char directory[4096];
    snprintf(directory, 4096, "%.*s", n, program);
    char shaders[4096];
    snprintf(shaders, 4096, "%.4040s/../shaders", directory);

    //printf("%zu\n", sizeof(ImageLoadThreadQueue));
    //return 0;

    /**************/
    /* GLFW BEGIN */

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    const char **files = &argv[1];    /* init state */
    int total = argc - 1;
    Civ state = {0};
    state.zoom = 1.0;
    state.filter = FILTER_NEAREST;

    pthread_mutex_t mutex_image;
    pthread_mutex_init(&mutex_image, 0);
    state.loader.files = files;
    state.loader.images = &state.images;
    state.loader.n = total;
    state.loader.mutex = &mutex_image;
    state.loader.cancel = &s_action.quit;
    state.loader.jobs = sysconf(_SC_NPROCESSORS_ONLN);
    images_load_async(&state.loader);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwWindowHint(GLFW_SAMPLES, 4);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_MULTISAMPLE);

    /* OPENGL START */
    Shader shader = shader_load(shaders, "rectangle.vert", shaders, "rectangle.frag");
    int loc_projection = get_uniform(shader, "projection");
    int loc_view = get_uniform(shader, "view");
    int loc_transform = get_uniform(shader, "transform");

    /* normalized device coordinates -- NDC */
    /* 0,1 top / -1,0 left */
    float vertices[] = {
        // positions          // texture coords
         1.0f,  1.0f,  0.0f,  0.0f,  0.0f, // top right
         1.0f, -1.0f,  0.0f,  0.0f,  1.0f, // bottom right
        -1.0f, -1.0f,  0.0f, -1.0f,  1.0f, // bottom left
        -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, // top left
    };
    unsigned int indices[] = { // note that we start from 0!
        0, 1, 3, // first triangle
        1, 2, 3 // second triangle
    };

    //text_init();

    unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)(sizeof(float) * 0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)(sizeof(float) * 3));

    glBindVertexArray(0);

    //glEnable(GL_DEPTH_TEST);

    s_action.gl_update = true;

    //text_init();
    Shader sh_text = shader_load(shaders, "text.vert", shaders, "text.frag");
    Shader sh_box = shader_load(shaders, "box.vert", shaders, "box.frag");
    int loc_text_projection = get_uniform(sh_text, "projection");
    glm_ortho(0.0f, s_state.wwidth, 0.0f, s_state.wheight, -1.0f, 1.0f, s_state.image_projection);

    glm_mat4_identity(s_state.image_view);
    glm_translate(s_state.image_view, (vec3){0.0f, 0.0f, 0.0f});

    //glUseProgram(0);
    int font_size = 24;
    //Font font = font_init("/usr/share/fonts/mikachan-font-ttf/mikachan.ttf", font_size, 1.0, 1.5, 1024);
    //Font font = font_init("/usr/share/fonts/lato/Lato-Regular.ttf", font_size, 1.0, 1.5, 1024);
    Font font = font_init("/usr/share/fonts/MonoLisa/ttf/MonoLisa-Regular.ttf", font_size, 1.0, 1.5, 1024);
    font_load(&font, 0, 256);

    char str_info[1024] = {0};
    bool run_timer = true;

    while(!glfwWindowShouldClose(window)) {
        bool rendered = false;

        /* done, timer */
        if(run_timer) {
            s_state.t_prev = s_state.t_now;
            s_state.t_delta = glfwGetTime();
            s_state.t_now += s_state.t_delta;
            //printf("t delta %f\n", s_state.t_delta);
        }
        glfwSetTime(0);

        /* input */
        process_input(window, &run_timer);

        /* process */
        process_action_map(window, &state);

        if(state.selected < vimage_length(state.images)) {
            pthread_mutex_lock(state.loader.mutex);
            state.active = vimage_get_at(&state.images, state.selected);
            send_texture_to_gpu(state.active, state.filter);
            pthread_mutex_unlock(state.loader.mutex);
        }

        /* render */
        if(s_action.gl_update) {
            rendered = true;
            s_action.gl_update = false;
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            if(state.active && state.active->data) {
                glUseProgram(shader);

                glm_mat4_identity(s_state.image_view);
                glm_mat4_identity(s_state.image_transform);
                glm_mat4_identity(s_state.image_projection);

                /* fit */
                glm_scale(s_state.image_transform, (vec3){ s_state.wwidth, s_state.wheight, 0 });

                float x = (float)s_state.wwidth / (float)state.active->width;
                float y = (float)s_state.wheight / (float)state.active->height;
                switch(state.stretch) {
#if 0
                    case FIT_STRETCH_XY: { /* identity is ok */ } break;
#endif
                    case FIT_XY: {
                        float s = (float)s_state.wwidth / (float)s_state.wheight;
                        float r = (float)state.active->width / (float)state.active->height;
                        if(s > r) goto FIT_Y;
                        else goto FIT_X;
                    } break;
                    case FIT_X: FIT_X: {
                        state.zoom = x;
                    } goto FIT_PAN;
                    case FIT_Y: FIT_Y: {
                        state.zoom = y;
                    } goto FIT_PAN;
                    case FIT_PAN: FIT_PAN: {
                        glm_scale(s_state.image_transform, (vec3){ 1.0f/x, 1.0f/y, 0 });
                    } break;
                    default: break;
                }

                /* pan */
                mat4 m_pan;
                float pan_x = 2 * state.pan[0];
                float pan_y = 2 * state.pan[1];
                glm_mat4_identity(m_pan);
                glm_translate(m_pan, (vec3){ pan_x, pan_y, 0 });

                mat4 m_zoom;
                glm_mat4_identity(m_zoom);
                glm_scale(m_zoom, (vec3){ state.zoom, state.zoom, 0 });

                glm_mat4_mul(m_zoom, m_pan, s_state.image_view);


                /* scale back */
                glm_scale(s_state.image_projection, (vec3){ 1.0f/s_state.wwidth, 1.0f/s_state.wheight, 0 });

                /* send to gpu */
                glBindVertexArray(VAO);
                glUniformMatrix4fv(loc_view, 1, GL_FALSE, (float *)s_state.image_view);
                glUniformMatrix4fv(loc_projection, 1, GL_FALSE, (float *)s_state.image_projection);
                glUniformMatrix4fv(loc_transform, 1, GL_FALSE, (float *)s_state.image_transform);

                glDisable(GL_BLEND);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, state.active->texture);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            if(state.active && state.show_description) {
                glUseProgram(sh_text);
                glUniformMatrix4fv(loc_text_projection, 1, GL_FALSE, (float *)s_state.text_projection);

                vec2 text_pos = { 5, s_state.theight - font.height * 1.25 };

                vec4 text_dim;
                if(state.pan[0] || state.pan[1]) {
                    snprintf(str_info, sizeof(str_info), "[%zu/%zu] %s (%ux%ux%u) [%s @ %i/%i]", state.selected, vimage_length(state.images) ? vimage_length(state.images) - 1 : 0, state.active->filename, state.active->width, state.active->height, state.active->channels, fit_cstr(state.stretch), -(int)state.pan[0], -(int)state.pan[1]);
                } else {
                    snprintf(str_info, sizeof(str_info), "[%zu/%zu] %s (%ux%ux%u) [%s]", state.selected, vimage_length(state.images) ? vimage_length(state.images) - 1 : 0, state.active->filename, state.active->width, state.active->height, state.active->channels, fit_cstr(state.stretch));
                }

                font_render(font, sh_text, str_info, text_pos[0], text_pos[1], 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, false);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                box_render(sh_box, s_state.text_projection, text_dim, (vec4){0.0f, 0.0f, 0.0f, 0.7f});
                font_render(font, sh_text, str_info, text_pos[0], text_pos[1], 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, true);
            }

            glBindVertexArray(0);
            glfwSwapBuffers(window);
        }

        if(rendered) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }
        if(!run_timer) {
            glfwSetTime(0);
        }
    }

    /* we can free stuff when it's hidden :) */
    glfwHideWindow(window);
    glfwSwapBuffers(window);

    s_action.quit = true;

    shader_free(shader);
    font_free(&font);
    text_free();

    civ_free(&state);

    glfwTerminate();

    return 0;
}

