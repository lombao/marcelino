
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>

#include <X11/keysym.h>

#include <xcb/xproto.h>
#include <xcb/xcb_util.h>


/* Internal Constants. */

/* We're currently moving a window with the mouse. */
#define MCWM_MOVE 2

/* We're currently resizing a window with the mouse. */
#define MCWM_RESIZE 3

/*
 * We're currently tabbing around the window list, looking for a new
 * window to focus on.
 */
#define MCWM_TABBING 4

/* Number of workspaces. */
#define WORKSPACES 10

/* Value in WM hint which means this window is fixed on all workspaces. */
#define NET_WM_FIXED 0xffffffff

/* This means we didn't get any window hint at all. */
#define MCWM_NOWS 0xfffffffe

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

struct sizepos
{
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
};
    
/*
 * Workspace list: Every workspace has a list of all visible
 * windows.
 */
struct item *wslist[WORKSPACES] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

struct monitor
{
    xcb_randr_output_t id;
    char *name;
    uint16_t x;                 /* X and Y. */
    uint16_t y;                 
    uint16_t width;     /* Width in pixels. */
    uint16_t height;    /* Height in pixels. */
    struct item *item; /* Pointer to our place in output list. */    
};

/* Window configuration data. */
struct winconf
{
    uint16_t      x;
    uint16_t      y;
    uint16_t     width;
    uint16_t     height;
    uint8_t      stackmode;
    xcb_window_t sibling;    
    uint16_t     borderwidth;
};


/* Everything we know about a window. */
struct client
{
    xcb_drawable_t id;          /* ID of this window. */
    bool usercoord;             /* X,Y was set by -geom. */
    uint16_t x;                 /* X coordinate. */
    uint16_t y;                 /* Y coordinate. */
    uint16_t width;             /* Width in pixels. */
    uint16_t height;            /* Height in pixels. */
    struct sizepos origsize;    /* Original size if we're currently maxed. */
    uint16_t min_width, min_height; /* Hints from application. */
    uint16_t max_width, max_height;
    int32_t width_inc, height_inc;
    int32_t base_width, base_height;
    bool vertmaxed;             /* Vertically maximized? */
    bool maxed;                 /* Totally maximized? */
    bool fixed;           /* Visible on all workspaces? */
    struct monitor *monitor;    /* The physical output this window is on. */
    struct item *winitem; /* Pointer to our place in global windows list. */
    struct item *wsitem[WORKSPACES]; /* Pointer to our place in every
                                             * workspace window list. */
};



static struct client *setupwin(xcb_window_t win);
static void finishtabbing(void);
static struct modkeycodes getmodkeys(xcb_mod_mask_t modmask);
static void cleanup(int code);
static void arrangewindows(void);
static void setwmdesktop(xcb_drawable_t win, uint32_t ws);
static int32_t getwmdesktop(xcb_drawable_t win);
static void addtoworkspace(struct client *client, uint32_t ws);
static void delfromworkspace(struct client *client, uint32_t ws);
static void changeworkspace(uint32_t ws);
static void fixwindow(struct client *client, bool setcolour);
static void forgetclient(struct client *client);
static void forgetwin(xcb_window_t win);
static void fitonscreen(struct client *client);
static void newwin(xcb_window_t win);

static xcb_keycode_t keysymtokeycode(xcb_keysym_t keysym,
                                     xcb_key_symbols_t *keysyms);
static int setupkeys(void);



void arrbymon(struct monitor *monitor);
static struct monitor *findmonitor(xcb_randr_output_t id);
static struct monitor *findclones(xcb_randr_output_t id, int16_t x, int16_t y);
static struct monitor *findmonbycoord(int16_t x, int16_t y);
static void delmonitor(struct monitor *mon);
static struct monitor *addmonitor(xcb_randr_output_t id, char *name, 
                                  uint32_t x, uint32_t y, uint16_t width,
                                  uint16_t height);
static void raisewindow(xcb_drawable_t win);
static void raiseorlower(struct client *client);
static void movelim(struct client *client);
static void movewindow(xcb_drawable_t win, uint16_t x, uint16_t y);
static struct client *findclient(xcb_drawable_t win);
static void focusnext(bool reverse);
static void setunfocus(xcb_drawable_t win);
static void setfocus(struct client *client);
static int start(char *program);
static void resizelim(struct client *client);
static void moveresize(xcb_drawable_t win, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height);
static void resize(xcb_drawable_t win, uint16_t width, uint16_t height);
static void resizestep(struct client *client, char direction);
static void mousemove(struct client *client, int rel_x, int rel_y);
static void mouseresize(struct client *client, int rel_x, int rel_y);
static void movestep(struct client *client, char direction);
static void setborders(struct client *client, int width);
static void unmax(struct client *client);
static void maximize(struct client *client);
static void maxvert(struct client *client);
static void hide(struct client *client);
static bool getpointer(xcb_drawable_t win, int16_t *x, int16_t *y);
static bool getgeom(xcb_drawable_t win, int16_t *x, int16_t *y, uint16_t *width,
                    uint16_t *height);
static void topleft(void);
static void topright(void);
static void botleft(void);
static void botright(void);
static void deletewin(void);
static void prevscreen(void);
static void nextscreen(void);
static void handle_keypress(xcb_key_press_event_t *ev);
static void configwin(xcb_window_t win, uint16_t mask, struct winconf wc);
static void configurerequest(xcb_configure_request_event_t *e);
static void events(void);
