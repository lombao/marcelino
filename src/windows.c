/*******************************************************/
/* MARCELINO. Yet another simplistic WM using XCB in C */
/* ----------------------------------------------------*/
/* Windows Management                                  */
/*   Here we manage the windows as objects, we paint   */
/*   them, we draw them, we kill them                  */
/* --------------------------------------------------- */
/* Author: Cesar Lombao                                */
/* Date:   July 2014                                   */
/* License: GPLv2                                      */
/*******************************************************/

#include "windows.h"
#include "aux.h"


/* Global variables */
t_wmnode * whead = NULL;
t_wmnode * wtail = NULL;

/* This is like a create or init or new in OOP */
/* but if a window id is already registered it will not create
 * another entry */
void mr_window_add (xcb_drawable_t id) {
  
    fprintf(stderr,"METIENDO VENTANA %d\n",id);
    
    t_wmnode * ptr = mr_window_locate(id);
    if (ptr != NULL) { fprintf(stderr,"tried to add a window that already existed\n");
		               return; }
  
    ptr=mr_aux_malloc(sizeof (t_wmnode));	    
    ptr->id = id;
    ptr->before = NULL;
    ptr->next   = NULL;
    
    if (whead == NULL) {
	 whead = ptr;
	 wtail = ptr;
	}
	else {
	 wtail->next = ptr;
	 ptr->before = wtail;
	 wtail = ptr;	
	}
    
     
    /* Set border color and width border */   
    uint32_t values[2];
    values[0]=pixelcolor;	
    xcb_change_window_attributes(xconn, id, XCB_CW_BORDER_PIXEL, values);
	values[0]=10;	
    xcb_configure_window(xconn, id, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
    
    /* We way yes, we "map" the bloody window */
	xcb_map_window(xconn, id);
	
    /* Declare window normal. */
    long data[] = { XCB_ICCCM_WM_STATE_NORMAL, XCB_NONE };
    xcb_change_property(xconn, XCB_PROP_MODE_REPLACE, id,
                              wm_state, wm_state, 32, 2, data);
    
    /* retrieve its position and size */               
    /* We have to decompose this request to benefit from XCB async */  
      xcb_get_geometry_reply_t * g =
                        xcb_get_geometry_reply(xconn, 
                                  xcb_get_geometry(xconn, id),
                                  NULL);                
     ptr->posx=g->x;
     ptr->posy=g->y;
     ptr->width=g->width;   
     ptr->height=g->height;
           
           
    /* stack above */                          
    values[0] =  XCB_STACK_MODE_ABOVE ;
    xcb_configure_window(xconn, id, XCB_CONFIG_WINDOW_STACK_MODE, values);                          

    /*
     * Move cursor into the middle of the window so we don't lose the
     * pointer to another window.
     */
    xcb_warp_pointer(xconn, XCB_NONE, id, 0, 0, 0, 0, 100, 100);
     
}
    
void mr_window_delete (xcb_drawable_t id) {
 
	t_wmnode * ptr = mr_window_locate(id);
	if (ptr != NULL) { 
	  if (ptr->before != NULL && ptr->next != NULL) {
		  ptr->before->next = ptr->next;
		  ptr->next->before = ptr->before;
	  }
	  else {
	    if (ptr->before == NULL && ptr->next == NULL) {
			 whead = NULL;
	         wtail = NULL;
	     } 
	    else {
			if (ptr->before == NULL) {
			 whead = ptr->next;
			 ptr->next->before = NULL;
			}
			else {
			 wtail = ptr->before;
			 ptr->before->next = NULL;
			}
		} /* else */
	  } /* else */
	  free(ptr);	/* free the memory of the node */	  
    } /* if */
   
		
}

/*******************************************************/
/*******************************************************/
/*******************************************************/
void mr_window_set_size(xcb_drawable_t id,uint32_t x, uint32_t y) {
uint32_t values[2];	           
    
  t_wmnode * ptr = mr_window_locate(id);
  if (ptr == NULL) { return; }
  
  fprintf(stderr,"****** Setting size %d x %d\n",x,y);  
    
  values[0] = ptr->width  = x;
  values[1] = ptr->height = y;

   xcb_configure_window(xconn,
                       id,
                       XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, 
                       values);

} 
 
void mr_window_get_size(xcb_drawable_t id,uint32_t * x, uint32_t * y) {
          
  t_wmnode * ptr = mr_window_locate(id);
  if (ptr == NULL) { return; }
  *x=ptr->width;
  *y=ptr->height;
} 

/*******************************************************/
/*******************************************************/
/*******************************************************/
void mr_window_set_pos(xcb_drawable_t id,uint32_t x, uint32_t y) {
uint32_t values[2];

  t_wmnode * ptr = mr_window_locate(id);
  if (ptr == NULL) { return; }
  
  fprintf(stderr,"**** Moving window %d to position %d %d \n",id,x,y);

  values[0] = ptr->posx = x;
  values[1] = ptr->posy = y;
  
  xcb_configure_window(xconn,
	                  id, 
	                  XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, 
	                  values); 	                  
}


void mr_window_get_pos(xcb_drawable_t id,uint32_t * x,uint32_t * y) {
	
  t_wmnode * ptr = mr_window_locate(id);
  if (ptr == NULL) { return;  }
  *x=ptr->posx;
  *y=ptr->posy;
}



/***************************************************************/
/* loop over the list sequentualy trying to find the window id */
t_wmnode * mr_window_locate(xcb_drawable_t id) {
	
   t_wmnode * ptr = whead;
   
   while ( ptr != NULL ) {
	 if (ptr->id == id ) { return ptr; }
	 ptr = ptr->next;   
   }	
  fprintf(stderr,"mr_window_locate.. id %ld  does not exist\n",(long)id); 
  return NULL;
}
/****************************************************************/
