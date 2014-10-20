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
 
 
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

#ifdef DEBUG
#include "events.h"
#endif



#include "conf.h"
#include "mrandr.h"
#include "list.h"
#include "windows.h"
#include "workspace.h"
#include "keyboard.h"


/* Globals */

extern xcb_connection_t *conn;         /* Connection to X server. */
extern xcb_screen_t     *screen;       /* Our current screen.  */
extern int randrbase;                  /* Beginning of RANDR extension events. */

struct client *focuswin;        /* Current focus window. */
struct client *lastfocuswin;        /* Last focused window. NOTE! Only
                                     * used to communicate between
                                     * start and end of tabbing
                                     * mode. */
struct item *winlist = NULL;    /* Global list of all client windows. */
struct item *monlist = NULL;    /* List of all physical monitor outputs. */
int mode = 0;                   /* Internal mode, such as move or resize */


extern xcb_atom_t atom_desktop;        /*
                                 * EWMH _NET_WM_DESKTOP hint that says
                                 * what workspace a window should be
                                 * on.
                                 */

extern xcb_atom_t wm_delete_window;    /* WM_DELETE_WINDOW event to close windows.  */
extern xcb_atom_t wm_change_state;
extern xcb_atom_t wm_state;
extern xcb_atom_t wm_protocols;        /* WM_PROTOCOLS.  */



/* Function bodies. */

/*
 * MODKEY was released after tabbing around the
 * workspace window ring. This means this mode is
 * finished and we have found a new focus window.
 *
 * We need to move first the window we used to focus
 * on to the head of the window list and then move the
 * new focus to the head of the list as well. The list
 * should always start with the window we're focusing
 * on.
 */
void finishtabbing(void)
{
    mode = 0;
    
    if (NULL != lastfocuswin)
    {
        movetohead(workspace_get_wslist_current(), lastfocuswin->wsitem[workspace_get_currentws()]);
        lastfocuswin = NULL;             
    }
                
    movetohead(workspace_get_wslist_current(), focuswin->wsitem[workspace_get_currentws()]);
}

/*
 * Set keyboard focus to follow mouse pointer. Then exit.
 *
 * We don't need to bother mapping all windows we know about. They
 * should all be in the X server's Save Set and should be mapped
 * automagically.
 */
void cleanup(int code)
{
    xcb_set_input_focus(conn, XCB_NONE,
                        XCB_INPUT_FOCUS_POINTER_ROOT,
                        XCB_CURRENT_TIME);
    xcb_flush(conn);
    xcb_disconnect(conn);    
    exit(code);
}

/*
 * Rearrange windows to fit new screen size.
 */ 
void arrangewindows(void)
{
    struct item *item;
    struct client *client;

    /*
     * Go through all windows. If they don't fit on the new screen,
     * move them around and resize them as necessary.
     */
    for (item = winlist; item != NULL; item = item->next)
    {
        client = item->data;
        fitonscreen(client);
    } /* for */
}

/* Set the EWMH hint that window win belongs on workspace ws. */
void setwmdesktop(xcb_drawable_t win, uint32_t ws)
{
    PDEBUG("Changing _NET_WM_DESKTOP on window %d to %d\n", win, ws);
    
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
                        atom_desktop, XCB_ATOM_CARDINAL, 32, 1,
                        &ws);
}

/*
 * Get EWWM hint so we might know what workspace window win should be
 * visible on.
 *
 * Returns either workspace, NET_WM_FIXED if this window should be
 * visible on all workspaces or MCWM_NOWS if we didn't find any hints.
 */
int32_t getwmdesktop(xcb_drawable_t win)
{
    xcb_get_property_reply_t *reply;
    xcb_get_property_cookie_t cookie;
    uint32_t *wsp;
    uint32_t ws;
    
    cookie = xcb_get_property(conn, false, win, atom_desktop,
                              XCB_GET_PROPERTY_TYPE_ANY, 0,
                              sizeof (int32_t));

    reply = xcb_get_property_reply(conn, cookie, NULL);
    if (NULL == reply)
    {
        fprintf(stderr, "mcwm: Couldn't get properties for win %d\n", win);
        return MCWM_NOWS;
    }

    /* Length is 0 if we didn't find it. */
    if (0 == xcb_get_property_value_length(reply))
    {
        PDEBUG("_NET_WM_DESKTOP reply was 0 length.\n");
        goto bad;
    }
        
    wsp = xcb_get_property_value(reply);

    ws = *wsp;

    PDEBUG("got _NET_WM_DESKTOP: %d stored at %p.\n", ws, (void *)wsp);
    
    free(reply);

    return ws;

bad:
    free(reply);
    return MCWM_NOWS;
}


/***************************************************/
/* Get the pixel values of a named colour colstr. */
uint32_t getcolor(const char *colstr)
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
        fprintf(stderr, "mcwm: Couldn't get pixel value for colour %s. "
                "Exiting.\n", colstr);

        xcb_disconnect(conn);
        exit(1);
    }

    return col_reply->pixel;
}


/*
 * Fix or unfix a window client from all workspaces. If setcolour is
 * set, also change back to ordinary focus colour when unfixing.
 */
void fixwindow(struct client *client, bool setcolour)
{
    uint32_t values[1];
    uint32_t ws;
    
    if (NULL == client)
    {
        return;
    }
    
    if (client->fixed)
    {
        client->fixed = false;
        setwmdesktop(client->id, workspace_get_currentws());

        if (setcolour)
        {
            /* Set border color to ordinary focus colour. */
            values[0] = getcolor(conf_get_focuscol());
            xcb_change_window_attributes(conn, client->id, XCB_CW_BORDER_PIXEL,
                                         values);
        }

        /* Delete from all workspace lists except current. */
        for (ws = 0; ws < WORKSPACES; ws ++)
        {
            if (ws != workspace_get_currentws())
            {
                delfromworkspace(client, ws);
            }
        }
    }
    else
    {
        /*
         * First raise the window. If we're going to another desktop
         * we don't want this fixed window to be occluded behind
         * something else.
         */
        raisewindow(client->id);
        
        client->fixed = true;
        setwmdesktop(client->id, NET_WM_FIXED);

        /* Add window to all workspace lists. */
        for (ws = 0; ws < WORKSPACES; ws ++)
        {
            if (ws != workspace_get_currentws())
            {
                addtoworkspace(client, ws);
            }
        }

        if (setcolour)
        {
            /* Set border color to fixed colour. */
            values[0] = conf_get_fixedcol();
            xcb_change_window_attributes(conn, client->id, XCB_CW_BORDER_PIXEL,
                                         values);
        }
    }

    xcb_flush(conn);    
}



/* Forget everything about client client. */
void forgetclient(struct client *client)
{
    uint32_t ws;
    
    if (NULL == client)
    {
        PDEBUG("forgetclient: client was NULL\n");
        return;
    }

    /*
     * Delete this client from whatever workspace lists it belongs to.
     * Note that it's OK to be on several workspaces at once even if
     * you're not fixed.
     */
    for (ws = 0; ws < WORKSPACES; ws ++)
    {
        if (NULL != client->wsitem[ws])
        {
            delfromworkspace(client, ws);                    
        }
    }

    /* Remove from global window list. */
    freeitem(&winlist, NULL, client->winitem);
}

/* Forget everything about a client with client->id win. */
void forgetwin(xcb_window_t win)
{
    struct item *item;
    struct client *client;
    uint32_t ws;
    
    /* Find this window in the global window list. */
    for (item = winlist; item != NULL; item = item->next)
    {
        client = item->data;

        /*
         * Forget about it completely and free allocated data.
         *
         * Note that it might already be freed by handling an
         * UnmapNotify, so it isn't necessarily an error if we don't
         * find it.
         */
        PDEBUG("Win %d == client ID %d\n", win, client->id);
        if (win == client->id)
        {
            /* Found it. */
            PDEBUG("Found it. Forgetting...\n");

            /*
             * Delete window from whatever workspace lists it belonged
             * to. Note that it's OK to be on several workspaces at
             * once.
             */
            for (ws = 0; ws < WORKSPACES; ws ++)
            {
                PDEBUG("Looking in ws #%d.\n", ws);
                if (NULL == client->wsitem[ws])
                {
                    PDEBUG("  but it wasn't there.\n");
                }
                else
                {
                    PDEBUG("  found it here. Deleting!\n");
                    delfromworkspace(client, ws);                    
                }
            }
            
            free(item->data);
            delitem(&winlist, item);

            return;
        }
    }
}

/*
 * Fit client on physical screen, moving and resizing as necessary.
 */
