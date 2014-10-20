/* XCB Sutff */
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>


/* X11 Keyboard handling */
#include <X11/keysym.h>



/* All our key shortcuts. */
typedef enum {
    KEY_F,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_R,
    KEY_RET,
    KEY_X,
    KEY_TAB,
    KEY_BACKTAB,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_Y,
    KEY_U,
    KEY_B,
    KEY_N,
    KEY_END,
    KEY_PREVSCR,
    KEY_NEXTSCR,
    KEY_ICONIFY,    
    KEY_PREVWS,
    KEY_NEXTWS,
    KEY_MAX
} key_enum_t;


/*
 * Use this modifier combined with other keys to control wm from
 * keyboard. Default is Mod4, which on my keyboard is the Alt key but
 * is usually the Windows key on more normal keyboard layouts.
 */
#define MODKEY XCB_MOD_MASK_4


/*
 * Modifier key to use with mouse buttons. Default Mod1, Meta on my
 * keyboard.
 */
#define MOUSEMODKEY XCB_MOD_MASK_1


/* Extra modifier for resizing. Default is Shift. */
#define SHIFTMOD XCB_MOD_MASK_SHIFT


/* Shortcut key type and initializiation. */
struct keys
{
    xcb_keysym_t keysym;
    xcb_keycode_t keycode;
}; 



/* All keycodes generating our MODKEY mask. */
struct modkeycodes
{
    xcb_keycode_t *keycodes;
    uint32_t len;
}; 



xcb_keycode_t keyboard_keys_get_keycode(int b);
void          keyboard_keys_set_keycode(int b,xcb_keycode_t keycode);

uint32_t	  keyboard_modkey_get_len();
xcb_keycode_t keyboard_modkey_get_keycodes(int b);
struct modkeycodes keyboard_getmodkeys(xcb_mod_mask_t modmask);
xcb_keycode_t keysymtokeycode(xcb_keysym_t keysym,xcb_key_symbols_t *keysyms);
int keyboard_setupkeys(void);
