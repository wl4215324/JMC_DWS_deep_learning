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
//#include "tools.h"

//#define DISPLAY_GRAY_PIC
using namespace std;
using namespace std::chrono;


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
#if 1
    "   float r, g, b, y, u, v;\n"
    "   y = texture2D(y_texture, yuvTexCoords).r;\n"
    "   u = texture2D(uv_texture, yuvTexCoords).a - 0.5;\n"
    "   v = texture2D(uv_texture, yuvTexCoords).r - 0.5;\n"
    "   r = y + 1.13983*v;\n"
    "   g = y - 0.39465*u - 0.58060*v;\n"
    "   b = y + 2.03211*u;\n"
    "   gl_FragColor = vec4(r, g, b, 1.0);\n"
#endif
    //"r = y;\n"
    //"g = y;\n"
    //"b = y;\n"
    //"   gl_FragColor = vec4(4, 23, 200, 1.0);\n"
    //"   gl_FragColor = texture2D(y_texture, yuvTexCoords);\n"
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

int FbDisplay::display_init()
{
    EGLint egl_major, egl_minor;
    EGLConfig config;
    EGLint num_config;
    EGLContext context;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLint ret;
    GLint width, height;
    GLenum erro_status;
#if 1
    con_data.in_w = 1280;
    con_data.in_h = 720;
    con_data.screen_w = 1024;
    con_data.screen_h = 600;
#endif

    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY) {
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

    return 0;
}

unsigned int FbDisplay::LoadTexture ( char *buffer)
{
   int width = con_data.in_w, height = con_data.in_h;

   glActiveTexture ( GL_TEXTURE0 );
   glGenTextures ( 1, &con_data.y_tex_id);
   glBindTexture ( GL_TEXTURE_2D, con_data.y_tex_id);

   if(dis_fmt == FB_DIS_NV21){
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer );
   }else if(dis_fmt == FB_DIS_RGB32){
       glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer );
   }

   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   if(dis_fmt == FB_DIS_NV21){

       glActiveTexture(GL_TEXTURE1);
       glGenTextures ( 1, &con_data.uv_tex_id);
       glBindTexture ( GL_TEXTURE_2D, con_data.uv_tex_id);

       glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width/2, height/2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buffer + width*height);

       glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
       glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
       glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
       glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
   }
   return 0;
}

FbDisplay::FbDisplay(DIS_FMT fmt):dis_fmt{fmt}
{
    display_init();
}

FbDisplay::~FbDisplay()
{

}

int FbDisplay::display_1280_720_nv21(char *buffer)
{
    auto t = steady_clock::now();
    LoadTexture(buffer);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glFlush();

    eglSwapBuffers(egl_display, egl_surface);

    glDeleteTextures(1, &con_data.y_tex_id);
    glDeleteTextures(1, &con_data.uv_tex_id);

    auto d = steady_clock::now()- t; // something took d time units
    //cout << "glTexImage2D took " << duration_cast<milliseconds>(d).count() << "ms"<<endl; // print as
    return 0;

}

//static GLfloat vVertices[] = {
/*
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
*/
int FbDisplay::setVertices(FbDisplay::DisRect *rect)
{
    vVertices[0] = rect->x;
    vVertices[1] = rect->y - rect->h;

    vVertices[3] = rect->x + rect->w;
    vVertices[4] = rect->y - rect->h;

    vVertices[6] = rect->x;
    vVertices[7] = rect->y;

    vVertices[9] = rect->x + rect->w;
    vVertices[10] = rect->y;

}

int FbDisplay::draw_pic_nv21(FbDisplay::BufData *buf, FbDisplay::DisRect *rect)
{
    int erro_status;
    //use program

    setVertices(rect);

    //config vertice and textcoords
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, vTextcoords);
    glEnableVertexAttribArray(1);
    if((erro_status = glGetError()))
        printf("Error:77 failed: 0x%08X\n",erro_status);


    int width = buf->w, height = buf->h;

    glActiveTexture ( GL_TEXTURE0 );
    glGenTextures ( 1, &con_data.y_tex_id);
    glBindTexture ( GL_TEXTURE_2D, con_data.y_tex_id);

    glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buf->buf);
#if 0
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
#else
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#endif


    glActiveTexture(GL_TEXTURE1);
    glGenTextures ( 1, &con_data.uv_tex_id);
    glBindTexture ( GL_TEXTURE_2D, con_data.uv_tex_id);

    glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width/2, height/2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buf->buf + width*height);

    //glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    //glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );


#if 0
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
#else
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#endif


    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


    glDeleteTextures(1, &con_data.y_tex_id);
    glDeleteTextures(1, &con_data.uv_tex_id);
    return 0;
}

int FbDisplay::flush_pic_nv21()
{
    glFlush();

    eglSwapBuffers(egl_display, egl_surface);

    return 0;
}