void fitonscreen(struct client *client)
{
    uint16_t mon_x;
    uint16_t mon_y;
    uint16_t mon_width;
    uint16_t mon_height;
    bool willmove = false;
    bool willresize = false;

    client->vertmaxed = false;

    if (client->maxed)
    {
        client->maxed = false;
        setborders(client, conf_get_borderwidth());
    }
        
    if (NULL == client->monitor)
    {
        /*
         * This window isn't attached to any physical monitor. This
         * probably means there is no RANDR, so we use the root window
         * size.
         */
        mon_x = 0;
        mon_y = 0;
        mon_width = screen->width_in_pixels;
        mon_height = screen->height_in_pixels;
    }
    else
    {
        mon_x = client->monitor->x;
        mon_y = client->monitor->y;
        mon_width = client->monitor->width;
        mon_height = client->monitor->height;        
    }

    PDEBUG("Is window outside monitor?\n");
    PDEBUG("x: %d between %d and %d?\n", client->x, mon_x, mon_x + mon_width);
    PDEBUG("y: %d between %d and %d?\n", client->y, mon_y, mon_y + mon_height);
    
    /* Is it outside the physical monitor? */
    if (client->x > mon_x + mon_width)
    {
        client->x = mon_x + mon_width - client->width;
        willmove = true;
    }
    if (client->y > mon_y + mon_height)
    {
        client->y = mon_y + mon_height - client->height;
        willmove = true;        
    }
    
    if (client->x < mon_x)
    {
        client->x = mon_x;
        willmove = true;
    }
    if (client->y < mon_y)
    {
        client->y = mon_y;
        willmove = true;
    }
    
    /* Is it smaller than it wants to  be? */
    if (0 != client->min_height && client->height < client->min_height)
    {
        client->height = client->min_height;
        willresize = true;
    }

    if (0 != client->min_width && client->width < client->min_width)
    {
        client->width = client->min_width;
        willresize = true;        
    }

    /*
     * If the window is larger than our screen, just place it in the
     * corner and resize.
     */
    if (client->width + conf_get_borderwidth() * 2 > mon_width)
    {
        client->x = mon_x;
        client->width = mon_width - conf_get_borderwidth() * 2;;
        willmove = true;
        willresize = true;
    }
    else if (client->x + client->width + conf_get_borderwidth() * 2
             > mon_x + mon_width)
    {
        client->x = mon_x + mon_width - (client->width + conf_get_borderwidth() * 2);
        willmove = true;
    }

    if (client->height + conf_get_borderwidth() * 2 > mon_height)
    {
        client->y = mon_y;
        client->height = mon_height - conf_get_borderwidth() * 2;
        willmove = true;
        willresize = true;
    }
    else if (client->y + client->height + conf_get_borderwidth() * 2
             > mon_y + mon_height)
    {
        client->y = mon_y + mon_height - (client->height + conf_get_borderwidth()
                                          * 2);
        willmove = true;
    }

    if (willmove)
    {
        PDEBUG("Moving to %d,%d.\n", client->x, client->y);
        movewindow(client->id, client->x, client->y);
    }

    if (willresize)
    {
        PDEBUG("Resizing to %d x %d.\n", client->width, client->height);
        resize(client->id, client->width, client->height);
    }
}

/*
 * Set position, geometry and attributes of a new window and show it
 * on the screen.
 */
void newwin(xcb_window_t win)
{
    struct client *client;

    if (NULL != findclient(win))
    {
        /*
         * We know this window from before. It's trying to map itself
         * on the current workspace, but since it's unmapped it
         * probably belongs on another workspace. We don't like that.
         * Silently ignore.
         */
        return;
    }
    
    /*
     * Set up stuff, like borders, add the window to the client list,
     * et cetera.
     */
    client = setupwin(win);
    if (NULL == client)
    {
        fprintf(stderr, "mcwm: Couldn't set up window. Out of memory.\n");
        return;
    }

    /* Add this window to the current workspace. */
    addtoworkspace(client, workspace_get_currentws());

    /*
     * If the client doesn't say the user specified the coordinates
     * for the window we map it where our pointer is instead.
     */
    if (!client->usercoord)
    {
        uint16_t pointx;
        uint16_t pointy;    
        PDEBUG("Coordinates not set by user. Using pointer: %d,%d.\n",
               pointx, pointy);

        /* Get pointer position so we can move the window to the cursor. */
        if (!getpointer(screen->root, &pointx, &pointy))
        {
            PDEBUG("Failed to get pointer coords!\n");
            pointx = 0;
            pointy = 0;
        }
        
        client->x = pointx;
        client->y = pointy;

        movewindow(client->id, client->x, client->y);
    }
    else
    {
        PDEBUG("User set coordinates.\n");
    }
        
    /* Find the physical output this window will be on if RANDR is active. */
    if (-1 != randrbase)
    {
        client->monitor = findmonbycoord(client->x, client->y);
        if (NULL == client->monitor)
        {
            /*
             * Window coordinates are outside all physical monitors.
             * Choose the first screen.
             */
            if (NULL != monlist)
            {
                client->monitor = monlist->data;
            }
        }
    }
    
    fitonscreen(client);
    
    /* Show window on screen. */
    xcb_map_window(conn, client->id);

    /* Declare window normal. */
    long data[] = { XCB_ICCCM_WM_STATE_NORMAL, XCB_NONE };
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
                              wm_state, wm_state, 32, 2, data);

    /*
     * Move cursor into the middle of the window so we don't lose the
     * pointer to another window.
     */
    xcb_warp_pointer(conn, XCB_NONE, win, 0, 0, 0, 0,
                     client->width / 2, client->height / 2);

    xcb_flush(conn);    
}

/* Set border colour, width and event mask for window. */
struct client *setupwin(xcb_window_t win)
{
    uint32_t mask = 0;    
    uint32_t values[2];
    struct item *item;
    struct client *client;
    xcb_size_hints_t hints;
    uint32_t ws;
    
    /* Set border color. */
    values[0] = getcolor(conf_get_unfocuscol());
    xcb_change_window_attributes(conn, win, XCB_CW_BORDER_PIXEL, values);

    /* Set border width. */
    values[0] = conf_get_borderwidth();
    mask = XCB_CONFIG_WINDOW_BORDER_WIDTH;
    xcb_configure_window(conn, win, mask, values);
    
    mask = XCB_CW_EVENT_MASK;
    values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
    xcb_change_window_attributes_checked(conn, win, mask, values);

    /* Add this window to the X Save Set. */
    xcb_change_save_set(conn, XCB_SET_MODE_INSERT, win);

    xcb_flush(conn);
    
    /* Remember window and store a few things about it. */
    
    item = additem(&winlist);
    
    if (NULL == item)
    {
        PDEBUG("newwin: Out of memory.\n");
        return NULL;
    }

    client = malloc(sizeof (struct client));
    if (NULL == client)
    {
        PDEBUG("newwin: Out of memory.\n");
        return NULL;
    }

    item->data = client;
    
    /* Initialize client. */
    client->id = win;
    client->usercoord = false;
    client->x = 0;
    client->y = 0;
    client->width = 0;
    client->height = 0;
    client->min_width = 0;
    client->min_height = 0;
    client->max_width = screen->width_in_pixels;
    client->max_height = screen->height_in_pixels;
    client->base_width = 0;
    client->base_height = 0;
    client->width_inc = 1;
    client->height_inc = 1;
    client->vertmaxed = false;
    client->maxed = false;
    client->fixed = false;
    client->monitor = NULL;

    client->winitem = item;

    for (ws = 0; ws < WORKSPACES; ws ++)
    {
        client->wsitem[ws] = NULL;
    }
    
    PDEBUG("Adding window %d\n", client->id);

    /* Get window geometry. */
    if (!getgeom(client->id, &client->x, &client->y, &client->width,
                 &client->height))
    {
        fprintf(stderr, "Couldn't get geometry in initial setup of window.\n");
    }
    
    /*
     * Get the window's incremental size step, if any.
     */
    if (!xcb_icccm_get_wm_normal_hints_reply(
            conn, xcb_icccm_get_wm_normal_hints_unchecked(
                conn, win), &hints, NULL))
    {
        PDEBUG("Couldn't get size hints.\n");
    }

    /*
     * The user specified the position coordinates. Remember that so
     * we can use geometry later.
     */
    if (hints.flags & XCB_ICCCM_SIZE_HINT_US_POSITION)
    {
        client->usercoord = true;
    }

    if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
    {
        client->min_width = hints.min_width;
        client->min_height = hints.min_height;
    }
    
    if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)
    {
        
        client->max_width = hints.max_width;
        client->max_height = hints.max_height;
    }
    
    if (hints.flags & XCB_ICCCM_SIZE_HINT_P_RESIZE_INC)
    {
        client->width_inc = hints.width_inc;
        client->height_inc = hints.height_inc;

        PDEBUG("widht_inc %d\nheight_inc %d\n", client->width_inc,
               client->height_inc);
    }

    if (hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE)
    {
        client->base_width = hints.base_width;
        client->base_height = hints.base_height;
    }

    return client;
}









void arrbymon(struct monitor *monitor)
{
    struct item *item;
    struct client *client;

    PDEBUG("arrbymon\n");
    /*
     * Go through all windows on this monitor. If they don't fit on
     * the new screen, move them around and resize them as necessary.
     *
     * FIXME: Use a per monitor workspace list instead of global
     * windows list.
     */
    for (item = winlist; item != NULL; item = item->next)
    {
        client = item->data;
        if (client->monitor == monitor)
        {
            fitonscreen(client);
        }
    } /* for */

}

struct monitor *findmonitor(xcb_randr_output_t id)
{
    struct item *item;
    struct monitor *mon;

    for (item = monlist; item != NULL; item = item->next)
    {
        mon = item->data;
        if (id == mon->id)
        {
            PDEBUG("findmonitor: Found it. Output ID: %d\n", mon->id);
            return mon;
        }
        PDEBUG("findmonitor: Goint to %p.\n", item->next);
    }

    return NULL;
}

struct monitor *findclones(xcb_randr_output_t id, int16_t x, int16_t y)    
{
    struct monitor *clonemon;
    struct item *item;

    for (item = monlist; item != NULL; item = item->next)
    {
        clonemon = item->data;

        PDEBUG("Monitor %s: x, y: %d--%d, %d--%d.\n",
               clonemon->name,
               clonemon->x, clonemon->x + clonemon->width,
               clonemon->y, clonemon->y + clonemon->height);

        /* Check for same position. */
        if (id != clonemon->id && clonemon->x == x && clonemon->y == y)
        {
            return clonemon;
        }
    }

