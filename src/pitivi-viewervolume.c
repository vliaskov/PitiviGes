/* 
 * PiTiVi
 * Copyright (C) <2004> Edward G. Hervey <hervey_e@epita.fr>
 *                      Guillaume Casanova <casano_g@epita.fr>
 *
 * This software has been written in EPITECH <http://www.epitech.net>
 * EPITECH is a computer science school in Paris - FRANCE -
 * under the direction of Flavien Astraud and Jerome Landrieu.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "pitivi.h"
#include "pitivi-viewervolume.h"


struct _PitiviViewerVolumePrivate
{
  /* instance private members */
  gboolean		dispose_has_run;
  GtkWidget		*parent_window;
  GtkWidget		*frame;
  GtkWidget		*vscale_vol;
};

typedef struct _ViewerVolume
{
  guint			  viewer_enum;
  gchar			  *stock_icon;
  GtkIconSize		  stock_size;
  GtkReliefStyle	  relief;
} ViewerVolume;

/*
 * forward definitions
 */

ViewerVolume stockvols[PITIVI_STATE_VOLIMG_ALL+1]=
  {
    { PITIVI_STATE_VOLIMG_ZERO, PITIVI_STOCK_VIEWER_VOLUMEZERO, GTK_ICON_SIZE_BUTTON, GTK_RELIEF_NONE},
    { PITIVI_STATE_VOLIMG_MIN, PITIVI_STOCK_VIEWER_VOLUMEMINIMUM, GTK_ICON_SIZE_BUTTON, GTK_RELIEF_NONE},
    { PITIVI_STATE_VOLIMG_MEDIUM, PITIVI_STOCK_VIEWER_VOLUMEMEDIUM, GTK_ICON_SIZE_BUTTON, GTK_RELIEF_NONE},
    { PITIVI_STATE_VOLIMG_MAX, PITIVI_STOCK_VIEWER_VOLUMEMAX, GTK_ICON_SIZE_BUTTON, GTK_RELIEF_NONE},
    { 0, 0},
  };

/*
 * Insert "added-value" functions here
 */

PitiviViewerVolume *
pitivi_viewervolume_new ()
{
  PitiviViewerVolume	*viewervolume;
  
  viewervolume = (PitiviViewerVolume *) g_object_new(PITIVI_VIEWERVOLUME_TYPE, NULL);
  g_assert(viewervolume != NULL);
  return viewervolume;
}

static GObject *
pitivi_viewervolume_constructor (GType type,
			     guint n_construct_properties,
			     GObjectConstructParam * construct_properties)
{
  GObject *obj;
  {
    /* Invoke parent constructor. */
    PitiviViewerVolumeClass *klass;
    GObjectClass *parent_class;
    klass = PITIVI_VIEWERVOLUME_CLASS (g_type_class_peek (PITIVI_VIEWERVOLUME_TYPE));
    parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
    obj = parent_class->constructor (type, n_construct_properties,
				     construct_properties);
  }

  /* do stuff. */

  return obj;
}

static void
pitivi_viewervolume_show (PitiviViewerVolume *self, GtkWidget *toggle)
{
  GtkAllocation allocation;
  GdkGrabStatus pointer, keyboard;
  gint x, y;
  
  /* Positionner le mixer en dessous du boutton volume */

  gdk_window_get_origin (GTK_WIDGET (toggle)->window, &x, &y);
  allocation = toggle->allocation;
  
  x += allocation.x + allocation.width;
  y += allocation.y + (allocation.height / 2);
  
  gtk_window_move (GTK_WINDOW (self), x, y);
  gtk_widget_show_all (GTK_WIDGET(self));

  allocation = GTK_WIDGET (self)->allocation;

  /* si on le voit plus le popup à droite positionne à gauche */
   
  if (x + allocation.width > gdk_screen_width ()) {
    x -= allocation.width + toggle->allocation.width;
    gtk_window_move (GTK_WINDOW (self), x, y);
  }
  
  /* Meme chose pour le bas  */
  
  if (y + allocation.height > gdk_screen_height ()) {
    y = gdk_screen_height () - allocation.height;
    gtk_window_move (GTK_WINDOW (self), x, y);
  }  
}

