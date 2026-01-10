/* Copyright 2023 Tronlong Elec. Tech. Co. Ltd. All Rights Reserved. */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <gst/gst.h>

#include "parameter_parser.h"

struct _GSTHANDLE {
    GMainLoop *loop;
    GstBus *bus;
    GstElement *pipeline;
    GstMessage *msg;
};

struct _GSTELES {
    GstElement *source,
               *demuxer,
               *depay,
               *parser,
               *decoder,
               *sink;
};

struct GSTOBJ {
    struct _GSTHANDLE handle;
    struct _GSTELES elements;
};

static struct GSTOBJ *gstobj = NULL;
static bool g_quit = false;
static bool g_replay = false;
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

/**
 * Gstreamer pipeline message bus callback function
 */
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
	struct GSTOBJ *obj = (struct GSTOBJ *)data;
	GMainLoop *loop = (&obj->handle)->loop;
    gchar  *debug;
    GError *error;

    switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_SEGMENT_DONE:
        /* 一个 segment 播放完，立刻从 0 开始下一个 */
		gst_element_seek(
			obj->handle.pipeline,
			1.0,
			GST_FORMAT_TIME,
			GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
			GST_SEEK_TYPE_SET, 0,
			GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
		break;
    case GST_MESSAGE_EOS:
        /* 一个 segment 播放完，立刻从 0 开始下一个 */
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
        g_free(debug);
        printf("Error: %s\n", error->message);
        g_error_free(error);
        g_main_loop_quit(loop);
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

/**
 * signal handler function
 */
static void sig_handle(int signal) {
    printf("\n\n\r\033[k");
    if (gstobj != NULL)
    {
         g_main_loop_quit(gstobj->handle.loop);
    }
    g_quit = true;
    sleep(1);
}

/**
 * initialize gst object
 */
bool initialize_gst(int argc, char **argv, struct _GSTHANDLE *handle) {
    
    if (handle == NULL)
        return false;

    /* Initialization of gstreamer */
    gst_init(&argc, &argv);
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
bool make_gst_elements(struct _GSTELES *elements , struct _Params *params) {
    
    if (params == NULL || elements == NULL) {
        return false;
	}

	if(params->is_rtsp) {
        /* Create the elements */
        elements->source        = gst_element_factory_make("rtspsrc", "_rtspsrc");
        elements->depay         = gst_element_factory_make("rtph264depay", "_rtph264depay");
        elements->parser        = gst_element_factory_make("h264parse", "_h264parse");
        elements->decoder       = gst_element_factory_make("mppvideodec", "_mppvideodec");
        elements->sink          = gst_element_factory_make("kmssink", "_kmssink");
    } else {
        /* Initialization of elements */
        elements->source        = gst_element_factory_make("filesrc", "_filesrc");
        elements->demuxer       = gst_element_factory_make("qtdemux", "_qtdemux");
        elements->decoder       = gst_element_factory_make("mppvideodec", "_mppvideodec");
        elements->sink          = gst_element_factory_make("kmssink", "_kmssink");

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
bool check_gst_elements(struct _GSTELES *elements) {
        
    if (elements == NULL)
    return false;

    /*Check if the element was successfully created*/
    if( !elements->source  ||
        !elements->parser || !elements->decoder || 
        !elements->sink)
    {
        printf("Failed to create elements. Exiting.\n");
        return false;
    }
 
    return TRUE ;
}

/**
 * config elements
 */
bool config_gst_elements(struct _Params *params, struct _GSTELES *elements) {

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
    if(params->plane_id)
        g_object_set(elements->sink, "connector-id", params->connector_id, "plane-id", params->plane_id, NULL);
    else
        g_object_set(elements->sink, "connector-id", params->connector_id, NULL);

    return true;
}

/**
 * link elements
 */
bool link_gst_elements(struct _GSTELES *elements, GstElement *pipeline) {
    
    if (elements == NULL || pipeline == NULL)
        return false;

    if(elements->depay) {
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

bool play_gst_pipeline(struct _GSTHANDLE *handle) {
    pthread_t id;
    gboolean ret;

    if (handle == NULL)
        return false;

    /* Get pipeline message bus and monitoring messages */
    handle->bus = gst_pipeline_get_bus(GST_PIPELINE(handle->pipeline));
    gst_bus_add_watch(handle->bus, bus_call, gstobj);
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
void release_gst(struct _GSTHANDLE *handle) {
    /* clean up */
    gst_element_set_state(handle->pipeline, GST_STATE_NULL);

    /* Release gst pipeline*/
    gst_object_unref(GST_OBJECT(handle->pipeline));

    /* Quit loop */
    g_main_loop_unref(handle->loop);
}

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

    /*gstobj Apply for heap space*/
    gstobj = (struct GSTOBJ *)malloc(sizeof(struct GSTOBJ));
    memset(gstobj, 0, sizeof(struct GSTOBJ));

    /* Initialization of gstreamer */
    ret = initialize_gst(argc, argv, &gstobj->handle);
    if (! ret) {
        printf("\n Initialization failed \n");
        goto err_release;
    }

    /* Create elements */
    ret = make_gst_elements(&gstobj->elements, &params);
    if (! ret) {
        printf("\n Make elements failed\n\n");
        goto err_release;
    }

    /* Check creation of elements */
    ret = check_gst_elements(&gstobj->elements);
    if (! ret) {
        printf("\n Elements error in check \n");
        goto err_release;
    }

    /* Configuration elements */
    ret = config_gst_elements(&params, &gstobj->elements);
    if (! ret) {
        printf("\n Config elements failed \n");
        goto err_release;
    }

    /* Link elements into pipeline */
    ret = link_gst_elements( &gstobj->elements,gstobj->handle.pipeline);
    if (! ret) {
        printf("\n Link elements failed \n");
        goto err_release;
    }

    /* Play the pipeline */
    ret = play_gst_pipeline(&gstobj->handle);
    if (! ret) {
        printf(" \n Play the pipeline failed \n");
        goto err_release;
    }

    /* Create loop, keep listen for pipeline event */
    g_main_loop_run(gstobj->handle.loop);

err_release:
    /* Release gst pipeline*/
    release_gst(&gstobj->handle);
    free(gstobj);
    if (! ret)
        return -1;
    printf("Exit\n");
    return 0;
}