    return NULL;
}

struct monitor *findmonbycoord(int16_t x, int16_t y)
{
    struct item *item;
    struct monitor *mon;

    for (item = monlist; item != NULL; item = item->next)
    {
        mon = item->data;

        PDEBUG("Monitor %s: x, y: %d--%d, %d--%d.\n",
               mon->name,
               mon->x, mon->x + mon->width,
               mon->y, mon->y + mon->height);

        PDEBUG("Is %d,%d between them?\n", x, y);
        
        if (x >= mon->x && x <= mon->x + mon->width
            && y >= mon->y && y <= mon->y + mon->height)
        {
            PDEBUG("findmonbycoord: Found it. Output ID: %d, name %s\n",
                   mon->id, mon->name);
            return mon;
        }
    }

    return NULL;
}

void delmonitor(struct monitor *mon)
{
    PDEBUG("Deleting output %s.\n", mon->name);
    free(mon->name);
    freeitem(&monlist, NULL, mon->item);
}

struct monitor *addmonitor(xcb_randr_output_t id, char *name, 
                           uint32_t x, uint32_t y, uint16_t width,
                           uint16_t height)
{
    struct item *item;
    struct monitor *mon;
    
    if (NULL == (item = additem(&monlist)))
    {
        fprintf(stderr, "Out of memory.\n");
        return NULL;
    }

    mon = malloc(sizeof (struct monitor));
    if (NULL == mon)
    {
        fprintf(stderr, "Out of memory.\n");
        return NULL;        
    }

    item->data = mon;
    
    mon->id = id;
    mon->name = name;
    mon->x = x;
    mon->y = y;
    mon->width = width;
    mon->height = height;
    mon->item = item;

    return mon;
}

/* Raise window win to top of stack. */
void raisewindow(xcb_drawable_t win)
{
    uint32_t values[] = { XCB_STACK_MODE_ABOVE };

    if (screen->root == win || 0 == win)
    {
        return;
    }
    
    xcb_configure_window(conn, win,
                         XCB_CONFIG_WINDOW_STACK_MODE,
                         values);
    xcb_flush(conn);
}

/*
 * Set window client to either top or bottom of stack depending on
 * where it is now.
 */
void raiseorlower(struct client *client)
{
    uint32_t values[] = { XCB_STACK_MODE_OPPOSITE };
    xcb_drawable_t win;
    
    if (NULL == client)
    {
        return;
    }

    win = client->id;
    
    xcb_configure_window(conn, win,
                         XCB_CONFIG_WINDOW_STACK_MODE,
                         values);
    xcb_flush(conn);
}

void movelim(struct client *client)
{
    int16_t mon_x;
    int16_t mon_y;
    uint16_t mon_width;
    uint16_t mon_height;
    
    if (NULL == client->monitor)
    {
        mon_x = 0;
        mon_y = 0;
        mon_width = screen->width_in_pixels;
        mon_height = screen->height_in_pixels;
    }
    else
    {
        mon_x = client->monitor->x;
        mon_y = client->monitor->y;
        mon_width = client->monitor->width;
        mon_height = client->monitor->height;        
    }

    /* Is it outside the physical monitor? */
    if (client->x < mon_x)
    {
        client->x = mon_x;
    }
    if (client->y < mon_y)
    {
        client->y = mon_y;
    }

    if (client->x + client->width > mon_x + mon_width - conf_get_borderwidth() * 2)
    {
        client->x = (mon_x + mon_width - conf_get_borderwidth() * 2) - client->width;
    }

    if (client->y + client->height > mon_y + mon_height - conf_get_borderwidth() * 2)
    {
        client->y = (mon_y + mon_height - conf_get_borderwidth() * 2)
            - client->height;
    }    

    movewindow(client->id, client->x, client->y);
}

/* Move window win to root coordinates x,y. */
void movewindow(xcb_drawable_t win, uint16_t x, uint16_t y)
{
    uint32_t values[2];

    if (screen->root == win || 0 == win)
    {
        /* Can't move root. */
        return;
    }
    
    values[0] = x;
    values[1] = y;

    xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_X
                         | XCB_CONFIG_WINDOW_Y, values);
    
    xcb_flush(conn);
}

/* Change focus to next in window ring. */
void focusnext(bool reverse)
{
    struct client *client = NULL;
    
#if DEBUG
    if (NULL != focuswin)
    {
        PDEBUG("Focus now in win %d\n", focuswin->id);
    }
#endif

    if (workspace_isempty(workspace_get_currentws()))
    {
        PDEBUG("No windows to focus on in this workspace.\n");
        return;
    }
    
    if (MCWM_TABBING != mode)
    {
        /*
         * Remember what we last focused on. We need this when the
         * MODKEY is released and we move the last focused window in
         * the tabbing order list.
         */        
        lastfocuswin = focuswin;
        mode = MCWM_TABBING;

        PDEBUG("Began tabbing.\n");        
    }
    
    /* If we currently have no focus focus first in list. */
    if (NULL == focuswin || NULL == focuswin->wsitem[workspace_get_currentws()])
    {
        PDEBUG("Focusing first in list: %p\n", wslist[workspace_get_currentws()]);
        client = workspace_get_firstitem(workspace_get_currentws())->data;

        if (NULL != focuswin && NULL == focuswin->wsitem[workspace_get_currentws()])
        {
            PDEBUG("XXX Our focused window %d isn't on this workspace!\n",
                   focuswin->id);
        }
    }
    else
    {
        if (reverse)
        {
            if (NULL == focuswin->wsitem[workspace_get_currentws()]->prev)
            {
                /*
                 * We were at the head of list. Focusing on last
                 * window in list unless we were already there.
                 */
                struct item *last = workspace_get_firstitem(workspace_get_currentws());
                while (NULL != last->next)
                    last = last->next;
                if (focuswin->wsitem[workspace_get_currentws()] != last->data)
                {
                    PDEBUG("Beginning of list. Focusing last in list: %p\n",
                           last);
                    client = last->data;
                }
            }
            else
            {
                /* Otherwise, focus the next in list. */
                PDEBUG("Tabbing. Focusing next: %p.\n",
                       focuswin->wsitem[workspace_get_currentws()]->prev);
                client = focuswin->wsitem[workspace_get_currentws()]->prev->data;
            }
        }
        else
        {
            if (NULL == focuswin->wsitem[workspace_get_currentws()]->next)
            {
                /*
                 * We were at the end of list. Focusing on first window in
                 * list unless we were already there.
                 */
                if (focuswin->wsitem[workspace_get_currentws()] != workspace_get_firstitem(workspace_get_currentws())->data)
                {
                    PDEBUG("End of list. Focusing first in list: %p\n",
                           wslist[workspace_get_currentws()]);
                    client = workspace_get_firstitem(workspace_get_currentws())->data;
                }
            }
            else
            {
                /* Otherwise, focus the next in list. */
                PDEBUG("Tabbing. Focusing next: %p.\n",
                       focuswin->wsitem[workspace_get_currentws()]->next);            
                client = focuswin->wsitem[workspace_get_currentws()]->next->data;
            }
        }
    } /* if NULL focuswin */

    if (NULL != client)
    {
        /*
         * Raise window if it's occluded, then warp pointer into it and
         * set keyboard focus to it.
         */
        uint32_t values[] = { XCB_STACK_MODE_TOP_IF };    

        xcb_configure_window(conn, client->id, XCB_CONFIG_WINDOW_STACK_MODE,
                             values);
        xcb_warp_pointer(conn, XCB_NONE, client->id, 0, 0, 0, 0,
                         client->width / 2, client->height / 2);
        setfocus(client);
    }
}

/* Mark window win as unfocused. */
void setunfocus(xcb_drawable_t win)
{
    uint32_t values[1];

    if (NULL == focuswin)
    {
        return;
    }
    
    if (focuswin->id == screen->root)
    {
        return;
    }

    /* Set new border colour. */
    values[0] = getcolor(conf_get_unfocuscol());
    xcb_change_window_attributes(conn, win, XCB_CW_BORDER_PIXEL, values);
    xcb_flush(conn);
}

/*
 * Find client with client->id win in global window list.
 *
 * Returns client pointer or NULL if not found.
 */
struct client *findclient(xcb_drawable_t win)
{
    struct item *item;
    struct client *client;

    for (item = winlist; item != NULL; item = item->next)
    {
        client = item->data;
        if (win == client->id)
        {
            PDEBUG("findclient: Found it. Win: %d\n", client->id);
            return client;
        }
    }

    return NULL;
}

/* Set focus on window client. */
void setfocus(struct client *client)
{
    uint32_t values[1];

    /*
     * If client is NULL, we focus on whatever the pointer is on.
     *
     * This is a pathological case, but it will make the poor user
     * able to focus on windows anyway, even though this window
     * manager might be buggy.
     */
    if (NULL == client)
    {
        PDEBUG("setfocus: client was NULL!\n");

        focuswin = NULL;

        xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_POINTER_ROOT,
                            XCB_CURRENT_TIME);
        xcb_flush(conn);
        
        return;
    }
    
    /*
     * Don't bother focusing on the root window or on the same window
     * that already has focus.
     */
    if (client->id == screen->root || client == focuswin)
    {
        return;
    }

    /* Set new border colour. */
    if (client->fixed)
    {
        values[0] = conf_get_fixedcol();            
    }
    else
    {
        values[0] = getcolor(conf_get_focuscol());
    }

    xcb_change_window_attributes(conn, client->id, XCB_CW_BORDER_PIXEL,
                                 values);

    /* Unset last focus. */
    if (NULL != focuswin)
    {
        setunfocus(focuswin->id);
    }

    /* Set new input focus. */

    xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, client->id,
                        XCB_CURRENT_TIME);

    xcb_flush(conn);
    
    /* Remember the new window as the current focused window. */
    focuswin = client;
}

