/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * relFns.c:
 *	Functions that handle relationship operators
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
#include <time.h>
#include <math.h>


#include "rtb.h"
#include "bomb.h"

#include "bool.h"
#include "rpnEval.h"

#include "relFns.h"


/*
 * doEquals:
 *	(And other comparisions)
 *	these can be overloaded for numbers or strings
 *********************************************************************************
 */

int doEquals (void *ptr)
{
  double  x, y ;
  char   *s1, *s2 ;

  if (twoNumbers ())
  {
    x = popN () ;
    y = popN () ;
    pushN (y == x) ;
    return TRUE ;
  }

  if (twoStrings ())
  {
    s1 = popS () ;
    s2 = popS () ;
    if (strcmp (s2, s1) == 0)
      pushN (1) ;
    else
      pushN (0) ;
    free (s1) ;
    free (s2) ;
    return TRUE ;
  }

  return syntaxError (E_EX_MIX) ;
}

int doNotEquals (void *ptr)
{
  double  x, y ;
  char   *s1, *s2 ;

  if (twoNumbers ())
  {
    x = popN () ;
    y = popN () ;
    pushN (y != x) ;
    return TRUE ;
  }

  if (twoStrings ())
  {
    s1 = popS () ;
    s2 = popS () ;
    if (strcmp (s2, s1) == 0)
      pushN (0) ;
    else
      pushN (1) ;
    free (s1) ;
    free (s2) ;
    return TRUE ;
  }

  return syntaxError (E_EX_MIX) ;
}

int doLessThan (void *ptr)
{
  double  x, y ;
  char   *s1, *s2 ;

  if (twoNumbers ())
  {
    x = popN () ;
    y = popN () ;
    pushN (y < x) ;
    return TRUE ;
  }

  if (twoStrings ())
  {
    s1 = popS () ;
    s2 = popS () ;
    if (strcmp (s2, s1) < 0)
      pushN (1) ;
    else
      pushN (0) ;
    free (s1) ;
    free (s2) ;
    return TRUE ;
  }

  return syntaxError (E_EX_MIX) ;
}

int doLessThanEq (void *ptr)
{
  double  x, y ;
  char   *s1, *s2 ;

  if (twoNumbers ())
  {
    x = popN () ;
    y = popN () ;
    pushN (y <= x) ;
    return TRUE ;
  }

  if (twoStrings ())
  {
    s1 = popS () ;
    s2 = popS () ;
    if ((strcmp (s2, s1) < 0) || (strcmp (s2, s1) == 0))
      pushN (1) ;
    else
      pushN (0) ;
    free (s1) ;
    free (s2) ;
    return TRUE ;
  }

  return syntaxError (E_EX_MIX) ;
}

int doGtThan (void *ptr)
{
  double  x, y ;
  char   *s1, *s2 ;

  if (twoNumbers ())
  {
    x = popN () ;
    y = popN () ;
    pushN (y > x) ;
    return TRUE ;
  }

  if (twoStrings ())
  {
    s1 = popS () ;
    s2 = popS () ;
    if (strcmp (s2, s1) > 0)
      pushN (1) ;
    else
      pushN (0) ;
    free (s1) ;
    free (s2) ;
    return TRUE ;
  }

  return syntaxError (E_EX_MIX) ;
}

int doGtThanEq (void *ptr)
{
  double  x, y ;
  char   *s1, *s2 ;

  if (twoNumbers ())
  {
    x = popN () ;
    y = popN () ;
    pushN (y >= x) ;
    return TRUE ;
  }

  if (twoStrings ())
  {
    s1 = popS () ;
    s2 = popS () ;
    if ((strcmp (s2, s1) > 0) || (strcmp (s2, s1) == 0))
      pushN (1) ;
    else
      pushN (0) ;
    free (s1) ;
    free (s2) ;
    return TRUE ;
  }

  return syntaxError (E_EX_MIX) ;
}

int doAnd (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN() ;
  y = popN() ;
  pushN (y && x) ;
  return TRUE ;
}

int doOr (void *ptr)
{
  double x, y ;

  if (!twoNumbers ())
    return syntaxError ("Numbers expected") ;

  x = popN() ;
  y = popN() ;
  pushN (y || x) ;
  return TRUE ;
}

int doNot (void *ptr)
{
  if (!oneNumber ())
    return syntaxError ("Number expected") ;

  pushN (!popN()) ;
  return TRUE ;
}
