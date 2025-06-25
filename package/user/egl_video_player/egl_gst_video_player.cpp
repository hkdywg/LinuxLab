
/****************************************************************************
 *  INCLUDES
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <pthread.h>

#include  <sys/time.h>
#include  <unistd.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm_fourcc.h"
#include "libkms.h"

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

#include "GLES3/gl32.h"
#include "GLES3/gl2ext.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "EGL/eglext_REL.h"

#include <gst/gst.h>
#include <gst/pbutils/gstdiscoverer.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <stdbool.h>

#include <sys/sysinfo.h>

typedef struct
{
    GstElement *source;
    GstElement *sink;
} ProgramData;

static GstAppSinkCallbacks loc_app_callbacks;
static ProgramData *loc_prog_data;
static GstAppSink *loc_vid_play_sink;
static struct timeval loc_time_stamp_base;
static double loc_rec_time;
static int loc_frame_rate = 0;
static NativeDisplayType loc_disp_port = 0;
static char *loc_mp4_file = NULL;
static bool loc_pri_mp4_info = false;

#define egl_gst_player_assert(_exp,_str)      if(!(_exp)){printf("\033[31;40m%s\033[0m",_str);exit(-1);}

/****************************************************************************
 *  DECLARATIONS
 ****************************************************************************/
void InitApplicationForPixmap(void);
void RenderToPixmap(void);
void CreateTexture(void);
void ChangeCurrent( GLuint );
void InitShaderForPixmap(void);
void SetupGLSLUniformForPixmap(void);
void DrawIndexStrips(void);
void *RenderThread(void *lpParameter);
void AppTerminate(void);
void *GstReplayThread(void *arg);
void InitApplicationForWindow(void);
void RenderToWindow(void);
void CreateTextureEGLImage( void );
void DrawSqure( void );
void InitShaderForWindow(void);
void SetupGLSLUniformForWindow( void );

/* EGL image function */
typedef EGLImageKHR (*EGLCREATEIMAGEKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, EGLint* attr_list);
typedef EGLBoolean (*EGLDESTROYIMAGEKHR)(EGLDisplay dpy, EGLImageKHR image);
static EGLCREATEIMAGEKHR eglCreateImageKHR;
static EGLDESTROYIMAGEKHR eglDestroyImageKHR;

/* EGL image to texture image function */
typedef void (*GLEGLIMAGETARGETRENDERBUFFERSTORAGEOES)(GLenum target, GLeglImageOES image);
static GLEGLIMAGETARGETRENDERBUFFERSTORAGEOES glEGLImageTargetTexture2D;

/****************************************************************************
 *  GLOBALS
 ****************************************************************************/

static int  g_nScreenWidth;
static int  g_nScreenHeight;

int         g_hThreadTerminateEvent;

const float  g_scaleFactor = 1.6f;

EGLDisplay  dpy;
EGLSurface  windowsurface;
EGLSurface  pixmapsurface;
EGLContext  context;

EGLImageKHR eglimage = EGL_NO_IMAGE_KHR;

GLuint      g_uiProgramObject;
GLuint      g_hTexture;
GLuint      g_hTexture_eglimage;
GLuint      g_uiFragShader, g_uiVertShader;

const char g_FragShader[] = "uniform   sampler2D    sampler2d;"
                            "varying   mediump vec2 vTexCoord;"
                            "void main (void)"
                            "{"
                            "    gl_FragColor  = texture2D(sampler2d, vTexCoord);"
                            "}";

const char g_VertShader[] = "attribute highp   vec3 aVertex;"
                            "attribute mediump vec3 aNormal;"
                            "attribute mediump vec2 aUv1;"
                            "attribute mediump vec4 aDiffuse;"
                            "uniform   mediump mat4 uPMVMatrix;"
                            "varying   mediump vec2 vTexCoord;"
                            "void main(void)"
                            "{"
                            "    gl_Position = uPMVMatrix * vec4(aVertex,1.0);"
                            "    vTexCoord.x = aUv1.s;"
                            "    vTexCoord.y = -aUv1.t;"
                            "}";
