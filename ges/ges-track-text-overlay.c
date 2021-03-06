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
 * SECTION:ges-track-text-overlay
 * @short_description: render text onto another video stream in a
 * #GESTimelineLayer
 *
 */

#include "ges-internal.h"
#include "ges-track-object.h"
#include "ges-track-title-source.h"
#include "ges-track-text-overlay.h"

G_DEFINE_TYPE (GESTrackTextOverlay, ges_track_text_overlay,
    GES_TYPE_TRACK_OPERATION);

struct _GESTrackTextOverlayPrivate
{
  gchar *text;
  gchar *font_desc;
  GESTextHAlign halign;
  GESTextVAlign valign;
  guint32 color;
  gdouble xpos;
  gdouble ypos;
  GstElement *text_el;
};

enum
{
  PROP_0,
};

static void ges_track_text_overlay_dispose (GObject * object);

static void ges_track_text_overlay_finalize (GObject * object);

static void ges_track_text_overlay_get_property (GObject * object, guint
    property_id, GValue * value, GParamSpec * pspec);

static void ges_track_text_overlay_set_property (GObject * object, guint
    property_id, const GValue * value, GParamSpec * pspec);

static GstElement *ges_track_text_overlay_create_element (GESTrackObject
    * self);

static void
ges_track_text_overlay_class_init (GESTrackTextOverlayClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GESTrackObjectClass *bg_class = GES_TRACK_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESTrackTextOverlayPrivate));

  object_class->get_property = ges_track_text_overlay_get_property;
  object_class->set_property = ges_track_text_overlay_set_property;
  object_class->dispose = ges_track_text_overlay_dispose;
  object_class->finalize = ges_track_text_overlay_finalize;

  bg_class->create_element = ges_track_text_overlay_create_element;
}

static void
ges_track_text_overlay_init (GESTrackTextOverlay * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_TRACK_TEXT_OVERLAY, GESTrackTextOverlayPrivate);

  self->priv->text = NULL;
  self->priv->font_desc = NULL;
  self->priv->text_el = NULL;
  self->priv->halign = DEFAULT_HALIGNMENT;
  self->priv->valign = DEFAULT_VALIGNMENT;
  self->priv->color = G_MAXUINT32;
  self->priv->xpos = 0.5;
  self->priv->ypos = 0.5;
}

static void
ges_track_text_overlay_dispose (GObject * object)
{
  GESTrackTextOverlay *self = GES_TRACK_TEXT_OVERLAY (object);
  if (self->priv->text) {
    g_free (self->priv->text);
  }

  if (self->priv->font_desc) {
    g_free (self->priv->font_desc);
  }

  if (self->priv->text_el) {
    g_object_unref (self->priv->text_el);
    self->priv->text_el = NULL;
  }

  G_OBJECT_CLASS (ges_track_text_overlay_parent_class)->dispose (object);
}

static void
ges_track_text_overlay_finalize (GObject * object)
{
  G_OBJECT_CLASS (ges_track_text_overlay_parent_class)->finalize (object);
}

static void
ges_track_text_overlay_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_track_text_overlay_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static GstElement *
ges_track_text_overlay_create_element (GESTrackObject * object)
{
  GstElement *ret, *text, *iconv, *oconv;
  GstPad *src_target, *sink_target;
  GstPad *src, *sink;
  GESTrackTextOverlay *self = GES_TRACK_TEXT_OVERLAY (object);

  text = gst_element_factory_make ("textoverlay", NULL);
  iconv = gst_element_factory_make ("ffmpegcolorspace", NULL);
  oconv = gst_element_factory_make ("ffmpegcolorspace", NULL);
  self->priv->text_el = text;
  g_object_ref (text);

  if (self->priv->text)
    g_object_set (text, "text", (gchar *) self->priv->text, NULL);
  if (self->priv->font_desc)
    g_object_set (text, "font-desc", (gchar *) self->priv->font_desc, NULL);

  g_object_set (text, "halignment", (gint) self->priv->halign, "valignment",
      (gint) self->priv->valign, NULL);
  g_object_set (text, "color", (guint32) self->priv->color, NULL);
  g_object_set (text, "xpos", (gdouble) self->priv->xpos, NULL);
  g_object_set (text, "ypos", (gdouble) self->priv->ypos, NULL);

  ret = gst_bin_new ("overlay-bin");
  gst_bin_add_many (GST_BIN (ret), text, iconv, oconv, NULL);
  gst_element_link_many (iconv, text, oconv, NULL);

  src_target = gst_element_get_static_pad (oconv, "src");
  sink_target = gst_element_get_static_pad (iconv, "sink");

  src = gst_ghost_pad_new ("src", src_target);
  sink = gst_ghost_pad_new ("video_sink", sink_target);
  g_object_unref (src_target);
  g_object_unref (sink_target);

  gst_element_add_pad (ret, src);
  gst_element_add_pad (ret, sink);

  return ret;
}