static void
pitivi_viewervolume_hide (PitiviViewerVolume *self)
{
  gtk_widget_hide (GTK_WIDGET(self));
}

static gboolean
pitivi_viewervolume_cb_key_pressed (GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
  PitiviViewerVolume *self = (PitiviViewerVolume *) user_data;
  
  return TRUE;
}

void
pitivi_viewervolume_cb_button_clicked (GtkWidget *volume_button, gpointer user_data)
{
  PitiviViewerVolume *self = (PitiviViewerVolume *) user_data;
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (volume_button))) {
    pitivi_viewervolume_show (self, volume_button);
  }
  else
    pitivi_viewervolume_hide (self);
}

int
pitivi_viewervolume_cb_button_released (GtkWidget *volume_button,
					GdkEventButton * event,
					gpointer  user_data)
{
  PitiviViewerVolume *self = (PitiviViewerVolume *) user_data;
  
  if (event->button == GDK_BUTTON_PRESS) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (volume_button), FALSE);
  }
  return FALSE;
}

static void
pitivi_viewervolume_changed (GtkAdjustment * adjustment, gpointer *user_data)
{
  int	 count;
  gfloat volume;
  
  PitiviViewerVolume *self = (PitiviViewerVolume *) user_data;
  
  volume = gtk_adjustment_get_value (GTK_ADJUSTMENT (self->public->adjust_vol));
  
  for (count = 0; count < PITIVI_STATE_VOLIMG_ALL; count++)
      gtk_widget_hide (GTK_WIDGET(self->public->state_vol[count]));
  for (count = 0; count < PITIVI_STATE_VOLIMG_ALL; count++)
    {
      if (volume > 0.75) {
	gtk_widget_show (GTK_WIDGET(self->public->state_vol[PITIVI_STATE_VOLIMG_ZERO]));
      } else if (volume <= 0.75 && volume > 0.5) {
	gtk_widget_show (GTK_WIDGET(self->public->state_vol[PITIVI_STATE_VOLIMG_MIN]));
      } else if (volume <= 0.5 && volume > 0.25) {
	gtk_widget_show (GTK_WIDGET(self->public->state_vol[PITIVI_STATE_VOLIMG_MEDIUM]));
      } else {
	gtk_widget_show (GTK_WIDGET(self->public->state_vol[PITIVI_STATE_VOLIMG_MAX]));
      }
    }
}

static void
pitivi_viewervolume_instance_init (GTypeInstance * instance, gpointer g_class)
{
  PitiviViewerVolume *self = (PitiviViewerVolume *) instance;
  GdkWindowAttr attributes;
  int		count;

  self->private = g_new0(PitiviViewerVolumePrivate, 1);
  self->public = g_new0(PitiviViewerVolumePublic, 1);
  
  /* initialize all public and private members to reasonable default values. */ 
  
  self->private->dispose_has_run = FALSE;
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  
  /* If you need specific consruction properties to complete initialization, 
   * delay initialization completion until the property is set. 
   */
  
  self->private->frame  = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (self), self->private->frame);
  gtk_container_set_border_width (GTK_CONTAINER (self->private->frame), 1);
  gtk_frame_set_shadow_type (GTK_FRAME (self->private->frame), GTK_SHADOW_OUT);
  
  self->public->adjust_vol = GTK_WIDGET (gtk_adjustment_new (0.9, 0.0, 1.0, 0.1, 0.10, 0.10));
  self->private->vscale_vol = gtk_vscale_new (GTK_ADJUSTMENT (self->public->adjust_vol));
  gtk_container_add (GTK_CONTAINER (self->private->frame), self->private->vscale_vol);
  gtk_widget_set_size_request (self->private->vscale_vol, -1, 100);
  gtk_scale_set_draw_value (GTK_SCALE (self->private->vscale_vol), FALSE);
  gtk_range_set_inverted (GTK_RANGE (self->private->vscale_vol), TRUE);

  for (count = 0; count < PITIVI_STATE_VOLIMG_ALL; count++)
    self->public->state_vol[count] = gtk_image_new_from_stock (stockvols[count].stock_icon, stockvols[count].stock_size);
  g_signal_connect (self->public->adjust_vol, "value-changed", G_CALLBACK (pitivi_viewervolume_changed), self);
  gtk_widget_hide_all (GTK_WIDGET (self));
}