const char g_VertShaderCom[] = "attribute highp   vec3 aVertex;"
                               "attribute mediump vec2 aMultiTexCoord0;"
                               "uniform   mediump mat4 uPMVMatrix;"
                               "varying   mediump vec2 vTexCoord;"
                               "void main(void)"
                               "{"
                               "    gl_Position = uPMVMatrix * vec4(aVertex,1.0);"
                               "    vTexCoord.x   = aMultiTexCoord0.x;"
                               "    vTexCoord.y   = aMultiTexCoord0.y;"
                               "}";

GLuint      g_uiProgramObjectCom;
GLuint      g_uiFragShaderCom, g_uiVertShaderCom;


static bool g_replay = false;

static EGLNativePixmapTypeREL  sNativePixmap;

static struct 
{
    int fd;

    struct kms_driver *kms;
    kms_bo *bo;
    uint8_t *vaddr;
} g_sDrm;
 
/****************************************************************************
 *  DEFINES
 ****************************************************************************/

#define VERTEX_ARRAY        0
#define NORMAL_ARRAY        1
#define UV1_ARRAY           2
#define DIFFUSE_ARRAY       3
#define UV2_ARRAY           1

static char g_pid_Path[64];
#if 1
typedef struct
{
    char name[20];
    unsigned int user;
    unsigned int nice;
    unsigned int system;
    unsigned int idle;
    unsigned int iowait;
    unsigned int irq;
    unsigned int softirq;
}cpu_occupy_t;
static cpu_occupy_t loc_cpu_stat;

static double cal_cpuoccupy (cpu_occupy_t *o, cpu_occupy_t *n)
{
    double od, nd;
    double id, sd;
    double cpu_use ;
    
    od = (double) (o->user + o->nice + o->system +o->idle+o->softirq+o->iowait+o->irq);
    nd = (double) (n->user + n->nice + n->system +n->idle+n->softirq+n->iowait+n->irq);
    
    id = (double) (n->idle); 
    sd = (double) (o->idle);
    
    if((nd-od) != 0){
        cpu_use =100.0 - ((id-sd))/(nd-od)*100.00; 
    }
    else {
        cpu_use = 0.0;
    }
    return cpu_use;
}

