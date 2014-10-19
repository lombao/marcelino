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

/* CORE includes */
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



#define MAX_SIZE_TERMINAL 128

/**************************************/
char terminal[MAX_SIZE_TERMINAL+1];    /* Path to terminal to start. */
         


uint32_t borderwidth;            /* Do we draw borders? If so, how large? */    
uint32_t focuscol;          /* Focused border colour. */
uint32_t unfocuscol;        /* Unfocused border colour.  */
uint32_t fixedcol;          /* Fixed windows border colour. */

bool     allowicons;            /* Allow windows to be unmapped. */    


void     conf_set_borderwidth(uint32_t b) { borderwidth = b; }
uint32_t conf_get_borderwidth(void)       { return borderwidth; }


void     conf_set_focuscol(uint32_t b)   { focuscol = b; }
uint32_t conf_get_focuscol(void)         { return focuscol; }


void     conf_set_unfocuscol(uint32_t b)   { unfocuscol = b; }
uint32_t conf_get_unfocuscol(void)         { return unfocuscol; }

void     conf_set_fixedcol(uint32_t b)   { fixedcol = b; }
uint32_t conf_get_fixedcol(void)         { return fixedcol; }

void     conf_set_allowicons(bool b)   { allowicons = b; }
bool     conf_get_allowicons(void)         { return allowicons; }

void     conf_set_terminal(char * b)       { strncpy(terminal,b,MAX_SIZE_TERMINAL); terminal[MAX_SIZE_TERMINAL]=0;}
char *   conf_get_terminal(void)           { return terminal; }



