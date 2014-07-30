/* This is Marcelino, yet another tiny WM XCB based in C
   to be used as the Desktop/WM of Lombix OS
   
   It was forked from :

     - TinyWM is written by Nick Welch <mack@incise.org>, 2005.
     - TinyWM-XCB is rewritten by Ping-Hsun Chen <penkia@gmail.com>, 2010

 
 Please read license GPLv2. */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>

/* *************    */
/* Global variables */
xcb_connection_t *xconn;
xcb_drawable_t root;
xcb_drawable_t focuswin; /* the current window that has the focus */
xcb_screen_t *screen;
int mode = 0;             /* Internal mode, such as move or resize */

/* Global definitions */
#define MODE_MOVE 2 /* We're currently moving a window with the mouse. */
#define MODE_RESIZE 3 /* We're currently resizing a window with the mouse. */


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
	 printf ("We have done click and and menu someday will appear\n");
	 return 0;
 }


/* **************************************************** */
/* Functions to deal with basic events in the main loop */
/* **************************************************** */


int mr_deal_with_button_press (xcb_button_press_event_t *ev)
 {

   xcb_get_geometry_reply_t *geom;
   uint32_t values[] = { XCB_STACK_MODE_ABOVE };
        
    /* if left click then we change focus window      */  
    /* on the window that receive the click.. I guess */     
      focuswin = ev->child;
    
    
    /* This should be that if we   click on the desktop then        */
    /* we open the menu                                             */
    if ( (focuswin == 0) ) {
		mr_deploy_desktop_menu();
		return 0;
	}
	
	/* Ahhh, finally something easy to understand and documented!!         */
	/* see doc http://xcb.freedesktop.org/windowcontextandmanipulation     */
	/* Obviously if we click on a window we want this to become the one on the top */
    
    xcb_configure_window(xconn, focuswin, XCB_CONFIG_WINDOW_STACK_MODE, values);
    
    /* We determine the geometry of the focused window to be used later
     * in case we resize the window                                      */
    geom = xcb_get_geometry_reply(xconn, xcb_get_geometry(xconn, focuswin), NULL);
    
    /* Now, this is copy pasted from other software, it seems that if the button 
     * pressed is the left one, we take an action of move, and to do that
     * for some reason we place the mouse pointer in the upper left, but there is 
     * no opreation here of movement, just mouse positioning */
    /* note that we setup a global variable mode indicating the status */
    if ( ev->detail == 1) { /* moving window */
       mode = MODE_MOVE;  
       xcb_warp_pointer(xconn, XCB_NONE, focuswin, 0, 0, 0, 0, 1, 1);
    }
    else {                  /* resizing window */
       mode = MODE_RESIZE;
       xcb_warp_pointer(xconn, XCB_NONE, focuswin, 0, 0, 0, 0, geom->width, geom->height);
    }
 
    /* seems this used to propagate this event to other components or event handlers 
     * but I am certainly not sure at all about this                                  */
    xcb_grab_pointer(xconn, 0, root, XCB_EVENT_MASK_BUTTON_RELEASE
                    | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE, XCB_CURRENT_TIME);
 
    xcb_flush(xconn);
  return 0;
 }



int mr_deal_with_map_request(xcb_map_request_event_t *mapreq) 
{
	/* We way yes, we "map" the bloody window */
	xcb_map_window(xconn, mapreq->window);
	
    /* we request to show it now */
	xcb_flush(xconn);
	
  return 0;
}


