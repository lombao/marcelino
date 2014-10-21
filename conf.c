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
#include <ctype.h>

#include "conf.h"

#define MAX_PATH 256
#define MAX_CFG_LINE 128
#define MAX_LEN_COLOR_NAME 128

#define PARAM_MOVE_STEP     		0
#define PARAM_TERMINAL      		1
#define PARAM_COLOR_FOCUS_WINDOW    2
#define PARAM_COLOR_UNFOCUS_WINDOW  3
#define PARAM_COLOR_FIXED_WINDOW    4
#define PARAM_BORDER_WIDTH          5
#define PARAM_NUM_WORKSPACES        6


char * paramstrings[] = {
	                        "MOVE_STEP",
							"TERMINAL",
							"COLOR_FOCUS_WINDOW",
							"COLOR_UNFOCUS_WINDOW",
							"COLOR_FIXED_WINDOW",
							"BORDER_WIDTH",
							"WORKSPACES"
						};
						

/****************************************/
/* Default user config file             */
char cfgfile[MAX_PATH]="~/.config/marcelino.cfg";
						
/****************************************/						
/* configuration variables and defaults */

char terminal[MAX_PATH]="/usr/bin/xterm";   /* path to the terminal console */
char colorfocus[MAX_LEN_COLOR_NAME]="chocolate1";
char colorunfocus[MAX_LEN_COLOR_NAME]="grey40";
char colorfixed[MAX_LEN_COLOR_NAME]="grey90";
uint32_t movesteppixels    = 32;         /* pixels for each moved step when moving windows */
uint32_t borderwidth = 1;          /* Do we draw borders? If so, how large? */    
bool     allowicons   = false;      /* Allow windows to be unmapped. */ 
uint32_t workspaces   = 4;   


/******************************************/
uint32_t 	conf_get_borderwidth(void)      { return borderwidth; }
char *    	conf_get_focuscol(void)         { return colorfocus; }
char *    	conf_get_unfocuscol(void)       { return colorunfocus; }
char *    	conf_get_fixedcol(void)         { return colorfixed; }
uint32_t 	conf_get_movestep(void)         { return movesteppixels; }
bool       	conf_get_allowicons(void)       { return allowicons; }
char *    	conf_get_terminal(void)         { return terminal; }
uint32_t  	conf_get_workspaces(void)      	{ return workspaces; }



/**************************************************************/
void conf_determine_user_configfile(int argc , char ** argv) {

char ch;
    		
	while (1)
    {
        ch = getopt(argc, argv, "c:");
        if (-1 == ch)
        {             
            /* No more options, break out of while loop. */
            break;
        }       
        switch (ch)
        {           
        case 'c':
		    strncpy(cfgfile,optarg,MAX_PATH);	
            break;
        default:
             printf("marcelino: Usage: marcelino [-c config-file] \n");
             printf("  If not config file provided it wil read first /etc/marcelino.cfg\n");
             printf("  and then $HOME/.config/marcelino.cfg\n");
             exit(0);
             break;
        } /* switch */
    }
   
   if (access(cfgfile,R_OK)==0) { conf_upload_conf_file(cfgfile); }
}


/*********************************************************/
void conf_upload_default_cfg_global_file(void) {
  	conf_upload_conf_file("/etc/marcelino.cfg");
}

/*********************************************************/
void conf_upload_conf_file(char * cfgfile) {
 
 FILE * fp;
 char line[MAX_CFG_LINE];
 char * linetrim;
 uint16_t counterline = 0;
 uint16_t a;
 

   fp = fopen(cfgfile,"r");
   if(fp == NULL) {
      fprintf(stderr,"Cannot open configuration file: %s.. Applying defaults.",cfgfile);
      exit(1);
   }  
   
   while(fgets(line, MAX_CFG_LINE, fp) != NULL)  {
	/* We increase line counter */
	counterline++;
	
	/* Trim initial whitespaces */
    linetrim=line;
    while(isspace(*linetrim)) linetrim++;
    
    /* If it's a comment line then ignore */
    if (*linetrim == '#' ) { continue; }
    
    char * key   = strtok(linetrim," ");
    char * value = strtok(NULL," ");
    
    if ( key   == NULL ) { continue; }
    if ( value == NULL ) { fprintf(stderr,"Syntax error in line number %d\n",counterline); exit(1); }
        
    char * valuetrim=value;
    while(*valuetrim != 0x0 ) { if (*valuetrim == '\n') { *valuetrim=0x0; break; } valuetrim++;}
		   
    for (a=0;a<=PARAM_BORDER_WIDTH;a++) {
      if (strncmp(paramstrings[a],key,strlen(paramstrings[a])) == 0) { conf_set(a,value); }
    }
		
   } /* end while */
   		
}


/******************************************************/
void conf_set( uint16_t p, char * val) {
  switch (p) {
	  case PARAM_MOVE_STEP: 			movesteppixels=strtol(val,NULL,10);
										break;
	  case PARAM_TERMINAL:  			strncpy(terminal,val,MAX_PATH);
										break;
	  case PARAM_BORDER_WIDTH: 			borderwidth=strtol(val,NULL,10);
										break;
	  case PARAM_COLOR_FOCUS_WINDOW: 	strncpy(colorfocus,val,MAX_LEN_COLOR_NAME);
										break;
	  case PARAM_COLOR_UNFOCUS_WINDOW: 	strncpy(colorunfocus,val,MAX_LEN_COLOR_NAME);
										break;
	  case PARAM_COLOR_FIXED_WINDOW: 	strncpy(colorfixed,val,MAX_LEN_COLOR_NAME);
										break;
	  case PARAM_NUM_WORKSPACES: 	  	workspaces=strtol(val,NULL,10);
										break;
  }	
}
