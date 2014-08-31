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

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>


#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>



/* type for the windows node */
typedef struct s_wmnode {
    xcb_drawable_t id; 
    uint32_t posx;
    uint32_t posy;
    uint32_t width;
    uint32_t height;
    struct s_wmnode * next;
    struct s_wmnode * before;
} t_wmnode;



 

  extern xcb_connection_t  *xconn;  /* the XCB Connection */
  extern xcb_screen_t      *xscreen; /* The screen info    */  

  extern uint32_t	plastx;
  extern uint32_t	plasty; 
  extern uint32_t   pixelcolor;
  
extern xcb_atom_t atom_desktop;        
extern xcb_atom_t wm_delete_window;    /* WM_DELETE_WINDOW event to close windows.  */
extern xcb_atom_t wm_change_state;
extern xcb_atom_t wm_state;
extern xcb_atom_t wm_protocols;        /* WM_PROTOCOLS.  */

/* ********************************* */
/*  Function declarations            */
t_wmnode * mr_window_locate(xcb_drawable_t id); 
void mr_window_add (xcb_drawable_t id);
void mr_window_delete (xcb_drawable_t id);
void mr_window_set_size(xcb_drawable_t id,uint32_t   x, uint32_t   y);
void mr_window_get_size(xcb_drawable_t id,uint32_t * x, uint32_t * y);
void mr_window_set_pos(xcb_drawable_t id,uint32_t   x,uint32_t   y);
void mr_window_get_pos(xcb_drawable_t id,uint32_t * x,uint32_t * y);



