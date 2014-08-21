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

#include "marcelino.h"
#include "menu.h"
#include "windows.h"
#include "events.h"


/* Array with the function pointers to the event callbacks */
void (*array_callbacks[HIGHER_XCB_EVENT_NUMBER])(xcb_generic_event_t *ev);


extern t_wmstatus wmstatus;
extern xcb_atom_t wm_state;

/**********************************************************************/
/* Auxiliary function to be sure the callbacks array is initialized   */
/**********************************************************************/
void mr_events_init_array_callbacks ()
 {
  int a;
  
	  for(a=0 ; a<HIGHER_XCB_EVENT_NUMBER ; a++) {
		  array_callbacks[a]=NULL;
	   }
	   
      array_callbacks[XCB_BUTTON_PRESS]=mr_deal_with_button_press;
      array_callbacks[XCB_BUTTON_RELEASE]=mr_deal_with_button_release;
      array_callbacks[XCB_KEY_PRESS]=mr_deal_with_key_press;
      array_callbacks[XCB_KEY_RELEASE]=mr_deal_with_key_release;

      array_callbacks[XCB_MOTION_NOTIFY]=mr_deal_with_motion_notify;
      array_callbacks[XCB_ENTER_NOTIFY]=mr_deal_with_enter_notify;
      array_callbacks[XCB_LEAVE_NOTIFY]=mr_deal_with_leave_notify;
      array_callbacks[XCB_EXPOSE]=mr_deal_with_expose;

      array_callbacks[XCB_MAP_REQUEST]=mr_deal_with_map_request;
      array_callbacks[XCB_DESTROY_NOTIFY]=mr_deal_with_destroy_notify;
      array_callbacks[XCB_CONFIGURE_REQUEST]=mr_deal_with_configure_request;

      array_callbacks[XCB_CONFIGURE_NOTIFY]=mr_deal_with_configure_notify;
      array_callbacks[XCB_MAPPING_NOTIFY]=mr_deal_with_mapping_notify;
      array_callbacks[XCB_CIRCULATE_REQUEST]=mr_deal_with_circulate_request; 
	   
 }


/*****************************************************/
/* helper function that will call the right callback */
/*****************************************************/
void mr_events_execute_callback (xcb_generic_event_t *ev)
{
	 if ( 
	        array_callbacks[ev->response_type] != NULL && 
            ev->response_type > 0 && 
            ev->response_type < HIGHER_XCB_EVENT_NUMBER  ) {
				 
         array_callbacks[ev->response_type](ev);
         
       }
      else
       {
		fprintf(stderr,">> I don't know what to do with event number %d. Ignoring it..\n",ev->response_type);
	   }
}


void mr_deal_with_button_release (xcb_generic_event_t *ev) 
{
  xcb_button_release_event_t *button = (xcb_button_release_event_t *)ev;
  fprintf(stderr,">> button release detail %d\n",button->detail);
  switch (wmstatus.mode) {
	  case MODE_MOVE:
	     			 xcb_ungrab_pointer(wmstatus.xconn, XCB_CURRENT_TIME);
	                 break;
	  case MODE_RESIZE:
	             	 xcb_ungrab_pointer(wmstatus.xconn, XCB_CURRENT_TIME);
	                 break;
	  default: /* do nothing */
	           break;
  }    
  wmstatus.mode=0;
  xcb_flush(wmstatus.xconn);
}



