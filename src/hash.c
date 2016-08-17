/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * hash.c:
 *	Simple short string hashing function.
 *	Based on: The Practice of Programming (HASH TABLES, pg. 57)
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

#include "hash.h"

#define	MULTIPLIER	37

unsigned int hash (char *str, int nHash)
{
  register unsigned int   h = 0 ;
  register unsigned char *p ;

  for (p = (unsigned char*)str ; *p ; ++p)
    h = MULTIPLIER * h + *p ;

  if (nHash == 0)
    return h ;
  else
    return h % nHash ;
}
