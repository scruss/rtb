/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * tokenize2.c:
 *	The second state of input line handling.
 *	Essentially more syntax checks on the basic tokenized line.
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

#undef	DEBUG_TOK2

#include <SDL/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>
#include <ctype.h>
#include <math.h>

#include "tokenize2.h"

//#include "bomb.h"

#include "screenKeyboard.h"

#include "bool.h"
#include "lines.h"
#include "keywords.h"
#include "symbolTable.h"

#include "rtb.h"

#ifdef	DEBUG_TOK2
static void prst (uint16_t val)
{
  uint16_t sym, tok ;

  sym = val &  TK_SYM_MASK ;
  tok = val & ~TK_SYM_MASK ;

  printf ("Token: %04X", val) ;
  printf (", %s", symbolNames [sym >> 12]) ;
  if (sym == 0)
    printf (", %s", keywords [tok].keyword) ;
  printf ("\n") ;
}
#endif


/*
 * countArgs:
 *	Count the number of arguments in the tokenised line
 *********************************************************************************
 */

static int countArgs (uint16_t *p)
{
  uint16_t *start = p - 1 ;
  struct keywordStruct *kw = &keywords [*start] ;
  int argsNeeded = kw->args ;
  int argsGot, braCount ;

// Skip over the BRA and make sure first token isn't a comma

  ++p ;
  if (*p == TK_COMMA)
    return syntaxError ("Unnexpected comma in %s", kw->keyword) ;
  
  argsGot = 1 ;
  for (braCount = 1 ;; ++p)
  {
    if (endOfLine (p))
      return syntaxError ("Mismatched ()'s after %s", kw->keyword) ;

    if (*p == TK_KET)
      if (--braCount == 0)
	break ;

    if (*p == TK_COMMA)
      ++argsGot ;

    if (*p == TK_BRA)
    {
      ++p ;
      for (++braCount ;; ++p)
      {
	if (endOfLine (p))
	  return syntaxError ("Mismatched ()'s after %s", kw->keyword) ;

	/**/ if (*p == TK_BRA)
	  ++braCount ;
	else if (*p == TK_KET)
	{
	  if (--braCount == 1)
	    break ;
	}
      }
    }
  }

  if (argsGot != argsNeeded)
    return syntaxError ("Incorrect argument count for %s (expected %d, got %d)",
	kw->keyword, argsNeeded, argsGot) ;

  return TRUE ;
}


int tokenize2 (uint16_t *tokenizedLine)
{
  struct keywordStruct *kw ;
  uint16_t  token, symbol, lastToken ;
  uint16_t *p ;

  p = tokenizedLine ;

// Check the start of the line

  token  = *p++ ;
  symbol = token &  TK_SYM_MASK ;
  if (symbol == 0)
  {
    kw = &keywords [token] ;
    if ((kw->flags & KYF_FUNC) != 0)
      return syntaxError ("Program lines can't start with a function call") ;

    if ((kw->flags & KYF_ARITH) != 0)
      return syntaxError ("Program lines can't start with an arithmetic operator") ;

    if (kw->token != TK_EQUALS)
      if ((kw->flags & KYF_REL) != 0)
	return syntaxError ("Program lines can't start with a relational operator") ;
  }

// Look for built-in functions or proceures that take arguments and make sure
//	there is a bracket after them
//	This test is sort of superfluous as it's also carried out as a side
//	effect of the argument counter, but it's handy to know here before the
//	next test.

  p = tokenizedLine ;
  while (!endOfLine (p))
  {
    token  = *p++ ;
    if ((token & TK_SYM_MASK) != 0)
      continue ;

    kw = &keywords [token] ;
    if ((kw->flags & (KYF_FUNC | KYF_PROC)) == 0)
      continue ;

    if (kw->args == 0)
      continue ;

    if (*p != TK_BRA)
      return syntaxError ("Expected a '(' after %s", kw->keyword) ;
  }

// There's an issue for the rpn stack after the shunter if we enter a keyword
//	after a number, or a number before a keyword - where the keyword
//	isn't an arithmetic operator. e.g. 5sin(45) or red.5
//	the latter case yields 0.5 leaving 'red' on the stack, so the stack
//	eventually overflows when it really ought to generate a syntax error.
//	It also happens with variables too.

  p = tokenizedLine ;
  while (!endOfLine (p))
  {
    token  = *p++ ;

    symbol = token & TK_SYM_MASK ;

    if ( ((symbol == TK_SYM_CONST_NUM)   || (symbol == TK_SYM_CONST_STR) ||
          (symbol == TK_SYM_VAR_NUM)     || (symbol == TK_SYM_VAR_STR) ) ||
	 ((symbol == 0) && ((keywords [token].flags & KYF_PV) != 0) ))
    {

// OK. We have a number - be it a variable or constant, or pseudo variable

// Check next token...

      if (endOfLine (p))	// End of line is fine.
	continue ;

// Can't have another symbol after a number.
//	e.g. 10 20, or 10 proc, 3 pi is invalid

      if ((*p & TK_SYM_MASK) != 0)		// Symbol
      {
	if (symbol == 0)
	  return syntaxError ("Missing Operator after %s", keywords [token].keyword) ;
	else
	  return syntaxError ("Missing Operator after symbol") ;
      }

// After a number, we must have an arithmetic operator, or a relational operator,
//	or comma, semicolon or ) or CYCLE

      kw = &keywords [*p] ;
      if (! ((kw->flags & (KYF_ARITH | KYF_REL)) ||
		(kw->token == TK_COMMA) || (kw->token == TK_SEMICO) || (kw->token == TK_KET) ||
		(kw->token == TK_CYCLE) || (kw->token == TK_TO) || (kw->token == TK_STEP) ||
		(kw->token == TK_THEN) ))
	return syntaxError ("Operator expected before '%s'", kw->keyword) ;

// Check previous token ...

// Can't have a symbol before a number.
//	e.g. 10 20, or proc 10, pi 3 is invalid

      if ((*(p - 2) & TK_SYM_MASK) != 0)		// Symbol
	return syntaxError ("Missing Operator before") ;
    }
  }

// Now scan for minus tokens
//	Checking to see if they're really unary minuses...

  p = tokenizedLine ;
  lastToken = 0 ;
  while (!endOfLine (p))
  {
    token  = *p++ ;

    if ((token & TK_SYM_MASK) != 0)	// Symbol?
      continue ;

    kw = &keywords [token] ;
    if (kw->token == TK_MINUS)
    {
      lastToken = *(p - 2) ;
      if ((lastToken & TK_SYM_MASK) != 0)
	continue ;

      kw = &keywords [lastToken] ;
      if (  ((kw->flags & (KYF_REL | KYF_ARITH)) != 0) ||
		(kw->token == TK_BRA) || (kw->token == TK_COMMA) )
	*(p - 1) = TK_UNMINUS ;
    }
  }

// Check number of arguments passesd into built-in procedures and functions

  p = tokenizedLine ;
  while (!endOfLine (p))
  {
    token  = *p++ ;

    if ((token & TK_SYM_MASK) != 0)	// Symbol
      continue ;

    kw = &keywords [token] ;
    if ((kw->flags & (KYF_FUNC | KYF_PROC)) == 0)
      continue ;

    if (kw->args == 0)
      continue ;

    if (!countArgs (p))
      return FALSE ;
  }

  return TRUE ;
}