/*******************************************************/
/* When we press a button mouse                        */
/*******************************************************/
void mr_deal_with_button_press (xcb_generic_event_t *ev)
 {
	 xcb_query_pointer_reply_t *p;
	    
   fprintf(stderr,"entrammos por el button press\n");
   /* We cast generic event into this particular type of event */
   xcb_button_press_event_t * button = ( xcb_button_press_event_t *)ev; 
                 
    /* This should be that if we  click on the desktop then      */
    /* we open the menu                                          */
    if ( (button->child == 0) ) {
		mr_desktop_menu_display();
		return;
	}
	
	/* Ahhh, finally something easy to understand and documented!!         */
	/* see doc http://xcb.freedesktop.org/windowcontextandmanipulation     */
	/* Obviously if we click on a window we want this to become the one on the top */
    uint32_t values[] = { XCB_STACK_MODE_ABOVE };
    xcb_configure_window(wmstatus.xconn, button->child, XCB_CONFIG_WINDOW_STACK_MODE, values);
        
    if ( button->detail == 1) { /* moving window */
    	p = xcb_query_pointer_reply(
	                            wmstatus.xconn, 
	                            xcb_query_pointer(wmstatus.xconn,
	                            wmstatus.screen->root), 
	                           0);
       wmstatus.pointerx = p->root_x;
       wmstatus.pointery = p->root_y;       
       wmstatus.mode = MODE_MOVE;  
    }
    else {                  /* resizing window */
       fprintf(stderr,">>>> RESIZE CALL\n");
       wmstatus.mode = MODE_RESIZE;
    }
 
    /* seems this used to propagate this event to other components or event handlers 
     * but I am certainly not sure at all about this                                  */
    xcb_grab_pointer(wmstatus.xconn,
                     0, 
                     wmstatus.screen->root, 
                         XCB_EVENT_MASK_BUTTON_RELEASE | 
                         XCB_EVENT_MASK_BUTTON_MOTION | 
                         XCB_EVENT_MASK_POINTER_MOTION_HINT,
                     XCB_GRAB_MODE_ASYNC,
                     XCB_GRAB_MODE_ASYNC,
                     wmstatus.screen->root,
                     XCB_NONE, 
                     XCB_CURRENT_TIME);
 
    /* We want the changes to be visible straight away */
    xcb_flush(wmstatus.xconn);
  
 }


/*******************************************************/
/* When a window wants to be displayed                 */
/*******************************************************/
void mr_deal_with_map_request (xcb_generic_event_t *ev) 
{
   fprintf(stderr,"entramos por el map request \n");
   /* We cast generic event into this particular type of event */
    xcb_map_request_event_t * mapreq = (xcb_map_request_event_t *)ev;
	
	/* allocate the map and from there extract the number associated with that color */
    /* there is no need to say that this is an abominable way of programming */
	/* obviously once I understand how to it works this must be rewritten */
         
    uint32_t values[2];
    
	
    /* don't know what is this for .. I ve just copied over from mcwm */
    uint32_t mask = 0;
    mask = XCB_CW_EVENT_MASK;
    values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
    xcb_change_window_attributes_checked(wmstatus.xconn, mapreq->window, mask, values);
    xcb_change_save_set(wmstatus.xconn, XCB_SET_MODE_INSERT, mapreq->window);
    
    /* we register this window in our internal list */
    mr_window_add(mapreq->window);
       
	/* We way yes, we "map" the bloody window */
	xcb_map_window(wmstatus.xconn, mapreq->window);

/* these lines break the window manager */	
/*     long data[] = { XCB_ICCCM_WM_STATE_NORMAL, XCB_NONE };
    xcb_change_property(wmstatus.xconn,XCB_PROP_MODE_REPLACE,
                        mapreq->window,wm_state,wm_state,
                        32,2,data);
*/   
    	
    /* we request to show it now */
    xcb_flush(wmstatus.xconn);
	
}


