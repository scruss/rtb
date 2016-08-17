/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * numFns.c:
 *	Functions that return numbers
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

#include <SDL/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include <ctype.h>
//#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



//#include <unistd.h>

#include "rtb.h"
#include "bomb.h"
#include "bool.h"

#include "screenKeyboard.h"

#include "hash.h"
#include "rpnEval.h"
#include "numFns.h"

// Globals

static unsigned int seed ;
static double       lastRnd ;

/*
 * doPlus:
 *	This is overloaded as it can concatenate strings as well as add numbers
 *********************************************************************************
 */

int doPlus (void *ptr)
{
  double x, y ;
  char *s1, *s2, *str ;
  int len ;

  if (twoNumbers ())
  {
    x = popN () ;
    y = popN () ;
    pushN (y + x) ;
    return TRUE ;
  }

  if (twoStrings ())
  {
    s1 = popS () ;
    s2 = popS () ;
    len = strlen (s1) + strlen (s2) + 1 ;
    if ((str = malloc (len)) == NULL)
      return syntaxError ("Out of memory") ;
    sprintf (str, "%s%s", s2, s1) ;
    pushS (str) ;
    free (s1) ;
    free (s2) ;
    free (str) ;
    return TRUE ;
  }

  return syntaxError (E_EX_MIX) ;
}

int doMinus (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN () ;
  y = popN () ;
  pushN (y - x) ;
  return TRUE ;
}

int doUnMinus (void *ptr)
{
  if (!oneNumber ())
    return syntaxError ("Number expected") ;

  pushN ( - popN ()) ;
  return TRUE ;
}

int doTimes (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN () ;
  y = popN () ;
  pushN (y * x) ;
  return TRUE ;
}

int doDiv (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN () ;
  y = popN () ;
  pushN (y / x) ;
  return TRUE ;
}

int doIdiv (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN () ;
  y = popN () ;
  pushN (floor (y / x)) ;
  return TRUE ;
}

int doMod (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN () ;
  y = popN () ;
  pushN (y - ((floor (y / x)* x))) ;
  return TRUE ;
}


int doPow (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN () ;
  y = popN () ;
  pushN (pow (y, x)) ;
  return TRUE ;
}

// Bitwise operators

int doBor (void *ptr)
{
  uint32_t x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = (int)rint (popN ()) ;
  y = (int)rint (popN ()) ;
  pushN ((double)(y | x)) ;
  return TRUE ;
}

int doBand (void *ptr)
{
  uint32_t x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = (int)rint (popN ()) ;
  y = (int)rint (popN ()) ;
  pushN ((double)(y & x)) ;
  return TRUE ;
}

int doBxor (void *ptr)
{
  uint32_t x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = (int)rint (popN ()) ;
  y = (int)rint (popN ()) ;
  pushN ((double)(y ^ x)) ;
  return TRUE ;
}


// Trig. and other similar ones

static int doSingleNumFn (double (*doFn)(double))
{
  if (!oneNumber ())
    return syntaxError ("Number expected") ;

  pushN (doFn (popN())) ;
  return TRUE ;
}

static int doTrig (double (*trigFn)(double))
{
  if (!oneNumber ())
    return syntaxError ("Number expected") ;

  pushN (trigFn (angleConversion * popN())) ;
  return TRUE ;
}

static int doInvTrig (double (*trigFn)(double))
{
  if (!oneNumber ())
    return syntaxError ("Number expected") ;

  pushN (trigFn (popN ()) / angleConversion) ;
  return TRUE ;
}

int doSin  (void *ptr)	{ return doTrig (&sin) ; }
int doCos  (void *ptr)	{ return doTrig (&cos) ; }
int doTan  (void *ptr)	{ return doTrig (&tan) ; }

int doAsin (void *ptr)	{ return doInvTrig (&asin) ; }
int doAcos (void *ptr)	{ return doInvTrig (&acos) ; }
int doAtan (void *ptr)	{ return doInvTrig (&atan) ; }

