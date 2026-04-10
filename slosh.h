/** 
 * slosh.h
 * 
 * SLOsh - San Luis Obispo Shell
 * CSC 453 - Operating Systems
 * Description: Header file for SLOsh shell implementation. Contains necessary includes,
 * definitions, and function prototypes for the shell.
 * 
 */

#ifndef SLOSH_H
#define SLOSH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

/* Define PATH_MAX if it's not available */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64



#endif /* SLOSH_H */