static void get_cpuoccupy (cpu_occupy_t *cpust, const char *pid_path)
{
    FILE *fd;
    int n;
    char buff[256];
    cpu_occupy_t *cpu_occupy;
    cpu_occupy=cpust;
    
    fd = fopen (pid_path, "r");

    fgets (buff, sizeof(buff), fd);
    
    sscanf (buff, "%s %u %u %u %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle ,&cpu_occupy->iowait,&cpu_occupy->irq,&cpu_occupy->softirq);
    
    fclose(fd);
}

static double get_memory_usage (void)
{
    struct sysinfo info;

    sysinfo(&info);

    return ((info.totalram - info.freeram) / (info.totalram * 1.) * 100.);
}
#endif
/*******************************************************************************
 * Function Name  : InitApplicationForPixmap
 * Description    : Beginning process.
 *******************************************************************************/
void InitApplicationForPixmap( void )
{
    EGLBoolean eRetStatus;

    eRetStatus = eglMakeCurrent(dpy, pixmapsurface, pixmapsurface, context);
    egl_gst_player_assert((eRetStatus == EGL_TRUE), "eglMakeCurrent failed. \n");
    
    InitShaderForPixmap( );

    glClearColor( 0.f, 0.f, 0.f, 1.0f );
    glViewport( 0, 0, g_nScreenWidth, g_nScreenHeight );
}

/*******************************************************************************
 * Function Name  : RenderToPixmap
 * Description    : Drawing process to Pixmap.
 *******************************************************************************/
void RenderToPixmap( void )
{
    static int nCount = 1, nColor = 0;
    EGLBoolean eRetStatus;

    eRetStatus = eglMakeCurrent(dpy, pixmapsurface, pixmapsurface, context);
    egl_gst_player_assert((eRetStatus == EGL_TRUE), "eglMakeCurrent failed. \n");
    
    glViewport( 0, 0,  g_nScreenWidth, g_nScreenHeight  );

    glEnable( GL_CULL_FACE );
    
    ChangeCurrent( g_hTexture );

    SetupGLSLUniformForPixmap( );
    glUseProgram( g_uiProgramObject );
}

/*******************************************************************************
 * Function Name  : InitApplicationForWindow
 * Description    : Beginning process.
 *******************************************************************************/
void InitApplicationForWindow( void )
{
    EGLBoolean eRetStatus;

    eRetStatus = eglMakeCurrent(dpy, windowsurface, windowsurface, context);
    egl_gst_player_assert((eRetStatus == EGL_TRUE), "eglMakeCurrent failed. \n");
    
    // Create texture
    CreateTextureEGLImage();

    InitShaderForWindow( );

    glClearColor( 0.f, 0.f, 0.f, 1.0f );
    glViewport( 0, 0, g_nScreenWidth, g_nScreenHeight );
}

/*******************************************************************************
 * Function Name  : RenderToWindow
 * Description    : Drawing process.
 *******************************************************************************/
void RenderToWindow( void )
{
    static int nCount = 1, nColor = 0;
    EGLBoolean eRetStatus;

    eRetStatus = eglMakeCurrent(dpy, windowsurface, windowsurface, context);
    egl_gst_player_assert((eRetStatus == EGL_TRUE), "eglMakeCurrent failed. \n");

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );


    glViewport( 0, 0, g_nScreenWidth, g_nScreenHeight );

    // Beginning of scene
    glClear( GL_COLOR_BUFFER_BIT );
    glDisable( GL_CULL_FACE );

    SetupGLSLUniformForWindow( );
    glUseProgram( g_uiProgramObjectCom );
    
    DrawSqure();
}

/*******************************************************************************
 * Function Name  : CreateTextureEGLImage
 * Description    : Create texture for EGLImage.
 *******************************************************************************/
void CreateTextureEGLImage( void )
{
    glEGLImageTargetTexture2D(GL_TEXTURE_2D, eglimage);
}


/*******************************************************************************
 * Function Name  : ChangeCurrent
 * Description    : Chage current status.
 *******************************************************************************/
void ChangeCurrent( GLuint htex )
{
    // Set drawing texture
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, htex );

    glUniform1i( glGetUniformLocation( g_uiProgramObject, "sampler2d"), 0 );

    // Set texture quality
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glUniform1i( glGetUniformLocation( g_uiProgramObject, "uTexFlag"), 1 );
}

/*******************************************************************************
 * Function Name  : SetupGLSLUniformForPixmap
 * Description    : 
 *******************************************************************************/
void SetupGLSLUniformForPixmap( void )
{
    float idtMat[16] = {
        1., 0., 0., 0.,
        0., 1., 0., 0.,
        0., 0., 1., 0.,
        0., 0., 0., 1.
    };
    int i32Location = glGetUniformLocation( g_uiProgramObject, "uPMVMatrix" );
    glUniformMatrix4fv( i32Location, 1, GL_FALSE, idtMat);
}

/*******************************************************************************
 * Function Name  : SetupGLSLUniformForWindow
 * Description    :
 *******************************************************************************/
void SetupGLSLUniformForWindow( void )
{
    float idtMat[16] = {
        1., 0., 0., 0.,
        0., 1., 0., 0.,
        0., 0., 1., 0.,
        0., 0., 0., 1.
    };

    int i32Location = glGetUniformLocation( g_uiProgramObjectCom, "uPMVMatrix" );
    glUniformMatrix4fv( i32Location, 1, GL_FALSE, idtMat);
}
/*******************************************************************************
 * Function Name    : DrawSqure
 * Description      : 
 *******************************************************************************/
