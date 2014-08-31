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


#include "events.h"
#include "windows.h"


  extern xcb_connection_t  *xconn;  /* the XCB Connection */
  extern xcb_screen_t      *xscreen; /* The screen info    */  
  
  static uint16_t           xmode   = 0;    /* Flag to say if we are moving or resizing */
  static xcb_drawable_t 	xwindow = 0;    /* Window which is being moved or resized */
	
  uint32_t			plastx;
  uint32_t			plasty; 
  

/*****************************************************/
/* helper function that will call the right callback */
/*****************************************************/
void mr_events_execute_callback (xcb_generic_event_t *ev)
{
	
	switch(ev->response_type & ~0x80) {
		case XCB_MOTION_NOTIFY:  	mr_deal_with_motion_notify(ev); break;
		case XCB_BUTTON_PRESS:   	mr_deal_with_button_press(ev); break;
		case XCB_BUTTON_RELEASE: 	mr_deal_with_button_release(ev); break;
	    case XCB_MAP_REQUEST:    	mr_deal_with_map_request(ev); break;
	    case XCB_MAP_NOTIFY:    	mr_deal_with_map_notify(ev); break;
	    case XCB_MAPPING_NOTIFY:    mr_deal_with_mapping_notify(ev); break;
	    case XCB_DESTROY_NOTIFY: 	mr_deal_with_destroy_notify(ev); break;
   	    case XCB_CREATE_NOTIFY:		mr_deal_with_create_notify(ev); break;
	    case XCB_CONFIGURE_REQUEST: mr_deal_with_configure_request(ev); break;
	    case XCB_CONFIGURE_NOTIFY:  mr_deal_with_configure_notify(ev); break;
	    case XCB_RESIZE_REQUEST:	mr_deal_with_resize_request(ev); break;
	    case XCB_CLIENT_MESSAGE:	mr_deal_with_client_message(ev); break;
	    default:                 fprintf(stderr,"Other event detected unknown: %d\n",ev->response_type & ~0x80); break;
	};
 }	
	
/*****************************************************/
/*****************************************************/
void mr_deal_with_button_release (xcb_generic_event_t *ev) 
{
  xcb_button_release_event_t *button = (xcb_button_release_event_t *)ev;
  
  fprintf(stderr,"Button release, detail: %d \nroot: %ld \nevent:  %ld\n child:  %ld\n",
               button->detail, (long)button->root,(long)button->event,(long)button->child);
	
  xcb_ungrab_pointer(xconn, XCB_CURRENT_TIME); 
  
  xmode=0;
  xwindow=0;
  
  xcb_flush(xconn);
  

}



/*******************************************************/
/*******************************************************/
void mr_deal_with_button_press (xcb_generic_event_t *ev)
 {
    
   xcb_query_pointer_cookie_t cookie=xcb_query_pointer(xconn,xscreen->root);   
        
   xcb_button_press_event_t * button = ( xcb_button_press_event_t *)ev; 
   fprintf(stderr,"Button press: \n detail: %d \n root: %ld \n event:  %ld\n child:  %ld\n",
           button->detail, (long)button->root,(long)button->event,(long)button->child);

               	
    xcb_drawable_t id = xwindow = button->child; /* window @clicked@ */            	
    
    if (id == 0) { /* because we clicked the root window */
                
		if ( button->detail == XCB_BUTTON_INDEX_3 ) {
                        fprintf(stderr,"trying to execute lxterminal\n");        
                        if (!fork()) {
                         if (!fork()) {
                           close(xcb_get_file_descriptor(xconn));
			   execl("/usr/bin/bash","/usr/bin/bash","-c","xclock",NULL);
			   exit(-1);
			  }
                         exit(0);
                        }
                       wait(NULL);
		}
	}
               	
	uint32_t values =  XCB_STACK_MODE_ABOVE;
    xcb_configure_window(xconn, id, XCB_CONFIG_WINDOW_STACK_MODE, &values);
    

    switch( button->detail ) {
		case XCB_BUTTON_INDEX_1:  /* moving window */   
                xmode = MODE_MOVE;  
                fprintf(stderr,"In button press we set xmode to %d\n",xmode);
                break;
        default: /* resizing window */
                xmode = MODE_RESIZE;
               fprintf(stderr,"In button press we set xmode to %d\n",xmode);
               break;
    };
    
    
    xcb_grab_pointer(xconn, 0, xscreen->root,
                                 XCB_EVENT_MASK_BUTTON_RELEASE
                                 | XCB_EVENT_MASK_BUTTON_MOTION
                                 | XCB_EVENT_MASK_POINTER_MOTION_HINT,
                                 XCB_GRAB_MODE_ASYNC,
                                 XCB_GRAB_MODE_ASYNC,
                                 xscreen->root,
                                 XCB_NONE,
                                 XCB_CURRENT_TIME);
                       
 
                            
    xcb_query_pointer_reply_t * p;
    p = xcb_query_pointer_reply(xconn,cookie,NULL); 
    plastx=p->root_x;
    plasty=p->root_y;
 
    free(p);
    
  fprintf(stderr,"Before we leave button press the xmode is %d and the xwindow is %d\n",xmode,xwindow);
 }


/*******************************************************/
/* When a window wants to be displayed                 */
/*******************************************************/
void mr_deal_with_map_request (xcb_generic_event_t *ev) 
{
   /* We cast generic event into this particular type of event */
    xcb_map_request_event_t * mapreq = (xcb_map_request_event_t *)ev;
	
    fprintf(stderr,"With map request, parent: %ld \nwindow: %ld\n",(long)mapreq->parent,(long)mapreq->window);

    /* we register this window in our internal list */
    mr_window_add(mapreq->window);
           	
    /* we request to show it now */
    xcb_flush(xconn);
	
}


