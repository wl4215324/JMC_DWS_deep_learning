#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/fb.h>
#include <errno.h>
#include <iostream>
#include <chrono>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "gl_display.h"

#include "t7_camera_v4l2.h"
#include "disp_num_on_image.h"
#include "serial_pack_parse.h"


#define DISPLAY_NV21 1
#define DISPLAY_NV21_GRAY
#define BUFFER_WIDTH 1280
#define BUFFER_HEIGHT 720

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600
//#define SCREEN_HEIGHT 576

//#define SCREEN_WIDTH 338
//#define SCREEN_HEIGHT 600

using namespace std;

static struct con_data_st{
    GLint y_loc;
    GLint uv_loc;
    GLuint  y_tex_id;
    GLuint  uv_tex_id;
    int screen_w;
    int screen_h;
    int in_w;
    int in_h;
}con_data;

static const char *vertex_shader_source =
                "attribute vec4 aPosition;    \n"
                "attribute vec2 TexCoords;    \n"
                "varying vec2 yuvTexCoords;\n"
                "void main()                  \n"
                "{                            \n"
                "    yuvTexCoords = TexCoords ;\n"
                "    gl_Position = aPosition; \n"
                "}                            \n";

static const char *fragment_shader_source =

    "precision mediump float;\n"

    "varying vec2 yuvTexCoords;\n"
    "uniform sampler2D y_texture;\n"
    "uniform sampler2D uv_texture;\n"

    "void main (void){\n"
#if DISPLAY_NV21
    "   float r, g, b, y, u, v;\n"

    "   y = texture2D(y_texture, yuvTexCoords).r;\n"
        #ifndef DISPLAY_NV21_GRAY

    "   u = texture2D(uv_texture, yuvTexCoords).a - 0.5;\n"
    "   v = texture2D(uv_texture, yuvTexCoords).r - 0.5;\n"
    "   r = y + 1.13983*v;\n"
    "   g = y - 0.39465*u - 0.58060*v;\n"
    "   b = y + 2.03211*u;\n"
        #else
    "r = y;\n"
    "g = y;\n"
    "b = y;\n"
        #endif


    "   gl_FragColor = vec4(r, g, b, 1.0);\n"
#endif
    "   gl_FragColor = texture2D(y_texture, yuvTexCoords);\n"

    "}\n";


static GLfloat vVertices[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
};

static GLfloat vTextcoords[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f,0.0f,
};

static EGLint const config_attribute_list[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_BUFFER_SIZE, 32,

        EGL_STENCIL_SIZE, 0,
        EGL_DEPTH_SIZE, 0,
        EGL_SAMPLE_BUFFERS, 1 ,
        EGL_SAMPLES, 4,

        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,

        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PIXMAP_BIT,

        EGL_NONE
};

static EGLint window_attribute_list[] = {
        EGL_NONE
};

static const EGLint context_attribute_list[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
};

EGLDisplay egl_display;
EGLSurface egl_surface;

GLuint LoadTexture ( char *fileName );

