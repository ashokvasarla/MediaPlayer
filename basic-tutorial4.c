#include <gst/gst.h>
   
/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *convert;
  GstElement *decoder;
  GstElement *sink;
  gboolean playing;
  gint64 duration;
  gboolean terminate;
} CustomData;


   
/* Handler for the pad-added signal */
static void handle_message(GstMessage *msg, CustomData *data);
   
int main(int argc, char *argv[]) {
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  data.terminate = FALSE;
  GstElement *filesrc;
  data.playing = FALSE;
  data.duration = GST_CLOCK_TIME_NONE;
   
  /* Initialize GStreamer */
  gst_init (&argc, &argv);
    
  /* Create the elements */
  data.source = gst_element_factory_make ("filesrc", "file-src1");
  data.decoder  = gst_element_factory_make ("mad","mad-dec");
  data.convert = gst_element_factory_make ("audioconvert", "convert");
  data.sink = gst_element_factory_make ("autoaudiosink", "sink");
   
  /* Create the empty pipeline */
  data.pipeline = gst_pipeline_new ("test-pipeline");
    
  if (!data.pipeline || !data.source || !data.convert || !data.sink ||  !data.decoder )  {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }
    /* we set the input filename to the source element */
  g_object_set (G_OBJECT (data.source), "location", argv[1], NULL);
   
  /* Build the pipeline. Note that we are NOT linking the source at this
   * point. We will do it later. */
  gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.decoder ,data.convert , data.sink, NULL);
  
  gst_element_link_many (data.source, data.decoder,data.convert,data.sink,NULL);
  /*
  if (!gst_element_link (data.source, data.decoder))
  {
	  g_printerr ("Elements could not be linked.\n");
	  gst_object_unref (data.pipeline);
	  
  }
  if (!gst_element_link (data.convert, data.sink)) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }
  
*/
   
  /* Set the URI to play */
  //g_object_set (filesrc, "location", "Front_Center.wav", NULL);
  //g_object_set (data.source, "uri", "http://docs.gstreamer.com/media/sintel_trailer-480p.webm", NULL);
  
  //g_object_set (filesrc, "location", argv[1], NULL);
   
  /* Connect to the pad-added signal */
  //g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler), &data);
   
  /* Start playing */
  ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }
   
  /* Listen to the bus */
  bus = gst_element_get_bus (data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered (bus, 100 * GST_MSECOND,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION);
   if ( msg != NULL)
   {
	   handle_message(msg,&data);
   }
   else
   {
	   if(data.playing)
	   {
		   GstFormat fmt = GST_FORMAT_TIME;
		   gint64 current = -1;
		   
		   if(!gst_element_query_position(data.pipeline,fmt,&current))
		   {
			   g_printerr("Could not query current position.\n");
		   }
		   
		   if( !GST_CLOCK_TIME_IS_VALID (data.duration)) {
			   if (!gst_element_query_duration (data.pipeline, fmt, &data.duration)) {
            g_printerr ("Could not query current duration.\n");
			}
			}
			/* Print current position and total duration */
			g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
            GST_TIME_ARGS (current), GST_TIME_ARGS (data.duration));
		   
		}
		printf("Timeout \n");
   }

  } while (!data.terminate);
   
  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (data.pipeline, GST_STATE_NULL);
  gst_object_unref (data.pipeline);
  return 0;
}

void handle_message(GstMessage *msg ,CustomData *data)
{
	    /* Parse message */
    
      GError *err;
      gchar *debug_info;
       
      switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error (msg, &err, &debug_info);
          g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
          g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
          g_clear_error (&err);
          g_free (debug_info);
          data->terminate = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print ("End-Of-Stream reached.\n");
          data->terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          /* We are only interested in state-changed messages from the pipeline */
          if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
            g_print ("Pipeline state changed from %s to %s:\n",
            gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
            /* Remember whether we are in the PLAYING state or not */
			data->playing = (new_state == GST_STATE_PLAYING);
          }
          break;
        default:
          /* We should not reach here */
          g_printerr ("Unexpected message received.\n");
          break;
      }
      gst_message_unref (msg);
    
}

