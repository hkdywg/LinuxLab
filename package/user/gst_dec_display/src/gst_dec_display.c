#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <gst/gst.h>

#include "parameter_parser.h"

struct gst_handle {
    GMainLoop *loop;
    GstBus *bus;
    GstElement *pipeline;
    GstMessage *msg;
};

struct gst_eles {
    GstElement *source;
    GstElement *demuxer;
    GstElement *depay;
    GstElement *parser;
    GstElement *decoder;
    GstElement *sink;
};

struct gst_obj {
    struct gst_handle handle;
    struct gst_eles elements;
};

struct rtsp_probe_ctx {
    GMainLoop *loop;
    gboolean   success;
};


static struct gst_obj *gst_obj = NULL;
static bool g_quit = false;
static bool g_replay = false;
static bool g_reconnect = false;


static gboolean probe_bus_cb(GstBus *bus, GstMessage *msg, gpointer data)
{
    struct rtsp_probe_ctx *ctx = data;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
        GError *err = NULL;
        gst_message_parse_error(msg, &err, NULL);
        g_printerr("RTSP probe error: %s\n", err->message);
        g_error_free(err);
        g_main_loop_quit(ctx->loop);
        break;
    }
    case GST_MESSAGE_ASYNC_DONE:
        ctx->success = TRUE;
        g_main_loop_quit(ctx->loop);
        break;
    default:
        break;
    }
    return TRUE;
}

static void pad_added_cb(GstElement *src, GstPad *pad, gpointer user_data)
{
    GstElement *sink = user_data;
    GstPad *sinkpad = gst_element_get_static_pad(sink, "sink");

    if (!gst_pad_is_linked(sinkpad)) {
        gst_pad_link(pad, sinkpad);
    }
    gst_object_unref(sinkpad);
}

static bool rtsp_server_ready(const char *url)
{
    GstElement *pipeline, *src, *sink;
    GstBus *bus;
    struct rtsp_probe_ctx ctx = {0};

    pipeline = gst_pipeline_new("rtsp-probe");
    src = gst_element_factory_make("rtspsrc", NULL);
    sink = gst_element_factory_make("fakesink", NULL);

    if (!pipeline || !src || !sink)
        return false;

    ctx.loop = g_main_loop_new(NULL, FALSE);
    ctx.success = FALSE;

    g_object_set(src,
        "location", url,
        "latency", 50,
        NULL);

    g_signal_connect(src, "pad-added", G_CALLBACK(pad_added_cb), sink);

    gst_bin_add_many(GST_BIN(pipeline), src, sink, NULL);

    bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, probe_bus_cb, &ctx);
    gst_object_unref(bus);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    //g_timeout_add(timeout_ms, (GSourceFunc)g_main_loop_quit, ctx.loop);
    g_main_loop_run(ctx.loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(ctx.loop);

    return ctx.success;
}

/* This thread is used to calculate the time spent */
static void *timing_thread(void *arg) {
    static struct timeval t_start, t_current;
    int duration = 0;
    int hour = 0, min = 0, sec = 0;

    /* get start timestamp */
    gettimeofday(&t_start, NULL);
    printf("starting play video \n");

    while (!g_quit) {
        /* get the current time */
        gettimeofday(&t_current, NULL);
        
        /* calculate thread duration time */
        duration = (t_current.tv_sec - t_start.tv_sec);
        hour     = duration / 3600;
        min      = duration / 60 - (hour * 60);
        sec      = duration % 60;

        printf("-- Duration %02d:%02d:%02d --", hour, min, sec);

        /* Clear the read-write buffer */
        fflush(stdout);
        /* Clear content from cursor to end of line */
        printf("\r\033[k");
        sleep(1);
    }
    
    printf("Stoping\n");
    return (void *)0;
}

#if 0
static void on_rtsp_pad_added(GstElement *src, GstPad *pad, gpointer data)
{
    GstElement *depay = (GstElement *)data;
    GstPad *sinkpad = gst_element_get_static_pad(depay, "sink");

    if (!gst_pad_is_linked(sinkpad)) {
        if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
            g_printerr("RTSP pad link failed\n");
        }
    }

    gst_object_unref(sinkpad);
}
#else
static void on_rtsp_pad_added(GstElement *src, GstPad *pad, gpointer data)
{
    GstElement *depay = (GstElement *)data;
    GstPad *sinkpad = gst_element_get_static_pad(depay, "sink");

    if (gst_pad_is_linked(sinkpad)) {
        gst_object_unref(sinkpad);
        return;
    }

    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps)
        caps = gst_pad_query_caps(pad, NULL);

    if (caps && gst_caps_is_fixed(caps)) {
        const GstStructure *str = gst_caps_get_structure(caps, 0);
        const gchar *name = gst_structure_get_name(str);

        if (g_str_has_prefix(name, "application/x-rtp")) {
            if (gst_pad_link(pad, sinkpad) == GST_PAD_LINK_OK) {
                g_print("Linked RTSP src pad to depayloader\n");
            } else {
                g_printerr("Failed to link RTSP src pad to depayloader\n");
            }
        }
    }

    if (caps) gst_caps_unref(caps);
    gst_object_unref(sinkpad);
}
#endif