int display_init()
{
    EGLint egl_major, egl_minor;
    EGLConfig config;
    EGLint num_config;
    EGLContext context;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLint ret;
    GLint width, height;
    GLenum erro_status;
#if 1
    con_data.in_w = BUFFER_WIDTH;
    con_data.in_h = BUFFER_HEIGHT;
    con_data.screen_w = SCREEN_WIDTH;
    con_data.screen_h = SCREEN_HEIGHT;
#endif

    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
    {
        fprintf(stderr, "Error: No display found!\n");
        return -1;
    }

    if (!eglInitialize(egl_display, &egl_major, &egl_minor)) {
        fprintf(stderr, "Error: eglInitialise failed!\n");
        return -1;
    }

    printf("EGL Version: \"%s\"\n",
           eglQueryString(egl_display, EGL_VERSION));
    printf("EGL Vendor: \"%s\"\n",
           eglQueryString(egl_display, EGL_VENDOR));
    printf("EGL Extensions: \"%s\"\n",
           eglQueryString(egl_display, EGL_EXTENSIONS));
    printf("\n");

    eglChooseConfig(egl_display, config_attribute_list, &config, 1, &num_config);

    context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT,
                               context_attribute_list);
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "Error: eglCreateContext failed: 0x%08X\n",
                eglGetError());
        return -1;
    }
    struct fbdev_window native_window;

    native_window.width = con_data.screen_w;
    native_window.height = con_data.screen_h;


    egl_surface = eglCreateWindowSurface(egl_display, config,
                                         &native_window,
                                         window_attribute_list);
    if (egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "Error: eglCreateWindowSurface failed: "
                        "0x%08X\n", eglGetError());
        return -1;
    }

    if (!eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &width) ||
                    !eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &height)) {
        fprintf(stderr, "Error: eglQuerySurface failed: 0x%08X\n",
                eglGetError());
        return -1;
    }
    printf("Surface size: %dx%d\n", width, height);

    if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, context)) {
        fprintf(stderr, "Error: eglMakeCurrent() failed: 0x%08X\n",
                eglGetError());
        return -1;
    }

    printf("GL Vendor: \"%s\"\n", glGetString(GL_VENDOR));
    printf("GL Renderer: \"%s\"\n", glGetString(GL_RENDERER));
    printf("GL Version: \"%s\"\n", glGetString(GL_VERSION));
    printf("GL Extensions: \"%s\"\n", glGetString(GL_EXTENSIONS));

    //vertex_shader
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertex_shader) {
        fprintf(stderr, "Error: glCreateShader(GL_VERTEX_SHADER) "
                        "failed: 0x%08X\n", glGetError());
        return -1;
    }
    printf("after glCreateShader\n\n");

    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
    if (!ret) {
        char *log;

        fprintf(stderr, "Error: vertex shader compilation failed!\n");
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &ret);

        if (ret > 1) {
            log = (char *)malloc(ret);
            glGetShaderInfoLog(vertex_shader, ret, NULL, log);
            fprintf(stderr, "%s", log);
            free ( log );
        }
        glDeleteShader ( vertex_shader );
        return -1;
    }
    //fragment_shader
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragment_shader) {
        fprintf(stderr, "Error: glCreateShader(GL_FRAGMENT_SHADER) "
                        "failed: 0x%08X\n", glGetError());
        return -1;
    }

    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
    if (!ret) {
        char *log;

        fprintf(stderr, "Error: fragment shader compilation failed!\n");
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &ret);

        if (ret > 1) {
            log = (char *)malloc(ret);
            glGetShaderInfoLog(fragment_shader, ret, NULL, log);
            fprintf(stderr, "%s", log);
            free ( log );
        }
        glDeleteShader ( fragment_shader );
        return -1;
    }

    //create program
    program = glCreateProgram();
    if (!program) {
        fprintf(stderr, "Error: failed to create program!\n");
        return -1;
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glBindAttribLocation(program, 0, "aPosition");
    glBindAttribLocation(program, 1, "TexCoords");

    //glLinkProgram
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &ret);
    if (!ret) {
        char *log;

        fprintf(stderr, "Error: program linking failed!\n");
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ret);

        if (ret > 1) {
            log = (char *)malloc(ret);
            glGetProgramInfoLog(program, ret, NULL, log);
            fprintf(stderr, "%s", log);
            free ( log );
        }
        glDeleteShader ( program );
        return -1;
    }
    glUseProgram(program);
    if((erro_status = glGetError()))
        printf("Error: 6666 failed: 0x%08X\n",erro_status);

    //config yuvTexSampler

    con_data.y_loc = glGetUniformLocation(program, "y_texture");
    glUniform1i(con_data.y_loc, 0);
    if((erro_status = glGetError())){
        printf("Error: 88 failed: 0x%08X\n",erro_status);
        exit(-1);
    }

    con_data.uv_loc = glGetUniformLocation(program, "uv_texture");
    glUniform1i(con_data.uv_loc, 1);
    if((erro_status = glGetError())){
        printf("Error: 88 failed: 0x%08X\n",erro_status);
        exit(-1);
    }
    glClearColor(0.2, 0.2, 0.2, 1.0);

    //config vertice and textcoords
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, vTextcoords);
    glEnableVertexAttribArray(1);
    if((erro_status = glGetError()))
        printf("Error:77 failed: 0x%08X\n",erro_status);

    return 0;
}

GLuint LoadTexture ( char *buffer)
{
   int width = con_data.in_w, height = con_data.in_h;

   glActiveTexture ( GL_TEXTURE0 );
   glGenTextures ( 1, &con_data.y_tex_id);
   glBindTexture ( GL_TEXTURE_2D, con_data.y_tex_id);
#if DISPLAY_NV21
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer );
#else
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer );
#endif
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

#ifndef DISPLAY_GRAY_PIC
   glActiveTexture(GL_TEXTURE1);
   glGenTextures ( 1, &con_data.uv_tex_id);
   glBindTexture ( GL_TEXTURE_2D, con_data.uv_tex_id);

   glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width/2, height/2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buffer + width*height);

   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
#endif
   return 0;
}

FbDisplay::FbDisplay()
{
    display_init();
}

FbDisplay::~FbDisplay()
{

}

int FbDisplay::display_buffer(char *buffer)
{
    LoadTexture(buffer);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glFlush();

    eglSwapBuffers(egl_display, egl_surface);
    glDeleteTextures(1, &con_data.y_tex_id);
    glDeleteTextures(1, &con_data.uv_tex_id);

    return 0;
}


int display_buffer(char *buffer)
{
    LoadTexture(buffer);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glFlush();

    eglSwapBuffers(egl_display, egl_surface);
    glDeleteTextures(1, &con_data.y_tex_id);
    glDeleteTextures(1, &con_data.uv_tex_id);

    return 0;
}

void* disp_image(void* argv)
{
	char disp_yuv420_buf[1280*720*3/2] = {0,};
	usleep(500000);
	display_init();
	Hz32Init();
	char disp_str[4] = {'\0', };

	while(true)
	{
		memcpy(disp_yuv420_buf, YUV420_buf, sizeof(YUV420_buf));
		//DrawNums32(100, 20, serial_output_var.reserved, (unsigned char *)disp_yuv420_buf);
		pthread_mutex_lock(&serial_output_var_mutex);
		sprintf(disp_str, "%d", serial_output_var.close_eye_one_level_warn);
		pthread_mutex_unlock(&serial_output_var_mutex);
		disp_str_on_monitor(50, 20, disp_str, (unsigned char *)disp_yuv420_buf);
		display_buffer(disp_yuv420_buf);
		usleep(25000);
	}

	return NULL;
}
