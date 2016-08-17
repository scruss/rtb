/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * switch.c:
 *	Handle SWITCH statements
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <unistd.h>

#include "rtb.h"
#include "bomb.h"
#include "bool.h"

#include "keywords.h"
#include "lines.h"
#include "symbolTable.h"
//#include "tokenise1.h"
#include "shuntingYard.h"
#include "rpnEval.h"

//#include "commands.h"

#include "run.h"
#include "switch.h"


/*
 * doSwitch:
 *	Handle a switch statement
 *********************************************************************************
 */

int doSwitch (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int       l ;
  double    testNum = 0.0 ;
  char     *testStr = NULL ;
  uint16_t  symbol, index ;
  int       defaultLine = -1 ;
  int       count = 0 ;
  int       switchNum = FALSE ;

  if (oneNumber ())
    switchNum = TRUE ;
  else if (!oneString ())
    return syntaxError ("SWITCH: Expression expected") ;

  if (switchNum)
    testNum = popN () ;
  else
    testStr = popS () ;

// Now search forwards from here, looking for CASE statements...

  for (l = linePtr + 1 ; l < numLines ; ++l)
  {
    p = programLines [l].data ;

    if (*p == TK_SWITCH)	// Nested switches?
    {
      ++count ;
      continue ;
    }

    if (*p == TK_DEFAULT)	// Remember where it is (or at least where the last one is!)
    {
      if (count == 0)
	defaultLine = l ;
      continue ;
    }

    if (*p == TK_ENDSWITCH)	// No matching cases -
    {
      if (count != 0)
      {
	--count ;
	continue ;
      }

      if (defaultLine == -1)	// Do we have a default?
	linePtr = l ;
      else
	linePtr = defaultLine ;
      if (testStr != NULL)
	free (testStr) ;
      return TRUE ;
    }

    if ((*p != TK_CASE) || (count != 0))
      continue ;

    ++p ;
    while (!endOfLine (p))
    {
      symbol = *p &  TK_SYM_MASK ;
      index  = *p & ~TK_SYM_MASK ;

// Constant numbers or strings:

      /**/ if ((switchNum) && (symbol == TK_SYM_CONST_NUM))
      {
	if (testNum == symbolTable [index].value.realVal)
	{
	  linePtr = l ;
	  if (testStr != NULL)
	    free (testStr) ;
	  return TRUE ;
	}
      }
      else if ((!switchNum) && (symbol == TK_SYM_CONST_STR))
      {
	if (strcmp (testStr, symbolTable [index].name) == 0)
	{
	  linePtr = l ;
	  if (testStr != NULL)
	    free (testStr) ;
	  return TRUE ;
	}
      }

// Pseudo variables - only read only numeric ones ...

      else if ( ((switchNum) && (symbol == 0)) &&
	((keywords [index].flags & (KYF_PV | KYF_RO)) == (KYF_PV | KYF_RO)) )
      {
	(void)keywords [index].function (NULL) ;
	if (!oneNumber ())
	  return syntaxError ("SWITCH: Wrong type (CASE line %d)", programLines [l].lineNumber) ;
	if (testNum == popN ())
	{
	  linePtr = l ;
	  return TRUE ;
	}
      }

// oops.

      else
	return syntaxError ("SWITCH: Wrong type (CASE line %d)", programLines [l].lineNumber) ;

      if (*++p == TK_COMMA)
	++p ;
    }

  }

  if (testStr != NULL)
    free (testStr) ;

  return syntaxError ("SWITCH: Missing ENDCASE") ;
}


/*
 * doEndSwitch:
 *	Do nothing, just skip over it.
 *********************************************************************************
 */

int doEndSwitch (void *ptr)
{
  return TRUE ;
}


/*
 * doCase:
 *	It is an error to execute a CASE statement...
 *********************************************************************************
 */

int doCase (void *ptr)
{
  return syntaxError ("CASE: Should not be executed") ;
}


/*
 * doEndCase:
 *	Handle the end of a case - jump forwards to the next matching endswitch
 *********************************************************************************
 */

int doEndCase (void *ptr)
{
  uint16_t *p ;
  int l ;
  int count = 0 ;

  for (l = linePtr + 1 ; l < numLines ; ++l)
  {
    p = programLines [l].data ;

// Some things indicate a runnaway SWITCH...

    if (*p == TK_DEF)
      return syntaxError ("Runnaway SWITCH (no ENDSWITCH)") ;

// Another SWITCH ?

    if (*p == TK_SWITCH)
    {
      ++count ;
      continue ;
    }

    if (*programLines [l].data == TK_ENDSWITCH)
    {
      if (count == 0)
      {
	linePtr = l ;
	return TRUE ;
      }
      --count ;
      continue ;
    }

  }

// No matching ENDSWITCH line

  return syntaxError ("SWITCH: Missing ENDSWITCH") ;
}



/*
 * doDefault:
 *	The default case (Do nothing if it's executed)
 *********************************************************************************
 */

int doDefault (void *ptr)
{
  return TRUE ;
}