/**
 * Gstreamer pipeline message bus callback function
 */
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
	struct gst_obj *obj = (struct gst_obj *)data;
	GMainLoop *loop = (&obj->handle)->loop;
    gchar  *debug;
    GError *error;

    switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_SEGMENT_DONE:
		gst_element_seek(
			obj->handle.pipeline,
			1.0,
			GST_FORMAT_TIME,
			GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
			GST_SEEK_TYPE_SET, 0,
			GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
		break;
    case GST_MESSAGE_EOS:
        if(g_replay) {
            g_print("End of stream, restarting ...\n");
            gst_element_seek(
                obj->handle.pipeline,
                1.0,
                GST_FORMAT_TIME,
                GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
                GST_SEEK_TYPE_SET, 0,
                GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
        } else {
            g_print("End of stream\n");
            g_main_loop_quit(loop);
        }
        break;

    case GST_MESSAGE_ERROR:
		gst_message_parse_error(msg, &error, &debug);
		g_printerr("RTSP error: %s\n", error->message);
		g_error_free(error);
		g_free(debug);

		g_main_loop_quit(loop);   
		g_reconnect = true;       
        break;

    default:
        break;
    }
    return true;
}

static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {

    GstPad *sinkpad;
    GstElement *parser = (GstElement*)data;
    GstCaps *caps;
    gchar *padname;

    padname = gst_pad_get_name(pad);
    g_print("New pad '%s' detected.\n", padname);
    g_free(padname);

    caps = gst_pad_query_caps(pad, NULL);
    if (!gst_caps_is_fixed(caps)) {
        printf("Pad caps are not fixed.\n");
        return;
    }

    /* Create an element to connect the src pad of the demuxer to the sink pad of the parser */
    sinkpad = gst_element_get_static_pad(parser, "sink");
    if (GST_PAD_IS_LINKED(sinkpad)) {
        printf("Parser sink pad is already linked.\n");
        return;
    }

    if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
        printf("Failed to link qtdemuxer and h264parser.\n");
    }

    gst_object_unref(sinkpad);
    gst_caps_unref(caps);
}

static void on_decodebin_pad_added(GstElement *decodebin,
                                   GstPad *pad,
                                   gpointer data)
{
    GstElement *convert = (GstElement *)data;
    GstPad *sinkpad = gst_element_get_static_pad(convert, "sink");

    if (gst_pad_is_linked(sinkpad)) {
        gst_object_unref(sinkpad);
        return;
    }

    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps)
        caps = gst_pad_query_caps(pad, NULL);

    if (caps) {
        gchar *caps_str = gst_caps_to_string(caps);
        g_print("decodebin caps: %s\n", caps_str);
        g_free(caps_str);
        gst_caps_unref(caps);
    }

    if (gst_pad_link(pad, sinkpad) == GST_PAD_LINK_OK)
        g_print("decodebin linked to videoconvert\n");

    gst_object_unref(sinkpad);
}

/**
 * signal handler function
 */
static void sig_handle(int signal) {
    printf("\n\n\r\033[k");
    if (gst_obj != NULL)
    {
         g_main_loop_quit(gst_obj->handle.loop);
    }
    g_quit = true;
    sleep(1);
}

/**
 * initialize gst object
 */
bool initialize_gst(struct gst_handle *handle) {
    
    if (handle == NULL)
        return false;

    handle->loop = g_main_loop_new(NULL, FALSE);

    /* Create a new pipeline */
    handle->pipeline = gst_pipeline_new("_pipeline");
    if (!handle->pipeline)
        return false;

    return true;
}

/**
 * Create the elements 
 */
