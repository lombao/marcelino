/*
 * MARCELINO: a small window manager using XCB
 * 
 *
 * Copyright (c) 2014 Cesar Lombao
 *
 * Copyright (c) 2010, 2011, 2012 Michael Cardell Widerkrantz
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */






#include "conf.h"
#include "windows.h"
#include "workspace.h"
#include "keyboard.h"
#include "mrandr.h"
#include "server.h"



extern struct client *focuswin;

/***********************************************************/
xcb_connection_t *conn;         /* Connection to X server. */
xcb_screen_t     *screen;       /* Our current screen.     */
xcb_drawable_t root;
/***********************************************************/

/*********************************************/
/* ATOMS                                     */
xcb_atom_t atom_desktop;        /*
                                 * EWMH _NET_WM_DESKTOP hint that says
                                 * what workspace a window should be
                                 * on.
                                 */

xcb_atom_t wm_delete_window;    /* WM_DELETE_WINDOW event to close windows.  */
xcb_atom_t wm_change_state;
xcb_atom_t wm_state;
xcb_atom_t wm_protocols;        /* WM_PROTOCOLS.  */
/**********************************************/


int scrno;

xcb_connection_t * server_get_conn(void) { return conn; }
xcb_screen_t *     server_get_screen(void) { return screen; }
xcb_drawable_t     server_get_root(void) { return root; }
void server_disconnect(void) { xcb_disconnect(conn); }
void server_flush(void) { xcb_flush(conn); }



void server_init(void) {
	
 int i;
 xcb_screen_iterator_t iter;	
 
     /*
     * Use $DISPLAY. After connecting scrno will contain the value of
     * the display's screen.
     */
    conn = xcb_connect(NULL, &scrno);
    if (xcb_connection_has_error(conn))
    {
        perror("marcelino: xcb_connect failed.");
        exit(1);
    }

    /* Find our screen. */
    iter = xcb_setup_roots_iterator(xcb_get_setup(conn));
    for (i = 0; i < scrno; ++ i) { xcb_screen_next(&iter);}
    screen = iter.data;
    if (!screen) {
        fprintf (stderr, "marcelino: Can't get the current screen. Exiting.\n");
        xcb_disconnect(conn);
        exit(1);
    }

    root = screen->root;
    
    PDEBUG("Screen size: %dx%d\nRoot window: %d\n", 
                 screen->width_in_pixels,
                 screen->height_in_pixels, screen->root);
    
    /* Get some atoms. */
    atom_desktop = getatom("_NET_WM_DESKTOP");
    wm_delete_window = getatom("WM_DELETE_WINDOW");
    wm_change_state = getatom("WM_CHANGE_STATE");
    wm_state = getatom("WM_STATE");
    wm_protocols = getatom("WM_PROTOCOLS");
 }   
 
 
 
 
 /***************************************************/
/* Get a defined atom from the X server.           */ 
xcb_atom_t getatom(char *atom_name)
{
    xcb_intern_atom_cookie_t atom_cookie;
    xcb_atom_t atom;
    xcb_intern_atom_reply_t *rep;
    
    atom_cookie = xcb_intern_atom(conn, 0, strlen(atom_name), atom_name);
    rep = xcb_intern_atom_reply(conn, atom_cookie, NULL);
    if (NULL != rep)
    {
        atom = rep->atom;
        free(rep);
        return atom;
    }

    /*
     * XXX Note that we return 0 as an atom if anything goes wrong.
     * Might become interesting.
     */
    return 0;
}
 

