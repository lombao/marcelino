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
 
/* CORE includes */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

/* XCB Sutff */
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>

/* Internal Configuration */
#include "conf.h"

/* Randr */
#include "mrandr.h"

/* Server */
#include "server.h"

/* Windows */
#include "windows.h"

/* Workspace */
#include "workspace.h"

/* Keyboard handling */
#include "keyboard.h"


/***********************************************************/
/*         MAIN                                          ***/
/***********************************************************/
int main(int argc, char **argv)
{
 
    /* We don't want SIGCHILD at all, ignore it */
    signal(SIGCHLD, SIG_IGN);
    
    /* Upload configuration */
    /* Note that we might have pass the config file as a parameter ( -c  ) */
    conf_upload_default_cfg_global_file();
    conf_determine_user_configfile(argc, argv);
    
    /* Initializations */
    workspace_init();
    server_init();
    randr_init(server_get_conn(),server_get_screen());
    
    /* Force update on the X Server */
    server_flush();
    
    /* Loop over all clients (windows already there) and set up stuff. */
    if (0 != server_setupscreen())
    {
        fprintf(stderr, "marcelino: Failed to initialize windows. Exiting.\n");
        server_disconnect();
        exit(1);
    }

    /* Set up key bindings. */
    if (0 != keyboard_init(server_get_conn()))
    {
        fprintf(stderr, "marcelino: Couldn't set up keycodes. Exiting.");
        server_disconnect();
        exit(1);
    }

    /* Grab mouse buttons. */
    server_mouse_init(server_get_conn(),server_get_screen());
    
    /* Register marcelino to listen to events */
    server_subscribe_to_events(server_get_conn(),server_get_screen());
    
    /* Loop over events. */
    events(server_get_conn(),server_get_screen());
    
    /* Exit in a orderly fashion */
    server_exit_cleanup(0);

}
