/* GStreamer Editing Services
 * Copyright (C) 2010 Brandon Lewis <brandon.lewis@collabora.co.uk>
 *               2010 Nokia Corporation
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:ges-track-video-transition
 * @short_description: implements video crossfade transition
 */

#include "ges-internal.h"
#include "ges-track-object.h"
#include "ges-timeline-transition.h"
#include "ges-track-video-transition.h"

G_DEFINE_TYPE (GESTrackVideoTransition, ges_track_video_transition,
    GES_TYPE_TRACK_TRANSITION);

enum
{
  PROP_0,
};

#define fast_element_link(a,b) gst_element_link_pads_full((a),"src",(b),"sink",GST_PAD_LINK_CHECK_NOTHING)

static GObject *link_element_to_mixer (GstElement * element,
    GstElement * mixer);

static GObject *link_element_to_mixer_with_smpte (GstBin * bin,
    GstElement * element, GstElement * mixer, gint type,
    GstElement ** smpteref);

static void
ges_track_video_transition_duration_changed (GESTrackObject * self,
    guint64 duration);

static GstElement *ges_track_video_transition_create_element (GESTrackTransition
    * self);

static void ges_track_video_transition_dispose (GObject * object);

static void ges_track_video_transition_finalize (GObject * object);

static void ges_track_video_transition_get_property (GObject * object, guint
    property_id, GValue * value, GParamSpec * pspec);

static void ges_track_video_transition_set_property (GObject * object, guint
    property_id, const GValue * value, GParamSpec * pspec);

static void
ges_track_video_transition_class_init (GESTrackVideoTransitionClass * klass)
{
  GObjectClass *object_class;
  GESTrackObjectClass *toclass;
  GESTrackTransitionClass *pclass;

  object_class = G_OBJECT_CLASS (klass);
  toclass = GES_TRACK_OBJECT_CLASS (klass);
  pclass = GES_TRACK_TRANSITION_CLASS (klass);

  object_class->get_property = ges_track_video_transition_get_property;
  object_class->set_property = ges_track_video_transition_set_property;
  object_class->dispose = ges_track_video_transition_dispose;
  object_class->finalize = ges_track_video_transition_finalize;

  toclass->duration_changed = ges_track_video_transition_duration_changed;

  pclass->create_element = ges_track_video_transition_create_element;
}

static void
ges_track_video_transition_init (GESTrackVideoTransition * self)
{
  self->controller = NULL;
  self->control_source = NULL;
  self->smpte = NULL;
  self->mixer = NULL;
  self->sinka = NULL;
  self->sinkb = NULL;
  self->type = GES_VIDEO_TRANSITION_TYPE_NONE;
  self->start_value = 0.0;
  self->end_value = 0.0;
}

static void
ges_track_video_transition_dispose (GObject * object)
{
  GESTrackVideoTransition *self = GES_TRACK_VIDEO_TRANSITION (object);

  GST_DEBUG ("disposing");
  GST_LOG ("mixer: %p smpte: %p sinka: %p sinkb: %p",
      self->mixer, self->smpte, self->sinka, self->sinkb);

  if (self->controller) {
    g_object_unref (self->controller);
    self->controller = NULL;
    if (self->control_source)
      gst_object_unref (self->control_source);
    self->control_source = NULL;
  }

  if (self->sinka && self->sinkb) {
    GST_DEBUG ("releasing request pads for mixer");
    gst_element_release_request_pad (self->mixer, self->sinka);
    gst_element_release_request_pad (self->mixer, self->sinkb);
    gst_object_unref (self->sinka);
    gst_object_unref (self->sinkb);
    self->sinka = NULL;
    self->sinkb = NULL;
  }

  if (self->mixer) {
    GST_LOG ("unrefing mixer");
    gst_object_unref (self->mixer);
    self->mixer = NULL;
  }

  G_OBJECT_CLASS (ges_track_video_transition_parent_class)->dispose (object);
}

static void
ges_track_video_transition_finalize (GObject * object)
{
  G_OBJECT_CLASS (ges_track_video_transition_parent_class)->finalize (object);
}

