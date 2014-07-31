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

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>



/* **************************************************** */
/* MAIN *********************************************** */
/* **************************************************** */
int main ()
 {
   t_wmstatus wmstatus;       /* The global status of the WM */
   

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
    mr_aux_init_array_callbacks(); /* Initializat the array of callbacks */
    while( (ev = xcb_wait_for_event(wmstatus.xconn)) ) { 
	  mr_events_execute_callback(&wmstatus,ev);
	  free(ev); /* free memory */
	}
     
    
    
    
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