void DrawSqure( void )
{
    const GLfloat vertex_rect_strip[] = {
    //  left-bottom        left-top          right-bottom      right-top
        -1.0f,-1.0f,1.0f , -1.0f,1.0f,1.0f , 1.0f,-1.0f,1.0f , 1.0f,1.0f,1.0f
    };
    const GLfloat teximage_coord[] = {
        0.0f,1.0f , 0.0f, 0.0f , 1.0f,1.0f , 1.0f,0.0f
    };

    glEnableVertexAttribArray( VERTEX_ARRAY );
    glEnableVertexAttribArray( UV2_ARRAY );
    glVertexAttribPointer( VERTEX_ARRAY,  3, GL_FLOAT, GL_FALSE, 0, vertex_rect_strip );
    glVertexAttribPointer( UV2_ARRAY,     2, GL_FLOAT, GL_FALSE, 0, teximage_coord );

    /*
        - set a primitive type and a position of vertices
    */
    glDrawArrays(GL_TRIANGLE_STRIP , 0 , 4);

    glDisableVertexAttribArray( UV2_ARRAY );
    glDisableVertexAttribArray( VERTEX_ARRAY );
}

/*******************************************************************************
 * Function Name  : InitShaderForPixmap
 * Description    : 
 *******************************************************************************/
void InitShaderForPixmap( void )
{
    g_uiProgramObject = glCreateProgram( );
    g_uiVertShader = glCreateShader( GL_VERTEX_SHADER );
    g_uiFragShader = glCreateShader( GL_FRAGMENT_SHADER );
    
    const char* pData;

    GLint glerr;
    
    pData = g_VertShader;
    glShaderSource( g_uiVertShader, 1, &pData, NULL);
    glCompileShader(g_uiVertShader);

    pData = g_FragShader;
    glShaderSource( g_uiFragShader, 1, &pData, NULL);
    glCompileShader(g_uiFragShader);

    glAttachShader( g_uiProgramObject, g_uiVertShader );
    glAttachShader( g_uiProgramObject, g_uiFragShader );

    glBindAttribLocation( g_uiProgramObject, VERTEX_ARRAY,  "aVertex"  );
    glBindAttribLocation( g_uiProgramObject, NORMAL_ARRAY,  "aNormal"  );
    glBindAttribLocation( g_uiProgramObject, UV1_ARRAY,     "aUv1"     );
    glBindAttribLocation( g_uiProgramObject, DIFFUSE_ARRAY, "aDiffuse" );

    glLinkProgram( g_uiProgramObject );
    glUseProgram( g_uiProgramObject );

}

/*******************************************************************************
 * Function Name  : InitShaderForWindow
 * Description    : 
 *******************************************************************************/
void InitShaderForWindow( void )
{
    g_uiProgramObjectCom = glCreateProgram( );
    g_uiVertShaderCom = glCreateShader( GL_VERTEX_SHADER );
    g_uiFragShaderCom = glCreateShader( GL_FRAGMENT_SHADER );

    GLint glerr;
    const char* pData;
    
    pData = g_VertShaderCom;
    glShaderSource( g_uiVertShaderCom, 1, &pData, NULL);
    glCompileShader(g_uiVertShaderCom);

    pData = g_FragShader;
    glShaderSource( g_uiFragShaderCom, 1, &pData, NULL);
    glCompileShader(g_uiFragShaderCom);

    glAttachShader( g_uiProgramObjectCom, g_uiVertShaderCom );
    glAttachShader( g_uiProgramObjectCom, g_uiFragShaderCom );

    glBindAttribLocation( g_uiProgramObjectCom, VERTEX_ARRAY,  "aVertex"  );
    glBindAttribLocation( g_uiProgramObjectCom, UV2_ARRAY, "aMultiTexCoord0" );

    glLinkProgram( g_uiProgramObjectCom );
    glUseProgram( g_uiProgramObjectCom );
}