static void
pitivi_viewervolume_dispose (GObject *object)
{
  PitiviViewerVolume	*self = PITIVI_VIEWERVOLUME(object);

  /* If dispose did already run, return. */
  if (self->private->dispose_has_run)
    return;
  
  /* Make sure dispose does not run twice. */
  self->private->dispose_has_run = TRUE;	

  /* 
   * In dispose, you are supposed to free all types referenced from this 
   * object which might themselves hold a reference to self. Generally, 
   * the most simple solution is to unref all members on which you own a 
   * reference. 
   */

}

static void
pitivi_viewervolume_finalize (GObject *object)
{
  PitiviViewerVolume	*self = PITIVI_VIEWERVOLUME(object);

  /* 
   * Here, complete object destruction. 
   * You might not need to do much... 
   */

  g_free (self->private);
}


static void
pitivi_viewervolume_set_parent (PitiviViewerVolume *self, GtkWidget *widget)
{
  if (GTK_IS_WINDOW(widget))
    { 
      self->private->parent_window = widget;
    }
}

static void
pitivi_viewervolume_set_property (GObject * object,
			      guint property_id,
			      const GValue * value, GParamSpec * pspec)
{
  PitiviViewerVolume *self = (PitiviViewerVolume *) object;

  switch (property_id)
    {
    default:
      /* We don't have any other property... */
      g_assert (FALSE);
      break;
    }
}

static void
pitivi_viewervolume_get_property (GObject * object,
			      guint property_id,
			      GValue * value, GParamSpec * pspec)
{
  PitiviViewerVolume *self = (PitiviViewerVolume *) object;

  switch (property_id)
    {
      /*  case PITIVI_VIEWERVOLUME_PROPERTY: { */
      /*     g_value_set_string (value, self->private->name); */
      /*   } */
      /*     break; */
    default:
      /* We don't have any other property... */
      g_assert (FALSE);
      break;
    }
}

static void
pitivi_viewervolume_class_init (gpointer g_class, gpointer g_class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
  PitiviViewerVolumeClass *klass = PITIVI_VIEWERVOLUME_CLASS (g_class);

  gobject_class->constructor = pitivi_viewervolume_constructor;
  gobject_class->dispose = pitivi_viewervolume_dispose;
  gobject_class->finalize = pitivi_viewervolume_finalize;

  gobject_class->set_property = pitivi_viewervolume_set_property;
  gobject_class->get_property = pitivi_viewervolume_get_property;
}

GType
pitivi_viewervolume_get_type (void)
{
  static GType type = 0;
 
  if (type == 0)
    {
      static const GTypeInfo info = {
	sizeof (PitiviViewerVolumeClass),
	NULL,			/* base_init */
	NULL,			/* base_finalize */
	pitivi_viewervolume_class_init,	/* class_init */
	NULL,			/* class_finalize */
	NULL,			/* class_data */
	sizeof (PitiviViewerVolume),
	0,			/* n_preallocs */
	pitivi_viewervolume_instance_init	/* instance_init */
      };
      type = g_type_register_static (GTK_TYPE_WINDOW,
				     "PitiviViewerVolumeType", &info, 0);
    }

  return type;
}
