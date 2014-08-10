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
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>


/* Function declaration */
void mr_events_init_array_callbacks();
void mr_events_execute_callback (xcb_generic_event_t *ev);
void mr_deal_with_button_press (xcb_generic_event_t *ev);
void mr_deal_with_map_request (xcb_generic_event_t *ev);
void mr_deal_with_motion_notify (xcb_generic_event_t *ev);
void mr_deal_with_button_release (xcb_generic_event_t *ev);
void mr_deal_with_key_press (xcb_generic_event_t *ev); 
void mr_deal_with_key_release (xcb_generic_event_t *ev); 
void mr_deal_with_enter_notify (xcb_generic_event_t *ev); 
void mr_deal_with_leave_notify (xcb_generic_event_t *ev); 
void mr_deal_with_expose (xcb_generic_event_t *ev); 
void mr_deal_with_destroy_notify (xcb_generic_event_t *ev); 
void mr_deal_with_configure_request (xcb_generic_event_t *ev); 
void mr_deal_with_configure_notify (xcb_generic_event_t *ev); 
void mr_deal_with_mapping_notify (xcb_generic_event_t *ev); 
void mr_deal_with_circulate_request (xcb_generic_event_t *ev); 



/* the array of callbacks will be this big, 100 is an arbitrary 
 * number I've given it buecase at this stage I simply don't know */
#define HIGHER_XCB_EVENT_NUMBER 100


