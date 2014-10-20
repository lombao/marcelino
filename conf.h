
/*
 * Use this modifier combined with other keys to control wm from
 * keyboard. Default is Mod4, which on my keyboard is the Alt key but
 * is usually the Windows key on more normal keyboard layouts.
 */
#define MODKEY XCB_MOD_MASK_4


/* Extra modifier for resizing. Default is Shift. */
#define SHIFTMOD XCB_MOD_MASK_SHIFT

/*
 * Modifier key to use with mouse buttons. Default Mod1, Meta on my
 * keyboard.
 */
#define MOUSEMODKEY XCB_MOD_MASK_1


/*
 * Do we allow windows to be iconified? Set to true if you want this
 * behaviour to be default. Can also be set by calling mcwm with -i.
 */ 
#define ALLOWICONS false

/*
 * Start these programs when pressing MOUSEMODKEY and mouse buttons on
 * root window.
 */
#define MOUSE1 ""
#define MOUSE2 ""
#define MOUSE3 "mcmenu"


/*
 * Keysym codes for window operations. Look in X11/keysymdefs.h for
 * actual symbols. Use XK_VoidSymbol to disable a function.
 */
#define USERKEY_FIX 		XK_F
#define USERKEY_MOVE_LEFT 	XK_H
#define USERKEY_MOVE_DOWN 	XK_J
#define USERKEY_MOVE_UP 	XK_K
#define USERKEY_MOVE_RIGHT 	XK_L
#define USERKEY_MAXVERT 	XK_M
#define USERKEY_RAISE 		XK_R
#define USERKEY_TERMINAL 	XK_Return
#define USERKEY_MAX 		XK_X
#define USERKEY_CHANGE 		XK_Tab
#define USERKEY_BACKCHANGE	XK_VoidSymbol
#define USERKEY_WS1				XK_1
#define USERKEY_WS2				XK_2
#define USERKEY_WS3				XK_3
#define USERKEY_WS4				XK_4
#define USERKEY_WS5				XK_5
#define USERKEY_WS6				XK_6
#define USERKEY_WS7				XK_7
#define USERKEY_WS8				XK_8
#define USERKEY_WS9				XK_9
#define USERKEY_WS10			XK_0
#define USERKEY_PREVWS          XK_C
#define USERKEY_NEXTWS          XK_V
#define USERKEY_TOPLEFT         XK_Y
#define USERKEY_TOPRIGHT        XK_U
#define USERKEY_BOTLEFT         XK_B
#define USERKEY_BOTRIGHT        XK_N
#define USERKEY_DELETE          XK_End
#define USERKEY_PREVSCREEN      XK_comma
#define USERKEY_NEXTSCREEN      XK_period
#define USERKEY_ICONIFY         XK_I

/* Number of workspaces. */
/* Honestly, I think 10 is more than enough for anybody
 * I don't think is necessary for this to be user configurable
 * option
 */
#define WORKSPACES 10


/**********************************/
#ifdef DEBUG
#define PDEBUG(Args...) \
  do { fprintf(stderr, "marcelino: "); fprintf(stderr, ##Args); } while(0)
#define D(x) x
#else
#define PDEBUG(Args...)
#define D(x)
#endif



uint32_t conf_get_borderwidth(void);
char * conf_get_focuscol(void);
char * conf_get_unfocuscol(void);
char * conf_get_fixedcol(void);
bool     conf_get_allowicons(void);
char *   conf_get_terminal(void);
uint32_t conf_get_movestep(void);


void conf_determine_user_configfile(int argc , char ** argv)
void conf_upload_conf_file(char * cfgfile);
void conf_set( uint16_t p, char * val);
void conf_upload_default_cfg_global_file(void);
