/*
 * returnToBasic.c:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * bomb.c:
 *	Something has gone wrong, tidyup and windowing, graphics, etc.
 *	and exit to the OS
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
#include <string.h>
#include <errno.h>

#include "bomb.h"
#include "screenKeyboard.h"

void bomb (char *s, int useErrno)
{
  int x = errno ;

  screenPuts ("\n\n\n*** BOMB ***\n") ;
  screenPrintf ("*** Fatal error: %s", s) ;

  if (useErrno)
    screenPrintf (": %s.\n", strerror (x)) ;
  else
    printf (".\n") ;

  while (1) ;
}