/*******************************************************/
/* When we move the mouse                              */
/*******************************************************/
void mr_deal_with_motion_notify (xcb_generic_event_t *ev) 
 {
	uint32_t newx,deltax;
	uint32_t newy,deltay;
	
    xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)ev;
    fprintf(stderr,"motion notify called %d\n",motion->child);
    
    xcb_drawable_t id =  motion->child;
    
    /* it seems that although *motion event contains a mouse position */
    /* this seems not realiable as the mouse could have been on the move */
    /* since the event was triggered, that's why we ask for the current mouse */
    /* position */
    
	xcb_query_pointer_reply_t *p;
	p = xcb_query_pointer_reply(
	                            wmstatus.xconn, 
	                            xcb_query_pointer(wmstatus.xconn,
	                            wmstatus.screen->root), 
	                           0);
	                                                                                        
	switch(wmstatus.mode) {
		case MODE_MOVE: 
		     deltax = wmstatus.pointerx - p->root_x;
		     deltay = wmstatus.pointery - p->root_y;
		     newx = mr_window_get_pos_x(id)+deltax;
		     newy = mr_window_get_pos_y(id)+deltay;
		     mr_window_set_position(id,newx,newy);      
		     break;
		        
		case MODE_RESIZE:         
             mr_window_set_size(id,p->root_x-mr_window_get_pos_x(id),p->root_y - mr_window_get_pos_y(id));          
		     break;
		     
		default: 
		     fprintf(stderr,"INTERNAL ERROR on mr_deal_with_motion_notify. Value of MODE: %d\n",wmstatus.mode);
		     exit(1); /* I guess we should quit more elegantly, to be reviewed */
		     break;
	}
	       
                           
  xcb_flush(wmstatus.xconn);          
  free(p); /* free some memory */        
  
 }


void mr_deal_with_key_press (xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
  fprintf(stderr,">> Entramos por el key press \n");
  xcb_key_press_event_t *key = (xcb_key_press_event_t *)ev; 
  fprintf(stderr,">> Key %d pressed\n", key->detail);
  fprintf(stderr,">> el modo en key press es %d\n",wmstatus.mode);
}

void mr_deal_with_key_release (xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
  fprintf(stderr,">> Entramos por el key release \n");
  xcb_key_release_event_t *key = (xcb_key_release_event_t *)ev; 
  fprintf(stderr,">> Key %d released\n", key->detail);
  fprintf(stderr,">> el modo en key release es %d\n",wmstatus.mode);
}

void mr_deal_with_enter_notify (xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
    
  xcb_enter_notify_event_t *n = (xcb_enter_notify_event_t *)ev; 
  fprintf(stderr,">> Entramos por el enter notify %d\n",n->child);
}

void mr_deal_with_leave_notify (xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
    xcb_leave_notify_event_t *n = (xcb_leave_notify_event_t *)ev; 
  fprintf(stderr,">> Entramos por el leave notify %d\n",n->child);
}

void mr_deal_with_expose (xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
  fprintf(stderr,">> Entramos por el expose %d\n",ev->response_type);
}


void mr_deal_with_destroy_notify (xcb_generic_event_t *ev) 
{
  fprintf(stderr,">> entramos por destroy notify\n");
  xcb_destroy_notify_event_t *n = (xcb_destroy_notify_event_t *)ev;
  mr_window_delete(n->window);
}

void mr_deal_with_configure_request (xcb_generic_event_t *ev) 
{
 uint32_t x = 0;
 uint32_t y = 0;
   
  xcb_configure_request_event_t * e = (xcb_configure_request_event_t *) ev;
  uint32_t  id=e->window;
  
   if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)  { x = e->width; }
   if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) { y = e->height; }
 
   if (x && y ) { mr_window_set_size(id,x,y); }
   
   if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)
     {
       uint32_t values[1];         
       values[0] = e->stack_mode;
       xcb_configure_window(wmstatus.xconn, id,
                                 XCB_CONFIG_WINDOW_STACK_MODE,
                                 values);
      }           
    xcb_flush(wmstatus.xconn);   
 }

void mr_deal_with_configure_notify (xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
  fprintf(stderr,">> Entramos por el configure notify %d\n",ev->response_type);
}

void mr_deal_with_mapping_notify (xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
  fprintf(stderr,">> Entramos por el mapping notify %d\n",ev->response_type);
}

void mr_deal_with_circulate_request (xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
  fprintf(stderr,">> Entramos por el circulate request %d\n",ev->response_type);
  xcb_circulate_request_event_t *e = (xcb_circulate_request_event_t *)ev;
  xcb_circulate_window(wmstatus.xconn, e->window, e->place);
}


