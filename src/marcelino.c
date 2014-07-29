/* This is Marcelino, yet another tiny WM XCB based in C
   to be used as the Desktop/WM of Lombix OS
   
   It was forked from :

     - TinyWM is written by Nick Welch <mack@incise.org>, 2005.
     - TinyWM-XCB is rewritten by Ping-Hsun Chen <penkia@gmail.com>, 2010

 
 Please read license GPLv2. */

#include <stdlib.h>
#include <stdio.h>
#include <xcb/xcb.h>


/* *************    */
/* Global variables */
xcb_connection_t *dpy;
xcb_drawable_t root;
xcb_screen_t *screen;
uint32_t values[3];


int mr_error_connection_check ( int connection_error )
 {
	 if (!connection_error) { return 0; }
	 switch (connection_error) {
		 case XCB_CONN_ERROR: perror("ERROR: socket errors, pipe errors or other stream errors");
		                       break;
		 case XCB_CONN_CLOSED_EXT_NOTSUPPORTED: perror("ERROR: Extension not supported");
		                       break;
		 case XCB_CONN_CLOSED_MEM_INSUFFICIENT: perror("ERROR: Insufficient Memory");
		                       break;
		 case XCB_CONN_CLOSED_REQ_LEN_EXCEED: perror("ERROR: Exceeding request length that server accepts");
		                       break;           
		 case XCB_CONN_CLOSED_PARSE_ERR: perror("ERROR: During parsing display string");
		                       break;    
		 case XCB_CONN_CLOSED_INVALID_SCREEN: perror("ERROR:  Because the server does not have a screen matching the display");
		                       break;               
		 }
  exit(1);
 }




/* **************************************************** */
/* Functions to deal with basic events in the main loop */
/* **************************************************** */
int mr_deal_with_button_press (xcb_generic_event_t *ev)
 {
   xcb_button_press_event_t *e;
   xcb_drawable_t win;
   xcb_get_geometry_reply_t *geom;
      
    e = ( xcb_button_press_event_t *) ev;
    win = e->child;
    values[0] = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
    geom = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, win), NULL);
    if (1 == e->detail) {
       values[2] = 1;
       xcb_warp_pointer(dpy, XCB_NONE, win, 0, 0, 0, 0, 1, 1);
    }
    else {
       values[2] = 3;
       xcb_warp_pointer(dpy, XCB_NONE, win, 0, 0, 0, 0, geom->width, geom->height);
    }
    xcb_grab_pointer(dpy, 0, root, XCB_EVENT_MASK_BUTTON_RELEASE
                    | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE, XCB_CURRENT_TIME);
    xcb_flush(dpy);	 
 }



int mr_deal_with_motion_notify(xcb_generic_event_t *ev)
 {
  xcb_query_pointer_reply_t *pointer;
  xcb_button_press_event_t *e;
  xcb_drawable_t win;
  xcb_get_geometry_reply_t *geom;
  
	pointer = xcb_query_pointer_reply(dpy, xcb_query_pointer(dpy, root), 0);
    if (values[2] == 1) {/* move */
       geom = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, win), NULL);
       values[0] = (pointer->root_x + geom->width > screen->width_in_pixels)?
                   (screen->width_in_pixels - geom->width):pointer->root_x;
       values[1] = (pointer->root_y + geom->height > screen->height_in_pixels)?
                   (screen->height_in_pixels - geom->height):pointer->root_y;
       xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
       xcb_flush(dpy);
     }
     else
       if (values[2] == 3) { /* resize */
             geom = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, win), NULL);
             values[0] = pointer->root_x - geom->x;
             values[1] = pointer->root_y - geom->y;
             xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
             xcb_flush(dpy);
            }	 
 }

/* **************************************************** */
/* MAIN *********************************************** */
/* **************************************************** */
int main (int argc, char **argv)
 {

   xcb_generic_event_t *ev;


    /* Connects to the X Server  */
    dpy = xcb_connect(NULL, NULL);
    mr_error_connection_check(xcb_connection_has_error(dpy));

    /* Get the data of the first screen, the root screen */
    screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
    root = screen->root;

    /* *********************************************************** */
    /* It assings something on the keyboard to the root windows */
    /* XCB_MOD_MASK_2 is 16 .. but not clue what does it mean   */
    xcb_grab_key(dpy, 1, root, XCB_MOD_MASK_2, XCB_NO_SYMBOL,
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    xcb_grab_button(dpy, 0, root, XCB_EVENT_MASK_BUTTON_PRESS |
                XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
                XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 1, XCB_MOD_MASK_1);

    xcb_grab_button(dpy, 0, root, XCB_EVENT_MASK_BUTTON_PRESS |
                XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
                XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 3, XCB_MOD_MASK_1);
    xcb_flush(dpy);
    /* *********************************************************** */

    /* Main loop */
    while( (ev = xcb_wait_for_event(dpy)) ) {
    
      switch (ev->response_type & ~0x80) {
        
        case XCB_BUTTON_PRESS: 
		  mr_deal_with_button_press(ev);
          break;

        case XCB_MOTION_NOTIFY: 
		  mr_deal_with_motion_notify(ev);    
          break;

        case XCB_BUTTON_RELEASE:
          xcb_ungrab_pointer(dpy, XCB_CURRENT_TIME);
          xcb_flush(dpy);
          break;
      } /* end switch */
    } /* end while */

  return 0;
 } 