int start(char *program)
{
    pid_t pid;
    
    pid = fork();
    if (-1 == pid)
    {
        perror("fork");
        return -1;
    }
    else if (0 == pid)
    {
        char *argv[2];

        /* In the child. */
        
        /*
         * Make this process a new process leader, otherwise the
         * terminal will die when the wm dies. Also, this makes any
         * SIGCHLD go to this process when we fork again.
         */
        if (-1 == setsid())
        {
            perror("setsid");
            exit(1);
        }
        
        argv[0] = program;
        argv[1] = NULL;

        if (-1 == execvp(program, argv))
        {
            perror("execve");            
            exit(1);
        }
        exit(0);
    }

    return 0;
}

/* Resize with limit. */
void resizelim(struct client *client)
{
    uint16_t mon_x;
    uint16_t mon_y;
    uint16_t mon_width;
    uint16_t mon_height;
    
    if (NULL == client->monitor)
    {
        mon_x = 0;
        mon_y = 0;
        mon_width = screen->width_in_pixels;
        mon_height = screen->height_in_pixels;
    }
    else
    {
        mon_x = client->monitor->x;
        mon_y = client->monitor->y;
        mon_width = client->monitor->width;
        mon_height = client->monitor->height;        
    }
    
    /* Is it smaller than it wants to  be? */
    if (0 != client->min_height && client->height < client->min_height)
    {
        client->height = client->min_height;
    }

    if (0 != client->min_width && client->width < client->min_width)
    {
        client->width = client->min_width;
    }

    if (client->x + client->width + conf_get_borderwidth() * 2 > mon_x + mon_width)
    {
        client->width = mon_width - ((client->x - mon_x) + conf_get_borderwidth()
                                     * 2);
    }
    
    if (client->y + client->height + conf_get_borderwidth() * 2 > mon_y + mon_height)
    {
        client->height = mon_height - ((client->y - mon_y) + conf_get_borderwidth()
                                       * 2);
    }
    
    resize(client->id, client->width, client->height);
}

void moveresize(xcb_drawable_t win, uint16_t x, uint16_t y,
                uint16_t width, uint16_t height)
{
    uint32_t values[4];
    
    if (screen->root == win || 0 == win)
    {
        /* Can't move or resize root. */
        return;
    }

    PDEBUG("Moving to %d, %d, resizing to %d x %d.\n", x, y, width, height);

    values[0] = x;
    values[1] = y;
    values[2] = width;
    values[3] = height;
                                        
    xcb_configure_window(conn, win,
                         XCB_CONFIG_WINDOW_X
                         | XCB_CONFIG_WINDOW_Y
                         | XCB_CONFIG_WINDOW_WIDTH
                         | XCB_CONFIG_WINDOW_HEIGHT, values);
    xcb_flush(conn);
}

/* Resize window win to width,height. */
void resize(xcb_drawable_t win, uint16_t width, uint16_t height)
{
    uint32_t values[2];

    if (screen->root == win || 0 == win)
    {
        /* Can't resize root. */
        return;
    }

    PDEBUG("Resizing to %d x %d.\n", width, height);
    
    values[0] = width;
    values[1] = height;
                                        
    xcb_configure_window(conn, win,
                         XCB_CONFIG_WINDOW_WIDTH
                         | XCB_CONFIG_WINDOW_HEIGHT, values);
    xcb_flush(conn);
}

/*
 * Resize window client in direction direction. Direction is:
 *
 * h = left, that is decrease width.
 *
 * j = down, that is, increase height.
 *
 * k = up, that is, decrease height.
 *
 * l = right, that is, increase width.
 */
void resizestep(struct client *client, char direction)
{
    uint32_t step_x = conf_get_movestep();
    uint32_t step_y = conf_get_movestep();
    
    if (NULL == client)
    {
        return;
    }

    if (client->maxed)
    {
        /* Can't resize a fully maximized window. */
        return;
    }
    
    raisewindow(client->id);
    
    if (client->width_inc > 1)
    {
        step_x = client->width_inc;
    }
    else
    {
        step_x = conf_get_movestep();
    }

    if (client->height_inc > 1)
    {
        step_y = client->height_inc;
    }
    else
    {
        step_y = conf_get_movestep();        
    }

    switch (direction)
    {
    case 'h':
        client->width = client->width - step_x;
        break;

    case 'j':
        client->height = client->height + step_y;
        break;

    case 'k':
        client->height = client->height - step_y;
        break;

    case 'l':
        client->width = client->width + step_x;
        break;

    default:
        PDEBUG("resizestep in unknown direction.\n");
        break;
    } /* switch direction */

    resizelim(client);
    
    /* If this window was vertically maximized, remember that it isn't now. */
    if (client->vertmaxed)
    {
        client->vertmaxed = false;
    }

    xcb_warp_pointer(conn, XCB_NONE, client->id, 0, 0, 0, 0,
                     client->width / 2, client->height / 2);
    xcb_flush(conn);
}

/*
 * Move window win as a result of pointer motion to coordinates
 * rel_x,rel_y.
 */
void mousemove(struct client *client, int rel_x, int rel_y)
{
    client->x = rel_x;
    client->y = rel_y;
    
    movelim(client);
}

void mouseresize(struct client *client, int rel_x, int rel_y)
{
    client->width = abs(rel_x - client->x);
    client->height = abs(rel_y - client->y);

    client->width -= (client->width - client->base_width) % client->width_inc;
    client->height -= (client->height - client->base_height)
        % client->height_inc;
    
    PDEBUG("Trying to resize to %dx%d (%dx%d)\n", client->width, client->height,
           (client->width - client->base_width) / client->width_inc,
           (client->height - client->base_height) / client->height_inc);

    resizelim(client);
    
    /* If this window was vertically maximized, remember that it isn't now. */
    if (client->vertmaxed)
    {
        client->vertmaxed = false;
    }
}

void movestep(struct client *client, char direction)
{
    uint16_t start_x;
    uint16_t start_y;
    
    if (NULL == client)
    {
        return;
    }

    if (client->maxed)
    {
        /* We can't move a fully maximized window. */
        return;
    }

    /* Save pointer position so we can warp pointer here later. */
    if (!getpointer(client->id, &start_x, &start_y))
    {
        return;
    }

    raisewindow(client->id);
    switch (direction)
    {
    case 'h':
        client->x = client->x - conf_get_movestep();
        break;

    case 'j':
        client->y = client->y + conf_get_movestep();
        break;

    case 'k':
        client->y = client->y - conf_get_movestep();
        break;

    case 'l':
        client->x = client->x + conf_get_movestep();
        break;

    default:
        PDEBUG("movestep: Moving in unknown direction.\n");
        break;
    } /* switch direction */

    movelim(client);
    
    /*
     * If the pointer was inside the window to begin with, move
     * pointer back to where it was, relative to the window.
     */
    if (start_x > 0 - conf_get_borderwidth() && start_x < client->width
        + conf_get_borderwidth() && start_y > 0 - conf_get_borderwidth() && start_y
        < client->height + conf_get_borderwidth())
    {
        xcb_warp_pointer(conn, XCB_NONE, client->id, 0, 0, 0, 0,
                         start_x, start_y);
        xcb_flush(conn);        
    }
}

void setborders(struct client *client, int width)
{
    uint32_t values[1];
    uint32_t mask = 0;
    
    values[0] = width;

    mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
    xcb_configure_window(conn, client->id, mask, &values[0]);
    xcb_flush(conn);
}

void unmax(struct client *client)
{
    uint32_t values[5];
    uint32_t mask = 0;

    if (NULL == client)
    {
        PDEBUG("unmax: client was NULL!\n");
        return;
    }

    client->x = client->origsize.x;
    client->y = client->origsize.y;
    client->width = client->origsize.width;
    client->height = client->origsize.height;
    
    /* Restore geometry. */
    if (client->maxed)
    {
        values[0] = client->x;
        values[1] = client->y;
        values[2] = client->width;
        values[3] = client->height;

        /* Set borders again. */
        values[4] = conf_get_borderwidth();
        
        mask =
            XCB_CONFIG_WINDOW_X
            | XCB_CONFIG_WINDOW_Y
            | XCB_CONFIG_WINDOW_WIDTH
            | XCB_CONFIG_WINDOW_HEIGHT
            | XCB_CONFIG_WINDOW_BORDER_WIDTH;
    }
    else
    {
        values[0] = client->y;
        values[1] = client->width;
        values[2] = client->height;

        mask = XCB_CONFIG_WINDOW_Y
            | XCB_CONFIG_WINDOW_WIDTH
            | XCB_CONFIG_WINDOW_HEIGHT;
    }

    xcb_configure_window(conn, client->id, mask, values);

    /* Warp pointer to window or we might lose it. */
    xcb_warp_pointer(conn, XCB_NONE, client->id, 0, 0, 0, 0,
                     client->width / 2, client->height / 2);

    xcb_flush(conn);
}

