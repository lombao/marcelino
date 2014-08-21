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


#include "marcelino.h"
#include "windows.h"
#include "aux.h"


extern t_wmstatus wmstatus;

/* Global variables */
t_wmnode * head_w_list = NULL;
t_wmnode * tail_w_list = NULL;


/* This is like a create or init or new in OOP */
/* but if a window id is already registered it will not create
 * another entry */
void mr_window_add (xcb_drawable_t id) {
  
    t_wmnode * ptr = mr_window_locate(id);
    if (ptr != NULL) { fprintf(stderr,"tried to add a window that alerady existed\n");
		               return; }
  
    ptr=mr_aux_malloc(sizeof (t_wmnode));	    
    ptr->window.id = id;
    ptr->window.hintsflag = false;
    
    ptr->before = NULL;
    ptr->next   = NULL;
    if (head_w_list == NULL) {
	 head_w_list = ptr;
	 tail_w_list = ptr;
	}
	else {
	 tail_w_list->next = ptr;
	 ptr->before = tail_w_list;
	 tail_w_list = ptr;	
	}
    
    fprintf(stderr,"hemos alocado memoria para el registro de la ventana nueva\n");
      
    /* take hints */
    if (xcb_icccm_get_wm_normal_hints_reply(
            wmstatus.xconn, xcb_icccm_get_wm_normal_hints_unchecked(
                wmstatus.xconn, id), &ptr->window.hints, NULL))
    {
      ptr->window.hintsflag = true;
      fprintf(stderr,"el hintsflag es cierto\n");
    }
    
    /* Set border color and width border */   
    uint32_t values[2];
    values[0]=wmstatus.pixel;	
    xcb_change_window_attributes(wmstatus.xconn, id, XCB_CW_BORDER_PIXEL, values);
	values[0]=8;	
    xcb_configure_window(wmstatus.xconn, id, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
    
    
    /* take geometry */
    xcb_get_geometry_reply_t * g =
                        xcb_get_geometry_reply(wmstatus.xconn, 
                                               xcb_get_geometry(wmstatus.xconn, id),
                                               NULL);
    
    
    fprintf(stderr,"la geometria a palo seco es %d x %d\n",g->width,g->height);                                        
    if (ptr->window.hintsflag && (ptr->window.hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE))
     {
      g->width =(g->width > ptr->window.hints.max_width)?ptr->window.hints.max_width:g->width;
      g->height=(g->height > ptr->window.hints.max_height)?ptr->window.hints.max_height:g->height;
     }
    if (ptr->window.hintsflag && (ptr->window.hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE))
     {
	  g->width =(g->width < ptr->window.hints.min_width)?ptr->window.hints.min_width:g->width;
      g->height=(g->height < ptr->window.hints.min_height)?ptr->window.hints.min_height:g->height;
     }
    /* 
    g->width = 600; 
    g->height = 600; 
    */ 
    mr_window_set_size(id,g->width,g->height);
    mr_window_set_position(id,g->x,g->y);
    free(g);
    

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
			 head_w_list = NULL;
	         tail_w_list = NULL;
	     }
	    else {
			if (ptr->before == NULL) {
			 head_w_list = ptr->next;
			 ptr->next->before = NULL;
			}
			else {
			 tail_w_list = ptr->before;
			 ptr->before->next = NULL;
			}
		} /* else */
	  } /* else */
	  free(ptr);	/* free the memory of the node */	  
    } /* if */
   
		
}



/*******************************************************/
/* DEALING WITH SIZE */
xcb_get_geometry_reply_t *  mr_window_get_size(xcb_drawable_t id) {
	
  t_wmnode * ptr = mr_window_locate(id);
  if (ptr == NULL) { return NULL; } 
  return (&ptr->window.size);
}	

void mr_window_set_size(xcb_drawable_t id,uint32_t x, uint32_t y) {
uint32_t values[2];	           
    
  t_wmnode * ptr = mr_window_locate(id);
  if (ptr == NULL) { fprintf (stderr,"no existe esa ventana\n"); return; }
  
  ptr->window.size.width = x;
  ptr->window.size.height = y;

  fprintf(stderr,"****** Setting size %d x %d\n",x,y);  
  values[0] = x; values[1] = y;
  xcb_configure_window(wmstatus.xconn,
                       id,
                       XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, 
                       values);

} 



/*******************************************************/

void mr_window_set_position(xcb_drawable_t id,uint32_t x, uint32_t y) {
uint32_t values[2];

  t_wmnode * ptr = mr_window_locate(id);
  if (ptr == NULL) { return; }

  ptr->window.size.x = x;
  ptr->window.size.y = y;
  
  values[0]=x; values[1]=y;	
  xcb_configure_window(wmstatus.xconn,
	                  id, 
	                  XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, 
	                  values); 
	                  
  xcb_flush(wmstatus.xconn);
}



uint32_t mr_window_get_pos_x(xcb_drawable_t id) {
  t_wmnode * ptr = mr_window_locate(id);
  if (ptr == NULL) { return 0; }
  return ptr->window.size.x;
  
}

uint32_t mr_window_get_pos_y(xcb_drawable_t id) {
  t_wmnode * ptr = mr_window_locate(id);
  if (ptr == NULL) { return 0; }
  return ptr->window.size.y;
  
}



/***************************************************************/
/* loop over the list sequentualy trying to find the window id */
t_wmnode * mr_window_locate(xcb_drawable_t id) {
   t_wmnode * ptr = head_w_list;
   
   while ( ptr != NULL ) {
	 if (ptr->window.id == id ) { return ptr; }
	 ptr = ptr->next;   
   }	
  fprintf(stderr,"en el mr_window_locate nos han pedido un id que  no existe\n"); 
  return NULL;
}
/****************************************************************/