int mr_deal_with_motion_notify(xcb_motion_notify_event_t *motion)
 {
  xcb_query_pointer_reply_t *pointer;
  xcb_get_geometry_reply_t *geom;
  uint32_t values[2];

    /* it seems that although *motion event contains a mouse position */
    /* this seems not realiable as the mouse could have been on the move */
    /* since the event was triggered, that's why we ask for the current mouse */
    /* position */
    
	pointer = xcb_query_pointer_reply(xconn, xcb_query_pointer(xconn, root), 0);
	
	        
    if (mode == MODE_MOVE) {/* we are moving windows, comes from the button press event */
       geom = xcb_get_geometry_reply(xconn, xcb_get_geometry(xconn, focuswin), NULL);
       values[0] = (pointer->root_x + geom->width > screen->width_in_pixels)?
                   (screen->width_in_pixels - geom->width):pointer->root_x;
       values[1] = (pointer->root_y + geom->height > screen->height_in_pixels)?
                   (screen->height_in_pixels - geom->height):pointer->root_y;
       xcb_configure_window(xconn, focuswin, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
       
     }
     else
       if (mode == MODE_RESIZE) { /* we are resizing */
             geom = xcb_get_geometry_reply(xconn, xcb_get_geometry(xconn, focuswin), NULL);
             values[0] = pointer->root_x - geom->x;
             values[1] = pointer->root_y - geom->y;
             xcb_configure_window(xconn, focuswin, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        
        }	 
                            
  xcb_flush(xconn);          
  free(pointer); /* free some memory */        
  return 0;
 }


/* **************************************************** */
/* MAIN *********************************************** */
/* **************************************************** */
int main ()
 {

   xcb_generic_event_t *ev;
   int scrno;
   xcb_void_cookie_t cookie;
   
    /* Connects to the X Server  */
    xconn = xcb_connect(NULL, &scrno);
    mr_error_connection_check(xcb_connection_has_error(xconn));
    fprintf(stderr,">>>>The screen number allocated is %d\n",scrno);

    /* Get the data of the first screen, the root screen */
    screen = xcb_setup_roots_iterator(xcb_get_setup(xconn)).data;
    root = screen->root;
    

     
    /* *********************************************************** */
    /* It assings something on the keyboard to the root window  */
    /* XCB_MOD_MASK_2 is 16 .. but not clue what that  means    */
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
                

    /* *********************************************************** */

    /* This code might be needed to actually subscribe the root window 
     * into the events                                               */
     uint32_t mask = XCB_CW_EVENT_MASK;  
     uint32_t values[2];
     values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY   | 
                 XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                 XCB_EVENT_MASK_STRUCTURE_NOTIFY      |
                 XCB_EVENT_MASK_BUTTON_PRESS          |
                 XCB_EVENT_MASK_BUTTON_RELEASE;
     cookie = xcb_change_window_attributes_checked(xconn, root, mask, values);
     if ( xcb_request_check(xconn,cookie) != NULL) {
      fprintf(stderr,"Seems there was a problem setting the attributes of the root window... bye bye\n");
      exit(1);
     }


    /* Main loop */
    xcb_flush(xconn);
    while( (ev = xcb_wait_for_event(xconn)) ) {
    
      switch (ev->response_type) {
        
        case XCB_BUTTON_PRESS: 
          fprintf(stderr,">>XCB_BUTTON_PRESS\n");
          mr_deal_with_button_press(( xcb_button_press_event_t *)ev);
	  break;
		  
        case XCB_BUTTON_RELEASE:
         fprintf(stderr,">>XCB_BUTTON_RELEASE\n");
          xcb_ungrab_pointer(xconn, XCB_CURRENT_TIME);
          break;
          
        case XCB_KEY_PRESS: 
          fprintf(stderr,">>XCB_KEY_PRESS\n");
	  break;
        
        case XCB_KEY_RELEASE: 
         fprintf(stderr,">>XCB_KEY_RELEASE\n");
	  break;
		            
        case XCB_MOTION_NOTIFY: 
          fprintf(stderr,">>XCB_MOTION_NOTIFY\n");
	  mr_deal_with_motion_notify((xcb_motion_notify_event_t *) ev); 
	  break;

        case XCB_ENTER_NOTIFY:
          fprintf(stderr,">>XCB_ENTER_NOTIFY\n"); 
	  break;
		  
        case XCB_LEAVE_NOTIFY:
          fprintf(stderr,">>XCB_LEAVE_NOTIFY\n"); 
	  break;
		           
        case XCB_EXPOSE:
          fprintf(stderr,">>XCB_EXPOSE\n");
          break;
          
        case XCB_MAP_REQUEST:
          /* It seem this event is called when a window is shown, or created */
          fprintf(stderr,">>XCB_MAP_REQUEST\n");
          mr_deal_with_map_request((xcb_map_request_event_t *)ev);
          break;
          
        case XCB_CREATE_NOTIFY:
          fprintf(stderr,">>XCB_CREATE_NOTIFY\n");
          break;
         
        case XCB_DESTROY_NOTIFY:
         fprintf(stderr,">>XCB_DESTROY_NOTIFY\n");
         break;
         
        case XCB_CONFIGURE_REQUEST:
         fprintf(stderr,">>XCB_CONFIGURE_REQUEST\n");
         break;
         
        case XCB_CONFIGURE_NOTIFY:
         fprintf(stderr,">>XCB_CONFIGURE_NOTIFY\n");
         break;
            
        case XCB_MAPPING_NOTIFY:
         fprintf(stderr,">>XCB_MAPPING_NOTIFY\n");
         break;
         
        case XCB_CIRCULATE_REQUEST:
         fprintf(stderr,">>XCB_CIRCULATE_REQUEST\n");
         break;
         
        default: 
          fprintf(stderr, ">> >>Unknown event: %d\n", ev->response_type);
          break; 
                   
      } /* end switch */
      
      free(ev); /* free memory */
    } /* end while */
 
  
  xcb_disconnect(xconn); /* this line should never be executed anyway */
  return 0;
 } 
