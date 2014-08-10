/*******************************************************/
/* MARCELINO. Yet another simplistic WM using XCB in C */
/* ----------------------------------------------------*/
/* Author: Cesar Lombao
 * Date:   July 2014 
 * License: GPLv2
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "marcelino.h"
#include "events.h"

   t_wmstatus wmstatus;       /* The global status of the WM */

/* **************************************************** */
/* MAIN *********************************************** */
/* **************************************************** */
int main ()
 {

   

   xcb_drawable_t    root; 
   
    /* Connects to the X Server  */
    wmstatus.xconn = xcb_connect(NULL, NULL);
    switch (xcb_connection_has_error(wmstatus.xconn)) {
		 case 0: /* everything is fine so we don't do anything*/
		       break;
		       
		 case XCB_CONN_ERROR: 
		       fprintf(stderr,"ERROR: socket errors, pipe errors or other stream errors");
		       exit(1);
		       break;
		 case XCB_CONN_CLOSED_EXT_NOTSUPPORTED: 
		       fprintf(stderr,"ERROR: Extension not supported");
		       exit(1);
		       break;
		 case XCB_CONN_CLOSED_MEM_INSUFFICIENT: 
		       fprintf(stderr,"ERROR: Insufficient Memory");
		       exit(1);
		       break;
		 case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
		       fprintf(stderr,"ERROR: Exceeding request length that server accepts");
		       exit(1);
		       break;           
		 case XCB_CONN_CLOSED_PARSE_ERR: 
		       fprintf(stderr,"ERROR: During parsing display string");
		       exit(1);
		       break;    
		 case XCB_CONN_CLOSED_INVALID_SCREEN: 
		       fprintf(stderr,"ERROR:  Because the server does not have a screen matching the display");
		       exit(1);
		       break;               
         default:
               fprintf(stderr,"ERROR: Unknnown error result when trying to contact the X Server");
               exit(1);
               break;
	}

    /* Get the data of the first screen, the root screen */
    wmstatus.screen = xcb_setup_roots_iterator(xcb_get_setup(wmstatus.xconn)).data;
    root = wmstatus.screen->root;
     
    /* about the keyboard, again no clue what I am doing */
    /*xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(wmstatus.xconn);*/
     
    /* *********************************************************** */
    /* It assings something on the keyboard to the root window  */
    /* XCB_MOD_MASK_2 is 16 .. but not clue what that  means    */
    xcb_grab_key(wmstatus.xconn, 1, root, XCB_MOD_MASK_2, XCB_NO_SYMBOL,
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    
    xcb_grab_button(wmstatus.xconn, 0, root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
                    1 /* left mouse button */,
                    XCB_MOD_MASK_1);
    
    xcb_grab_button(wmstatus.xconn, 0, root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
                    2 /* middle mouse button */,
                    XCB_MOD_MASK_1);

    xcb_grab_button(wmstatus.xconn, 0, root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
                    3 /* right mouse button */,
                    XCB_MOD_MASK_1);            
                

    /* *********************************************************** */
    /* Tho subscribe the WM to the events we want to listen        */
     uint32_t mask = XCB_CW_EVENT_MASK;  
     uint32_t values[2];
     values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY   | 
                 XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                 XCB_EVENT_MASK_STRUCTURE_NOTIFY      |
                 XCB_EVENT_MASK_BUTTON_PRESS          |
                 XCB_EVENT_MASK_BUTTON_RELEASE;
     xcb_void_cookie_t cookie = xcb_change_window_attributes_checked(wmstatus.xconn, root, mask, values);
     if ( xcb_request_check(wmstatus.xconn,cookie) != NULL) {
      fprintf(stderr,"Seems there was a problem setting the attributes of the root window... bye bye\n");
      exit(1);
     }

    /*************/
    /* Main loop */
    xcb_flush(wmstatus.xconn); /* We want all the settings to take effect before going into the loop */ 
    xcb_generic_event_t *ev;
    mr_events_init_array_callbacks(); /* Initializat the array of callbacks */
    while( (ev = xcb_wait_for_event(wmstatus.xconn)) ) { 
	  mr_events_execute_callback(ev);
	  free(ev); /* free memory */
	}
     
  /* This is not the way to get out, there is more stuff to do */
  /* For the time being is enough, anyway this is not working yet so, who 
   * cares how does it exit */  
  xcb_disconnect(wmstatus.xconn); /* this line should never be executed anyway */
  return 0;
 } 
