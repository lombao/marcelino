/*******************************************************/
/* MARCELINO. Yet another simplistic WM using XCB in C */
/* ----------------------------------------------------*/
/* Author: Cesar Lombao
 * Date:   July 2014 
 * License: GPLv2
*/



#include "marcelino.h"
#include "events.h"


  xcb_connection_t  *xconn;  /* the XCB Connection */
  xcb_screen_t      *xscreen; /* The screen info    */  	

  uint32_t pixelcolor;
 
 
xcb_atom_t atom_desktop;        
xcb_atom_t wm_delete_window;    /* WM_DELETE_WINDOW event to close windows.  */
xcb_atom_t wm_change_state;
xcb_atom_t wm_state;
xcb_atom_t wm_protocols;        /* WM_PROTOCOLS.  */

  

/***************************************
 * Get a defined atom from the X server.
 *****************************************/ 
xcb_atom_t getatom(char *atom_name)
{
    xcb_intern_atom_cookie_t atom_cookie;
    xcb_atom_t atom;
    xcb_intern_atom_reply_t *rep;
    
    atom_cookie = xcb_intern_atom(xconn, 0, strlen(atom_name), atom_name);
    rep = xcb_intern_atom_reply(xconn, atom_cookie, NULL);
    if (NULL != rep)
    {
        atom = rep->atom;
        free(rep);
        return atom;
    }

    /*
     * XXX Note that we return 0 as an atom if anything goes wrong.
     * Might become interesting.
     */
    return 0;
} 
 
  
/* **************************************************** */
/* MAIN *********************************************** */
/* **************************************************** */
int main ()
 {
    
    /* Connects to the X Server  */
    xconn = xcb_connect(NULL, NULL);
    
    if  ( xcb_connection_has_error(xconn)) {
      fprintf(stderr,"Cannot connect to the X Server. Abort..\n");
	}

    /* Get the data of the first screen, the root screen */
    xscreen = xcb_setup_roots_iterator(xcb_get_setup(xconn)).data;
    fprintf(stderr,"The root window is number %ld\n",(long)xscreen->root);

    /************************************************/
    const char * colstr = "blue";
	xcb_alloc_named_color_reply_t * col_reply;    
    xcb_colormap_t colormap; 
    xcb_generic_error_t *error;
    xcb_alloc_named_color_cookie_t colcookie;
    colormap = xscreen->default_colormap;
    colcookie = xcb_alloc_named_color(xconn, colormap, strlen(colstr), colstr);
    col_reply = xcb_alloc_named_color_reply(xconn, colcookie, &error);
    pixelcolor = col_reply->pixel;
    free(col_reply);
    /**************************************************/
 

                                                           
    atom_desktop = getatom("_NET_WM_DESKTOP");
    wm_delete_window = getatom("WM_DELETE_WINDOW");
    wm_change_state = getatom("WM_CHANGE_STATE");
    wm_state = getatom("WM_STATE");
    wm_protocols = getatom("WM_PROTOCOLS");
           

    /* *********************************************************** */
    /* Tho subscribe the WM to the events we want to listen        */
    const uint32_t  mask   = XCB_CW_EVENT_MASK;
    const uint32_t  values =  XCB_EVENT_MASK_STRUCTURE_NOTIFY |	XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                              XCB_EVENT_MASK_BUTTON_PRESS     | XCB_EVENT_MASK_BUTTON_RELEASE;
	
    xcb_void_cookie_t cookie =
        xcb_change_window_attributes_checked(xconn, xscreen->root, mask, &values);
    xcb_generic_error_t * xerror = xcb_request_check(xconn, cookie);
    
    xcb_warp_pointer(xconn, XCB_NONE, xscreen->root, 0, 0, 0, 0, 400, 400); 
    
    xcb_flush(xconn); /* We want all the settings to take effect before going into the loop */
    
    if (NULL != xerror)
    {
        fprintf(stderr, "Can't get SUBSTRUCTURE REDIRECT. "
                "Error code: %d\n"
                "Another window manager running? Exiting.\n",
                error->error_code);

        xcb_disconnect(xconn);
        
        exit(1);
    }
    
    
    /*************/
    /* Main loop */
     
    xcb_generic_event_t *ev;
    while( (ev = xcb_wait_for_event(xconn)) ) { 
	  mr_events_execute_callback(ev);
	  free(ev); /* free memory */
	}
     
  /* This is not the way to get out, there is more stuff to do */
  /* For the time being is enough, anyway this is not working yet so, who 
   * cares how does it exit */  
  xcb_disconnect(xconn); /* this line should never be executed anyway */
  return 0;
 } 