/**
 * ges_track_text_overlay_set_text:
 * @self: the #GESTrackTextOverlay* to set text on
 * @text: the text to render. an internal copy of this text will be
 * made.
 *
 * Sets the text this track object will render.
 *
 */
void
ges_track_text_overlay_set_text (GESTrackTextOverlay * self, const gchar * text)
{
  GST_DEBUG ("self:%p, text:%s", self, text);

  if (self->priv->text)
    g_free (self->priv->text);

  self->priv->text = g_strdup (text);
  if (self->priv->text_el)
    g_object_set (self->priv->text_el, "text", text, NULL);
}

/**
 * ges_track_text_overlay_set_font_desc:
 * @self: the #GESTrackTextOverlay
 * @font_desc: the pango font description
 *
 * Sets the pango font description of the text this track object
 * will render.
 *
 */
void
ges_track_text_overlay_set_font_desc (GESTrackTextOverlay * self,
    const gchar * font_desc)
{
  GST_DEBUG ("self:%p, font_desc:%s", self, font_desc);

  if (self->priv->font_desc)
    g_free (self->priv->font_desc);

  self->priv->font_desc = g_strdup (font_desc);
  GST_LOG ("setting font-desc to '%s'", font_desc);
  if (self->priv->text_el)
    g_object_set (self->priv->text_el, "font-desc", font_desc, NULL);
}

/**
 * ges_track_text_overlay_set_valignment:
 * @self: the #GESTrackTextOverlay* to set text on
 * @valign: The #GESTextVAlign defining the vertical alignment
 * of the text render by @self.
 *
 * Sets the vertical aligment of the text.
 *
 */
void
ges_track_text_overlay_set_valignment (GESTrackTextOverlay * self,
    GESTextVAlign valign)
{
  GST_DEBUG ("self:%p, halign:%d", self, valign);

  self->priv->valign = valign;
  if (self->priv->text_el)
    g_object_set (self->priv->text_el, "valignment", valign, NULL);
}

/**
 * ges_track_text_overlay_set_halignment:
 * @self: the #GESTrackTextOverlay* to set text on
 * @halign: The #GESTextHAlign defining the horizontal alignment
 * of the text render by @self.
 *
 * Sets the horizontal aligment of the text.
 *
 */
void
ges_track_text_overlay_set_halignment (GESTrackTextOverlay * self,
    GESTextHAlign halign)
{
  GST_DEBUG ("self:%p, halign:%d", self, halign);

  self->priv->halign = halign;
  if (self->priv->text_el)
    g_object_set (self->priv->text_el, "halignment", halign, NULL);
}

/**
 * ges_track_text_overlay_set_color:
 * @self: the #GESTrackTextOverlay* to set
 * @color: The color @self is being set to
 *
 * Sets the color of the text.
 *
 * Since: 0.10.2
 */
void
ges_track_text_overlay_set_color (GESTrackTextOverlay * self, guint32 color)
{
  GST_DEBUG ("self:%p, color:%d", self, color);

  self->priv->color = color;
  if (self->priv->text_el)
    g_object_set (self->priv->text_el, "color", color, NULL);
}

