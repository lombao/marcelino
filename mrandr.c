

#define _GNU_SOURCE
/* CORE includes */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


/* XCB Sutff */
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>

#include "config.h"
#include "list.h"
#include "windows.h"
#include "mrandr.h"


int randrbase;                  /* Beginning of RANDR extension events. */


extern xcb_connection_t *conn;         /* Connection to X server. */
extern xcb_screen_t     *screen;       /* Our current screen.  */


extern struct item *winlist ;    /* Global list of all client windows. */
extern struct item *monlist ;    /* List of all physical monitor outputs. */




/***************************************************/
/* Set up RANDR extension                          */
int setuprandr(void)
{
    const xcb_query_extension_reply_t *extension;
    int base;
        
    extension = xcb_get_extension_data(conn, &xcb_randr_id);
    if (!extension->present)
    {
        PDEBUG("No RANDR extension.\n");
        return -1;
    }
    else
    {
        getrandr();
    }

    base = extension->first_event;
    PDEBUG("randrbase is %d.\n", base);
        
    xcb_randr_select_input(conn, screen->root,
                           XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE |
                           XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE |
                           XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE |
                           XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);

    xcb_flush(conn);

    return base;
}


/*
 * Get RANDR resources and figure out how many outputs there are.
 */ 
void getrandr(void)
{
    xcb_randr_get_screen_resources_current_cookie_t rcookie;
    xcb_randr_get_screen_resources_current_reply_t *res;
    xcb_randr_output_t *outputs;
    int len;    
    xcb_timestamp_t timestamp;
    
    rcookie = xcb_randr_get_screen_resources_current(conn, screen->root);
    res = xcb_randr_get_screen_resources_current_reply(conn, rcookie, NULL);
    if (NULL == res)
    {
        printf("No RANDR extension available.\n");
        return;
    }
    timestamp = res->config_timestamp;

    len = xcb_randr_get_screen_resources_current_outputs_length(res);
    outputs = xcb_randr_get_screen_resources_current_outputs(res);

    PDEBUG("Found %d outputs.\n", len);
    
    /* Request information for all outputs. */
    getoutputs(outputs, len, timestamp);

    free(res);
}



/*
 * Walk through all the RANDR outputs (number of outputs == len) there
 * was at time timestamp.
 */
void getoutputs(xcb_randr_output_t *outputs, int len, xcb_timestamp_t timestamp)
{
    char *name;
    xcb_randr_get_crtc_info_cookie_t icookie;
    xcb_randr_get_crtc_info_reply_t *crtc = NULL;
    xcb_randr_get_output_info_reply_t *output;
    struct monitor *mon;
    struct monitor *clonemon;
    xcb_randr_get_output_info_cookie_t ocookie[len];
    int i;
    
    for (i = 0; i < len; i++)
    {
        ocookie[i] = xcb_randr_get_output_info(conn, outputs[i], timestamp);
    }

    /* Loop through all outputs. */
    for (i = 0; i < len; i ++)
    {
        output = xcb_randr_get_output_info_reply(conn, ocookie[i], NULL);
        
        if (output == NULL)
        {
            continue;
        }

        asprintf(&name, "%.*s",
                 xcb_randr_get_output_info_name_length(output),
                 xcb_randr_get_output_info_name(output));

        PDEBUG("Name: %s\n", name);
        PDEBUG("id: %d\n" , outputs[i]);
        PDEBUG("Size: %d x %d mm.\n", output->mm_width, output->mm_height);

        if (XCB_NONE != output->crtc)
        {
            icookie = xcb_randr_get_crtc_info(conn, output->crtc, timestamp);
            crtc = xcb_randr_get_crtc_info_reply(conn, icookie, NULL);
            if (NULL == crtc)
            {
                return;
            }
            
            PDEBUG("CRTC: at %d, %d, size: %d x %d.\n", crtc->x, crtc->y,
                   crtc->width, crtc->height);

            /* Check if it's a clone. */
            clonemon = findclones(outputs[i], crtc->x, crtc->y);
            if (NULL != clonemon)
            {
                PDEBUG("Monitor %s, id %d is a clone of %s, id %d. Skipping.\n",
                       name, outputs[i],
                       clonemon->name, clonemon->id);
                continue;
            }

            /* Do we know this monitor already? */
            if (NULL == (mon = findmonitor(outputs[i])))
            {
                PDEBUG("Monitor not known, adding to list.\n");
                addmonitor(outputs[i], name, crtc->x, crtc->y, crtc->width,
                           crtc->height);
            }
            else
            {
                bool changed = false;
                
                /*
                 * We know this monitor. Update information. If it's
                 * smaller than before, rearrange windows.
                 */
                PDEBUG("Known monitor. Updating info.\n");

                if (crtc->x != mon->x)
                {
                    mon->x = crtc->x;
                    changed = true;
                }
                if (crtc->y != mon->y)
                {
                    mon->y = crtc->y;
                    changed = true;
                }

                if (crtc->width != mon->width)
                {
                    mon->width = crtc->width;                    
                    changed = true;
                }
                if (crtc->height != mon->height)
                {
                    mon->height = crtc->height;                    
                    changed = true;
                }
                
                if (changed)
                {
                    arrbymon(mon);
                }
            }

            free(crtc);
        }
        else
        {
            PDEBUG("Monitor not used at the moment.\n");
            /*
             * Check if it was used before. If it was, do something.
             */
            if ((mon = findmonitor(outputs[i])))
            {
                struct item *item;
                struct client *client;

                /* Check all windows on this monitor and move them to
                 * the next or to the first monitor if there is no
                 * next.
                 *
                 * FIXME: Use per monitor workspace list instead of
                 * global window list.
                 */
                for (item = winlist; item != NULL; item = item->next)
                {
                    client = item->data;
                    if (client->monitor == mon)
                    {
                        if (NULL == client->monitor->item->next)
                        {
                            if (NULL == monlist)
                            {
                                client->monitor = NULL;
                            }
                            else
                            {
                                client->monitor = monlist->data;
                            }
                        }
                        else
                        {
                            client->monitor =
                                client->monitor->item->next->data;
                        }

                        fitonscreen(client);
                    }
                } /* for */
                
                /* It's not active anymore. Forget about it. */
                delmonitor(mon);
            }
        }

        free(output);
    } /* for */
}
