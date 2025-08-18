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

#include <stb/stb_image.h>
#include "gl_text.h"
#include <rlc/vec.h>
#include <rlc/array.h>
#include <rlc/err.h>
#include "civ.h"
#include "queue.h"

#include "gl_uniform.h"
#include "gl_box.h"
#include "glad.h"
#include "gl_image.h"
#include <pthread.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <time.h>
#include "timer.h"

#include "shaders/shaders_generated.h"

#include <unistd.h>
//#include <magic.h>

//static ActionMap s_action_init = {0};

//static ActionMap civ->action_map;
//static StateMap s_state;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Civ *civ = glfwGetWindowUserPointer(window);
    civ->action_map.resized = true;
    civ->state_map.wwidth = width;
    civ->state_map.wheight = height;
    glViewport(0, 0, width, height);
    glm_perspective(glm_rad(45), (float)width / (float)height, 0.1, 100, civ->state_map.image_projection);

#if 0
    /* update text projection */
    double ratio_want = 16.0 / 9.0;
    double ratio_have = (double)width / (double)height;
    double ratio2 = ratio_have / ratio_want;

    civ->state_map.twidth = 2560; // get monitor dimensions here ...
    civ->state_map.theight = 1920;
    if(ratio_have > ratio_want) {
        civ->state_map.twidth = round(civ->state_map.twidth * ratio2);
    } else if(ratio_have < ratio_want) {
        civ->state_map.theight = round(civ->state_map.theight / ratio2);
    }
#endif

    civ->state_map.twidth = civ->state_map.wwidth;
    civ->state_map.theight = civ->state_map.wheight;

    glm_ortho(0.0f, civ->state_map.twidth, 0.0f, civ->state_map.theight, -1.0f, 1.0f, civ->state_map.text_projection);

    civ->action_map.gl_update = true;
}

void process_input(GLFWwindow *window, double t_delta, bool *run_timer) {
    Civ *civ = glfwGetWindowUserPointer(window);
    bool t_run = false;
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        civ->action_map.quit = true;
    }
    if((glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS /*&& glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS*/) ||
       (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        civ->action_map.pan_x += t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        civ->action_map.pan_y -= t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        civ->action_map.pan_y += t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS /*&& glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS*/) ||
       (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        civ->action_map.pan_x -= t_delta * 500;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        civ->action_map.zoom += t_delta;
        t_run = true;
    };
    if((glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) ||
       (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
        civ->action_map.zoom -= t_delta;
        t_run = true;
    };
    *run_timer = t_run;
}

void key_callback(GLFWwindow* window, int key, int scancode, int act, int mods)
{
    Civ *civ = glfwGetWindowUserPointer(window);
    if(act == GLFW_PRESS || act == GLFW_REPEAT) {
        switch(key) {
            case GLFW_KEY_LEFT: {
                //if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                //    civ->action_map.select_previous = true;
                //}
            } break;
            case GLFW_KEY_RIGHT: {
                //if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                //    civ->action_map.select_next = true;
                //}
            } break;
            case GLFW_KEY_K: {
                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) {
                    --civ->action_map.select_image;
                }
            } break;
            case GLFW_KEY_J: {
                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) {
                    ++civ->action_map.select_image;
                }
            } break;
            case GLFW_KEY_R: {
                civ->action_map.select_random = true;
            } break;
        }
    }
    if(act == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_S: {
                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    civ->action_map.filter_next = true;
                } else {
                    civ->action_map.fit_next = true;
                }
            } break;
            case GLFW_KEY_F: { civ->action_map.toggle_fullscreen = true; } break;
            case GLFW_KEY_Q: { civ->action_map.quit = true; } break;
            case GLFW_KEY_D: { civ->action_map.toggle_description = true; } break;
            case GLFW_KEY_P: { civ->action_map.print_stdout = true; } break;
        }
    }
}

