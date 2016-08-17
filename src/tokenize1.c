/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * tokenize1.c:
 *	The first stage of input line tokenisation. We do some crude
 *	syntax checking and the initial tokenisation of all keywords
 *	and symbols, and symbol table building of variables, constants,
 *	and what we can.
 *	The end result is a tokenized line of data which we'll then
 *	feed into a 2nd pass.
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

#include <string.h>
#include <ctype.h>
#include <math.h>
//#include <errno.h>

#include "tokenize1.h"

//#include "bomb.h"

#include "screenKeyboard.h"

#include "bool.h"
#include "lines.h"
#include "keywords.h"
#include "symbolTable.h"

#include "rtb.h"

/*
 * findXXXX:
 *	Scan the input string for the desired entity. Return TRUE or FALSE,
 *	but if TRUE, then set the parameters len and token - len is the
 *	number of characters found in the input string and token is the
 *	returned 16-bit token - either an internal keyword or the index
 *	plus type of the entity in the symbol table.
 *********************************************************************************
 */


/*
 * findProcFnName
 *	Scan from the given location and see if it's a Proc/Fn name.
 *********************************************************************************
 */

static int findProcFnName (register char *p, uint16_t tokenType, int *len, uint16_t *token)
{
  char  c ;
  char *q ;
  int len2 = 0 ;

// Skip space

  while (isspace (*p))
  {
    ++p ;
    ++len2 ;
  }

// Must start with a letter or underscore (old BBC Basic compatability)

  if (isalpha (*p) || (*p == '_'))
  {

// Scan to the end

    q = p ;
    while (isalpha (*p) || isdigit (*p) || (*p == '_'))
      { ++len2 ; ++p ; }

    c = *p ; *p = '\0' ;		// Hack - Temporarily zero terminate it
      *token = newSymbolProcFn (q, tokenType) ;
    *p = c ;			// Restore hack

    *len = len2 ;
    return TRUE ;
  }
  return syntaxError ("Invalid Procedure/Function name") ;
}


/*
 * findVariableName
 *	Scan from the given location and see if it's a variable name.
 *********************************************************************************
 */

static int findVariableName (register char *p, int *len, uint16_t *token)
{
  char c ;
  char *r ;
  char *q = p ;
  int len2 = 0 ;
  int isArray, isString ;

  isArray = isString = FALSE ;

// Variables must start with a letter

  if (!isalpha (*p))
    return FALSE ;

// Scan to the end

  while (isalpha (*p) || isdigit (*p) || (*p == '_'))
    { ++len2 ; ++p ; }

// Variables can end with $ to indicate strings

  if (*p == '$')
  {
    isString = TRUE ;
    ++len2 ; ++p ;
  }

// See if it's an array
//	ie. look for a bracket immediately after the variable name

  r = p ;
  while (isspace (*r))
    ++r ;
  if (*r == '(')
    isArray = TRUE ;

  c = *p ; *p = '\0' ;		// Hack - Temporarily zero terminate it
    if (isArray)
    {
      /**/ if (isString)
	*token = newSymbolVariableStringArray (q) ;
      else
	*token = newSymbolVariableRealArray   (q) ;
    }
    else
    {
      if (isString)
	*token = newSymbolVariableString (q) ;
      else
	*token = newSymbolVariableReal   (q) ;
    }
  *p = c ;			// Restore hack

  *len = len2 ;
  return TRUE ;
}


/*
 * findString:
 *	Scan from the given location and see if it's a string.
 *********************************************************************************
 */

static int findString (register char *p, int *len, uint16_t *token)
{
  int len2 = 0 ;
  char *q ;

  *len = 0 ;

  if (*p != '"')
    return FALSE ;

// Get the length

  q = ++p ;
  while (*p && (*p != '"'))
  {
    ++len2 ;
    ++p ;
  }

// Unterminated string?

  if (*p != '"')
    return FALSE ;

  *len = len2 + 2 ;

  *p = '\0' ;		// Hack - temporarily zero terminate it
    *token = newSymbolConstantString (q) ;
  *p = '\"' ;		// Undo hack
  
  return TRUE ;
}


/*
 * findNumber:
 *	Scan from the given location and see if it's a number.
 *********************************************************************************
 */

static int findNumber (register char *p, int *len, uint16_t *token)
{
  char c ;
  int n, l ;
  double realNum ;

//      ( (*p == '+') && (isdigit (*(p+1)) || (*(p+1) == '.'))))		// Positive

  if (isdigit (*p)						||	// Simple
      ( (*p == '.') &&  isdigit (*(p+1)))			||	// Start with a dot
      ( (*p == '-') && (isdigit (*(p+1)) || (*(p+1) == '.'))))		// Negative
  {
    n = sscanf (p, "%lf%n", &realNum, &l) ;
    if (n == 1)
    {
      c = p [l] ;	// Hack - temporarily zero terminate it
      p [l] = '\0' ;

      *token = newSymbolConstantReal (p, realNum) ;

      *len = l ;
      p [l] = c ;	// Restore hack
      return TRUE ;
    }
  }
  return FALSE ;
}


/*
 * findLineNum:
 *	Scan from the given location and see if it's a line number.
 *********************************************************************************
 */