/*******************************************************************************
 * Function Name  : AcquireDRMMem
 * Description    : AcquireDRMMem
 *******************************************************************************/
void AcquireDRMMem(void)
{
    unsigned bo_attribs[] = {
                    KMS_WIDTH,     (unsigned int) g_nScreenWidth,
                    KMS_HEIGHT,    (unsigned int) g_nScreenHeight,
                    KMS_BO_TYPE,    KMS_BO_TYPE_SCANOUT_X8R8G8B8,
                    KMS_TERMINATE_PROP_LIST
    };


    g_sDrm.fd = drmOpen("rcar-du", NULL);
    if (g_sDrm.fd >= 0)
    {
        drmDropMaster(g_sDrm.fd);

        kms_create(g_sDrm.fd, &g_sDrm.kms);
        kms_bo_create(g_sDrm.kms, bo_attribs, &g_sDrm.bo);
        kms_bo_map(g_sDrm.bo, (void **)&g_sDrm.vaddr);
    }
    
    sNativePixmap.width     = (EGLint)g_nScreenWidth/g_scaleFactor;
    sNativePixmap.height    = (EGLint)g_nScreenHeight/g_scaleFactor;
    sNativePixmap.format    = EGL_NATIVE_PIXFORMAT_RGB565_REL;
    sNativePixmap.stride    = (EGLint) g_nScreenWidth/g_scaleFactor;
    sNativePixmap.usage     = 0;
    sNativePixmap.pixelData = g_sDrm.vaddr;
}

/*******************************************************************************
 * Function Name  : _get_timestamp
 * Description    : 
 *******************************************************************************/
static inline double _get_timestamp()
{
    struct timeval time_now;

    gettimeofday(&time_now,NULL);
    return ((time_now.tv_sec - loc_time_stamp_base.tv_sec) * 1.0 + (time_now.tv_usec - loc_time_stamp_base.tv_usec) / 1000000.0);
}
/*******************************************************************************
 * Function Name  : on_new_sample_from_source
 * Description    : 
 *******************************************************************************/

static GstFlowReturn on_new_sample_from_source (GstAppSink * elt, gpointer user_data)
{
    ProgramData *data = (ProgramData *) user_data;
    GstSample *sample;
    GstBuffer *buffer = 0;
    GstElement *source;
    GstCaps * caps;
    GstStructure *s;
    gint width, height;
    gboolean res;
    static uint32_t frame_cnt = 0;

    frame_cnt++;
    if(frame_cnt >= 60){
        
        double cur_time = _get_timestamp();
        cpu_occupy_t cur_cpu_stat;
        
        get_cpuoccupy((cpu_occupy_t *)&cur_cpu_stat,g_pid_Path);
        frame_cnt = 0;
        g_print("\r    %.2f/1          %.2f          %.2f", 60./(cur_time - loc_rec_time), cal_cpuoccupy(&loc_cpu_stat,&cur_cpu_stat),get_memory_usage());
        loc_rec_time = cur_time;
        loc_cpu_stat = cur_cpu_stat;
    }
    
    sample = gst_app_sink_pull_sample (GST_APP_SINK (elt));
    buffer = gst_sample_get_buffer (sample);
    caps = gst_sample_get_caps(sample);
    if (!caps) {
        g_print ("could not get snapshot format\n");
        exit (-1);
    }
    s = gst_caps_get_structure (caps, 0);

    /* we need to get the final caps on the buffer to get the size */
    res = gst_structure_get_int (s, "width", &width);
    res |= gst_structure_get_int (s, "height", &height);

    if (!res) {
        g_print ("could not get snapshot dimension\n");
        exit (-1);
    }
    gst_buffer_extract(buffer, 0, g_sDrm.vaddr, width*height*2);

    gst_buffer_unref(buffer);

    return GST_FLOW_OK;
}
/*******************************************************************************
 * Function Name  : usage
 * Description    : 
 *******************************************************************************/
