

#include <xcb/randr.h>

int setuprandr(void);
void getrandr(void);
void getoutputs(xcb_randr_output_t *outputs, int len,xcb_timestamp_t timestamp);
