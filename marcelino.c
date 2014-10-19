/*
 * MARCELINO: a small window manager using XCB
 * 
 *
 * Copyright (c) 2014 Cesar Lombao
 *
 * Copyright (c) 2010, 2011, 2012 Michael Cardell Widerkrantz, mc at
 * the domain hack.org.
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
 
/* CORE includes */
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

/* XCB Sutff */
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>

/* X11 Keyboard handling */
#include <X11/keysym.h>


/* Internal Configuration */
#include "conf.h"

/* Custom configuration */
#include "config.h"

/* Randr */
#include "mrandr.h"

/* Windows */
#include "windows.h"

/* Workspace */
#include "workspace.h"

/* Keyboard handling */
#include "keyboard.h"

/*************************************************/
/* GLOBAL VARIABLES                              */
xcb_connection_t *conn;         /* Connection to X server. */
xcb_screen_t     *screen;       /* Our current screen.  */
xcb_atom_t atom_desktop;
xcb_atom_t wm_delete_window;    /* WM_DELETE_WINDOW event to close windows.  */
xcb_atom_t wm_change_state;
xcb_atom_t wm_state;
xcb_atom_t wm_protocols;        /* WM_PROTOCOLS.  */
extern int randrbase;                  /* Beginning of RANDR extension events. */

extern struct client *focuswin;        /* Current focus window. */

int sigcode;                    /* Signal code. Non-zero if we've been
                                 * interruped by a signal. */

/***********************/
/* Function prototypes */    
static void printhelp(void);
static void sigcatch(int sig);    
static uint32_t getcolor(const char *colstr);
static xcb_atom_t getatom(char *atom_name);
static int setupscreen(void);
    
                                 
/****************************/
/* Print Help               */
void printhelp(void)
{
    printf("marcelino: Usage: marcelino [-b] [-t terminal-program] [-f colour] "
           "[-u colour] [-x colour] \n");
    printf("  -b means draw no borders\n");
    printf("  -t urxvt will start urxvt when MODKEY + Return is pressed\n");
    printf("  -f colour sets colour for focused window borders of focused "
           "to a named color.\n");
    printf("  -u colour sets colour for unfocused window borders.");
    printf("  -x color sets colour for fixed window borders.");    
}

/****************************/
/* Signal management        */
void sigcatch(int sig)
{
    sigcode = sig;
}



/******************************************************/
/* Walk through all existing windows and set them up. */
/* Returns 0 on success.  */
int setupscreen(void)
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
                if (-1 != randrbase)
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


/* Let's start */
int main(int argc, char **argv)
{
    uint32_t mask = 0;
    uint32_t values[2];
    int ch;                    /* Option character */
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *error;
    xcb_drawable_t root;
    char *focuscol;
    char *unfocuscol;
    char *fixedcol;    
    int scrno;
    xcb_screen_iterator_t iter;
    
    /* Install signal handlers. */
    /* We ignore child exists. Don't create zombies. */
    if (SIG_ERR == signal(SIGCHLD, SIG_IGN))
    {
        perror("marcelino: signal");
        exit(1);
    }

    if (SIG_ERR == signal(SIGINT, sigcatch))
    {
        perror("marcelino: signal");
        exit(1);
    }

    if (SIG_ERR == signal(SIGTERM, sigcatch))
    {
        perror("marcelino: signal");
        exit(1);
    }
    
    /* Set up defaults. */
    
    conf_set_borderwidth(BORDERWIDTH);
    conf_set_terminal(TERMINAL);
    conf_set_allowicons(ALLOWICONS);
    focuscol = FOCUSCOL;
    unfocuscol = UNFOCUSCOL;
    fixedcol = FIXEDCOL;
    
    while (1)
    {
        ch = getopt(argc, argv, "b:it:f:u:x:");
        if (-1 == ch)
        {
                
            /* No more options, break out of while loop. */
            break;
        }
        
        switch (ch)
        {
        case 'b':
            /* Border width */
            conf_set_borderwidth(atoi(optarg));            
            break;

        case 'i':
            conf_set_allowicons(true);
            break;
            
        case 't':
            conf_set_terminal(optarg);
            break;

        case 'f':
            focuscol = optarg;
            break;

        case 'u':
            unfocuscol = optarg;
            break;

        case 'x':
            fixedcol = optarg;
            break;
            
        default:
            printhelp();
            exit(0);
        } /* switch */
    }

    /*
     * Use $DISPLAY. After connecting scrno will contain the value of
     * the display's screen.
     */
    conn = xcb_connect(NULL, &scrno);
    if (xcb_connection_has_error(conn))
    {
        perror("xcb_connect");
        exit(1);
    }

    /* Find our screen. */
    iter = xcb_setup_roots_iterator(xcb_get_setup(conn));
    int i;
    for (i = 0; i < scrno; ++ i)
    {
        xcb_screen_next(&iter);
    }

    screen = iter.data;
    if (!screen)
    {
        fprintf (stderr, "marcelino: Can't get the current screen. Exiting.\n");
        xcb_disconnect(conn);
        exit(1);
    }

    root = screen->root;
    
    PDEBUG("Screen size: %dx%d\nRoot window: %d\n", screen->width_in_pixels,
           screen->height_in_pixels, screen->root);
    
    /* Get some colours. */
    conf_set_focuscol(getcolor(focuscol));
    conf_set_unfocuscol(getcolor(unfocuscol));
    conf_set_fixedcol(getcolor(fixedcol));
    
    /* Get some atoms. */
    atom_desktop = getatom("_NET_WM_DESKTOP");
    wm_delete_window = getatom("WM_DELETE_WINDOW");
    wm_change_state = getatom("WM_CHANGE_STATE");
    wm_state = getatom("WM_STATE");
    wm_protocols = getatom("WM_PROTOCOLS");
    
    /* Check for RANDR extension and configure. */
    randrbase = setuprandr();

    /* Loop over all clients and set up stuff. */
    if (0 != setupscreen())
    {
        fprintf(stderr, "marcelino: Failed to initialize windows. Exiting.\n");
        xcb_disconnect(conn);
        exit(1);
    }

    /* Set up key bindings. */
    if (0 != setupkeys())
    {
        fprintf(stderr, "marcelino: Couldn't set up keycodes. Exiting.");
        xcb_disconnect(conn);
        exit(1);
    }

    /* Grab mouse buttons. */

    xcb_grab_button(conn, 0, root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
                    1 /* left mouse button */,
                    MOUSEMODKEY);
    
    xcb_grab_button(conn, 0, root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
                    2 /* middle mouse button */,
                    MOUSEMODKEY);

    xcb_grab_button(conn, 0, root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
                    3 /* right mouse button */,
                    MOUSEMODKEY);

    /* Subscribe to events. */
    mask = XCB_CW_EVENT_MASK;

    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    cookie =
        xcb_change_window_attributes_checked(conn, root, mask, values);
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

    /* Loop over events. */
    events();

    /* Die gracefully. */
    cleanup(sigcode);

    exit(0);
}
