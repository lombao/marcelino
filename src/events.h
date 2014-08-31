/*******************************************************/
/* MARCELINO. Yet another simplistic WM using XCB in C */
/* ----------------------------------------------------*/
/* EVENTS Module                                       */
/*   Here we have the functions that handle the        */
/*   different events the WM can receive               */
/* --------------------------------------------------- */
/* Author: Cesar Lombao                                */
/* Date:   July 2014                                   */
/* License: GPLv2                                      */
/*******************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>


/* Function declaration */

void mr_events_execute_callback (xcb_generic_event_t *ev);
void mr_deal_with_button_press (xcb_generic_event_t *ev);
void mr_deal_with_map_request (xcb_generic_event_t *ev);
void mr_deal_with_map_notify (xcb_generic_event_t *ev);
void mr_deal_with_mapping_notify (xcb_generic_event_t *ev);
void mr_deal_with_motion_notify (xcb_generic_event_t *ev);
void mr_deal_with_button_release (xcb_generic_event_t *ev);
void mr_deal_with_destroy_notify (xcb_generic_event_t *ev);
void mr_deal_with_configure_request (xcb_generic_event_t *ev);
void mr_deal_with_configure_notify (xcb_generic_event_t *ev);
void mr_deal_with_resize_request (xcb_generic_event_t *ev);
void mr_deal_with_create_notify (xcb_generic_event_t *ev);
void mr_deal_with_client_message (xcb_generic_event_t *ev);




/* Global definitions */
#define MODE_MOVE 2   /* We're currently moving a window with the mouse.   */
#define MODE_RESIZE 3 /* We're currently resizing a window with the mouse. */
