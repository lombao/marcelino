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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>

/* *********************** */

#define MAX_WINDOWS_ALLOWED 200 


xcb_atom_t getatom(char *atom_name);
 
 