void maximize(struct client *client)
{
    uint32_t values[4];
    uint32_t mask = 0;
    uint16_t mon_x;
    uint16_t mon_y;
    uint16_t mon_width;
    uint16_t mon_height;
    
    if (NULL == client)
    {
        PDEBUG("maximize: client was NULL!\n");
        return;
    }

    if (NULL == client->monitor)
    {
        mon_x = 0;
        mon_y = 0;
        mon_width = screen->width_in_pixels;
        mon_height = screen->height_in_pixels;
    }
    else
    {
        mon_x = client->monitor->x;
        mon_y = client->monitor->y;
        mon_width = client->monitor->width;
        mon_height = client->monitor->height;                    
    }

    /*
     * Check if maximized already. If so, revert to stored
     * geometry.
     */
    if (client->maxed)
    {
        unmax(client);
        client->maxed = false;
        return;
    }

    /* Raise first. Pretty silly to maximize below something else. */
    raisewindow(client->id);
    
    /* FIXME: Store original geom in property as well? */
    client->origsize.x = client->x;
    client->origsize.y = client->y;
    client->origsize.width = client->width;
    client->origsize.height = client->height;
    
    /* Remove borders. */
    values[0] = 0;
    mask = XCB_CONFIG_WINDOW_BORDER_WIDTH;
    xcb_configure_window(conn, client->id, mask, values);
    
    /* Move to top left and resize. */
    client->x = mon_x;
    client->y = mon_y;
    client->width = mon_width;
    client->height = mon_height;
    
    values[0] = client->x;
    values[1] = client->y;
    values[2] = client->width;
    values[3] = client->height;
    xcb_configure_window(conn, client->id, XCB_CONFIG_WINDOW_X
                         | XCB_CONFIG_WINDOW_Y
                         | XCB_CONFIG_WINDOW_WIDTH
                         | XCB_CONFIG_WINDOW_HEIGHT, values);

    xcb_flush(conn);

    client->maxed = true;
}

void maxvert(struct client *client)
{
    uint32_t values[2];
    uint16_t mon_y;
    uint16_t mon_height;
    
    if (NULL == client)
    {
        PDEBUG("maxvert: client was NULL\n");
        return;
    }

    if (NULL == client->monitor)
    {
        mon_y = 0;
        mon_height = screen->height_in_pixels;
    }
    else
    {
        mon_y = client->monitor->y;
        mon_height = client->monitor->height;                    
    }

    /*
     * Check if maximized already. If so, revert to stored geometry.
     */
    if (client->vertmaxed)
    {
        unmax(client);
        client->vertmaxed = false;
        return;
    }

    /* Raise first. Pretty silly to maximize below something else. */
    raisewindow(client->id);
    
    /*
     * Store original coordinates and geometry.
     * FIXME: Store in property as well?
     */
    client->origsize.x = client->x;
    client->origsize.y = client->y;
    client->origsize.width = client->width;
    client->origsize.height = client->height;

    client->y = mon_y;
    /* Compute new height considering height increments and screen height. */
    client->height = mon_height - conf_get_borderwidth() * 2;
    client->height -= (client->height - client->base_height)
        % client->height_inc;

    /* Move to top of screen and resize. */
    values[0] = client->y;
    values[1] = client->height;
    
    xcb_configure_window(conn, client->id, XCB_CONFIG_WINDOW_Y
                         | XCB_CONFIG_WINDOW_HEIGHT, values);
    xcb_flush(conn);

    /* Remember that this client is vertically maximized. */
    client->vertmaxed = true;    
}

void hide(struct client *client)
{
    long data[] = { XCB_ICCCM_WM_STATE_ICONIC, XCB_NONE };

    /*
     * Unmap window and declare iconic.
     * 
     * Unmapping will generate an UnmapNotify event so we can forget
     * about the window later.
     */
    xcb_unmap_window(conn, client->id);
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
                        wm_state, wm_state, 32, 2, data);    
    xcb_flush(conn);
}

bool getpointer(xcb_drawable_t win, uint16_t *x, uint16_t *y)
{
    xcb_query_pointer_reply_t *pointer;
    
    pointer
        = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, win), 0);
    if (NULL == pointer)
    {
        return false;
    }

    *x = pointer->win_x;
    *y = pointer->win_y;

    free(pointer);

    return true;
}

bool getgeom(xcb_drawable_t win, uint16_t *x, uint16_t *y, uint16_t *width,
             uint16_t *height)
{
    xcb_get_geometry_reply_t *geom;
    
    geom
        = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, win), NULL);
    if (NULL == geom)
    {
        return false;
    }

    *x = geom->x;
    *y = geom->y;
    *width = geom->width;
    *height = geom->height;
    
    free(geom);

    return true;
}

void topleft(void)
{
    uint16_t pointx;
    uint16_t pointy;
    uint16_t mon_x;
    uint16_t mon_y;
    
    if (NULL == focuswin)
    {
        return;
    }

    if (NULL == focuswin->monitor)
    {
        mon_x = 0;
        mon_y = 0;
    }
    else
    {
        mon_x = focuswin->monitor->x;
        mon_y = focuswin->monitor->y;        
    }
        
    raisewindow(focuswin->id);
    
    if (!getpointer(focuswin->id, &pointx, &pointy))
    {
        return;
    }

    focuswin->x = mon_x;
    focuswin->y = mon_y;
    movewindow(focuswin->id, focuswin->x, focuswin->y);
    xcb_warp_pointer(conn, XCB_NONE, focuswin->id, 0, 0, 0, 0,
                     pointx, pointy);
    xcb_flush(conn);
}

void topright(void)
{
    uint16_t pointx;
    uint16_t pointy;
    uint16_t mon_x;    
    uint16_t mon_y;    
    uint16_t mon_width;
    
    if (NULL == focuswin)
    {
        return;
    }

    if (NULL == focuswin->monitor)
    {
        mon_width = screen->width_in_pixels;
        mon_x = 0;
        mon_y = 0;
    }
    else
    {
        mon_width = focuswin->monitor->width;
        mon_x = focuswin->monitor->x;        
        mon_y = focuswin->monitor->y;
    }

    raisewindow(focuswin->id);
    
    if (!getpointer(focuswin->id, &pointx, &pointy))
    {
        return;
    }

    focuswin->x = mon_x + mon_width -
        (focuswin->width + conf_get_borderwidth() * 2);    

    focuswin->y = mon_y;
    
    movewindow(focuswin->id, focuswin->x, focuswin->y);

    xcb_warp_pointer(conn, XCB_NONE, focuswin->id, 0, 0, 0, 0,
                     pointx, pointy);
    xcb_flush(conn);
}

void botleft(void)
{
    uint16_t pointx;
    uint16_t pointy;
    uint16_t mon_x;
    uint16_t mon_y;
    uint16_t mon_height;
    
    if (NULL == focuswin)
    {
        return;
    }

    if (NULL == focuswin->monitor)
    {
        mon_x = 0;
        mon_y = 0;
        mon_height = screen->height_in_pixels;
    }
    else
    {
        mon_x = focuswin->monitor->x;
        mon_y = focuswin->monitor->y;
        mon_height = focuswin->monitor->height;
    }
    
    raisewindow(focuswin->id);
    
    if (!getpointer(focuswin->id, &pointx, &pointy))
    {
        return;
    }

    focuswin->x = mon_x;
    focuswin->y = mon_y + mon_height - (focuswin->height + conf_get_borderwidth()
                                        * 2);
    movewindow(focuswin->id, focuswin->x, focuswin->y);
    xcb_warp_pointer(conn, XCB_NONE, focuswin->id, 0, 0, 0, 0,
                     pointx, pointy);
    xcb_flush(conn);
}

void botright(void)
{
    uint16_t pointx;
    uint16_t pointy;
    uint16_t mon_x;
    uint16_t mon_y;
    uint16_t mon_width;
    uint16_t mon_height;

    if (NULL == focuswin)
    {
        return;
    }

    if (NULL == focuswin->monitor)
    {
        mon_x = 0;
        mon_y = 0;
        mon_width = screen->width_in_pixels;;        
        mon_height = screen->height_in_pixels;
    }
    else
    {
        mon_x = focuswin->monitor->x;
        mon_y = focuswin->monitor->y;
        mon_width = focuswin->monitor->width;
        mon_height = focuswin->monitor->height;
    }

    raisewindow(focuswin->id);
    
    if (!getpointer(focuswin->id, &pointx, &pointy))
    {
        return;
    }

    focuswin->x = mon_x + mon_width - (focuswin->width + conf_get_borderwidth() * 2);
    focuswin->y =  mon_y + mon_height - (focuswin->height + conf_get_borderwidth()
                                         * 2);
    movewindow(focuswin->id, focuswin->x, focuswin->y);
    xcb_warp_pointer(conn, XCB_NONE, focuswin->id, 0, 0, 0, 0,
                     pointx, pointy);
    xcb_flush(conn);
}

