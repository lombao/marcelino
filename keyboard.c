/*
 * MARCELINO: a small window manager using XCB
 * 
 *
 * Copyright (c) 2014 Cesar Lombao
 *
 * Copyright (c) 2010, 2011, 2012 Michael Cardell Widerkrantz
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
 
 
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <assert.h>


#include "conf.h"
#include "server.h"
#include "keyboard.h"




static struct keys keys[KEY_MAX] =
{
    { USERKEY_FIX, 0 },
    { USERKEY_MOVE_LEFT, 0 },
    { USERKEY_MOVE_DOWN, 0 },
    { USERKEY_MOVE_UP, 0 },
    { USERKEY_MOVE_RIGHT, 0 },
    { USERKEY_MAXVERT, 0 },
    { USERKEY_RAISE, 0 },
    { USERKEY_TERMINAL, 0 },
    { USERKEY_MAX, 0 },
    { USERKEY_CHANGE, 0 },
    { USERKEY_BACKCHANGE, 0 },
    { USERKEY_WS1, 0 },
    { USERKEY_WS2, 0 },
    { USERKEY_WS3, 0 },
    { USERKEY_WS4, 0 },
    { USERKEY_WS5, 0 },
    { USERKEY_WS6, 0 },
    { USERKEY_WS7, 0 },
    { USERKEY_WS8, 0 },
    { USERKEY_WS9, 0 },
    { USERKEY_WS10, 0 },
    { USERKEY_TOPLEFT, 0 },
    { USERKEY_TOPRIGHT, 0 },
    { USERKEY_BOTLEFT, 0 },
    { USERKEY_BOTRIGHT, 0 },
    { USERKEY_DELETE, 0 },
    { USERKEY_PREVSCREEN, 0 },
    { USERKEY_NEXTSCREEN, 0 },
    { USERKEY_ICONIFY, 0 },    
    { USERKEY_PREVWS, 0 },
    { USERKEY_NEXTWS, 0 },
};    


static struct modkeycodes modkeys = {	NULL, 0	};



/***************************************************/
xcb_keycode_t keyboard_keys_get_keycode(int b) 						 { return keys[b].keycode; }
void          keyboard_keys_set_keycode(int b,xcb_keycode_t keycode) {  keys[b].keycode=keycode ; }

uint32_t	  keyboard_modkey_get_len() { return modkeys.len; }
xcb_keycode_t keyboard_modkey_get_keycodes(int b) { return modkeys.keycodes[b]; }

/*
 * Find out what keycode modmask is bound to. Returns a struct. If the
 * len in the struct is 0 something went wrong.
 */
struct modkeycodes keyboard_getmodkeys(xcb_mod_mask_t modmask)
{
    xcb_get_modifier_mapping_cookie_t cookie;
    xcb_get_modifier_mapping_reply_t *reply;
    xcb_keycode_t *modmap;
    struct modkeycodes keycodes = {
        NULL,
        0
    };
    int mask;
    unsigned i;
    const xcb_mod_mask_t masks[8] = { XCB_MOD_MASK_SHIFT,
                                      XCB_MOD_MASK_LOCK,
                                      XCB_MOD_MASK_CONTROL,
                                      XCB_MOD_MASK_1,
                                      XCB_MOD_MASK_2,
                                      XCB_MOD_MASK_3,
                                      XCB_MOD_MASK_4,
                                      XCB_MOD_MASK_5 };

    cookie = xcb_get_modifier_mapping_unchecked(server_get_conn());

    if ((reply = xcb_get_modifier_mapping_reply(server_get_conn(), cookie, NULL)) == NULL)
    {
        return keycodes;
    }

    if (NULL == (keycodes.keycodes = calloc(reply->keycodes_per_modifier,
                                            sizeof (xcb_keycode_t))))
    {
        PDEBUG("Out of memory.\n");
        return keycodes;
    }
    
    modmap = xcb_get_modifier_mapping_keycodes(reply);

