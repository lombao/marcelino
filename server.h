

/* XCB Sutff */
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>


xcb_connection_t * server_get_conn(void);
xcb_screen_t *     server_get_screen(void);
xcb_drawable_t     server_get_root(void);
void server_flush(void);
void server_init(void);
void server_disconnect(void);
xcb_atom_t getatom(char *atom_name);
int server_setupscreen(void);
void server_mouse_init(xcb_connection_t * conn,xcb_screen_t * screen);
void server_subscribe_to_events(xcb_connection_t * conn,xcb_screen_t * screen);
void server_exit_cleanup(int code);
uint32_t server_getcolor(const char *colstr);