static int findLineNum (register char *p, uint16_t tokenType, int *len, uint16_t *symbol)
{
  int l1, lineNum ;

  lineNum = l1 = 0 ;

  while (isspace (*p))
  {
    ++l1 ;
    ++p ;
  }

// Must start with a digit

  if (!isdigit (*p))
    if (tokenType != TK_SYM_RESTORE)
      return syntaxError ("Invalid line number") ;

  while (isdigit (*p))
  {
    if (lineNum > (MAX_LINENUM / 10))
      return syntaxError ("Line number too big. (Max. %d)\n", MAX_LINENUM) ;
    else
      lineNum = lineNum * 10 + (*p - '0') ;
    ++p ;
    ++l1 ;
  }

  *symbol = newSymbolGotoSubRest (lineNum, tokenType) ;

  *len = l1 ;
  return TRUE ;
}


/*
 * findKeyword:
 *	Scan from the given location, and compare against our keyword
 *	list.
 *********************************************************************************
 */

static int findKeyword (register char *p, int *len, uint16_t *token)
{
  struct keywordStruct *keyword ;
  uint16_t tok ;
  int      len1, len2 ;

// We'll scan the both whole lists in-case we find one that shadows another
//	by being shorter.

  len1 = len2 = -1 ;

  for (keyword = keywords ; keyword->keyword != NULL ; ++keyword)
  {
    len2 = strlen (keyword->keyword) ;
    if (strncasecmp (keyword->keyword, p, len2) == 0)
    {
      if (len2 > len1)
      {
	len1  = len2 ;
	tok = keyword->token ;
      }
    }
  }

  for (keyword = keywordAliases ; keyword->keyword != NULL ; ++keyword)
  {
    len2 = strlen (keyword->keyword) ;
    if (strncasecmp (keyword->keyword, p, len2) == 0)
    {
      if (len2 > len1)
      {
	len1  = len2 ;
	tok = keyword->token ;
      }
    }
  }

  if (len1 == -1)	// Not found
    return FALSE ;

// Special handling

  p += len1 ;
  len2 = 0 ;

  switch (tok)
  {
    case TK_RESTORE:
      if (!findLineNum (p, TK_SYM_RESTORE, &len2, &tok))
	return FALSE ;
      break ;

    case TK_GOTO:
      if (!findLineNum (p, TK_SYM_GOTO, &len2, &tok))
	return FALSE ;
      break ;

    case TK_GOSUB:
      if (!findLineNum (p, TK_SYM_GOSUB, &len2, &tok))
	return FALSE ;
      break ;

    case TK_PROC:
      if (!findProcFnName (p, TK_SYM_PROC, &len2, &tok))
	return FALSE ;
      break ;

    case TK_FN:
      if (!findProcFnName (p, TK_SYM_FUNC, &len2, &tok))
	return FALSE ;
      break ;

    case TK_REM1:
      tok = newSymbolRem1 (p) ;
      len2 = strlen (p) ;
      break ;

    case TK_REM2:
      tok = newSymbolRem2 (p) ;
      len2 = strlen (p) ;
      break ;

  }

  *len   = len1 + len2 ;
  *token = tok ;
  return TRUE ;
}


/*
 * tokenize1:
 *	Take the textual line (NUL terminated) and do the first pass of
 *	tokenisation on it. We return a line with the end of line token
 *	in it.
 *********************************************************************************
 */

int tokenize1 (char *input, uint16_t *tokenizedLine, int *length)
{
  register char *p = input ;
  int      len ;
  uint16_t token, symbol ;
  int needKeyword = FALSE ;

  uint16_t *stuff    = tokenizedLine ;
  int       stuffLen = 0 ;

  while (isspace (*p))
    ++p ;

// Start of the line.
//	We're expecting a keyword or variable name

  if (findKeyword (p, &len, &token))
  {
    p        += len ;
    *stuff++  = token ;
    stuffLen += sizeof (token) ;
    needKeyword = FALSE ;
  }
  else if (findVariableName (p, &len, &symbol))
  {
    p        += len ;
    *stuff++  = symbol ;
    stuffLen += sizeof (symbol) ;
    needKeyword = TRUE ;
  }
  else
    return syntaxError ("Keyword or Variable name expected at start of line") ;

// Carrying on...

  while (*p)
  {

// Skip any leading space

    while (isspace (*p))
      ++p ;

// Check for a (constant) number:

    if (!needKeyword)
    {
      if (stuffLen != 0)	// Can't start a line with a number
	if (findNumber (p, &len, &symbol))
	{
	  p        += len ;
	  *stuff++  = symbol ;
	  stuffLen += sizeof (symbol) ;
	  needKeyword = TRUE ;
	  continue ;
	}
    }

// Keyword? (or other token)

    if (findKeyword (p, &len, &token))
    {
      needKeyword = FALSE ;
      p          += len ;

      *stuff++  = token ;
      stuffLen += sizeof (token) ;

      continue ;
    }

// Variable?

    if (!needKeyword)
    {
      if (findVariableName (p, &len, &symbol))
      {
	p        += len ;
	*stuff++  = symbol ;
	stuffLen += sizeof (symbol) ;
	needKeyword = TRUE ;
	continue ;
      }
    }

// Check for a (constant) string

    if (!needKeyword)
    {
      if (stuffLen != 0)	// Can't start a line with a string
	if (findString (p, &len, &symbol))
	{
	  p        += len ;
	  *stuff++  = symbol ;
	  stuffLen += sizeof (symbol) ;
	  needKeyword = TRUE ;
	  continue ;
	}
    }


// Er... ?

    screenPrintf ("*** Syntax error trying to parse: ... %s\n", p) ;
    return FALSE ;
  }

  *stuff++  = TK_SYM_EOL ;
  stuffLen += sizeof (uint16_t) ;	// Add EOL to length
  *length   = stuffLen ;
  return TRUE ;
}
