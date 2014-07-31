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

#include "marcelino.h"
#include "menu.h"


/* Function declaration */
void mr_deal_with_button_press (t_wmstatus * wmstatus,xcb_generic_event_t *ev);
void mr_deal_with_map_request (t_wmstatus * wmstatus,xcb_generic_event_t *ev);
void mr_deal_with_map_request (t_wmstatus * wmstatus,xcb_generic_event_t *ev);
void mr_deal_with_motion_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev);
void mr_deal_with_button_release (t_wmstatus * wmstatus,xcb_generic_event_t *ev);
void mr_deal_with_key_press (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_key_release (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_enter_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_leave_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_expose (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_create_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_destroy_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_configure_request (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_configure_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_mapping_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 
void mr_deal_with_circulate_request (t_wmstatus * wmstatus,xcb_generic_event_t *ev); 



/* the array of callbacks will be this big, 100 is an arbitrary 
 * number I've given it buecase at this stage I simply don't know */
#define HIGHER_XCB_EVENT_NUMBER 100

/* Array with the function pointers to the event callbacks */
void (*array_callbacks[HIGHER_XCB_EVENT_NUMBER])(t_wmstatus * wmstatus,xcb_generic_event_t *ev);





/**********************************************************************/
/* Auxiliary function to be sure the callbacks array is initialized   */
/**********************************************************************/
void mr_aux_init_array_callbacks ( )
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
      array_callbacks[XCB_CREATE_NOTIFY]=mr_deal_with_create_notify;
      array_callbacks[XCB_DESTROY_NOTIFY]=mr_deal_with_destroy_notify;
      array_callbacks[XCB_CONFIGURE_REQUEST]=mr_deal_with_configure_request;

      array_callbacks[XCB_CONFIGURE_NOTIFY]=mr_deal_with_configure_notify;
      array_callbacks[XCB_MAPPING_NOTIFY]=mr_deal_with_mapping_notify;
      array_callbacks[XCB_CIRCULATE_REQUEST]=mr_deal_with_circulate_request; 
	   
 }


/*****************************************************/
/* helper function that will call the right callback */
/*****************************************************/
void mr_events_execute_callback (t_wmstatus * wmstatus,xcb_generic_event_t *ev)
{
	 if ( 
	        array_callbacks[ev->response_type] != NULL && 
            ev->response_type > 0 && 
            ev->response_type < HIGHER_XCB_EVENT_NUMBER  ) {
				 
         array_callbacks[ev->response_type](wmstatus,ev);
         
       }
      else
       {
		fprintf(stderr,">> I don't know what to do with event number %d. Ignoring it..\n",ev->response_type);
	   }
}

/*******************************************************/
/* When we press a button mouse                        */
/*******************************************************/
void mr_deal_with_button_press (t_wmstatus * wmstatus,xcb_generic_event_t *ev)
 {

   /* We cast generic event into this particular type of event */
   xcb_button_press_event_t * button = ( xcb_button_press_event_t *)ev; 
                 
    /* This should be that if we  click on the desktop then      */
    /* we open the menu                                          */
    if ( (button->child == 0) ) {
		mr_desktop_menu_display();
	}
	
	/* Ahhh, finally something easy to understand and documented!!         */
	/* see doc http://xcb.freedesktop.org/windowcontextandmanipulation     */
	/* Obviously if we click on a window we want this to become the one on the top */
    uint32_t values[] = { XCB_STACK_MODE_ABOVE };
    xcb_configure_window(wmstatus->xconn, button->child, XCB_CONFIG_WINDOW_STACK_MODE, values);
    
    
    /* Now, this is copy pasted from other software, it seems that if the button 
     * pressed is the left one, we take an action of move, and to do that
     * for some reason we place the mouse pointer in the upper left, but there is 
     * no opreation here of movement, just mouse positioning */
    /* note that we setup a global variable mode indicating the status */
    if ( button->detail == 1) { /* moving window */
       wmstatus->mode = MODE_MOVE;  
       xcb_warp_pointer(wmstatus->xconn, XCB_NONE, button->child  , 0, 0, 0, 0, 1, 1);
    }
    else {                  /* resizing window */
       xcb_get_geometry_reply_t *g = xcb_get_geometry_reply(wmstatus->xconn, xcb_get_geometry(wmstatus->xconn,button->child ), NULL);
       wmstatus->mode = MODE_RESIZE;
       xcb_warp_pointer(wmstatus->xconn, XCB_NONE, button->child , 0, 0, 0, 0, g->width, g->height);
    }
 
    /* seems this used to propagate this event to other components or event handlers 
     * but I am certainly not sure at all about this                                  */
    xcb_grab_pointer(wmstatus->xconn, 0, wmstatus->screen->root, XCB_EVENT_MASK_BUTTON_RELEASE
                    | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, wmstatus->screen->root, XCB_NONE, XCB_CURRENT_TIME);
 
    /* We want the changes to be visible straight away */
    xcb_flush(wmstatus->xconn);
  
 }


/*******************************************************/
/* When a window wants to be displayed                 */
/*******************************************************/
void mr_deal_with_map_request (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{

   /* We cast generic event into this particular type of event */
    xcb_map_request_event_t * mapreq = (xcb_map_request_event_t *)ev;
	
	/* allocate the map and from there extract the number associated with that color */
    /* there is no need to say that this is an abominable way of programming */
	/* obviously once I understand how to it works this must be rewritten */
    const char * colstr = "blue";
	xcb_alloc_named_color_reply_t *col_reply;    
    xcb_colormap_t colormap; 
    xcb_generic_error_t *error;
    xcb_alloc_named_color_cookie_t colcookie;
	colormap = wmstatus->screen->default_colormap;
    colcookie = xcb_alloc_named_color(wmstatus->xconn, colormap, strlen(colstr), colstr);
    col_reply = xcb_alloc_named_color_reply(wmstatus->xconn, colcookie, &error);
 
    uint32_t values[2];
	/* Set border color. */   
    values[0] = col_reply->pixel;
    xcb_change_window_attributes(wmstatus->xconn, mapreq->window, XCB_CW_BORDER_PIXEL, values);
    
	/* Set border width. */
    values[0] = 2; /* size in pixels */
    xcb_configure_window(wmstatus->xconn, mapreq->window, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
    
    
	/* We way yes, we "map" the bloody window */
	xcb_map_window(wmstatus->xconn, mapreq->window);
	
    /* we request to show it now */
	xcb_flush(wmstatus->xconn);
	
}


/*******************************************************/
/* When we move the mouse                              */
/*******************************************************/
void mr_deal_with_motion_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
 {

    xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)ev;
    
    /* it seems that although *motion event contains a mouse position */
    /* this seems not realiable as the mouse could have been on the move */
    /* since the event was triggered, that's why we ask for the current mouse */
    /* position */
    
	xcb_query_pointer_reply_t *p = xcb_query_pointer_reply(
	                                             wmstatus->xconn, 
	                                             xcb_query_pointer(wmstatus->xconn,
	                                                               wmstatus->screen->root), 
	                                             0);
	
	xcb_get_geometry_reply_t * g = xcb_get_geometry_reply(wmstatus->xconn, 
	                                                      xcb_get_geometry(wmstatus->xconn, motion->child),
	                                                      NULL);
	                                                      
	/* Note, I am assuming that motion->child is the window which is being moved or resized */    
	/* this might be wrong */                                                  
	uint32_t values[2];														
	switch(wmstatus->mode) {
		case MODE_MOVE: 
		     values[0] = (p->root_x + g->width > wmstatus->screen->width_in_pixels)?
                         (wmstatus->screen->width_in_pixels - g->width):p->root_x;
			 values[1] = (p->root_y + g->height > wmstatus->screen->height_in_pixels)?
						 (wmstatus->screen->height_in_pixels - g->height):p->root_y;
			 xcb_configure_window(wmstatus->xconn,
				                  motion->child, 
				                  XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, 
				                  values);       
		     break;
		        
		case MODE_RESIZE:
             values[0] = p->root_x - g->x;
             values[1] = p->root_y - g->y;
             xcb_configure_window(wmstatus->xconn,
                                 motion->child,
                                 XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, 
                                 values);
		     break;
		     
		default: 
		     fprintf(stderr,"INTERNAL ERROR on mr_deal_with_motion_notify. Value of MODE: %d\n",wmstatus->mode);
		     exit(1); /* I guess we should quit more elegantly, to be reviewed */
		     break;
	}
	       
                           
  xcb_flush(wmstatus->xconn);          
  free(p); /* free some memory */        
  
 }

void mr_deal_with_button_release (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
  xcb_ungrab_pointer(wmstatus->xconn, XCB_CURRENT_TIME);
}


void mr_deal_with_key_press (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_key_release (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_enter_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_leave_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_expose (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_create_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_destroy_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_configure_request (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_configure_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_mapping_notify (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}

void mr_deal_with_circulate_request (t_wmstatus * wmstatus,xcb_generic_event_t *ev) 
{
  /* At this stage I don not know what else to do */
}