/*******************************************************/
/* When we move the mouse                              */
/*******************************************************/
void mr_deal_with_motion_notify (xcb_generic_event_t *ev) 
 {
	int32_t deltax,deltay;
	uint32_t currx,curry;
	
	
    xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)ev;
    fprintf(stderr,"Motion Notify, detail: %d\n root: %ld \nevent:  %ld\n child:  %ld\n",motion->detail, (long)motion->root,(long)motion->event,(long)motion->child);
   
    /* we move the window which was selected */
    xcb_drawable_t id =  xwindow;
    if (id == 0 || id == xscreen->root ) {
	  fprintf(stderr,"Sorry mate, we cannot move this window: %d\n",id);
      return;
    }
    
    xcb_query_pointer_cookie_t cookie=xcb_query_pointer(xconn,xscreen->root);
    
  
    /* Where is pointer NOW */
    xcb_query_pointer_reply_t * p;
    p = xcb_query_pointer_reply(xconn,cookie,NULL); 
    
    /* Calculate the delta of the movement */                
    deltax = p->root_x - plastx;
	deltay = p->root_y - plasty;
	             
	switch(xmode) {
		case MODE_MOVE: 
		     mr_window_get_pos(id,&currx,&curry);
		     mr_window_set_pos(id,currx+deltax,curry+deltay);
		           
		     break;
		        
		case MODE_RESIZE:   
		     mr_window_get_size(id,&currx,&curry);   
		     if ( (int32_t)(currx+deltax) > 0 && (int32_t)(curry+deltay) > 0 ) {   
               mr_window_set_size(id,currx+deltax,curry+deltay);
			 }            
		     break;
		     
		default: 
		     /* ignore */
		     break;
	}
	 
	 /* BTW, we reset the pointer of the last cursor position to what has been calculated node */      
     plastx =  p->root_x;                                           
	 plasty =  p->root_y;                                           
	                       
     free(p);    
 
   
 }



/*******************************************************/
/* Destroy Window                                      */
/*******************************************************/
void mr_deal_with_destroy_notify (xcb_generic_event_t *ev) 
{
  fprintf(stderr,">> Within destroy notify\n");
  xcb_destroy_notify_event_t *n = (xcb_destroy_notify_event_t *)ev;
  mr_window_delete(n->window);
}


/*******************************************************/
/* Configure Request                                   */
/*******************************************************/
void mr_deal_with_configure_request (xcb_generic_event_t *ev) 
{
 uint32_t x = 0;
 uint32_t y = 0;
   
  xcb_configure_request_event_t * e = (xcb_configure_request_event_t *) ev;
  uint32_t  id=e->window;
  fprintf(stderr,">> Within Configure Request for window %d\n",id);
  
   /* If the request was to resize */
   if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)  { x = e->width; }
   if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) { y = e->height; }
   if (x && y ) { 
	              fprintf(stderr,"Within configure request we want to change size to %d %d\n",x,y);
	              mr_window_set_size(id,x,y);
	            }
   
   /* If the request was to come upfront */
   if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)
     {
       uint32_t values[1];         
       values[0] = e->stack_mode;
       xcb_configure_window(xconn, id,
                                 XCB_CONFIG_WINDOW_STACK_MODE,
                                 values);
      }             
 }


/*******************************************************/
/* Configure Notify                                    */
/*******************************************************/
void mr_deal_with_configure_notify (xcb_generic_event_t *ev) 
{
  xcb_configure_notify_event_t * e = (xcb_configure_notify_event_t *) ev;
  uint32_t  id=e->window;
  fprintf(stderr,">> Entramos por el configure notify para la ventana %d\n",id);
}


/*******************************************************/
/* Resize Request                                   */
/*******************************************************/
void mr_deal_with_resize_request (xcb_generic_event_t *ev) 
{
  xcb_resize_request_event_t * e = (xcb_resize_request_event_t *) ev;
  uint32_t  id=e->window;
  fprintf(stderr,">> Within Resize Request for window %d\n",id);
  
}


/*******************************************************/
/* Create Notify                                       */
/*******************************************************/
void mr_deal_with_create_notify (xcb_generic_event_t *ev) 
{
  xcb_create_notify_event_t * e = (xcb_create_notify_event_t *) ev;
  uint32_t  id=e->window;
  fprintf(stderr,">> Within Create Notify for window %d\n",id);
  
}


/*******************************************************/
/* Map Notify                                       */
/*******************************************************/
void mr_deal_with_map_notify (xcb_generic_event_t *ev) 
{
  xcb_map_notify_event_t * e = (xcb_map_notify_event_t *) ev;
  uint32_t  id=e->window;
  fprintf(stderr,">> Within Map Notify for window %d\n",id);
  
}


/*******************************************************/
/* Mapping Notify                                       */
/*******************************************************/
void mr_deal_with_mapping_notify (xcb_generic_event_t *ev) 
{
  xcb_mapping_notify_event_t * e = (xcb_mapping_notify_event_t *) ev;
  fprintf(stderr,">> Within Mapping Notify with request %d\n",e->request);
  
}


/*******************************************************/
/* Client Message                                      */
/*******************************************************/
void mr_deal_with_client_message (xcb_generic_event_t *ev) 
{
  xcb_client_message_event_t * e = (xcb_client_message_event_t *) ev;
  uint32_t  id=e->window;
  
  xcb_client_message_data_t data = e->data;
  uint32_t x = data.data32[0]; 
  fprintf(stderr,">> Within Client Message for window %d\n",id);
  fprintf(stderr,"          Type %d\n",e->type);
  fprintf(stderr,"          Data %d \n", x );
  
}