void deletewin(void)
{
    xcb_get_property_cookie_t cookie;
    xcb_icccm_get_wm_protocols_reply_t protocols;
    bool use_delete = false;
    uint32_t i;

    if (NULL == focuswin)
    {
        return;
    }

    /* Check if WM_DELETE is supported.  */
    cookie = xcb_icccm_get_wm_protocols_unchecked(conn, focuswin->id,
                                                  wm_protocols);
    if (xcb_icccm_get_wm_protocols_reply(conn, cookie, &protocols, NULL) == 1)
    {
        for (i = 0; i < protocols.atoms_len; i++)
        {
            if (protocols.atoms[i] == wm_delete_window)
            {
                 use_delete = true;
            }
        }
    }

    xcb_icccm_get_wm_protocols_reply_wipe(&protocols);

    if (use_delete)
    {
        xcb_client_message_event_t ev = {
          .response_type = XCB_CLIENT_MESSAGE,
          .format = 32,
          .sequence = 0,
          .window = focuswin->id,
          .type = wm_protocols,
          .data.data32 = { wm_delete_window, XCB_CURRENT_TIME }
        };

        xcb_send_event(conn, false, focuswin->id,
                       XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
    }
    else
    {
        xcb_kill_client(conn, focuswin->id);
    }

    xcb_flush(conn);    
}

void prevscreen(void)
{
    struct item *item;
    
    if (NULL == focuswin || NULL == focuswin->monitor)
    {
        return;
    }

    item = focuswin->monitor->item->prev;

    if (NULL == item)
    {
        return;
    }
    
    focuswin->monitor = item->data;

    raisewindow(focuswin->id);    
    fitonscreen(focuswin);
    movelim(focuswin);

    xcb_warp_pointer(conn, XCB_NONE, focuswin->id, 0, 0, 0, 0,
                     0, 0);
    xcb_flush(conn);    
}

void nextscreen(void)
{
    struct item *item;
    
    if (NULL == focuswin || NULL == focuswin->monitor)
    {
        return;
    }

    item = focuswin->monitor->item->next;

    if (NULL == item)
    {
        return;
    }
    
    focuswin->monitor = item->data;

    raisewindow(focuswin->id);    
    fitonscreen(focuswin);
    movelim(focuswin);

    xcb_warp_pointer(conn, XCB_NONE, focuswin->id, 0, 0, 0, 0,
                     0, 0);
    xcb_flush(conn);    
}

void handle_keypress(xcb_key_press_event_t *ev)
{
    int i;
    key_enum_t key;
    
    for (key = KEY_MAX, i = KEY_F; i < KEY_MAX; i ++)
    {
        if (ev->detail == keyboard_keys_get_keycode(i) && 0 != keyboard_keys_get_keycode(i))
        {
            key = i;
            break;
        }
    }
    if (key == KEY_MAX)
    {
        PDEBUG("Unknown key pressed.\n");

        /*
         * We don't know what to do with this key. Send this key press
         * event to the focused window.
         */
        xcb_send_event(conn, false, XCB_SEND_EVENT_DEST_ITEM_FOCUS,
                       XCB_EVENT_MASK_NO_EVENT, (char *) ev);
        xcb_flush(conn);
        return;
    }

    if (MCWM_TABBING == mode && key != KEY_TAB && key != KEY_BACKTAB)
    {
        /* First finish tabbing around. Then deal with the next key. */
        finishtabbing();
    }
    
    /* Is it shifted? */
    if (ev->state & SHIFTMOD)
    {
        switch (key)
        {
        case KEY_H: /* h */
            resizestep(focuswin, 'h');
            break;

        case KEY_J: /* j */
            resizestep(focuswin, 'j');
            break;

        case KEY_K: /* k */
            resizestep(focuswin, 'k');
            break;

        case KEY_L: /* l */
            resizestep(focuswin, 'l');
            break;

        case KEY_TAB: /* shifted tab counts as backtab */
            focusnext(true);
            break;

        default:
            /* Ignore other shifted keys. */
            break;
        }
    }
    else
    {
        switch (key)
        {
        case KEY_RET: /* return */
            start(conf_get_terminal());
            break;

        case KEY_F: /* f */
            fixwindow(focuswin, true);
            break;
            
        case KEY_H: /* h */
            movestep(focuswin, 'h');
            break;

        case KEY_J: /* j */
            movestep(focuswin, 'j');
            break;

        case KEY_K: /* k */
            movestep(focuswin, 'k');
            break;

        case KEY_L: /* l */
            movestep(focuswin, 'l');
            break;

        case KEY_TAB: /* tab */
            focusnext(false);
            break;

        case KEY_BACKTAB: /* backtab */
            focusnext(true);
            break;

        case KEY_M: /* m */
            maxvert(focuswin);
            break;

        case KEY_R: /* r*/
            raiseorlower(focuswin);
            break;
                    
        case KEY_X: /* x */
            maximize(focuswin);
            break;

        case KEY_1:
            changeworkspace(0);
            break;
            
        case KEY_2:
            changeworkspace(1);            
            break;

        case KEY_3:
            changeworkspace(2);            
            break;

        case KEY_4:
            changeworkspace(3);            
            break;

        case KEY_5:
            changeworkspace(4);            
            break;

        case KEY_6:
            changeworkspace(5);            
            break;

        case KEY_7:
            changeworkspace(6);            
            break;

        case KEY_8:
            changeworkspace(7);            
            break;

        case KEY_9:
            changeworkspace(8);            
            break;

        case KEY_0:
            changeworkspace(9);            
            break;

        case KEY_Y:
            topleft();
            break;

        case KEY_U:
            topright();
            break;

        case KEY_B:
            botleft();
            break;

        case KEY_N:
            botright();
            break;

        case KEY_END:
            deletewin();
            break;

        case KEY_PREVSCR:
            prevscreen();
            break;

        case KEY_NEXTSCR:
            nextscreen();            
            break;

        case KEY_ICONIFY:
            if (conf_get_allowicons())
            {
                hide(focuswin);
            }
            break;

	case KEY_PREVWS:
	    if (workspace_get_currentws() > 0)
	    {
		changeworkspace(workspace_get_currentws() - 1);
	    }
	    else
	    {
		changeworkspace(WORKSPACES - 1);
	    }
	    break;

	case KEY_NEXTWS:
	    changeworkspace((workspace_get_currentws() + 1) % WORKSPACES);
	    break;

        default:
            /* Ignore other keys. */
            break;            
        } /* switch unshifted */
    }
} /* handle_keypress() */

/* Helper function to configure a window. */
void configwin(xcb_window_t win, uint16_t mask, struct winconf wc)
{
    uint32_t values[7];
    int i = -1;
    
    if (mask & XCB_CONFIG_WINDOW_X)
    {
        mask |= XCB_CONFIG_WINDOW_X;
        i ++;
        values[i] = wc.x;
    }

    if (mask & XCB_CONFIG_WINDOW_Y)
    {
        mask |= XCB_CONFIG_WINDOW_Y;
        i ++;
        values[i] = wc.y;
    }

    if (mask & XCB_CONFIG_WINDOW_WIDTH)
    {
        mask |= XCB_CONFIG_WINDOW_WIDTH;
        i ++;
        values[i] = wc.width;
    }

    if (mask & XCB_CONFIG_WINDOW_HEIGHT)
    {
        mask |= XCB_CONFIG_WINDOW_HEIGHT;
        i ++;
        values[i] = wc.height;
    }

    if (mask & XCB_CONFIG_WINDOW_SIBLING)
    {
        mask |= XCB_CONFIG_WINDOW_SIBLING;
        i ++;
        values[i] = wc.sibling;     
    }

    if (mask & XCB_CONFIG_WINDOW_STACK_MODE)
    {
        mask |= XCB_CONFIG_WINDOW_STACK_MODE;
        i ++;
        values[i] = wc.stackmode;
    }

    if (-1 != i)
    {
        xcb_configure_window(conn, win, mask, values);
        xcb_flush(conn);
    }        
}

void configurerequest(xcb_configure_request_event_t *e)
{
    struct client *client;
    struct winconf wc;
    uint16_t mon_x;
    uint16_t mon_y;
    uint16_t mon_width;
    uint16_t mon_height;

    PDEBUG("event: Configure request. mask = %d\n", e->value_mask);

    /* Find the client. */
    if ((client = findclient(e->window)))
    {
        /* Find monitor position and size. */
        if (NULL == client || NULL == client->monitor)
        {
            mon_x = 0;
            mon_y = 0;
            mon_width = screen->width_in_pixels;
            mon_height = screen->height_in_pixels;
        }
        else
        {
            mon_x = client->monitor->x;
            mon_y = client->monitor->y;
            mon_width = client->monitor->width;
            mon_height = client->monitor->height;
        }

#if 0
        /*
         * We ignore moves the user haven't initiated, that is do
         * nothing on XCB_CONFIG_WINDOW_X and XCB_CONFIG_WINDOW_Y
         * ConfigureRequests.
         *
         * Code here if we ever change our minds or if you, dear user,
         * wants this functionality.
         */
        
        if (e->value_mask & XCB_CONFIG_WINDOW_X)
        {
            /* Don't move window if maximized. Don't move off the screen. */
            if (!client->maxed && e->x > 0)
            {
                client->x = e->x;
            }
        }

        if (e->value_mask & XCB_CONFIG_WINDOW_Y)
        {
            /*
             * Don't move window if maximized. Don't move off the
             * screen.
             */
            if (!client->maxed && !client->vertmaxed && e->y > 0)
            {
                client->y = e->y;
            }
        }
#endif
        
        if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)
        {
            /* Don't resize if maximized. */
            if (!client->maxed)
            {
                client->width = e->width;
            }
        }

        if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
        {
            /* Don't resize if maximized. */            
            if (!client->maxed && !client->vertmaxed)
            {
                client->height = e->height;
            }
        }

        /*
         * XXX Do we really need to pass on sibling and stack mode
         * configuration? Do we want to?
         */
        if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING)
        {
            uint32_t values[1];

            values[0] = e->sibling;
            xcb_configure_window(conn, e->window,
                                 XCB_CONFIG_WINDOW_SIBLING,
                                 values);
            xcb_flush(conn);
            
        }

        if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)
        {
            uint32_t values[1];
            
            values[0] = e->stack_mode;
            xcb_configure_window(conn, e->window,
                                 XCB_CONFIG_WINDOW_STACK_MODE,
                                 values);
            xcb_flush(conn);
        }

        /* Check if window fits on screen after resizing. */

        if (client->x + client->width + 2 * conf_get_borderwidth()
            > mon_x + mon_width)
        {
            /*
             * See if it fits if we move away the window from the
             * right edge of the screen.
             */
            client->x = mon_x + mon_width
                - (client->width + 2 * conf_get_borderwidth());

            /*
             * If we moved over the left screen edge, move back and
             * fit exactly on screen.
             */
            if (client->x < mon_x)
            {
                client->x = mon_x;
                client->width = mon_width - 2 * conf_get_borderwidth();
            }            
        }
        
        if (client->y + client->height + 2 * conf_get_borderwidth()
            > mon_y + mon_height)
        {
            /*
             * See if it fits if we move away the window from the
             * bottom edge.
             */
            client->y = mon_y + mon_height
                - (client->height + 2 * conf_get_borderwidth());

            /*
             * If we moved over the top screen edge, move back and fit
             * on screen.
             */
            if (client->y < mon_y)
            {
                PDEBUG("over the edge: y < %d\n", mon_y);
                client->y = mon_y;
                client->height = mon_height - 2 * conf_get_borderwidth();
            }
        }
        
        moveresize(client->id, client->x, client->y, client->width,
                   client->height);
    }
    else
    {
        PDEBUG("We don't know about this window yet.\n");

        /*
         * Unmapped window. Just pass all options except border
         * width.
         */
        wc.x = e->x;
        wc.y = e->y;
        wc.width = e->width;
        wc.height = e->height;
        wc.sibling = e->sibling;
        wc.stackmode = e->stack_mode;

        configwin(e->window, e->value_mask, wc);
    }
}

