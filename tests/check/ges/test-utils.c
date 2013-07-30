/**
 * Gstreamer Editing Services
 *
 * Copyright (C) <2012> Thibault Saunier <thibault.saunier@collabora.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "test-utils.h"
#include <gio/gio.h>

typedef struct _DestroyedObjectStruct
{
  GObject *object;
  gboolean destroyed;
} DestroyedObjectStruct;

gchar *
ges_test_get_audio_only_uri (void)
{
  gchar *uri;
  GFile *cfile, *fdir, *f_audio_only;

  cfile = g_file_new_for_path (__FILE__);
  fdir = g_file_get_parent (cfile);

  f_audio_only = g_file_get_child (fdir, "audio_only.ogg");
  uri = g_file_get_uri (f_audio_only);

  gst_object_unref (cfile);
  gst_object_unref (fdir);
  gst_object_unref (f_audio_only);

  return uri;
}

gchar *
ges_test_get_audio_video_uri (void)
{
  gchar *uri;
  GFile *cfile, *fdir, *f_audio_video;

  cfile = g_file_new_for_path (__FILE__);
  fdir = g_file_get_parent (cfile);

  f_audio_video = g_file_get_child (fdir, "audio_video.ogg");
  uri = g_file_get_uri (f_audio_video);

  gst_object_unref (cfile);
  gst_object_unref (fdir);
  gst_object_unref (f_audio_video);

  return uri;
}

gchar *
ges_test_get_image_uri (void)
{
  return ges_test_file_uri ("image.png");
}

gchar *
ges_test_file_uri (const gchar * filename)
{
  gchar *uri;
  GFile *cfile, *fdir, *f;

  cfile = g_file_new_for_path (__FILE__);
  fdir = g_file_get_parent (cfile);

  f = g_file_get_child (fdir, filename);
  uri = g_file_get_uri (f);

  gst_object_unref (cfile);
  gst_object_unref (fdir);
  gst_object_unref (f);

  return uri;
}

GESTimelinePipeline *
ges_test_create_pipeline (GESTimeline * timeline)
{
  GESTimelinePipeline *pipeline;

  pipeline = ges_timeline_pipeline_new ();
  fail_unless (ges_timeline_pipeline_add_timeline (pipeline, timeline));

  g_object_set (pipeline, "audio-sink", gst_element_factory_make ("fakesink",
          "test-audiofakesink"), "video-sink",
      gst_element_factory_make ("fakesink", "test-videofakesink"), NULL);

  return pipeline;
}

gchar *
ges_test_file_name (const gchar * filename)
{
  return g_strjoin ("/", "file:/", g_get_current_dir (), filename, NULL);
}

gboolean
ges_generate_test_file_audio_video (const gchar * filedest,
    const gchar * audio_enc,
    const gchar * video_enc,
    const gchar * mux, const gchar * video_pattern, const gchar * audio_wave,
    const guint32 width, guint height)
{
  GError *error = NULL;
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *message;
  gchar *pipeline_str;
  gboolean done = FALSE;
  gboolean ret = FALSE;

  if (g_file_test (filedest, G_FILE_TEST_EXISTS)) {
    GST_INFO ("The file %s already existed.", filedest);
    return TRUE;
  }

  pipeline_str = g_strdup_printf ("audiotestsrc num-buffers=430 wave=%s "
      "%c %s ! %s name=m ! filesink location= %s/%s "
      "videotestsrc pattern=%s num-buffers=300 ! videoscale !  video/x-raw,width=%d,height=%d ! %s ! m.",
      audio_wave,
      audio_enc ? '!' : ' ',
      audio_enc ? audio_enc : "",
      mux, g_get_current_dir (), filedest, video_pattern, width, height,
      video_enc);

  pipeline = gst_parse_launch (pipeline_str, &error);

  if (pipeline == NULL)
    return FALSE;

  g_free (pipeline_str);

  bus = gst_element_get_bus (GST_ELEMENT (pipeline));
  gst_bus_add_signal_watch (bus);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  while (!done) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, GST_CLOCK_TIME_NONE);
    if (GST_MESSAGE_TYPE (message) & GST_MESSAGE_EOS) {
      done = TRUE;
      ret = TRUE;
    } else if (GST_MESSAGE_TYPE (message) & GST_MESSAGE_ERROR) {
      done = TRUE;
      ret = FALSE;
      g_print ("Error");
    }
  }

  gst_bus_remove_signal_watch (bus);
  gst_object_unref (bus);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  return ret;
}

static void
weak_notify (DestroyedObjectStruct * destroyed, GObject ** object)
{
  destroyed->destroyed = TRUE;
}

void
check_destroyed (GObject * object_to_unref, GObject * first_object, ...)
{
  GObject *object;
  GList *objs = NULL, *tmp;
  DestroyedObjectStruct *destroyed = g_slice_new0 (DestroyedObjectStruct);

  destroyed->object = object_to_unref;
  g_object_weak_ref (object_to_unref, (GWeakNotify) weak_notify, destroyed);
  objs = g_list_prepend (objs, destroyed);

  if (first_object) {
    va_list varargs;

    object = first_object;

    va_start (varargs, first_object);
    while (object) {
      destroyed = g_slice_new0 (DestroyedObjectStruct);
      destroyed->object = object;
      g_object_weak_ref (object, (GWeakNotify) weak_notify, destroyed);
      objs = g_list_prepend (objs, destroyed);
      object = va_arg (varargs, GObject *);
    }
    va_end (varargs);
  }
  gst_object_unref (object_to_unref);

  for (tmp = objs; tmp; tmp = tmp->next) {
    fail_unless (((DestroyedObjectStruct *) tmp->data)->destroyed == TRUE,
        "%p is not destroyed", ((DestroyedObjectStruct *) tmp->data)->object);
    g_slice_free (DestroyedObjectStruct, tmp->data);
  }
  g_list_free (objs);

}
