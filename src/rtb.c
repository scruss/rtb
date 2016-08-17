/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * rtb.c:
 *	The main startup/shutdown code.
 ***********************************************************************
 * This file is part of RTB:
 *	Return To Basic
 *	http://projects.drogon.net/return-to-basic
 *
 *    RTB is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    RTB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RTB.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <SDL/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <termios.h>
#include <sys/time.h>
#include <pthread.h>

#include <unistd.h>

#ifdef	RASPBERRY_PI
#   warning Raspberry Pi
#  include <wiringPi.h>
#endif

#include "bool.h"
#include "bomb.h"

#include "readline.h"
#include "screenKeyboard.h"

#include "lines.h"
#include "keywords.h"
#include "symbolTable.h"
#include "parseInput.h"
#include "cycle.h"
#include "goto.h"
#include "procFn.h"
#include "fileFns.h"
#include "serial.h"

#include "numFns.h"
#include "spriteFns.h"

#include "load.h"
#include "commands.h"
#include "rtb.h"


// Globals

int gotSyntaxError = FALSE ;
int exitRTB        = FALSE ;

int escapePressed  = FALSE ;
int checkForEscape = FALSE ;

struct timeval startTime ;

char *savedFilename = NULL ;		// Used for repeated saves

int rootRevoked = FALSE ;

/*
 * angleConversion:
 *	Used to convert from Degrees, Radians and Clock.
 *	Internally, the cLib uses Radians.
 *	We'll default to degrees
 *********************************************************************************
 */

double angleConversion = M_PI / 180.0 ;

int programChanged = FALSE ;

// Line number tracking

int programRunning ;
int linePtr ;
int continuePtr ;

// DATA/RESTORE

uint16_t *dataPtr,  *firstDataPtr ;
int       dataIndex, firstDataIndex ;

// Printing format

char fmtString [32] ;

// File off the command line

char *initialFilename = NULL ;

/*
 * escapeThread:
 *	This thread does nothing but sleeps and sets a flag to let you know
 *	it's time to check to see if the escape key has been presses.
 *	This whole escape key business is a real PITA )-:
 *********************************************************************************
 */

static void *escapeThread (void *dummy)
{
  struct timespec sleeper ;

  sleeper.tv_sec  = (time_t)0 ;
  sleeper.tv_nsec = (long)(100 * 1000) ;

  for (;;)
  {
    nanosleep (&sleeper, NULL) ;
    checkForEscape = TRUE ;
  }
  return NULL ;
}


/*
 * syntaxError:
 *	Output our parse and run-time error messages in a common format.
 *	Always return FALSE to make life easier for calling code
 *********************************************************************************
 */

int syntaxError (char *message, ...) 
{
  va_list argp ;
  char buffer [1024] ;

  va_start (argp, message) ;
    vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

// Only the first syntax error holds weight
//	We get all sorts of cascades unwinding recursions...

  if (gotSyntaxError)
    return FALSE ;

  gotSyntaxError = TRUE ;

  if (programRunning)
    screenPrintf ("<!> Error at line %d: %s\n", programLines [linePtr].lineNumber, buffer) ;
  else
    screenPrintf ("*** Error: %s\n", buffer) ;

  return FALSE ;
}


/*
 * revokeRoot:
 *	Revoke any root suid privs. Normally (on the Raspberry Pi)
 *	we want RTB installed as a set uid program so we can access
 *	the GPIO port, but we need to revoke root access when ever
 *	we do anything concerning the filesystem - just in-case.
 *********************************************************************************
 */

void revokeRoot (void)
{
  if (getuid () + geteuid () == 0)	// Really running as root
    return ;

  if (geteuid () == 0)			// Running setuid root
  {
    seteuid (getuid ()) ;		// Change effective uid to the uid of the caller
    rootRevoked = TRUE ;
    return ;
  }
}