void process_action_map(GLFWwindow *window, Civ *civ) {

    bool update = civ->action_map.gl_update;
    ActionMap action_map_init = {0};
    if(memcmp(&action_map_init, &civ->action_map, sizeof(civ->action_map))) update = true;

    if(civ->action_map.quit) glfwSetWindowShouldClose(window, true);
    if(civ->action_map.resized) {}
    if(civ->action_map.toggle_fullscreen) {}

    civ_cmd_random(civ, civ->action_map.select_random);
    civ_cmd_print_stdout(civ, civ->action_map.print_stdout);
    civ_cmd_select(civ, civ->action_map.select_image);
    civ_cmd_fit(civ, civ->action_map.fit_next);
    civ_cmd_zoom(civ, civ->action_map.zoom);
    civ_cmd_filter(civ, civ->action_map.filter_next);
    civ_cmd_description(civ, civ->action_map.toggle_description);
    civ_cmd_pan(civ, (vec2){ civ->action_map.pan_x, civ->action_map.pan_y });

    civ->action_map = action_map_init;
    civ->action_map.gl_update = update;

}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    Civ *civ = glfwGetWindowUserPointer(window);
    civ->state_map.mouse_x = xpos;
    civ->state_map.mouse_y = ypos;
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if(state == GLFW_PRESS) {
        float dx = xpos - civ->state_map.mouse_down_x;
        float dy = ypos - civ->state_map.mouse_down_y;
        //printf("delta %f %f @ %f %f\n", dx, dy, xpos, ypos);
        civ->state_map.mouse_down_x = xpos;
        civ->state_map.mouse_down_y = ypos;
        civ->action_map.pan_x += dx;
        civ->action_map.pan_y += dy;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Civ *civ = glfwGetWindowUserPointer(window);
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &civ->state_map.mouse_down_x, &civ->state_map.mouse_down_y);
    }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    Civ *civ = glfwGetWindowUserPointer(window);
    civ->action_map.zoom += yoffset < 0 ? -0.25 : 0.25;
}


struct timespec diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

