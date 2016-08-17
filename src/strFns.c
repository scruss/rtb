/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * strFns.c:
 *	Functions that return strings
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
#include <stdint.h>
#include <string.h>
//#include <ctype.h>
//#include <errno.h>
#include <time.h>
#include <math.h>

//#include <unistd.h>

#include "rtb.h"
//#include "bomb.h"

#include "bool.h"
//#include "keywords.h"
//#include "symbolTable.h"

//#include "shuntingYard.h"

#include "rpnEval.h"
#include "strFns.h"

// chr$ (n)

int doChrD (void *ptr)
{
  char c ;
  char str [2] ;

  if (!oneNumber ())
    return FALSE ;

  c = (int)floor (popN ()) & 0x7F ;
  str [0] = c ;
  str [1] = '\0' ;
  pushS (str) ;
  return TRUE ;
}

// asc (str)

int doAsc (void *ptr)
{
  char *s ;

  if (!oneString ())
    return syntaxError ("String expected") ;

  s = popS () ;
  pushN ((int)s [0]) ;
  free (s) ;
  return TRUE ;
}


// len (str)
//	Returns a number

int doLen (void *ptr)
{
  char *s ;

  if (!oneString ())
    return syntaxError ("String expected") ;

  s = popS () ;
  pushN (strlen (s)) ;
  free (s) ;
  return TRUE ;
}

// val (str)
//	Returns a number

int doVal (void *ptr)
{
  char *s ;
  double num ;

  if (!oneString ())
    return syntaxError ("String expected") ;

  s = popS () ;
  sscanf (s, "%lf", &num) ;
  pushN  (num) ;
  free   (s) ;
  return TRUE ;
}

// left$ (str, num)

int doLeftD (void *ptr)
{
  char *oldStr, *newStr ;
  int    newLen, oldLen ;

  if (stackOrderPtr < 2)
    return syntaxError (E_EX_UNDERFLOW) ;

  if ((stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM) && (stackOrder [stackOrderPtr - 2] == EVAL_STACK_STR))
  {
    newLen = (int)floor (popN ()) ;
    oldStr = popS () ;
    oldLen = strlen (oldStr) ;

    /**/ if (newLen == 0)		// They want to copy over zero characters? Oh well..
      pushS ("") ;
    else if (newLen >= oldLen)		// The same or more? Just push it back...
      pushS (oldStr) ;
    else				// Copy the first newLen characters
    {
      if ((newStr = malloc (newLen + 1)) == NULL)
	return syntaxError ("Out of memory") ;
      strncpy (newStr, oldStr, newLen) ;
      newStr [newLen] = '\0' ;
      pushS (newStr) ;
      free  (newStr) ;
      free  (oldStr) ;
    }
    return TRUE ;
  }

  return syntaxError ("LEFT$: Expecting a string and a number") ;
}


// mid$ (str, start,

int doMidD (void *ptr)
{
  char *oldStr, *newStr ;
  int   start, newLen, oldLen ;

  if (stackOrderPtr < 3)
    return syntaxError (E_EX_UNDERFLOW) ;

  if ((stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM) &&
	(stackOrder [stackOrderPtr - 2] == EVAL_STACK_NUM) && (stackOrder [stackOrderPtr - 3] == EVAL_STACK_STR))
  {
    newLen = (int)floor (popN ()) ;
    start  = (int)floor (popN ()) ;
    oldStr = popS () ;
    oldLen = strlen (oldStr) ;

    if (newLen > oldLen)
      newLen = oldLen ;

    /**/ if (start >= oldLen)
      return syntaxError ("MID$: Start is bigger than the string length") ;
    else if (newLen == 0)		// They want to copy over zero characters? Oh well..
      pushS ("") ;
    else if ((start == 0) && (newLen == oldLen))
      pushS (oldStr) ;
    else
    {
      if ((start + newLen) > oldLen)
	newLen = oldLen - start ;
	
      if ((newStr = malloc (newLen + 1)) == NULL)
	return syntaxError ("Out of memory") ;
      strncpy (newStr, &oldStr [start], newLen) ;
      newStr [newLen] = '\0' ;
      pushS (newStr) ;
      free  (newStr) ;
      free  (oldStr) ;
    }
    return TRUE ;
  }

  return syntaxError ("MID$: Expecting a string and two numbers") ;
}


// right$ (str, num)

int doRightD (void *ptr)
{
  char *oldStr, *newStr ;
  int    newLen, oldLen ;

  if (stackOrderPtr < 2)
    return syntaxError (E_EX_UNDERFLOW) ;

  if ((stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM) && (stackOrder [stackOrderPtr - 2] == EVAL_STACK_STR))
  {
    newLen = (int)floor (popN ()) ;
    oldStr = popS () ;
    oldLen = strlen (oldStr) ;

    /**/ if (newLen == 0)		// They want to copy over zero characters? Oh well..
      pushS ("") ;
    else if (newLen >= oldLen)		// The same or more? Just push it back...
      pushS (oldStr) ;
    else				// Copy the last newLen characters
    {
      if ((newStr = malloc (newLen + 1)) == NULL)
	return syntaxError ("Out of memory") ; ;
      strncpy (newStr, &oldStr [oldLen - newLen], newLen + 1) ;
      pushS (newStr) ;
      free  (newStr) ;
      free  (oldStr) ;
    }
    return TRUE ;
  }

  return syntaxError ("RIGHT$: Expecting a string and a number") ;
}

// str$ (n) - return number as a string
 
int doStrD (void *ptr)
{
  char string [20] ;

  if (!oneNumber ())
    return syntaxError ("Number expected") ;

  sprintf (string, "%-.10g", popN ()) ;
  pushS (string) ;
  return TRUE ;
}

// space$ (num)
//	Return num spaces in a string

int doSpaceD (void *ptr)
{
  register char *s, *p ;
  register int i, n ;

  if (!oneNumber ())
    return syntaxError ("Number expected") ;

  n = (int)floor (popN ()) ;

  if ((s = malloc (n + 1)) == NULL)
    return syntaxError ("Out of memory") ;

  p = s ;
  for (i = 0 ; i < n ; ++i)
    *p++ = ' ' ;

  *p = '\0' ;

  pushS (s) ;
  free  (s) ;
  return TRUE ;
}

/*
 * DATE$
 * TIME$
 *	Pseudo variables
 *********************************************************************************
 */

int doDateD (void *ptr)
{
  time_t     currentTime ;
  struct tm *mytime ;
  char       clock [32] ;

  time (&currentTime) ;
  mytime = localtime (&currentTime) ;

  sprintf (clock, "%04d-%02d-%02d", mytime->tm_year + 1900, mytime->tm_mon, mytime->tm_mday) ;

  pushS (clock) ;
  return TRUE ;
}

int doTimeD (void *ptr)
{
  time_t     currentTime ;
  struct tm *mytime ;
  char       clock [32] ;

  time (&currentTime) ;
  mytime = localtime (&currentTime) ;

  sprintf (clock, "%02d:%02d:%02d", mytime->tm_hour, mytime->tm_min, mytime->tm_sec) ;

  pushS (clock) ;
  return TRUE ;
}