static void
ges_track_video_transition_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_track_video_transition_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static GstElement *
ges_track_video_transition_create_element (GESTrackTransition * object)
{
  GstElement *topbin, *iconva, *iconvb, *oconv;
  GObject *target = NULL;
  const gchar *propname = NULL;
  GstElement *mixer = NULL;
  GstPad *sinka_target, *sinkb_target, *src_target, *sinka, *sinkb, *src;
  GstController *controller;
  GstInterpolationControlSource *control_source;
  GESTrackVideoTransition *self;

  self = GES_TRACK_VIDEO_TRANSITION (object);

  GST_LOG ("creating a video bin");

  topbin = gst_bin_new ("transition-bin");
  iconva = gst_element_factory_make ("ffmpegcolorspace", "tr-csp-a");
  iconvb = gst_element_factory_make ("ffmpegcolorspace", "tr-csp-b");
  oconv = gst_element_factory_make ("ffmpegcolorspace", "tr-csp-output");

  gst_bin_add_many (GST_BIN (topbin), iconva, iconvb, oconv, NULL);
  /* Prefer videomixer2 to videomixer */
  mixer = gst_element_factory_make ("videomixer2", NULL);
  if (mixer == NULL)
    mixer = gst_element_factory_make ("videomixer", NULL);
  g_object_set (G_OBJECT (mixer), "background", 1, NULL);
  gst_bin_add (GST_BIN (topbin), mixer);

  if (self->type != GES_VIDEO_TRANSITION_TYPE_CROSSFADE) {
    self->sinka =
        (GstPad *) link_element_to_mixer_with_smpte (GST_BIN (topbin), iconva,
        mixer, self->type, NULL);
    self->sinkb =
        (GstPad *) link_element_to_mixer_with_smpte (GST_BIN (topbin), iconvb,
        mixer, self->type, &self->smpte);
    target = (GObject *) self->smpte;
    propname = "position";
    self->start_value = 1.0;
    self->end_value = 0.0;
  } else {
    self->sinka = (GstPad *) link_element_to_mixer (iconva, mixer);
    self->sinkb = (GstPad *) link_element_to_mixer (iconvb, mixer);
    target = (GObject *) self->sinkb;
    propname = "alpha";
    self->start_value = 0.0;
    self->end_value = 1.0;
  }

  self->mixer = gst_object_ref (mixer);

  fast_element_link (mixer, oconv);

  sinka_target = gst_element_get_static_pad (iconva, "sink");
  sinkb_target = gst_element_get_static_pad (iconvb, "sink");
  src_target = gst_element_get_static_pad (oconv, "src");

  sinka = gst_ghost_pad_new ("sinka", sinka_target);
  sinkb = gst_ghost_pad_new ("sinkb", sinkb_target);
  src = gst_ghost_pad_new ("src", src_target);

  gst_element_add_pad (topbin, src);
  gst_element_add_pad (topbin, sinka);
  gst_element_add_pad (topbin, sinkb);

  gst_object_unref (sinka_target);
  gst_object_unref (sinkb_target);
  gst_object_unref (src_target);

  /* set up interpolation */

  g_object_set (target, propname, (gfloat) 0.0, NULL);

  controller = gst_object_control_properties (target, propname, NULL);

  control_source = gst_interpolation_control_source_new ();
  gst_controller_set_control_source (controller,
      propname, GST_CONTROL_SOURCE (control_source));
  gst_interpolation_control_source_set_interpolation_mode (control_source,
      GST_INTERPOLATE_LINEAR);

  self->controller = controller;
  self->control_source = control_source;

  return topbin;
}

static GObject *
link_element_to_mixer (GstElement * element, GstElement * mixer)
{
  GstPad *sinkpad = gst_element_get_request_pad (mixer, "sink_%d");
  GstPad *srcpad = gst_element_get_static_pad (element, "src");

  gst_pad_link_full (srcpad, sinkpad, GST_PAD_LINK_CHECK_NOTHING);
  gst_object_unref (srcpad);

  return G_OBJECT (sinkpad);
}

static GObject *
link_element_to_mixer_with_smpte (GstBin * bin, GstElement * element,
    GstElement * mixer, gint type, GstElement ** smpteref)
{
  GstPad *srcpad, *sinkpad;
  GstElement *smptealpha = gst_element_factory_make ("smptealpha", NULL);

  g_object_set (G_OBJECT (smptealpha),
      "type", (gint) type, "invert", (gboolean) TRUE, NULL);
  gst_bin_add (bin, smptealpha);

  fast_element_link (element, smptealpha);

  /* crack */
  if (smpteref) {
    *smpteref = smptealpha;
  }

  srcpad = gst_element_get_static_pad (smptealpha, "src");
  sinkpad = gst_element_get_request_pad (mixer, "sink_%d");
  gst_pad_link_full (srcpad, sinkpad, GST_PAD_LINK_CHECK_NOTHING);
  gst_object_unref (srcpad);

  return G_OBJECT (sinkpad);
}

static void
ges_track_video_transition_duration_changed (GESTrackObject * object,
    guint64 duration)
{
  GValue start_value = { 0, };
  GValue end_value = { 0, };
  GstElement *gnlobj = object->gnlobject;
  GESTrackVideoTransition *self = GES_TRACK_VIDEO_TRANSITION (object);

  GST_LOG ("updating controller");

  if (G_UNLIKELY (!gnlobj || !self->control_source))
    return;

  GST_INFO ("duration: %" G_GUINT64_FORMAT, duration);
  g_value_init (&start_value, G_TYPE_DOUBLE);
  g_value_init (&end_value, G_TYPE_DOUBLE);
  g_value_set_double (&start_value, self->start_value);
  g_value_set_double (&end_value, self->end_value);

  GST_LOG ("setting values on controller");

  gst_interpolation_control_source_unset_all (self->control_source);
  gst_interpolation_control_source_set (self->control_source, 0, &start_value);
  gst_interpolation_control_source_set (self->control_source,
      duration, &end_value);

  GST_LOG ("done updating controller");
}

gboolean
ges_track_video_transition_set_type (GESTrackVideoTransition * self,
    GESVideoTransitionType type)
{
  GST_DEBUG ("%p %d => %d", self, self->type, type);

  if (self->type && (self->type != type) &&
      ((type == GES_VIDEO_TRANSITION_TYPE_CROSSFADE) ||
          (self->type == GES_VIDEO_TRANSITION_TYPE_CROSSFADE))) {
    GST_WARNING
        ("Changing between 'crossfade' and other types is not supported");
    return FALSE;
  }

  self->type = type;
  if (self->smpte && (type != GES_VIDEO_TRANSITION_TYPE_CROSSFADE))
    g_object_set (self->smpte, "type", (gint) type, NULL);
  return TRUE;
}

GESTrackVideoTransition *
ges_track_video_transition_new (void)
{
  return g_object_new (GES_TYPE_TRACK_VIDEO_TRANSITION, NULL);
}