int main(const int argc, const char **argv) {
    if(argc < 1) return -1;
    if(!argv) return -1;
    if(!argv[0]) return -1;

    srand(time(0));

    int err = 0;
    bool quit_early = false;

    GLFWwindow *window = 0;
    Shader sh_text = {0};
    Shader sh_box = {0};
    Shader sh_rect = {0};
    bool font_loaded = false;

    Civ civ = {0};
    civ.filter = FILTER_NEAREST;
    civ.zoom.current = 1.0;

    civ_arg(&civ, argv[0]);
    TRYC(civ_config_defaults(&civ));
    TRYC(arg_parse(civ.arg, argc, argv, &quit_early));
    if(quit_early) goto clean;
    if(!civ.pending_pipe && !array_len(civ.filenames)) quit_early = true;

    pw_init(&civ.pw, civ.config.jobs);
    pw_dispatch(&civ.pw);

    pw_init(&civ.pipe_observer, 1);
    pw_dispatch(&civ.pipe_observer);

    civ.state_map.wwidth = 800;
    civ.state_map.wheight = 600;

    /**************/
    /* GLFW BEGIN */

    glfwInit();
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(civ.state_map.wwidth, civ.state_map.wheight, "civ", NULL, NULL);
    glfwSetWindowUserPointer(window, &civ);
    if(window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    framebuffer_size_callback(window, civ.state_map.wwidth, civ.state_map.wheight);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_MULTISAMPLE);

    /* OPENGL START */

#define SHADER_FROM(sh, x) do { \
        const char *sh_f0 = (char *)__##x##_frag; \
        const char *sh_v0 = (char *)__##x##_vert; \
        const unsigned int sh_fE = __##x##_frag_len; \
        const unsigned int sh_vE = __##x##_vert_len; \
        sh = shader_load(sh_v0, sh_vE, sh_f0, sh_fE); \
    } while(0)

    SHADER_FROM(sh_box, box);
    SHADER_FROM(sh_rect, rectangle);
    SHADER_FROM(sh_text, text);
    
    GlImage image = {0};
    gl_image_shader(&image, sh_rect);

    //glEnable(GL_DEPTH_TEST);

    civ.action_map.gl_update = true;

    text_init();
    glm_ortho(0.0f, civ.state_map.wwidth, 0.0f, civ.state_map.wheight, -1.0f, 1.0f, civ.state_map.image_projection);

    glm_mat4_identity(civ.state_map.image_view);
    glm_translate(civ.state_map.image_view, (vec3){0.0f, 0.0f, 0.0f});

    Font font = font_init(&civ.config.font_path, civ.config.font_size, 1.0, 1.5, 2048);
    font_loaded = true;
    civ.font = &font;
    font_shader(&font, sh_text);
    font_load(&font, 0, 256);

    char str_info[1024] = {0};
    char str_load[1024] = {0};
    char str_popup[1024] = {0};
    bool run_timer = true;

    //clock_gettime(CLOCK_REALTIME, &civ.state_map.t_now);
    glBindVertexArray(0);
    timer_start(&civ.state_map.t_global, CLOCK_REALTIME, 0);

    stbi_set_flip_vertically_on_load(true);

    QueueState qstate = {0};
    civ.qstate = &qstate;
    QueueDo qd = {
        .civ = &civ
    };

    So pwd = SO;
    for(size_t i = 0; i < array_len(civ.filenames); ++i) {
        So file_or_dir = array_at(civ.filenames, i);
        if(so_cmp(file_or_dir, so("."))) {
            queue_walk(file_or_dir, queue_do(&qd, file_or_dir));
        } else {
            so_env_get(&pwd, so("PWD"));
            //printff("PWD:%.*s",SO_F(pwd));
            queue_walk(pwd, queue_do(&qd, pwd));
        }
    }
    pw_when_done(&civ.pw, when_done_gathering, queue_do(&qd, SO));
    if(civ.pending_pipe) {
        if(civ.config.pipe_and_args || !(!civ.config.pipe_and_args && array_len(civ.filenames))) {
            pw_queue(&civ.pipe_observer, observe_pipe, queue_do(&qd, SO));
        }
    }

    for(;;) {
        if(glfwWindowShouldClose(window)) break;

        bool rendered = false;

        /* done, timer */
        if(run_timer) {
            timer_stop(&civ.state_map.t_global);
            timer_continue(&civ.state_map.t_global);
        }

        /* input */
        process_input(window, civ.state_map.t_global.delta, &run_timer);

        /* process */
        process_action_map(window, &civ);
#if 1
        pthread_mutex_lock(&civ.images_mtx);
        if(vimage_length(civ.images)) {
            if(civ.selected > vimage_length(civ.images)) civ.selected = vimage_length(civ.images) - 1;
            civ.active = vimage_get_at(&civ.images, civ.selected);
            send_texture_to_gpu(civ.active, civ.filter, &civ.action_map.gl_update);
            /* also make sure the full character set is available */
            So_Uc_Point point = {0};
            for(size_t i = 0; i < so_len(civ.active->filename); ++i) {
                if(so_uc_point(so_i0(civ.active->filename, i), &point)) {
                    THROW(ERR_UNREACHABLE("invalid utf8 codepoint"));
                }
                font_load_single(&font, point.val);
                i += (point.bytes - 1);
            }
        }
        pthread_mutex_unlock(&civ.images_mtx);
#endif

#if 0
        if(civ.config.qafl) {
            if(!pthread_mutex_trylock(&civ.images_mtx)) {
                if(civ.images_loaded >= vimage_length(civ.images)) {
                    civ.action_map.quit = true;
                }
                pthread_mutex_unlock(&civ.images_mtx);
            }
        }
#endif

        if(civ.popup.active && timer_timedout(&civ.popup.timer)) {
            civ_popup_set(&civ, POPUP_NONE);
            civ.action_map.gl_update = true;
        }

        /* render */
        if(civ.action_map.gl_update) {
            rendered = true;
            civ.action_map.gl_update = false;
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            if(civ.active && civ.active->data) {
                glUseProgram(sh_rect.id);

                glm_mat4_identity(civ.state_map.image_view);
                glm_mat4_identity(civ.state_map.image_transform);
                glm_mat4_identity(civ.state_map.image_projection);

                /* fit */
                glm_scale(civ.state_map.image_transform, (vec3){ civ.state_map.wwidth, civ.state_map.wheight, 0 });

                float s = (float)civ.state_map.wwidth / (float)civ.state_map.wheight;
                float r = (float)civ.active->width / (float)civ.active->height;
                float x = (float)civ.state_map.wwidth / (float)civ.active->width;
                float y = (float)civ.state_map.wheight / (float)civ.active->height;
                switch(civ.fit.current) {
                    case FIT_XY: {
                        if(s > r) goto FIT__Y;
                        else goto FIT__X;
                    } break;
                    case FIT_FILL: {
                        if(s > r) goto FIT__X;
                        else goto FIT__Y;
                    } break;
                    case FIT__X: FIT__X: {
                        civ.zoom.current = x;
                    } goto FIT_PAN;
                    case FIT__Y: FIT__Y: {
                        civ.zoom.current = y;
                    } goto FIT_PAN;
                    case FIT_PAN: FIT_PAN: {
                        glm_scale(civ.state_map.image_transform, (vec3){ 1.0f/x, 1.0f/y, 0 });
                    } break;
                    default: break;
                }

                /* pan */
                mat4 m_pan;
                float pan_x = 2 * civ.pan[0];
                float pan_y = 2 * civ.pan[1];
                glm_mat4_identity(m_pan);
                glm_translate(m_pan, (vec3){ pan_x, pan_y, 0 });

                mat4 m_zoom;
                glm_mat4_identity(m_zoom);
                glm_scale(m_zoom, (vec3){ civ.zoom.current, civ.zoom.current, 0 });

                glm_mat4_mul(m_zoom, m_pan, civ.state_map.image_view);

                /* scale back */
                glm_scale(civ.state_map.image_projection, (vec3){ 1.0f/civ.state_map.wwidth, 1.0f/civ.state_map.wheight, 0 });

                glDisable(GL_BLEND);
                gl_image_render(&image, civ.active->texture, civ.state_map.image_projection, civ.state_map.image_view, civ.state_map.image_transform);
            }

            if(civ.active && civ.config.show_description) {
                vec2 text_pos = { 5, civ.state_map.theight - font.height * 1.25 };

                vec4 text_dim;
                FitList fit = civ.fit.current;
                if(civ.pan[0] || civ.pan[1]) {
                    snprintf(str_info, sizeof(str_info), "[%zu/%zu] %.*s (%ux%ux%u) [%.1f%% %s @ %.0f,%.0f]", civ.selected + 1, vimage_length(civ.images), SO_F(civ.active->filename), civ.active->width, civ.active->height, civ.active->channels, 100.0f * civ.zoom.current, fit_cstr(fit), -civ.pan[0], -civ.pan[1]);
                } else {
                    snprintf(str_info, sizeof(str_info), "[%zu/%zu] %.*s (%ux%ux%u) [%.1f%% %s]", civ.selected + 1, vimage_length(civ.images), SO_F(civ.active->filename), civ.active->width, civ.active->height, civ.active->channels, 100.0f * civ.zoom.current, fit_cstr(fit));
                }

                font_render(font, str_info, civ.state_map.text_projection, text_pos, 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, TEXT_ALIGN_LEFT);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                box_render(sh_box, civ.state_map.text_projection, text_dim, (vec4){0.0f, 0.0f, 0.0f, 0.7f}, 6);
                font_render(font, str_info, civ.state_map.text_projection, text_pos, 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, TEXT_ALIGN_RENDER);
            }

            {
                pthread_mutex_lock(&civ.images_mtx);
                bool busy = pw_is_busy(&civ.pw);
                size_t len = civ.images_loaded;
                size_t cap = civ.config.image_cap;
                if(cap && len > cap) len = cap;
                if(civ.config.show_loaded && busy) {
                    if(cap) {
                        snprintf(str_load, sizeof(str_load), "Loaded %.1f%% (%zu/%zu)", 100.0 * (double)len / (double)cap, len, cap);
                    } else {
                        snprintf(str_load, sizeof(str_load), "Loaded %zu..", len);
                    }
                    vec2 text_pos = { civ.state_map.twidth - 5, civ.state_map.theight - font.height * 1.25 };
                    vec4 text_dim;
                    font_render(font, str_load, civ.state_map.text_projection, text_pos, 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, TEXT_ALIGN_RIGHT);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    box_render(sh_box, civ.state_map.text_projection, text_dim, (vec4){0.0f, 0.0f, 0.0f, 0.7f}, 6);
                    font_render(font, str_load, civ.state_map.text_projection, text_pos, 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, TEXT_ALIGN_RENDER);
                }
                pthread_mutex_unlock(&civ.images_mtx);
            }

            str_popup[0] = 0;
            switch(civ.popup.active) {
                case POPUP_DESCRIPTION: {
                    snprintf(str_popup, sizeof(str_popup), "%s", civ.config.show_description ? "show description" : "hide description");
                } break;
                case POPUP_SELECT: {
                    snprintf(str_popup, sizeof(str_popup), "[%zu/%zu] %s", civ.selected + 1, vimage_length(civ.images), fit_cstr(civ.fit.current));
                } break;
                case POPUP_FIT: {
                    snprintf(str_popup, sizeof(str_popup), "[%s]", fit_cstr(civ.fit.current));
                } break;
                case POPUP_PAN: {
                    snprintf(str_popup, sizeof(str_popup), "%.0f,%.0f", -civ.pan[0], -civ.pan[1]);
                } break;
                case POPUP_ZOOM: {
                    snprintf(str_popup, sizeof(str_popup), "%.1f%%", 100 * civ.zoom.current);
                } break;
                case POPUP_FILTER: {
                    snprintf(str_popup, sizeof(str_popup), "%s", filter_cstr(civ.filter));
                } break;
                case POPUP_PRINT_STDOUT: {
                    if(civ.active) snprintf(str_popup, sizeof(str_popup), "stdout: %.*s", SO_F(civ.active->filename));
                } break;
                default: case POPUP__COUNT: case POPUP_NONE: break;
            }
            if(strlen(str_popup)) {
                vec2 text_pos = { (float)civ.state_map.twidth / 2.0f, (float)civ.state_map.theight / 2.0f };
                vec4 text_dim;

                font_render(font, str_popup, civ.state_map.text_projection, text_pos, 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, TEXT_ALIGN_CENTER);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                box_render(sh_box, civ.state_map.text_projection, text_dim, (vec4){0.0f, 0.0f, 0.0f, 0.7f}, 6);
                font_render(font, str_popup, civ.state_map.text_projection, text_pos, 1.0, 1.0, (vec3){1.0f, 1.0f, 1.0f}, text_dim, TEXT_ALIGN_RENDER);
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
            timer_restart(&civ.state_map.t_global);
        }
    }

clean:
    /* we can free stuff when it's hidden :) */
    if(window) glfwHideWindow(window);
    //glfwSwapBuffers(window);

    civ.action_map.quit = true;

    shader_free(sh_rect);
    shader_free(sh_text);
    shader_free(sh_box);
    if(font_loaded) {
        font_free(&font);
    }
    text_free();

    civ_free(&civ);

    if(window) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    //printf("Done, quitting\n");

    return err;
error:
    ERR_CLEAN;
}