static void usage()
{
    printf("\nUsage as follow:\n");
    printf("  -f --video file \n"
           "  -r --video replay enable\n"
           "  -v --output frame rate ,range:[1,60] \n"
           "  -d --output display port ,0:lvds0(C001) 1:lvds1(B561)\n"
           "  -p --print video informations:width/height/framerate/duration etc.\n"
           "\n\n  -h/H --print this help information \n\n"
          );
    exit(-1);
}
/*******************************************************************************
 * Function Name  : input_para_parse
 * Description    : 
 *******************************************************************************/
static const char optstr[] = "f:v:d:rphH";
static void input_para_parse(int argc, char *argv[])
{
     int c;
     uint8_t arg = 0;
     int tmp;
     
     while ((c = getopt(argc, argv, optstr)) != -1) {
         arg++;
         switch(c){
             case 'f':
                 loc_mp4_file = optarg;
                 break;
             case 'r':
                 g_replay = true;
                 printf("Video replay is On\n");
                 break;
             case 'p':
                 loc_pri_mp4_info = true;
                 break;
             case 'd':
                 tmp  = atoi(optarg);
                 
                 if((tmp > 2) || (tmp < 0)){
                     printf("Display port is invaild, use default port:0\n");
                 }
                 else {
                     loc_disp_port = NativeDisplayType(tmp);
                 }
                 break;
             case 'v':
                 loc_frame_rate = atoi(optarg);
                 if((loc_frame_rate < 0) || (loc_frame_rate > 60)){
                     loc_frame_rate = 0;
                     printf("Frame rate vaule is not valid, use video itself's frame rate. \n");
                 }
                 break;
             case 'h':
             case 'H':
                 usage();
                 break;
             default:
                 usage();
                 break;
         }
     }
     if((arg == 0) || (loc_mp4_file == NULL)){
         usage();
     }
}
/*******************************************************************************
 * Function Name  : egl_gst_lunch
 * Description    : 
 *******************************************************************************/
extern "C" int gst_discoverer_main ( const char *filename);

static int egl_gst_lunch(int w, int h)
{
    
    GError *error = NULL;
    GstBus *bus;
    int frame_rate = 0;
    char lunchPara[256] = "filesrc location=";
    char frame_out_para[256] = {0};
    const char appsink_para[] = "! appsink sync=1 name=video_play_sink";

    egl_gst_player_assert(((access(loc_mp4_file,F_OK) != -1)), "video file is not exist \n");

    if(loc_frame_rate > 0){
        sprintf(frame_out_para," ! qtdemux ! queue ! h264parse ! omxh264dec ! videorate ! video/x-raw, framerate=%d/1 "
                               "! vspfilter ! video/x-raw, format=RGB16, width=%d, height=%d ", loc_frame_rate, w, h);
    }
    else{
        sprintf(frame_out_para," ! qtdemux ! queue ! h264parse ! omxh264dec "
                "! vspfilter ! video/x-raw, format=RGB16, width=%d, height=%d ", w, h);
    }

    strcat(lunchPara,loc_mp4_file);
    strcat(lunchPara, frame_out_para);
    strcat(lunchPara, appsink_para);
    
    gettimeofday(&loc_time_stamp_base,NULL);
    gst_init (NULL, NULL);
    
    if(loc_pri_mp4_info){
        gst_discoverer_main(loc_mp4_file);
    }
    
    loc_prog_data = g_new0 (ProgramData, 1);
 
    g_print("lunch parameters: %s \n\n",lunchPara);
    loc_prog_data->source = gst_parse_launch (lunchPara,&error);

    loc_app_callbacks.new_sample = on_new_sample_from_source;
    loc_vid_play_sink = (GstAppSink *)gst_bin_get_by_name (GST_BIN (loc_prog_data->source), "video_play_sink");
    gst_app_sink_set_callbacks(loc_vid_play_sink,&loc_app_callbacks,loc_prog_data, NULL);
    egl_gst_player_assert((loc_vid_play_sink != NULL), "video play sink failed!\n");
    
    gst_element_set_state(loc_prog_data->source,GST_STATE_PLAYING);

    /* wait for preroll */
    gst_element_get_state (loc_prog_data->source, NULL, NULL, GST_CLOCK_TIME_NONE);

    
    g_usleep (50 * (G_USEC_PER_SEC / 1000));
    
    g_print("\r\033[1;32;40m FrameRate(fps)   Cpu Usage(%)    Ram Usage(%)\033[0m\n");

}