/******************************************************/
/* Walk through all existing windows and set them up. */
/* Returns 0 on success.  */
int server_setupscreen(void)
{
    xcb_query_tree_reply_t *reply;
    xcb_query_pointer_reply_t *pointer;
    int i;
    int len;
    xcb_window_t *children;
    xcb_get_window_attributes_reply_t *attr;
    struct client *client;
    uint32_t ws;
    
    /* Get all children. */
    reply = xcb_query_tree_reply(conn,
                                 xcb_query_tree(conn, screen->root), 0);
    if (NULL == reply)
    {
        return -1;
    }

    len = xcb_query_tree_children_length(reply);    
    children = xcb_query_tree_children(reply);
    
    /* Set up all windows on this root. */
    for (i = 0; i < len; i ++)
    {
        attr = xcb_get_window_attributes_reply(
            conn, xcb_get_window_attributes(conn, children[i]), NULL);

        if (!attr)
        {
            fprintf(stderr, "Couldn't get attributes for window %d.",
                    children[i]);
            continue;
        }

        /*
         * Don't set up or even bother windows in override redirect
         * mode.
         *
         * This mode means they wouldn't have been reported to us
         * with a MapRequest if we had been running, so in the
         * normal case we wouldn't have seen them.
         *
         * Only handle visible windows. 
         */    
        if (!attr->override_redirect
            && attr->map_state == XCB_MAP_STATE_VIEWABLE)
        {
            client = setupwin(children[i]);
            if (NULL != client)
            {
                /*
                 * Find the physical output this window will be on if
                 * RANDR is active.
                 */
                if (-1 != randr_get_base())
                {
                    PDEBUG("Looking for monitor on %d x %d.\n", client->x,
                        client->y);
                    client->monitor = findmonbycoord(client->x, client->y);
#if DEBUG
                    if (NULL != client->monitor)
                    {
                        PDEBUG("Found client on monitor %s.\n",
                               client->monitor->name);
                    }
                    else
                    {
                        PDEBUG("Couldn't find client on any monitor.\n");
                    }
#endif
                }
                
                /* Fit window on physical screen. */
                fitonscreen(client);
                
                /*
                 * Check if this window has a workspace set already as
                 * a WM hint.
                 *
                 */
                ws = getwmdesktop(children[i]);

                if (ws == NET_WM_FIXED)
                {
                    /* Add to current workspace. */
                    addtoworkspace(client, workspace_get_currentws());                    
                    /* Add to all other workspaces. */
                    fixwindow(client, false);
                }
                else if (MCWM_NOWS != ws && ws < WORKSPACES)
                {
                    addtoworkspace(client, ws);
                    /* If it's not our current workspace, hide it. */
                    if (ws != workspace_get_currentws())
                    {
                        xcb_unmap_window(conn, client->id);
                    }
                }
                else
                {
                    /*
                     * No workspace hint at all. Just add it to our
                     * current workspace.
                     */
                    addtoworkspace(client, workspace_get_currentws());
                }
            }
        }
        
        free(attr);
    } /* for */

    changeworkspace(0);
        
    /*
     * Get pointer position so we can set focus on any window which
     * might be under it.
     */
    pointer = xcb_query_pointer_reply(
        conn, xcb_query_pointer(conn, screen->root), 0);

    if (NULL == pointer)
    {
        focuswin = NULL;
    }
    else
    {
        setfocus(findclient(pointer->child));
        free(pointer);
    }
    
    xcb_flush(conn);
    
    free(reply);
    
    return 0;
} 

/********************************************************************/
void server_mouse_init(xcb_connection_t * conn,xcb_screen_t * screen) {
	
	 xcb_grab_button(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE,
                    1 /* left mouse button */,
                    MOUSEMODKEY);
    
    xcb_grab_button(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE,
                    2 /* middle mouse button */,
                    MOUSEMODKEY);

    xcb_grab_button(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE,
                    3 /* right mouse button */,
                    MOUSEMODKEY);
	
}


/********************************************************************/
void server_subscribe_to_events(xcb_connection_t * conn,xcb_screen_t * screen) {

  uint32_t mask = 0;
  uint32_t values[2];
  xcb_void_cookie_t cookie;
  xcb_generic_error_t *error;
      	    
    mask = XCB_CW_EVENT_MASK;

    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
              | XCB_EVENT_MASK_STRUCTURE_NOTIFY
              | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    cookie = xcb_change_window_attributes_checked(conn, screen->root, mask, values);
    error = xcb_request_check(conn, cookie);
    xcb_flush(conn);
    
    if (NULL != error)
    {
        fprintf(stderr, "marcelino: Can't get SUBSTRUCTURE REDIRECT. "
                "Error code: %d\n"
                "Another window manager running? Exiting.\n",
                error->error_code);

        xcb_disconnect(conn);    
        exit(1);
    }
}


/********************************************************/
/*
 * Set keyboard focus to follow mouse pointer. Then exit.
 *
 * We don't need to bother mapping all windows we know about. They
 * should all be in the X server's Save Set and should be mapped
 * automagically.
 */
void server_exit_cleanup(int code)
{
    xcb_set_input_focus(conn, XCB_NONE,
                        XCB_INPUT_FOCUS_POINTER_ROOT,
                        XCB_CURRENT_TIME);
    xcb_flush(conn);
    xcb_disconnect(conn);    
    exit(code);
}



/***************************************************/
/* Get the pixel values of a named colour colstr. */
uint32_t server_getcolor(const char *colstr)
{
    xcb_alloc_named_color_reply_t *col_reply;    
    xcb_colormap_t colormap; 
    xcb_generic_error_t *error;
    xcb_alloc_named_color_cookie_t colcookie;

    colormap = screen->default_colormap;
    colcookie = xcb_alloc_named_color(conn, colormap, strlen(colstr), colstr);
    col_reply = xcb_alloc_named_color_reply(conn, colcookie, &error);
    if (NULL != error)
    {
        fprintf(stderr, "marcelino: Couldn't get pixel value for colour %s.\n ", colstr);
        
        xcb_disconnect(conn);
        exit(1);
    }

    return col_reply->pixel;
}
