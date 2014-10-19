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

#include "list.h"
#include "windows.h"

/* Number of workspaces. */
#define WORKSPACES 10

#ifdef DEBUG
#define PDEBUG(Args...) \
  do { fprintf(stderr, "mcwm: "); fprintf(stderr, ##Args); } while(0)
#define D(x) x
#else
#define PDEBUG(Args...)
#define D(x)
#endif

extern xcb_connection_t *conn;         /* Connection to X server. */
extern xcb_screen_t     *screen;       /* Our current screen.  */
extern uint32_t curws;             /* Current workspace. */
extern struct client *focuswin;        /* Current focus window. */

/*
 * Workspace list: Every workspace has a list of all visible
 * windows.
 */
static struct item *wslist[WORKSPACES] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};



/* Return a pointer to the address where the list of items start, in short the 
 * the list of items of the given workspace */
struct item * * workspace_get(int b) {
 return &wslist[b];	
}

void workspace_set(int b,struct item * i) {
  wslist[b] = i;	
}

/* Returns the first item of a given workspace */
struct item * workspace_get_firstitem(int b) {
  return wslist[b];
}

/* returns true if there are not windows in that workspace */
bool workspace_isempty(int b) {
	return (wslist[b] == NULL);
}


/* Add a window, specified by client, to workspace ws. */
void addtoworkspace(struct client *client, uint32_t ws)
{
    struct item *item;
    
    item = additem(&wslist[ws]);
    if (NULL == item)
    {
        PDEBUG("addtoworkspace: Out of memory.\n");
        return;
    }

    /* Remember our place in the workspace window list. */
    client->wsitem[ws] = item;

    /* Remember the data. */
    item->data = client;

    /*
     * Set window hint property so we can survive a crash.
     * 
     * Fixed windows have their own special WM hint. We don't want to
     * mess with that.
     */
    if (!client->fixed)
    {
        setwmdesktop(client->id, ws);
    }
}

/* Delete window client from workspace ws. */
void delfromworkspace(struct client *client, uint32_t ws)
{
    delitem(&wslist[ws], client->wsitem[ws]);

    /* Reset our place in the workspace window list. */
    client->wsitem[ws] = NULL;
}

/* Change current workspace to ws. */
void changeworkspace(uint32_t ws)
{
    struct item *item;
    struct client *client;
    
    if (ws == curws)
    {
        PDEBUG("Changing to same workspace!\n");
        return;
    }

    PDEBUG("Changing from workspace #%d to #%d\n", curws, ws);

    /*
     * We lose our focus if the window we focus isn't fixed. An
     * EnterNotify event will set focus later.
     */
    if (NULL != focuswin && !focuswin->fixed)
    {
        setunfocus(focuswin->id);
        focuswin = NULL;
    }
    
    /* Go through list of current ws. Unmap everything that isn't fixed. */
    for (item = wslist[curws]; item != NULL; item = item->next)
    {
        client = item->data;

        PDEBUG("changeworkspace. unmap phase. ws #%d, client-fixed: %d\n",
               curws, client->fixed);

        if (!client->fixed)
        {
            /*
             * This is an ordinary window. Just unmap it. Note that
             * this will generate an unnecessary UnmapNotify event
             * which we will try to handle later.
             */
            xcb_unmap_window(conn, client->id);
        }
    } /* for */
    
    /* Go through list of new ws. Map everything that isn't fixed. */
    for (item = wslist[ws]; item != NULL; item = item->next)
    {
        client = item->data;

        PDEBUG("changeworkspace. map phase. ws #%d, client-fixed: %d\n",
               ws, client->fixed);

        /* Fixed windows are already mapped. Map everything else. */
        if (!client->fixed)
        {
            xcb_map_window(conn, client->id);
        }
    }

    xcb_flush(conn);

    curws = ws;
}


