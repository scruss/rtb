/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * serialFns.c:
 *	Functions to handle serial port(s)
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

//#include <SDL/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
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
#include "serial.h"
#include "serialFns.h"


/*
 * doSopen:
 *	Open a serial port
 *********************************************************************************
 */

int doSopen (void *ptr)
{
  int   handle ;
  int   baud ;
  char *filename ;

  if (!((stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM) && (stackOrder [stackOrderPtr - 2] == EVAL_STACK_STR)))
    return syntaxError ("SOPEN: Bad parameters") ;

  baud     = (int)rint (popN ()) ;
  filename = popS () ;

  if ((handle = serialOpen (filename, baud)) == -1)
  {
    free (filename) ;
    return syntaxError ("SOPEN: Device open failed") ;
  }

  pushN ((double)handle) ;

  free (filename) ;

  return TRUE ;
}


/*
 * doSclose:
 *	Close a serial port
 *********************************************************************************
 */

int doSclose (void *ptr)
{
  if (!oneNumber ())
    return syntaxError ("SCLOSE: Expected a number") ;

  serialClose ((int)popN ()) ;
  return TRUE ;
}


/*
 * doSget:
 *	Wait for and return a single byte from the serial line
 *********************************************************************************
 */

int doSget (void *ptr)
{
  int data ;

  if (!oneNumber ())
    return syntaxError ("SGET: Number expected") ;

  data = serialGetchar ((int)rint (popN ())) ;

  pushN ((double)data) ;

  return TRUE ;
}


/*
 * doSgetD:
 *	Wait for and return a single byte from the serial line as a string
 *********************************************************************************
 */

int doSgetD (void *ptr)
{
  int data ;
  char buf [2] ;

  if (!oneNumber ())
    return syntaxError ("SGET$: Number expected") ;

  data = serialGetchar ((int)rint (popN ())) ;

  buf [1] = 0 ;
  if (data == -1)
    buf [0] = 0 ;
  else
    buf [0] = data & 0xFF ;

  pushS (buf) ;

  return TRUE ;
}


/*
 * doSput:
 *	Send a single byte down the serial line
 *********************************************************************************
 */

int doSput (void *ptr)
{
  int x, y ;

  if (!twoNumbers ())
    return syntaxError ("SPUT: Expected two numbers") ;
  
  y = (int)rint (popN ()) ;
  x = (int)rint (popN ()) ;

  serialPutchar (x, y) ;
  return TRUE ;
}


/*
 * doSputD:
 *	Send a string down the serial line
 *********************************************************************************
 */

int doSputD (void *ptr)
{
  int handle ;
  char *data ;
  char *q ;

  if (!((stackOrder [stackOrderPtr - 1] == EVAL_STACK_STR) && (stackOrder [stackOrderPtr - 2] == EVAL_STACK_NUM)))
    return syntaxError ("SPUT$: Bad parameters") ;

  data   = popS () ;
  handle = (int)rint (popN ()) ;

  for (q = data ; *q ; ++q)
    serialPutchar (handle, (uint8_t)*q) ;

  free (data) ;
  return TRUE ;
}


/*
 * doSready:
 *	Return the number of characters waiting to be read from the remote
 *********************************************************************************
 */

int doSready (void *ptr)
{
  int data ;

  if (!oneNumber ())
    return syntaxError ("SREADY: Number expected") ;

  data = serialDataAvail ((int)rint (popN ())) ;

  pushN ((double)data) ;

  return TRUE ;
}
