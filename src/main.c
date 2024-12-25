#define STB_IMAGE_IMPLEMENTATION

//#include "sh_rect.h"

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
#include "civ.h"

#include "uniform.h"
#include "box.h"
#include "glad.h"
#include "gl_image.h"
#include <pthread.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <time.h>
#include "timer.h"

#include <unistd.h>
//#include <magic.h>

typedef struct ActionMap {
    bool gl_update;
    bool stretch_next;
    bool resized;
    bool filter_next;
    bool toggle_fullscreen;
    bool toggle_description;
    bool quit;
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

static ActionMap s_action;
static StateMap s_state;


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

void process_input(GLFWwindow *window, double t_delta, bool *run_timer) {
    bool t_run = false;
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        s_action.quit = true;
    }
    if((glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS /*&& glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS*/) ||
       (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.pan_x += t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.pan_y -= t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.pan_y += t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS /*&& glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS*/) ||
       (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.pan_x -= t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.zoom += t_delta;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        s_action.zoom -= t_delta;
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
                    --s_action.select_image;
                }
            } break;
            case GLFW_KEY_J: {
                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) {
                    ++s_action.select_image;
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

void process_action_map(GLFWwindow *window, Civ *state) {
    if(s_action.select_image) {
        s_action.gl_update = true;
        glm_vec2_zero(state->pan);
        state->zoom = 1.0f;
        size_t n_img = vimage_length(state->images);
        if(n_img) {
            if(s_action.select_image < 0) {
                size_t sel = (-s_action.select_image) % n_img;
                state->selected += n_img - sel;
            } else {
                state->selected += s_action.select_image;
            }
            state->selected %= n_img;
            state->fit.current = state->fit.initial;
        }
        //if(state->fit. == FIT_PAN && state->zoom != 1.0f) state->stretch = FIT_XY;
        s_action.select_image = 0;
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
        ++state->fit.initial;
        state->fit.initial %= FIT__COUNT;
        state->fit.current = state->fit.initial;
        state->zoom = 1.0;
        glm_vec2_zero(state->pan);
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
        state->fit.current = FIT_PAN;
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
        state->fit.current = FIT_PAN;
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
    s_action.zoom += yoffset < 0 ? -0.1 : 0.1;
}

int main(const int argc, const char **argv) {

    if(argc < 1) return -1;

    s_state.wwidth = 800;
    s_state.wheight = 600;

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
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(s_state.wwidth, s_state.wheight, "civ", NULL, NULL);
    if(window == NULL) {
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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    framebuffer_size_callback(window, s_state.wwidth, s_state.wheight);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_MULTISAMPLE);

    /* OPENGL START */
    Shader sh_text = shader_load(shaders, "text.vert", shaders, "text.frag");
    Shader sh_box = shader_load(shaders, "box.vert", shaders, "box.frag");
    Shader sh_rect = shader_load(shaders, "rectangle.vert", shaders, "rectangle.frag");
    int loc_projection = get_uniform(sh_rect, "projection");
    int loc_view = get_uniform(sh_rect, "view");
    int loc_transform = get_uniform(sh_rect, "transform");

    GlImage image = {0};
    gl_image_shader(&image, sh_rect);

    //glEnable(GL_DEPTH_TEST);

    s_action.gl_update = true;

    //text_init();
    glm_ortho(0.0f, s_state.wwidth, 0.0f, s_state.wheight, -1.0f, 1.0f, s_state.image_projection);

    glm_mat4_identity(s_state.image_view);
    glm_translate(s_state.image_view, (vec3){0.0f, 0.0f, 0.0f});

    //glUseProgram(0);
    int font_size = 24;
    //Font font = font_init("/usr/share/fonts/mikachan-font-ttf/mikachan.ttf", font_size, 1.0, 1.5, 1024);
    //Font font = font_init("/usr/share/fonts/lato/Lato-Regular.ttf", font_size, 1.0, 1.5, 1024);
    Font font = font_init("/usr/share/fonts/MonoLisa/ttf/MonoLisa-Regular.ttf", font_size, 1.0, 1.5, 1024);
    font_shader(&font, sh_text);
    font_load(&font, 0, 256);

    char str_info[1024] = {0};
    char str_load[1024] = {0};
    bool run_timer = true;

    //clock_gettime(CLOCK_REALTIME, &s_state.t_now);
    glBindVertexArray(0);
    timer_start(&s_state.t_global, CLOCK_REALTIME, 0);
    while(!glfwWindowShouldClose(window)) {
        bool rendered = false;

        /* done, timer */
        if(run_timer) {
            timer_stop(&s_state.t_global);
            timer_continue(&s_state.t_global);
        }

        /* input */
        process_input(window, s_state.t_global.delta, &run_timer);

        /* process */
        process_action_map(window, &state);

        if(state.selected < vimage_length(state.images)) {
            pthread_mutex_lock(state.loader.mutex);
            state.active = vimage_get_at(&state.images, state.selected);
            send_texture_to_gpu(state.active, state.filter, &s_action.gl_update);
            pthread_mutex_unlock(state.loader.mutex);
        }

        if(state.loader.done < vimage_length(state.images)) {
            s_action.gl_update = true;
        }

        /* render */
        if(s_action.gl_update) {
            rendered = true;
            s_action.gl_update = false;
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            if(state.active && state.active->data) {
                glUseProgram(sh_rect);

                glm_mat4_identity(s_state.image_view);
                glm_mat4_identity(s_state.image_transform);
                glm_mat4_identity(s_state.image_projection);

                /* fit */
                glm_scale(s_state.image_transform, (vec3){ s_state.wwidth, s_state.wheight, 0 });

                float s = (float)s_state.wwidth / (float)s_state.wheight;
                float r = (float)state.active->width / (float)state.active->height;
                float x = (float)s_state.wwidth / (float)state.active->width;
                float y = (float)s_state.wheight / (float)state.active->height;
                switch(state.fit.current) {
#if 0
                    case FIT_STRETCH_XY: { /* identity is ok */ } break;
#endif
                    case FIT_XY: {
                        if(s > r) goto FIT_Y;
                        else goto FIT_X;
                    } break;
                    case FIT_FILL_XY: {
                        if(s > r) goto FIT_X;
                        else goto FIT_Y;
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
                gl_image_render(&image, state.active->texture, s_state.image_projection, s_state.image_view, s_state.image_transform);
            }

            if(state.active && state.show_description) {
                vec2 text_pos = { 5, s_state.theight - font.height * 1.25 };

                vec4 text_dim;
                FitList fit = state.fit.current;
                if(state.pan[0] || state.pan[1]) {
                    snprintf(str_info, sizeof(str_info), "[%zu/%zu] %s (%ux%ux%u) [%.1f%% %s @ %i/%i]", state.selected + 1, vimage_length(state.images), state.active->filename, state.active->width, state.active->height, state.active->channels, 100.0f * state.zoom, fit_cstr(fit), -(int)state.pan[0], -(int)state.pan[1]);
                } else {
                    snprintf(str_info, sizeof(str_info), "[%zu/%zu] %s (%ux%ux%u) [%.1f%% %s]", state.selected + 1, vimage_length(state.images), state.active->filename, state.active->width, state.active->height, state.active->channels, 100.0f * state.zoom, fit_cstr(fit));
                }

                font_render(font, str_info, s_state.text_projection, text_pos[0], text_pos[1], 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, false);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                box_render(sh_box, s_state.text_projection, text_dim, (vec4){0.0f, 0.0f, 0.0f, 0.7f}, 6);
                font_render(font, str_info, s_state.text_projection, text_pos[0], text_pos[1], 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, true);
            }

            if(state.loader.done < vimage_length(state.images)) {
                snprintf(str_load, sizeof(str_load), "Loaded %zu/%zu", state.loader.done, vimage_length(state.images));

                vec2 text_pos = { 5, s_state.theight - font.height * 1.25 * 4 };
                vec4 text_dim;

                font_render(font, str_load, s_state.text_projection, text_pos[0], text_pos[1], 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, false);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                box_render(sh_box, s_state.text_projection, text_dim, (vec4){0.0f, 0.0f, 0.0f, 0.7f}, 6);
                font_render(font, str_load, s_state.text_projection, text_pos[0], text_pos[1], 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, true);
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
            timer_restart(&s_state.t_global);
        }
    }

    /* we can free stuff when it's hidden :) */
    glfwHideWindow(window);
    //glfwSwapBuffers(window);

    s_action.quit = true;

    shader_free(sh_rect);
    shader_free(sh_text);
    shader_free(sh_box);
    font_free(&font);
    text_free();

    civ_free(&state);

    glfwDestroyWindow(window);
    glfwTerminate();
    printf("done\n");

    return 0;
}