/**
 * ges_track_text_overlay_set_xpos:
 * @self: the #GESTrackTextOverlay* to set
 * @position: The horizontal position @self is being set to
 *
 * Sets the horizontal position of the text.
 *
 * Since: 0.10.2
 */
void
ges_track_text_overlay_set_xpos (GESTrackTextOverlay * self, gdouble position)
{
  GST_DEBUG ("self:%p, xpos:%f", self, position);

  self->priv->xpos = position;
  if (self->priv->text_el)
    g_object_set (self->priv->text_el, "xpos", position, NULL);
}

/**
 * ges_track_text_overlay_set_ypos:
 * @self: the #GESTrackTextOverlay* to set
 * @position: The vertical position @self is being set to
 *
 * Sets the vertical position of the text.
 *
 * Since: 0.10.2
 */
void
ges_track_text_overlay_set_ypos (GESTrackTextOverlay * self, gdouble position)
{
  GST_DEBUG ("self:%p, ypos:%f", self, position);

  self->priv->ypos = position;
  if (self->priv->text_el)
    g_object_set (self->priv->text_el, "ypos", position, NULL);
}

/**
 * ges_track_text_overlay_get_text:
 * @self: a GESTrackTextOverlay
 *
 * Get the text currently set on @source.
 *
 * Returns: The text currently set on @source.
 */
const gchar *
ges_track_text_overlay_get_text (GESTrackTextOverlay * self)
{
  return self->priv->text;
}

/**
 * ges_track_text_overlay_get_font_desc:
 * @self: a GESTrackTextOverlay
 *
 * Get the pango font description currently set on @source.
 *
 * Returns: The pango font description currently set on @source.
 */
const char *
ges_track_text_overlay_get_font_desc (GESTrackTextOverlay * self)
{
  return self->priv->font_desc;
}

/**
 * ges_track_text_overlay_get_halignment:
 * @self: a GESTrackTextOverlay
 *
 * Get the horizontal aligment used by @source.
 *
 * Returns: The horizontal aligment used by @source.
 */
GESTextHAlign
ges_track_text_overlay_get_halignment (GESTrackTextOverlay * self)
{
  return self->priv->halign;
}

/**
 * ges_track_text_overlay_get_valignment:
 * @self: a GESTrackTextOverlay
 *
 * Get the vertical aligment used by @source.
 *
 * Returns: The vertical aligment used by @source.
 */
GESTextVAlign
ges_track_text_overlay_get_valignment (GESTrackTextOverlay * self)
{
  return self->priv->valign;
}

/**
 * ges_track_text_overlay_get_color:
 * @self: a GESTrackTextOverlay
 *
 * Get the color used by @source.
 *
 * Returns: The color used by @source.
 *
 * Since: 0.10.2
 */
const guint32
ges_track_text_overlay_get_color (GESTrackTextOverlay * self)
{
  return self->priv->color;
}

/**
 * ges_track_text_overlay_get_xpos:
 * @self: a GESTrackTextOverlay
 *
 * Get the horizontal position used by @source.
 *
 * Returns: The horizontal position used by @source.
 *
 * Since: 0.10.2
 */
const gdouble
ges_track_text_overlay_get_xpos (GESTrackTextOverlay * self)
{
  return self->priv->xpos;
}

/**
 * ges_track_text_overlay_get_ypos:
 * @self: a GESTrackTextOverlay
 *
 * Get the vertical position used by @source.
 *
 * Returns: The vertical position used by @source.
 *
 * Since: 0.10.2
 */
const gdouble
ges_track_text_overlay_get_ypos (GESTrackTextOverlay * self)
{
  return self->priv->ypos;
}

/**
 * ges_track_text_overlay_new:
 *
 * Creates a new #GESTrackTextOverlay.
 *
 * Returns: The newly created #GESTrackTextOverlay or %NULL if something went
 * wrong.
 */
GESTrackTextOverlay *
ges_track_text_overlay_new (void)
{
  return g_object_new (GES_TYPE_TRACK_TEXT_OVERLAY, NULL);
}
