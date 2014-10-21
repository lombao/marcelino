

/* XCB Sutff */
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>
#include <xcb/randr.h>

void randr_init(xcb_connection_t * conn,xcb_screen_t * screen);
void randr_get(xcb_connection_t * conn,xcb_screen_t * screen);
int randr_get_base();