bool make_gst_elements(struct gst_eles *elements , struct _Params *params) {
    
    if (params == NULL || elements == NULL) {
        return false;
	}

    if(params->is_rtsp) {
        /* Create the elements */
        elements->source        = gst_element_factory_make("rtspsrc", "_rtspsrc");
        elements->depay         = gst_element_factory_make("rtph264depay", "_rtph264depay");
        if(params->use_wayland) {
            elements->decoder   = gst_element_factory_make("decodebin", "_decodebin");
            elements->sink      = gst_element_factory_make("waylandsink", "_waylandsink");
            if (!elements->source || !elements->sink || !elements->decoder || !elements->depay) {
                printf("Failed to create uridecodebin or waylandsink\n");
                return false;
            }
        } else {
            elements->parser    = gst_element_factory_make("h264parse", "_h264parse");
            elements->decoder   = gst_element_factory_make("mppvideodec", "_mppvideodec");
            elements->sink      = gst_element_factory_make("kmssink", "_kmssink");
            if (!elements->source || !elements->sink || !elements->decoder || !elements->depay || !elements->parser) {
                printf("Failed to create rtsp sink\n");
                return false;
            }
        }
    } else {
        /* Initialization of elements */
        elements->source      = gst_element_factory_make("filesrc", "_filesrc");
        elements->demuxer     = gst_element_factory_make("qtdemux", "_qtdemux");
        if(params->use_wayland) {
            elements->decoder = gst_element_factory_make("decodebin", "_decodebin");
            elements->sink    = gst_element_factory_make("waylandsink", "_waylandsink");
        } else {
            elements->decoder = gst_element_factory_make("mppvideodec", "_mppvideodec");
            elements->sink    = gst_element_factory_make("kmssink", "_kmssink");
        }

        /*Determine the parameters entered by the user*/
        if(strcmp(params->h26x,"h264") == 0 )
        {
            elements->parser = gst_element_factory_make("h264parse", "_h264parse");
        }
        else if(strcmp(params->h26x,"h265") == 0 )
        {
            elements->parser = gst_element_factory_make("h265parse", "_h265parse");   
        }
        else
        {   
            printf("\033[34m please input h264 or h265\n \033[0m");
            return false;
        }
    }

    return true;
}

/**
 * check if element was created successfully
 */
bool check_gst_elements(struct gst_eles *elements) {
        
    if (elements == NULL)
    return false;

#if 0
    /*Check if the element was successfully created*/
    if( !elements->source  ||
        !elements->parser || !elements->decoder || 
        !elements->sink)
    {
        printf("Failed to create elements. Exiting.\n");
        return false;
    }
#endif
 
    return TRUE ;
}

/**
 * config elements
 */
bool config_gst_elements(struct _Params *params, struct gst_eles *elements) {

    if (params == NULL || elements == NULL)
        return false;

    if(params->is_rtsp) {
        /* Set rtsp url*/
		g_object_set(G_OBJECT(elements->source), "location", params->rtsp_url, NULL);
    } else {
        /* Set filesrc Path */
        g_object_set(G_OBJECT(elements->source), "location", params->location, NULL);
    }

    /*Config the kmssink */
    if(!params->use_wayland) {
        if(params->plane_id)
            g_object_set(elements->sink, "connector-id", params->connector_id, 
                "plane-id", params->plane_id, "fullscreen", TRUE, NULL);
        else
            g_object_set(elements->sink, "connector-id", params->connector_id,
                         "fullscreen", TRUE, NULL);
    }	

    return true;
}

/**
 * link elements
 */
bool link_gst_elements(struct gst_eles *elements, GstElement *pipeline) {
    
    if (elements == NULL || pipeline == NULL) {
        return false;
	}

	if (elements->decoder && g_str_has_prefix(GST_OBJECT_NAME(elements->decoder), "_decodebin")) {
        /* 1. add pipeline */
        gst_bin_add_many(GST_BIN(pipeline),
                         elements->source,
                         elements->depay,
                         elements->decoder,
                         elements->sink,
                         NULL);

        /* 2. rtspsrc  depay（dynamic pad） */
        g_signal_connect(elements->source,
                         "pad-added",
                         G_CALLBACK(on_rtsp_pad_added),
                         elements->depay);

        /* 3. depay  decodebin（static link） */
        if (!gst_element_link(elements->depay, elements->decoder)) {
            g_printerr("Failed to link depay to decodebin\n");
            return false;
        }

        /* 4. decodebin  waylandsink(dynamic pad） */
        g_signal_connect(elements->decoder,
                         "pad-added",
                         G_CALLBACK(on_decodebin_pad_added),
                         elements->sink);
    } else if(elements->depay) {
        /* RTSP pipeline */
        gst_bin_add_many(GST_BIN(pipeline),
                         elements->source, elements->depay,
                         elements->parser, elements->decoder, elements->sink,
                         NULL);

        gst_element_link_many(elements->depay, elements->parser, elements->decoder, elements->sink, NULL);
        g_signal_connect(elements->source, "pad-added",
                         G_CALLBACK(on_rtsp_pad_added), elements->depay);
    } else {
        /* Add elements to pipeline*/
        gst_bin_add_many(GST_BIN(pipeline), 
                            elements->source, 
                            elements->demuxer,
                            elements->parser, 
                            elements->decoder, 
                            elements->sink,
                            NULL);

        /* link elements to pipeline*/
        if((gst_element_link(elements->source, elements->demuxer) &&
                gst_element_link(elements->parser, elements->decoder)&&
                gst_element_link(elements->decoder, elements->sink)) != TRUE){

                printf("Failed to link elements. Exiting.\n");
                return false;
            }

        /* Set the callback function for the "qtdemuxer" to receive stream data */
        g_signal_connect(elements->demuxer, "pad-added", G_CALLBACK(on_pad_added), elements->parser);
    }
    
    return true;
}