/*******************************************************************************
 * Function Name  : ReleaseDRMMem
 * Description    : 
 *******************************************************************************/
void ReleaseDRMMem(void)
{
    if (g_sDrm.fd >= 0)
    {
        kms_bo_destroy(&g_sDrm.bo);

        kms_destroy(&g_sDrm.kms);
        drmClose(g_sDrm.fd);
    }
}
/*******************************************************************************
 * Function Name  : signalRegister()
 * Description    : 
 *******************************************************************************/
static void signalProcess(int signo)
{
    (void)signo;
    
    g_hThreadTerminateEvent = 0;
}
/*******************************************************************************
 * Function Name  : signalRegister()
 * Description    : 
 *******************************************************************************/
static void signalRegister(void)
{
    signal(SIGINT,signalProcess); 
    signal(SIGTERM,signalProcess); 
}

/*******************************************************************************
 * Function Name  : main()
 * Description    : 
 *******************************************************************************/
int main(int argc, char** argv)
{
    EGLConfig configs[2];
    EGLBoolean eRetStatus;
    EGLint config_count;
    EGLint major, minor;
    EGLint cfg_attribs[] = {EGL_BUFFER_SIZE,     16,
                            EGL_DEPTH_SIZE,      8,
                            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT | EGL_PIXMAP_BIT,
                            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                            EGL_NONE};

    NativeWindowType eglWindow = 0;
    NativePixmapType eglPixmap = 0;

    pthread_t gst_id;
    pthread_t RenderThread_id;
    eglCreateImageKHR  = (EGLCREATEIMAGEKHR)eglGetProcAddress("eglCreateImageKHR");
    eglDestroyImageKHR = (EGLDESTROYIMAGEKHR)eglGetProcAddress("eglDestroyImageKHR");
    glEGLImageTargetTexture2D = (GLEGLIMAGETARGETRENDERBUFFERSTORAGEOES)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    
    input_para_parse(argc, argv);
    dpy = eglGetDisplay(loc_disp_port);
    egl_gst_player_assert((dpy != EGL_NO_DISPLAY),"eglInitialize failed\n");

    eRetStatus = eglInitialize(dpy, &major, &minor);
    egl_gst_player_assert((eRetStatus == EGL_TRUE),"eglInitialize failed\n");

    eRetStatus = eglBindAPI(EGL_OPENGL_ES_API);
    egl_gst_player_assert((eRetStatus == EGL_TRUE),"eglBindAPI failed\n");


    eRetStatus = eglChooseConfig(dpy, cfg_attribs, configs, 2, &config_count);

    windowsurface = eglCreateWindowSurface(dpy, configs[0], eglWindow, NULL);
    egl_gst_player_assert((windowsurface != EGL_NO_SURFACE),"eglCreateWindowSurface failed\n");
    
    // Get screen width and height
    eRetStatus = eglQuerySurface(dpy, windowsurface, EGL_WIDTH, &g_nScreenWidth);
    egl_gst_player_assert((eRetStatus == EGL_TRUE),"eglQuerySurface failed\n");
    
    eRetStatus = eglQuerySurface(dpy, windowsurface, EGL_HEIGHT, &g_nScreenHeight);
    egl_gst_player_assert((eRetStatus == EGL_TRUE),"eglQuerySurface failed\n");

    AcquireDRMMem();
    eglPixmap = (NativeWindowType)&sNativePixmap;
    pixmapsurface = eglCreatePixmapSurface(dpy, configs[0], eglPixmap, NULL);
    egl_gst_player_assert((pixmapsurface != EGL_NO_SURFACE),"eglCreatePixmapSurface failed\n");


    EGLint ai32ContextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context = eglCreateContext(dpy, configs[0], EGL_NO_CONTEXT, ai32ContextAttribs);
    egl_gst_player_assert((context != EGL_NO_CONTEXT),"eglCreateContext failed\n");



    eglimage = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, eglPixmap, NULL);
    egl_gst_player_assert((eglimage != EGL_NO_IMAGE_KHR),"eglCreateImageKHR failed.\n");


    g_hThreadTerminateEvent = 1;
 
    pthread_create(&RenderThread_id, NULL, RenderThread, NULL);
    pthread_create(&gst_id, NULL, GstReplayThread, NULL);

    sprintf(g_pid_Path,"/proc/stat");
    
    get_cpuoccupy((cpu_occupy_t *)&loc_cpu_stat,g_pid_Path);
    
    egl_gst_lunch((int)(g_nScreenWidth/g_scaleFactor), (int)(g_nScreenHeight/g_scaleFactor));
    
    signalRegister();
    pthread_join(RenderThread_id, NULL);
    pthread_join(gst_id, NULL);

    eglDestroyImageKHR(dpy, eglimage);

    eglDestroyContext(dpy, context);
    eglDestroySurface(dpy, windowsurface);
    eglDestroySurface(dpy, pixmapsurface);

    eglTerminate(dpy);

    ReleaseDRMMem();

    printf("%s Terminated.\n",argv[0]);

    return 0;

}
/***********************************************************************************
 * Function Name  : RenderThread()
 * Description    : Rendering Thread
 ************************************************************************************/
