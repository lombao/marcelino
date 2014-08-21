/*******************************************************/
/* MARCELINO. Yet another simplistic WM using XCB in C */
/* ----------------------------------------------------*/
/* GLOBAL Declarations                                 */
/*   Here we have the data types and other definitions */
/*   which are relevant trough all the WM              */
/* --------------------------------------------------- */
/* Author: Cesar Lombao                                */
/* Date:   July 2014                                   */
/* License: GPLv2                                      */
/*******************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>


/* ********************** */
/* The WM Current status  */
typedef struct 
{
  xcb_connection_t *xconn;  /* the XCB Connection */
  xcb_screen_t     *screen; /* The screen info    */  
  int              mode;    /* Flag to say if we are moving or resizing */	
  uint32_t pixel;
  uint32_t pointerx;
  uint32_t pointery;
} t_wmstatus;


/* *********************** */


/* Global definitions */
#define MODE_MOVE 2   /* We're currently moving a window with the mouse.   */
#define MODE_RESIZE 3 /* We're currently resizing a window with the mouse. */

#define MAX_WINDOWS_ALLOWED 200 
