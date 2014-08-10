/*******************************************************/
/* MARCELINO. Yet another simplistic WM using XCB in C */
/* ----------------------------------------------------*/
/* Auxiliar functions                                  */
/* --------------------------------------------------- */
/* Author: Cesar Lombao                                */
/* Date:   July 2014                                   */
/* License: GPLv2                                      */
/*******************************************************/

#include "aux.h"

void * mr_aux_malloc(uint32_t s) {
  void * ptr;
  
    ptr = malloc(s);	  
    if ( ptr == NULL) {
	  fprintf(stderr,">>>> memory out, missing %d bytes\n",s);
	  fprintf(stderr,">>>> let's try again\n");
	  ptr = malloc(s);	  
      if ( ptr == NULL) {
	    fprintf(stderr,">>>> memory out by second time, missing %d bytes\n",s);
        exit (1);
      }
	}
	
  return ptr;
}