void *GstReplayThread(void *arg)
{
    while(g_hThreadTerminateEvent){

        if(g_replay){
            if(gst_app_sink_is_eos((GstAppSink *)loc_vid_play_sink)){
            
                printf("%lf : replay start!\n", _get_timestamp());
                gst_element_seek(loc_prog_data->source,
                                 1.0,
                                 GST_FORMAT_TIME,
                                 (GstSeekFlags)(GST_SEEK_FLAG_FLUSH |GST_SEEK_FLAG_KEY_UNIT), 
                                 GST_SEEK_TYPE_SET, 
                                 0,
                                 GST_SEEK_TYPE_SET,
                                 GST_CLOCK_TIME_NONE);
            }
        }
        usleep(500*1000);
    }
    
    return arg;
}
/***********************************************************************************
 * Function Name  : RenderThread()
 * Description    : Rendering Thread
 ************************************************************************************/
void *RenderThread(void *lpParameter)
{
    InitApplicationForPixmap();
    InitApplicationForWindow();

    while(g_hThreadTerminateEvent){

        RenderToPixmap();
        eglWaitGL();
        RenderToWindow();
        eglSwapBuffers(dpy, windowsurface);
    }

    AppTerminate();
    
    return lpParameter;
}

/***********************************************************************************
 * Function Name  : AppTerminate()
 * Description    : Terminate Process
 ************************************************************************************/
void AppTerminate(void)
{
    gst_element_set_state(loc_prog_data->source,GST_STATE_NULL);
    
    eglWaitGL();
    glDeleteTextures(1, &g_hTexture);

    eglSwapBuffers(dpy, windowsurface);
    glDeleteTextures(1, &g_hTexture_eglimage);

    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglReleaseThread();

    system("/usr/local/bin/dlcsrv_REL -q");
}
