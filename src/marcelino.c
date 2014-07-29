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
xcb_connection_t *xconn;
xcb_drawable_t root;
xcb_drawable_t focuswin;
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
         default: perror("ERROR: Unknnown error result when trying to contact the X Server");
                                       break;
		 }
  exit(1);
 }




int mr_deploy_desktop_menu ( )
 { 
	 return 0;
 }


/* **************************************************** */
/* Functions to deal with basic events in the main loop */
/* **************************************************** */
int mr_deal_with_button_press (xcb_button_press_event_t *ev)
 {

   xcb_get_geometry_reply_t *geom;
        
    focuswin = ev->child;
    
    if ( (focuswin == 0) && (ev->detail == 3)) {
		mr_deploy_desktop_menu();
		return 0;
	}
	 
    values[0] = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(xconn, focuswin, XCB_CONFIG_WINDOW_STACK_MODE, values);
    geom = xcb_get_geometry_reply(xconn, xcb_get_geometry(xconn, focuswin), NULL);
    if ( ev->detail == 1) {
       values[2] = 1;
       xcb_warp_pointer(xconn, XCB_NONE, focuswin, 0, 0, 0, 0, 1, 1);
    }
    else {
       values[2] = 3;
       xcb_warp_pointer(xconn, XCB_NONE, focuswin, 0, 0, 0, 0, geom->width, geom->height);
    }
    xcb_grab_pointer(xconn, 0, root, XCB_EVENT_MASK_BUTTON_RELEASE
                    | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE, XCB_CURRENT_TIME);
 
  return 0;
 }



int mr_deal_with_motion_notify()
 {
  xcb_query_pointer_reply_t *pointer;
  xcb_get_geometry_reply_t *geom;
  

    
	pointer = xcb_query_pointer_reply(xconn, xcb_query_pointer(xconn, root), 0);
	
	        
    if (values[2] == 1) {/* move */
       geom = xcb_get_geometry_reply(xconn, xcb_get_geometry(xconn, focuswin), NULL);
       values[0] = (pointer->root_x + geom->width > screen->width_in_pixels)?
                   (screen->width_in_pixels - geom->width):pointer->root_x;
       values[1] = (pointer->root_y + geom->height > screen->height_in_pixels)?
                   (screen->height_in_pixels - geom->height):pointer->root_y;
       xcb_configure_window(xconn, focuswin, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
       

     }
     else
       if (values[2] == 3) { /* resize */
             geom = xcb_get_geometry_reply(xconn, xcb_get_geometry(xconn, focuswin), NULL);
             values[0] = pointer->root_x - geom->x;
             values[1] = pointer->root_y - geom->y;
             xcb_configure_window(xconn, focuswin, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        
        }	 
                            
            
            
  return 0;
 }

/* **************************************************** */
/* MAIN *********************************************** */
/* **************************************************** */
int main ()
 {

   xcb_generic_event_t *ev;
   int scrno;

    /* Connects to the X Server  */
    xconn = xcb_connect(NULL, &scrno);
    mr_error_connection_check(xcb_connection_has_error(xconn));

    /* Get the data of the first screen, the root screen */
    screen = xcb_setup_roots_iterator(xcb_get_setup(xconn)).data;
    root = screen->root;

    /* *********************************************************** */
    /* It assings something on the keyboard to the root windows */
    /* XCB_MOD_MASK_2 is 16 .. but not clue what does it mean   */
    xcb_grab_key(xconn, 1, root, XCB_MOD_MASK_2, XCB_NO_SYMBOL,
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    
    xcb_grab_button(xconn, 0, root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
                    1 /* left mouse button */,
                    XCB_MOD_MASK_1);
    
    xcb_grab_button(xconn, 0, root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
                    2 /* middle mouse button */,
                    XCB_MOD_MASK_1);

    xcb_grab_button(xconn, 0, root, XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
                    3 /* right mouse button */,
                    XCB_MOD_MASK_1);            
                
                
    xcb_flush(xconn);
    /* *********************************************************** */

    /* Main loop */
    while( (ev = xcb_wait_for_event(xconn)) ) {
    
      switch (ev->response_type & ~0x80) {
        
        case XCB_BUTTON_PRESS: 
		  mr_deal_with_button_press(( xcb_button_press_event_t *)ev);
		  xcb_flush(xconn);
          break;

        case XCB_MOTION_NOTIFY: 
		  mr_deal_with_motion_notify(); 
		  xcb_flush(xconn);   
          break;

        case XCB_BUTTON_RELEASE:
          xcb_ungrab_pointer(xconn, XCB_CURRENT_TIME);
          xcb_flush(xconn);
          break;
          

          
      } /* end switch */
    } /* end while */

  return 0;
 } 