bool play_gst_pipeline(struct gst_handle *handle) {
    pthread_t id;
    gboolean ret;

    if (handle == NULL)
        return false;

    /* Get pipeline message bus and monitoring messages */
    handle->bus = gst_pipeline_get_bus(GST_PIPELINE(handle->pipeline));
    gst_bus_add_watch(handle->bus, bus_call, gst_obj);
    gst_object_unref(handle->bus);

    /* Start the pipeline */
    ret = gst_element_set_state(handle->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_ASYNC || ret == GST_STATE_CHANGE_SUCCESS) {
        /* Crete timing thread */
        if (pthread_create(&id, NULL, timing_thread, NULL)) {
            printf("Creating timing thread failed !\n");
            return false;
        }
        pthread_detach(id);
    }

    return true;
}

/**
 * Release gstreamer pipeline
 */
void release_gst(struct gst_handle *handle) {
    if (!handle)
        return;

    if (handle->pipeline) {
        gst_element_set_state(handle->pipeline, GST_STATE_NULL);
        gst_object_unref(handle->pipeline);
        handle->pipeline = NULL;
    }

    if (handle->loop) {
        g_main_loop_unref(handle->loop);
        handle->loop = NULL;
    }
}

/*
 * gst_dec_display main function
 */
int main(int argc, char *argv[]) {

    gboolean ret; 
    struct _Params params;

    memset(&params, 0, sizeof(params));
    if (parse_parameter(&params, argc, argv) == false) {
        printf("Please try --help to see usage.\n");
        exit(2);
    }

    g_replay = params.replay;

    /* Ctrl+c handler */
    signal(SIGINT, sig_handle);

    /*gst_obj Apply for heap space*/
    gst_obj = (struct gst_obj *)malloc(sizeof(struct gst_obj));
    memset(gst_obj, 0, sizeof(struct gst_obj));

    /* Initialization of gstreamer */
    gst_init(&argc, &argv);

    while(!g_quit) {
        if(params.is_rtsp) {
            while(!g_quit && !rtsp_server_ready(params.rtsp_url)) {
                sleep(1);
            }
            if(g_quit)
                break;
        }

        /* Initialization of gstreamer */
        ret = initialize_gst( &gst_obj->handle);
        if (! ret) {
            printf("\n Initialization failed \n");
            goto err_release;
        }

        /* Create elements */
        ret = make_gst_elements(&gst_obj->elements, &params);
        if (! ret) {
            printf("\n Make elements failed\n\n");
            goto err_release;
        }

        /* Check creation of elements */
        ret = check_gst_elements(&gst_obj->elements);
        if (! ret) {
            printf("\n Elements error in check \n");
            goto err_release;
        }

        /* Configuration elements */
        ret = config_gst_elements(&params, &gst_obj->elements);
        if (! ret) {
            printf("\n Config elements failed \n");
            goto err_release;
        }

        /* Link elements into pipeline */
        ret = link_gst_elements( &gst_obj->elements,gst_obj->handle.pipeline);
        if (! ret) {
            printf("\n Link elements failed \n");
            goto err_release;
        }

        /* Play the pipeline */
        ret = play_gst_pipeline(&gst_obj->handle);
        if (! ret) {
            printf(" \n Play the pipeline failed \n");
            goto err_release;
        }

        /* Create loop, keep listen for pipeline event */
        g_main_loop_run(gst_obj->handle.loop);

        /* Release gst pipeline*/
        release_gst(&gst_obj->handle);
        
        if(!g_reconnect || g_quit) {
            break;
		}
    }

err_release:
    /* Release gst pipeline*/
    release_gst(&gst_obj->handle);
    free(gst_obj);
    if (! ret)
        return -1;
    printf("Exit\n");
    return 0;
}