int doAbs  (void *ptr)	{ return doSingleNumFn (&fabs) ;	}
int doExp  (void *ptr)	{ return doSingleNumFn (&exp) ;		}
int doLog  (void *ptr)	{ return doSingleNumFn (&log) ;		}
int doSqrt (void *ptr)	{ return doSingleNumFn (&sqrt) ;	}
int doInt  (void *ptr)	{ return doSingleNumFn (&floor) ;	}		// BASIC INT() is C's floor()

int doSgn  (void *ptr)
{
  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  pushN (popN() < 0 ? -1 : 1) ;
  return TRUE ;
}


int doMax  (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN() ;
  y = popN() ;
  pushN (y > x ? y : x) ;
  return TRUE ;
}

int doMin  (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN() ;
  y = popN() ;
  pushN (y < x ? y : x) ;
  return TRUE ;
}

/*
 * The random number generator
 *********************************************************************************
 */

void setupRND (void)
{
  int fd ;

  if ((fd = open ("/dev/urandom", O_RDONLY)) == -1)
    bomb ("Unable to initialise the random number generator", TRUE) ;

  if (read (fd, &seed, sizeof (seed)) != sizeof (seed))
    bomb ("Unable to seed the random number generator", TRUE) ;

  close (fd) ;

  srand48 (seed) ;
  lastRnd = drand48 () ;
}

int doRnd (void *ptr)
{
  double x, y ;

  if (!oneNumber ())
    return syntaxError ("Number expected") ;

  x = popN () ;

  /**/ if (x == 0.0)
    y = lastRnd ;
  else if (x == 1.0)
    y =  drand48 () ;
  else
    y = floor (drand48 () * x) ;

  lastRnd = y ;
  pushN (y) ;
  return TRUE ;
}

/*
 * doSeed:
 *	Pseudo variable SEED
 *********************************************************************************
 */

int doSeed (void *ptr)
{
  double x ;

// Reading?

  if (ptr == NULL)
  {
    pushN ((double)seed) ;
    return TRUE ;
  }

// Writing

  x = *(double *)ptr ;

  if (x == 0.0)
    setupRND () ;
  else
  {
    seed = (int)rint (x) ;
    srand48 (seed) ;
  }

  return TRUE ;
}

/*
 * doPI: doPI2
 *	Pseudo variable PI and PI2 (PI/2)
 *********************************************************************************
 */

int doPI (void *ptr)
{
  if (ptr != NULL)
    return FALSE ;

  pushN (M_PI) ;
  return TRUE ;
}

int doPI2 (void *ptr)
{
  if (ptr != NULL)
    return FALSE ;

  pushN (M_PI_2) ;
  return TRUE ;
}

/*
 * doTrue: doFalse:
 *	Pseudo variables TRUE and FALSE (1 and 0)
 *********************************************************************************
 */

int doTrue (void *ptr)
{
  if (ptr != NULL)
    return FALSE ;

  pushN (1.0) ;
  return TRUE ;
}

int doFalse (void *ptr)
{
  if (ptr != NULL)
    return FALSE ;

  pushN (0.0) ;
  return TRUE ;
}


/*
 * doTime:
 *	Pseudo variable containing the time in mS since the program was RUN
 *********************************************************************************
 */

int doTime (void *ptr)
{
  struct timeval tim ;
  unsigned int thous ;

  if (gettimeofday (&tim, NULL) != 0)
    return syntaxError ("TIME: No TIME") ;

  thous = ((tim.tv_sec - startTime.tv_sec) * 1000) + ((tim.tv_usec - startTime.tv_usec) / 1000) ;

  pushN ((double)thous) ;
  return TRUE ;
}

/*
 * hash (str)
 *	Returns the hash value for the given string
 *********************************************************************************
 */

int doHash (void *ptr)
{
  char *str ;
  unsigned int modulo ;

  if (stackOrderPtr < 2)
    return syntaxError (E_EX_UNDERFLOW) ;

  if ((stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM) && (stackOrder [stackOrderPtr - 2] == EVAL_STACK_STR))
  {
    modulo = (int)rint (popN ()) ;
    str    = popS () ;

    pushN ((double)hash (str, modulo)) ;
    free (str) ;
    return TRUE ;
  }

  return syntaxError ("HASH: Expecting a string and a number") ;
}
