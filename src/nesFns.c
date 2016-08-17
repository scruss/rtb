/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * nesFns.c:
 *	Functions to handle a Nintendo Entertainment System Joysitck on
 *	a Raspberry Pi
 *********************************************************************************
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
 *********************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <math.h>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <unistd.h>

#include "bomb.h"

#include "bool.h"
#include "keywords.h"
#include "symbolTable.h"

//#include "screenKeyboard.h"
//#include "readline.h"

#include "shuntingYard.h"
#include "rpnEval.h"
#include "rtb.h"
#include "nesFns.h"

// Local data

#ifdef	RASPBERRY_PI
#warning "Raspberry Pi NES"
#  include <wiringPi.h>
#  include <piNes.h>
#endif



/*
 * doNesOpen:
 *	Open a NES Joystick interface
 *	Pass in the 3 pins needed: data, clock, latch
 *********************************************************************************
 */

int doNesOpen (void *ptr)
{
#ifdef	RASPBERRY_PI
  int data, clock, latch ;
  int handle ;

  if (!threeNumbers ())
    return syntaxError ("nesOpen: Expected three numbers") ;

  latch = popN () ;
  clock = popN () ;
  data  = popN () ;

  handle = setupNesJoystick (data, clock, latch) ;
  if (handle == -1)
    return syntaxError ("nesOpen: Too many Joysticks opened") ;

// Prime the hardware? 

  while (readNesJoystick (handle) != 0)
    delay (1) ;

  pushN ((double)handle) ;
#else
  pushN (0.0) ;
#endif

  return TRUE ;
}


/*
 * doNesRead:
 *	Read the NES joystick
 *********************************************************************************
 */

int doNesRead (void *ptr)
{
#ifdef	RASPBERRY_PI
  int handle ;
  unsigned int buttons ;

  if (!oneNumber ())
    return syntaxError ("nesRead: Expected a number") ;

  handle = (int)rint (popN ()) ;

  buttons = readNesJoystick (handle) & 0xFF ;

  pushN ((double)buttons) ;
#else
  pushN (0.0) ;
#endif

  return TRUE ;
}


/*
 * doNesButton:
 *	Return TRUE if the given button is being pushed
 *********************************************************************************
 */

#ifdef	RASPBERRY_PI
static int doNesButton (void *ptr, int button)
{
  int handle ;
  unsigned int buttons ;

  if (!oneNumber ())
    return syntaxError ("nesButton: Expected a number") ;

  handle  = (int)rint (popN ()) ;
  buttons = readNesJoystick (handle) ;

  if ((buttons & button) == 0)
    pushN (0.0) ;
  else
    pushN (1.0) ;

  return TRUE ;
}

int doNesUp     (void *ptr)	{ return doNesButton (ptr, NES_UP) ;     }
int doNesDown   (void *ptr)	{ return doNesButton (ptr, NES_DOWN) ;   }
int doNesLeft   (void *ptr)	{ return doNesButton (ptr, NES_LEFT) ;   }
int doNesRight  (void *ptr)	{ return doNesButton (ptr, NES_RIGHT) ;  }
int doNesSelect (void *ptr)	{ return doNesButton (ptr, NES_SELECT) ; }
int doNesStart  (void *ptr)	{ return doNesButton (ptr, NES_START) ;  }
int doNesA      (void *ptr)	{ return doNesButton (ptr, NES_A) ;      }
int doNesB      (void *ptr)	{ return doNesButton (ptr, NES_A) ;      }

#else

int doNesUp     (void *ptr)	{ pushN (1.0) ; return TRUE ; }
int doNesDown   (void *ptr)	{ pushN (1.0) ; return TRUE ; }
int doNesLeft   (void *ptr)	{ pushN (1.0) ; return TRUE ; }
int doNesRight  (void *ptr)	{ pushN (1.0) ; return TRUE ; }
int doNesSelect (void *ptr)	{ pushN (1.0) ; return TRUE ; }
int doNesStart  (void *ptr)	{ pushN (1.0) ; return TRUE ; }
int doNesA      (void *ptr)	{ pushN (1.0) ; return TRUE ; }
int doNesB      (void *ptr)	{ pushN (1.0) ; return TRUE ; }
#endif
