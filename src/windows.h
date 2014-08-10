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


typedef struct {
	int16_t x;             /* X coordinate. */
    int16_t y;             /* Y coordinate. */
} t_wm_pos;



/* type for the windows node */
typedef struct {
    xcb_drawable_t id; 
    xcb_get_geometry_reply_t size;
    t_wm_pos  pos;
    xcb_size_hints_t hints;
    bool hintsflag;
    int flag_max;          /* flag to indicate if it is maxi'ed */
} t_wmwindow;

typedef struct s_wmnode {
	t_wmwindow window;
	struct s_wmnode * before;
	struct s_wmnode * next;	
} t_wmnode;




/* ********************************* */
/*  Function declarations            */
t_wmnode * mr_window_locate(xcb_drawable_t id); 
void mr_window_delete (xcb_drawable_t id);
void mr_window_add (xcb_drawable_t id);
void mr_window_set_max(xcb_drawable_t id);
xcb_get_geometry_reply_t *  mr_window_get_size(xcb_drawable_t id);
void mr_window_set_size(xcb_drawable_t id,uint32_t x, uint32_t y);
void mr_window_set_position(xcb_drawable_t id,uint32_t x, uint32_t y);
uint32_t mr_window_get_pos_x(xcb_drawable_t id);
uint32_t mr_window_get_pos_y(xcb_drawable_t id);