    /*
     * The modmap now contains keycodes.
     *
     * The number of keycodes in the list is 8 *
     * keycodes_per_modifier. The keycodes are divided into eight
     * sets, with each set containing keycodes_per_modifier elements.
     *
     * Each set corresponds to a modifier in masks[] in the order
     * specified above.
     *
     * The keycodes_per_modifier value is chosen arbitrarily by the
     * server. Zeroes are used to fill in unused elements within each
     * set.
     */
    for (mask = 0; mask < 8; mask ++)
    {
        if (masks[mask] == modmask)
        {
            for (i = 0; i < reply->keycodes_per_modifier; i ++)
            {
                if (0 != modmap[mask * reply->keycodes_per_modifier + i])
                {
                    keycodes.keycodes[i]
                        = modmap[mask * reply->keycodes_per_modifier + i];
                    keycodes.len ++;
                }
            }
            
            PDEBUG("Got %d keycodes.\n", keycodes.len);
        }
    } /* for mask */
    
    free(reply);
    
    return keycodes;
}



/*
 * Get a keycode from a keysym.
 *
 * Returns keycode value. 
 */
xcb_keycode_t keysymtokeycode(xcb_keysym_t keysym, xcb_key_symbols_t *keysyms)
{
    xcb_keycode_t *keyp;
    xcb_keycode_t key;

    /* We only use the first keysymbol, even if there are more. */
    keyp = xcb_key_symbols_get_keycode(keysyms, keysym);
    if (NULL == keyp)
    {
        fprintf(stderr, "mcwm: Couldn't look up key. Exiting.\n");
        exit(1);
        return 0;
    }

    key = *keyp;
    free(keyp);
    
    return key;
}


/*
 * Set up all shortcut keys.
 *
 * Returns 0 on success, non-zero otherwise. 
 */
int keyboard_init(xcb_connection_t * conn)
{
    xcb_key_symbols_t *keysyms;
    unsigned i;
    
    /* Get all the keysymbols. */
    keysyms = xcb_key_symbols_alloc(conn);

    /*
     * Find out what keys generates our MODKEY mask. Unfortunately it
     * might be several keys.
     */
    if (NULL != modkeys.keycodes)
    {
        free(modkeys.keycodes);
    }
    modkeys = keyboard_getmodkeys(MODKEY);

    if (0 == modkeys.len)
    {
        fprintf(stderr, "We couldn't find any keycodes to our main modifier "
                "key!\n");
        return -1;
    }

    for (i = 0; i < modkeys.len; i ++)
    {
        /*
         * Grab the keys that are bound to MODKEY mask with any other
         * modifier.
         */
        xcb_grab_key(conn, 1, server_get_root(), XCB_MOD_MASK_ANY,
                     modkeys.keycodes[i],
                     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);    
    }
    
    /* Now grab the rest of the keys with the MODKEY modifier. */
    for (i = KEY_F; i < KEY_MAX; i ++)
    {
	if (XK_VoidSymbol == keys[i].keysym)
	{
	    keys[i].keycode = 0;
	    continue;
	}

        keys[i].keycode = keysymtokeycode(keys[i].keysym, keysyms);        
        if (0 == keys[i].keycode)
        {
            /* Couldn't set up keys! */
    
            /* Get rid of key symbols. */
            xcb_key_symbols_free(keysyms);

            return -1;
        }
            
        /* Grab other keys with a modifier mask. */
        xcb_grab_key(conn, 1, server_get_root(), MODKEY, keys[i].keycode,
                     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

        /*
         * XXX Also grab it's shifted counterpart. A bit ugly here
         * because we grab all of them not just the ones we want.
         */
        xcb_grab_key(conn, 1, server_get_root(), MODKEY | SHIFTMOD,
                     keys[i].keycode,
                     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    } /* for */


    
    /* Get rid of the key symbols table. */
    xcb_key_symbols_free(keysyms);
    
    return 0;
}