void events(void)
{
		
    xcb_generic_event_t *ev;
    
    int16_t mode_x = 0;             /* X coord when in special mode */
    int16_t mode_y = 0;             /* Y coord when in special mode */
    int fd;                         /* Our X file descriptor */
    fd_set in;                      /* For select */
    int found;                      /* Ditto. */

    /* Get the file descriptor so we can do select() on it. */
    fd = xcb_get_file_descriptor(conn);

    while(1)
    {
        /* Prepare for select(). */
        FD_ZERO(&in);
        FD_SET(fd, &in);

        /*
         * Check for events, again and again. When poll returns NULL
         * (and it does that a lot), we block on select() until the
         * event file descriptor gets readable again.
         *
         * We do it this way instead of xcb_wait_for_event() since
         * select() will return if we were interrupted by a signal. We
         * like that.
         */
        ev = xcb_poll_for_event(conn);
        if (NULL == ev)
        {
            PDEBUG("xcb_poll_for_event() returned NULL.\n");

            /*
             * Check if we have an unrecoverable connection error,
             * like a disconnected X server.
             */
            if (xcb_connection_has_error(conn))
            {
                cleanup(0);
                exit(1);
            }

            found = select(fd + 1, &in, NULL, NULL, NULL);
            if (-1 == found)
            {
                if (EINTR == errno)
                {
                    /* We received a signal. Break out of loop. */
                    break;
                }
                else
                {
                    /* Something was seriously wrong with select(). */
                    fprintf(stderr, "mcwm: select failed.");
                    cleanup(0);
                    exit(1);
                }
            }
            else
            {
                /* We found more events. Goto start of loop. */
                continue;
            }
        }

#ifdef DEBUG
        if (ev->response_type <= MAXEVENTS)
        {
            PDEBUG("Event: %s\n", evnames[ev->response_type]);
        }
        else
        {
            PDEBUG("Event: #%d. Not known.\n", ev->response_type);
        }
#endif

        /* Note that we ignore XCB_RANDR_NOTIFY. */
        if (ev->response_type 
            == randrbase + XCB_RANDR_SCREEN_CHANGE_NOTIFY)
        {
            PDEBUG("RANDR screen change notify. Checking outputs.\n");
            getrandr();
            free(ev);
            continue;
        }
            
        switch (ev->response_type & ~0x80)
        {
        case XCB_MAP_REQUEST:
        {
            xcb_map_request_event_t *e;

            PDEBUG("event: Map request.\n");
            e = (xcb_map_request_event_t *) ev;
            newwin(e->window);
        }
        break;
        
        case XCB_DESTROY_NOTIFY:
        {
            xcb_destroy_notify_event_t *e;

            e = (xcb_destroy_notify_event_t *) ev;

            /*
             * If we had focus or our last focus in this window,
             * forget about the focus.
             *
             * We will get an EnterNotify if there's another window
             * under the pointer so we can set the focus proper later.
             */
            if (NULL != focuswin)
            {
                if (focuswin->id == e->window)
                {
                    focuswin = NULL;
                }
            }
            if (NULL != lastfocuswin)
            {
                if (lastfocuswin->id == e->window)
                {
                    lastfocuswin = NULL;
                }
            }
            
            /*
             * Find this window in list of clients and forget about
             * it.
             */
            forgetwin(e->window);
        }
        break;
            
        case XCB_BUTTON_PRESS:
        {
            xcb_button_press_event_t *e;

            e = (xcb_button_press_event_t *) ev;
            PDEBUG("Button %d pressed in window %ld, subwindow %d "
                    "coordinates (%d,%d)\n",
                   e->detail, (long)e->event, e->child, e->event_x,
                   e->event_y);

            if (0 == e->child)
            {
                /* Mouse click on root window. Start programs? */

                switch (e->detail)
                {
                case 1: /* Mouse button one. */
                    start(MOUSE1);
                    break;

                case 2: /* Middle mouse button. */
                    start(MOUSE2);                    
                    break;

                case 3: /* Mouse button three. */
                    start(MOUSE3);
                    break;

                default:
                    break;
                } /* switch */

                /* Break out of event switch. */
                break;
            }
            
            /*
             * If we don't have any currently focused window, we can't
             * do anything. We don't want to do anything if the mouse
             * cursor is in the wrong window (root window or a panel,
             * for instance). There is a limit to sloppy focus.
             */
            if (NULL == focuswin || focuswin->id != e->child)
            {
                break;
            }

            /*
             * If middle button was pressed, raise window or lower
             * it if it was already on top.
             */
            if (2 == e->detail)
            {
                raiseorlower(focuswin);
            }
            else
            {
                uint16_t pointx;
                uint16_t pointy;

                /* We're moving or resizing. */

                /*
                 * Get and save pointer position inside the window
                 * so we can go back to it when we're done moving
                 * or resizing.
                 */
                if (!getpointer(focuswin->id, &pointx, &pointy))
                {
                    break;
                }

                mode_x = pointx;
                mode_y = pointy;

                /* Raise window. */
                raisewindow(focuswin->id);
                
                /* Mouse button 1 was pressed. */
                if (1 == e->detail)
                {
                    mode = MCWM_MOVE;

                    /*
                     * Warp pointer to upper left of window before
                     * starting move.
                     */
                    xcb_warp_pointer(conn, XCB_NONE, focuswin->id, 0, 0, 0, 0,
                                     1, 1);                        
                }
                else
                {
                    /* Mouse button 3 was pressed. */

                    mode = MCWM_RESIZE;

                    /* Warp pointer to lower right. */
                    xcb_warp_pointer(conn, XCB_NONE, focuswin->id, 0, 0, 0,
                                     0, focuswin->width, focuswin->height);
                }

                /*
                 * Take control of the pointer in the root window
                 * and confine it to root.
                 *
                 * Give us events when the key is released or if
                 * any motion occurs with the key held down.
                 *
                 * Keep updating everything else.
                 *
                 * Don't use any new cursor.
                 */
                xcb_grab_pointer(conn, 0, screen->root,
                                 XCB_EVENT_MASK_BUTTON_RELEASE
                                 | XCB_EVENT_MASK_BUTTON_MOTION
                                 | XCB_EVENT_MASK_POINTER_MOTION_HINT,
                                 XCB_GRAB_MODE_ASYNC,
                                 XCB_GRAB_MODE_ASYNC,
                                 screen->root,
                                 XCB_NONE,
                                 XCB_CURRENT_TIME);

                xcb_flush(conn);

                PDEBUG("mode now : %d\n", mode);
            }
        }
        break;

        case XCB_MOTION_NOTIFY:
        {
            xcb_query_pointer_reply_t *pointer;

            /*
             * We can't do anything if we don't have a focused window
             * or if it's fully maximized.
             */
            if (NULL == focuswin || focuswin->maxed)
            {
                break;
            }

            /*
             * This is not really a real notify, but just a hint that
             * the mouse pointer moved. This means we need to get the
             * current pointer position ourselves.
             */
            pointer = xcb_query_pointer_reply(
                conn, xcb_query_pointer(conn, screen->root), 0);

            if (NULL == pointer)
            {
                PDEBUG("Couldn't get pointer position.\n");
                break;
            }
            
            /*
             * Our pointer is moving and since we even get this event
             * we're either resizing or moving a window.
             */
            if (mode == MCWM_MOVE)
            {
                mousemove(focuswin, pointer->root_x, pointer->root_y);
            }
            else if (mode == MCWM_RESIZE)
            {
                mouseresize(focuswin, pointer->root_x, pointer->root_y);
            }
            else
            {
                PDEBUG("Motion event when we're not moving our resizing!\n");
            }

            free(pointer);            
        }
        
        break;

        case XCB_BUTTON_RELEASE:
            PDEBUG("Mouse button released! mode = %d\n", mode);

            if (0 == mode)
            {
                /*
                 * Mouse button released, but not in a saved mode. Do
                 * nothing.
                 */
                break;
            }
            else
            {
                int16_t x;
                int16_t y;
                
                /* We're finished moving or resizing. */

                if (NULL == focuswin)
                {
                    /*
                     * We don't seem to have a focused window! Just
                     * ungrab and reset the mode.
                     */
                    PDEBUG("No focused window when finished moving or "
                           "resizing!");
                
                    xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
                    xcb_flush(conn); /* Important! */
                    
                    mode = 0;
                    break;
                }
                
                /*
                 * We will get an EnterNotify and focus another window
                 * if the pointer just happens to be on top of another
                 * window when we ungrab the pointer, so we have to
                 * warp the pointer before to prevent this.
                 *
                 * Move to saved position within window or if that
                 * position is now outside current window, move inside
                 * window.
                 */
                if (mode_x > focuswin->width)
                {
                    x = focuswin->width / 2;
                    if (0 == x)
                    {
                        x = 1;
                    }
                    
                }
                else
                {
                    x = mode_x;
                }

                if (mode_y > focuswin->height)
                {
                    y = focuswin->height / 2;
                    if (0 == y)
                    {
                        y = 1;
                    }
                }
                else
                {
                    y = mode_y;
                }

                xcb_warp_pointer(conn, XCB_NONE, focuswin->id, 0, 0, 0, 0,
                                 x, y);
                xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
                xcb_flush(conn); /* Important! */
                
                mode = 0;
                PDEBUG("mode now = %d\n", mode);
            }
        break;
                
        case XCB_KEY_PRESS:
        {
            xcb_key_press_event_t *e = (xcb_key_press_event_t *)ev;
            
            PDEBUG("Key %d pressed\n", e->detail);

            handle_keypress(e);
        }
        break;

        case XCB_KEY_RELEASE:
        {
            xcb_key_release_event_t *e = (xcb_key_release_event_t *)ev;
            unsigned i;
            
            PDEBUG("Key %d released.\n", e->detail);

            if (MCWM_TABBING == mode)
            {
                /*
                 * Check if it's the that was released was a key
                 * generating the MODKEY mask.
                 */
                for (i = 0; i < keyboard_modkey_get_len(); i ++)
                {
                    PDEBUG("Is it %d?\n", keyboard_modkey_get_keycodes(i));
                    
                    if (e->detail == keyboard_modkey_get_keycodes(i))
                    {
                        finishtabbing();

                        /* Get out of for... */
                        break;
                    }
                } /* for keycodes. */
            } /* if tabbing. */
        }
        break;
            
        case XCB_ENTER_NOTIFY:
        {
            xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *)ev;
            struct client *client;
            
            PDEBUG("event: Enter notify eventwin %d, child %d, detail %d.\n",
                   e->event,
                   e->child,
                   e->detail);

            /*
             * If this isn't a normal enter notify, don't bother.
             *
             * We also need ungrab events, since these will be
             * generated on button and key grabs and if the user for
             * some reason presses a button on the root and then moves
             * the pointer to our window and releases the button, we
             * get an Ungrab EnterNotify.
             * 
             * The other cases means the pointer is grabbed and that
             * either means someone is using it for menu selections or
             * that we're moving or resizing. We don't want to change
             * focus in those cases.
             */
            if (e->mode == XCB_NOTIFY_MODE_NORMAL
                || e->mode == XCB_NOTIFY_MODE_UNGRAB)
            {
                /*
                 * If we're entering the same window we focus now,
                 * then don't bother focusing.
                 */
                if (NULL == focuswin || e->event != focuswin->id)
                {
                    /*
                     * Otherwise, set focus to the window we just
                     * entered if we can find it among the windows we
                     * know about. If not, just keep focus in the old
                     * window.
                     */
                    client = findclient(e->event);
                    if (NULL != client)
                    {
                        if (MCWM_TABBING != mode)
                        {
                            /*
                             * We are focusing on a new window. Since
                             * we're not currently tabbing around the
                             * window ring, we need to update the
                             * current workspace window list: Move
                             * first the old focus to the head of the
                             * list and then the new focus to the head
                             * of the list.
                             */
                            if (NULL != focuswin)
                            {
                                movetohead(workspace_get_wslist_current(),
                                           focuswin->wsitem[workspace_get_currentws()]);
                                lastfocuswin = NULL;                 
                            }

                            movetohead(workspace_get_wslist_current(), client->wsitem[workspace_get_currentws()]);
                        } /* if not tabbing */

                        setfocus(client);
                    }
                }
            }

        }
        break;        
        
        case XCB_CONFIGURE_NOTIFY:
        {
            xcb_configure_notify_event_t *e
                = (xcb_configure_notify_event_t *)ev;
            
            if (e->window == screen->root)
            {
                /*
                 * When using RANDR or Xinerama, the root can change
                 * geometry when the user adds a new screen, tilts
                 * their screen 90 degrees or whatnot. We might need
                 * to rearrange windows to be visible.
                 *
                 * We might get notified for several reasons, not just
                 * if the geometry changed. If the geometry is
                 * unchanged we do nothing.
                 */
                PDEBUG("Notify event for root!\n");
                PDEBUG("Possibly a new root geometry: %dx%d\n",
                       e->width, e->height);

                if (e->width == screen->width_in_pixels
                    && e->height == screen->height_in_pixels)
                {
                    /* Root geometry is really unchanged. Do nothing. */
                    PDEBUG("Hey! Geometry didn't change.\n");
                }
                else
                {
                    screen->width_in_pixels = e->width;
                    screen->height_in_pixels = e->height;

                    /* Check for RANDR. */
                    if (-1 == randrbase)
                    {
                        /* We have no RANDR so we rearrange windows to
                         * the new root geometry here.
                         *
                         * With RANDR enabled, we handle this per
                         * screen getrandr() when we receive an
                         * XCB_RANDR_SCREEN_CHANGE_NOTIFY event.
                         */
                        arrangewindows();
                    }
                }
            }
        }
        break;
        
        case XCB_CONFIGURE_REQUEST:
            configurerequest((xcb_configure_request_event_t *) ev);
        break;

        case XCB_CLIENT_MESSAGE:
        {
            xcb_client_message_event_t *e
                = (xcb_client_message_event_t *)ev;

            if (conf_get_allowicons())
            {
                if (e->type == wm_change_state
                    && e->format == 32
                    && e->data.data32[0] == XCB_ICCCM_WM_STATE_ICONIC)
                {
                    long data[] = { XCB_ICCCM_WM_STATE_ICONIC, XCB_NONE };

                    /* Unmap window and declare iconic. */
                    
                    xcb_unmap_window(conn, e->window);
                    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, e->window,
                                        wm_state, wm_state, 32, 2, data);
                    xcb_flush(conn);
                }
            } /* if */
        }
        break;

        case XCB_CIRCULATE_REQUEST:
        {
            xcb_circulate_request_event_t *e
                = (xcb_circulate_request_event_t *)ev;

            /*
             * Subwindow e->window to parent e->event is about to be
             * restacked.
             *
             * Just do what was requested, e->place is either
             * XCB_PLACE_ON_TOP or _ON_BOTTOM. We don't care.
             */
            xcb_circulate_window(conn, e->window, e->place);
        }
        break;

        case XCB_MAPPING_NOTIFY:
        {
            xcb_mapping_notify_event_t *e
                = (xcb_mapping_notify_event_t *)ev;

            /*
             * XXX Gah! We get a new notify message for *every* key!
             * We want to know when the entire keyboard is finished.
             * Impossible? Better handling somehow?
             */

            /*
             * We're only interested in keys and modifiers, not
             * pointer mappings, for instance.
             */
            if (e->request != XCB_MAPPING_MODIFIER
                && e->request != XCB_MAPPING_KEYBOARD)
            {
                break;
            }
            
            /* Forget old key bindings. */
            xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

            /* Use the new ones. */
            setupkeys();
        }
        break;
        
        case XCB_UNMAP_NOTIFY:
        {
            xcb_unmap_notify_event_t *e =
                (xcb_unmap_notify_event_t *)ev;
            struct item *item;
            struct client *client;

            /*
             * Find the window in our *current* workspace list, then
             * forget about it. If it gets mapped, we add it to our
             * lists again then.
             *
             * Note that we might not know about the window we got the
             * UnmapNotify event for. It might be a window we just
             * unmapped on *another* workspace when changing
             * workspaces, for instance, or it might be a window with
             * override redirect set. This is not an error.
             *
             * XXX We might need to look in the global window list,
             * after all. Consider if a window is unmapped on our last
             * workspace while changing workspaces... If we do this,
             * we need to keep track of our own windows and ignore
             * UnmapNotify on them.
             */
            for (item = workspace_get_firstitem(workspace_get_currentws()); item != NULL; item = item->next)
            {
                client = item->data;
                
                if (client->id == e->window)
                {
                    PDEBUG("Forgetting about %d\n", e->window);
                    if (focuswin == client)
                    {
                        focuswin = NULL;
                    }

                    forgetclient(client);
                    /* We're finished. Break out of for loop. */
                    break; 
                }
            } /* for */
        } 
        break;
            
        } /* switch */

        /* Forget about this event. */
        free(ev);
    }
}