void regainRoot (void)
{
  if (getuid () + geteuid () == 0)	// Really running as root
    return ;

  if (!rootRevoked)
    return ;

  seteuid (0) ;
  rootRevoked = FALSE ;
}



/*
 * main:
 *	Start here
 *********************************************************************************
 */

int main (int argc, char *argv[])
{
  char *line ;
  int opt ;
  uint32_t sdlFlags = SDL_SWSURFACE ;
  int largeFont  = FALSE ;
  int dumpVmodes = FALSE ;
  int videoMode  = -1 ;
  int xSize, ySize, zSize ;
  pthread_t myThread ;

  xSize = ySize = zSize = 0 ;

  opterr = 0 ;
  while ((opt = getopt (argc, argv, "fdhDlm:x:y:z:")) != -1)
  {
    switch (opt)
    {
      case 'f':
	sdlFlags |= SDL_FULLSCREEN ;
	break ;

      case 'd':
	sdlFlags |= SDL_DOUBLEBUF ;
        break ;

      case 'h':
	sdlFlags &= ~SDL_SWSURFACE ;
	sdlFlags |= ~SDL_HWSURFACE ;
        break ;

      case 'D':
	dumpVmodes = TRUE ;
	break ;

      case 'l':
	largeFont = TRUE ;
	break ;

      case 'm':
	videoMode = atoi (optarg) ;
	break ;

      case 'x':
	xSize = atoi (optarg) ;
	break ;

      case 'y':
	ySize = atoi (optarg) ;
	break ;

      case 'z':
	zSize = atoi (optarg) ;
	break ;

      default:
	fprintf (stderr, "Usage: %s [-f] [-d] [-h] [-m mode] [-x xSize] [-y ySize] [-z bpp] [-l] [-D] <filename>\n", argv [0]) ;
	exit (EXIT_FAILURE) ;
    }
  }

  if (zSize != 0)
  {
    if (!((zSize == 8) || (zSize == 16) || (zSize == 24) || (zSize == 32)) )
    {
      fprintf (stderr, "%s: zSize must be 8, 16, 24 or 32\n", argv [0]) ;
      exit (EXIT_FAILURE) ;
    }
  }


  if (optind < argc)
    initialFilename = argv [optind] ;

  openScreenKeyboard (sdlFlags) ;

  if (dumpVmodes)
  {
    dumpVideoModes () ;
    return 0 ;
  }

#ifdef	RASPBERRY_PI
  if (wiringPiSetup () == -1)
    bomb ("Unable to open Raspberry Pi GPIO", TRUE) ;
#endif

  setupScreenKeyboard (sdlFlags, videoMode, xSize, ySize, zSize, largeFont) ;

  setupSprites      () ;
  setupRND          () ;
  setupReadline     () ;
  setupProgram      () ;
  setupSymbolTable  () ;
  setupSerial       () ;
  setupFiles        () ;


  (void)pthread_create (&myThread, NULL, escapeThread, NULL) ;

  continuePtr    = -1 ;
  programRunning = FALSE ;
  gotSyntaxError = FALSE ;

  strcpy (fmtString, DEFAULT_NUM_FORMAT) ;

  if (gettimeofday (&startTime, NULL) != 0)
    bomb ("Setting initial time failled", TRUE) ;

  if (initialFilename == NULL)
  {
    while (!exitRTB)
    {
      screenPutchar ('>', TRUE) ;
      line = readLine (NULL) ;
      gotSyntaxError = FALSE ;
      parseInput (line) ;
    }
  }
  else
  {
    _doLoad (initialFilename) ;
    doRunCommand (0, NULL) ;
  }

  serialCloseAll () ;
  fileCloseAll   () ;
  clearSubs      () ;
  clearProcs     () ;
  clearCycle     () ;
  clearVariables () ;
  deleteSymbols  () ;
  deleteSprites  () ;

  closeScreenKeyboard () ;

  return 0 ;
}
