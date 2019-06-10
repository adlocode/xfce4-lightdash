/* Copyright (C) 2016 adlo
 * 
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>
 */
 
 #include "lightdash-compositor.h"
 
 G_DEFINE_TYPE (LightdashCompositor, lightdash_compositor, G_TYPE_OBJECT);

 static void lightdash_compositor_on_window_opened
	(WnckScreen *screen, WnckWindow *window, LightdashCompositor *compositor);
static void lightdash_compositor_on_window_closed
	(WnckScreen *screen, WnckWindow *window, LightdashCompositor *compositor);
static void lightdash_compositor_active_workspace_changed (WnckScreen          *screen,
                                                  WnckWorkspace       *previously_active_workspace,
                                                  LightdashCompositor *compositor);
 static int lightdash_compositor_xhandler_xerror (Display *dpy, XErrorEvent *e);
 
 static LightdashCompositor *_lightdash_compositor_singleton = NULL;

enum {
  WINDOW_OPENED_SIGNAL,
  WINDOW_CLOSED_SIGNAL,
  ACTIVE_WORKSPACE_CHANGED_SIGNAL,
	LAST_SIGNAL
};

static guint compositor_signals[LAST_SIGNAL]={0};
 
 static void lightdash_compositor_class_init (LightdashCompositorClass *klass)
 {
	 GObjectClass *object_class = G_OBJECT_CLASS (klass);

   compositor_signals [WINDOW_OPENED_SIGNAL] =
		g_signal_new ("window-opened",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST,
		0,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE, 1, WNCK_TYPE_WINDOW);

   compositor_signals [WINDOW_CLOSED_SIGNAL] =
		g_signal_new ("window-closed",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST,
		0,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE, 1, WNCK_TYPE_WINDOW);

   compositor_signals [ACTIVE_WORKSPACE_CHANGED_SIGNAL] =
		g_signal_new ("active-workspace-changed",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST,
		0,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE, 1, WNCK_TYPE_WORKSPACE);
	 
 }
 
 static void lightdash_compositor_init (LightdashCompositor *compositor)
 {
	int dv, dr;
	
	compositor->excluded_gdk_window = NULL;
	compositor->screen = wnck_screen_get_default();
	compositor->gdk_screen = gdk_screen_get_default ();
	compositor->dpy = gdk_x11_get_default_xdisplay ();
  compositor->gdk_display = gdk_display_get_default ();
	
	/* Trap all X errors throughout the lifetime of this object */
	gdk_x11_display_error_trap_push (compositor->gdk_display);
	wnck_screen_force_update (compositor->screen);

	for (int i = 0; i < ScreenCount (compositor->dpy); i++)
		XCompositeRedirectSubwindows (compositor->dpy, RootWindow (compositor->dpy, i),
			CompositeRedirectAutomatic);

	XDamageQueryExtension (compositor->dpy, &dv, &dr);
	gdk_x11_register_standard_event_type (gdk_screen_get_display (compositor->gdk_screen),
		dv, dv + XDamageNotify);

   g_signal_connect (compositor->screen, "window-opened",
                G_CALLBACK (lightdash_compositor_on_window_opened), compositor);

   g_signal_connect (compositor->screen, "window-closed",
                G_CALLBACK (lightdash_compositor_on_window_closed), compositor);

   g_signal_connect (compositor->screen, "active-workspace-changed",
               G_CALLBACK (lightdash_compositor_active_workspace_changed), compositor);
 }
 
static void lightdash_compositor_on_window_opened
	(WnckScreen *screen, WnckWindow *window, LightdashCompositor *compositor)
{
  g_signal_emit (compositor, compositor_signals[WINDOW_OPENED_SIGNAL],
                 0, window);
}

static void lightdash_compositor_on_window_closed
	(WnckScreen *screen, WnckWindow *window, LightdashCompositor *compositor)
{
  g_signal_emit (compositor, compositor_signals[WINDOW_CLOSED_SIGNAL],
                 0, window);
}

static void lightdash_compositor_active_workspace_changed (WnckScreen          *screen,
                                                  WnckWorkspace       *previously_active_workspace,
                                                  LightdashCompositor *compositor)
{
  g_signal_emit (compositor, compositor_signals[ACTIVE_WORKSPACE_CHANGED_SIGNAL],
                 0, previously_active_workspace);
}

 WnckScreen * lightdash_compositor_get_wnck_screen (LightdashCompositor *compositor)
 {
	 return compositor->screen;
 }

void lightdash_compositor_set_excluded_window (LightdashCompositor *compositor, GdkWindow *gdk_window)
{
	compositor->excluded_gdk_window = gdk_window;
}

 WnckWindow * lightdash_compositor_get_root_window (LightdashCompositor *compositor)
 {
	 GList *li;
	 
	 for (li = wnck_screen_get_windows (compositor->screen); li != NULL; li = li->next)
	 {
		 WnckWindow *win = WNCK_WINDOW (li->data);
		 
		 if (wnck_window_get_window_type (win) == WNCK_WINDOW_DESKTOP)
		 {
			 return win;
		 }
	 }
	 
	 return NULL;
 }
 
 LightdashCompositor * lightdash_compositor_get_default ()
 {
	if (_lightdash_compositor_singleton == NULL)
	{
		_lightdash_compositor_singleton =
			LIGHTDASH_COMPOSITOR (g_object_new (LIGHTDASH_TYPE_COMPOSITOR, NULL));
	}
	else g_object_ref (_lightdash_compositor_singleton);
	
	return _lightdash_compositor_singleton;
}
