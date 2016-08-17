/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * clipper.c:
 *	Implement the Cohen-Suterland line endpoint clipping algorithm
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

#include <stdint.h>

#include "bool.h"
#include "screenKeyboard.h"
#include "clipper.h"

// The "outcode" Matrix:
//
//    +------+------+------+
//    | 1001 | 1000 | 1010 |
//    +------+------+------+
//    | 0001 | 0000 | 0010 |
//    +------+------+------+
//    | 0101 | 0100 | 0110 |
//    +------+------+------+
 
#define	OC_INSIDE	0
#define	OC_LEFT		1
#define	OC_RIGHT	2
#define	OC_BOTTOM	4
#define	OC_TOP		8


/*
 * outCode:
 *	Return the outcode for the position supplied
 *********************************************************************************
 */

static uint8_t outCode (register int x, register int y)
{
  uint8_t code = OC_INSIDE ;

  if (lores)
  {
    /**/ if (x <  0)
      code |= OC_LEFT ;
    else if (x >= lgWidth)
      code |= OC_RIGHT ;

    /**/ if (y < 0)
      code |= OC_BOTTOM ;
    else if (y >= lgHeight)
      code |= OC_TOP ;
  }
  else
  {
    /**/ if (x <  0)
      code |= OC_LEFT ;
    else if (x >= hgWidth)
      code |= OC_RIGHT ;

    /**/ if (y < 0)
      code |= OC_BOTTOM ;
    else if (y >= hgHeight)
      code |= OC_TOP ;
  }

  return code ;
}


/*
 * lineClip:
 *	Return the new end-points of the line, or FALSE if line is
 *	entirely outside the window.
 *********************************************************************************
 */

int lineClip (int *ix0, int *iy0, int *ix1, int *iy1)
{
  uint8_t oc, oc0, oc1 ;

  int x,  y ;
  int x0, y0 ;
  int x1, y1 ;

  int xMax, xMin, yMax, yMin ;


// Convert to screen co-ords and set min & max parameters

  if (lores)
  {
    *ix0 += xOrigin ;
    *ix1 += xOrigin ;

    *iy0  = lgHeight - 1 - (*iy0 + yOrigin) ;
    *iy1  = lgHeight - 1 - (*iy1 + yOrigin) ;

    xMin = yMin = 0 ;
    xMax = lgWidth  - 1 ;
    yMax = lgHeight - 1 ;
  }
  else
  {
    *ix0 += xOrigin ;
    *ix1 += xOrigin ;

    *iy0  = hgHeight - 1 - (*iy0 + yOrigin) ;
    *iy1  = hgHeight - 1 - (*iy1 + yOrigin) ;

    xMin = yMin = 0 ;
    xMax = hgWidth  - 1 ;
    yMax = hgHeight - 1 ;
  }

  x0 = *ix0 ;
  y0 = *iy0 ;
  x1 = *ix1 ;
  y1 = *iy1 ;

  for (;;)
  {
    oc0 = outCode (x0, y0) ;
    oc1 = outCode (x1, y1) ;

    /**/ if ((oc0 | oc1) == 0)	// Both points inside
    {
      break ;
    }
    else if ((oc0 & oc1) != 0)	// Both points outside
      return FALSE ;
    else
    {
      oc = oc0 ? oc0 : oc1 ;
      /**/ if (oc & OC_TOP)
      {
	x = x0 + (x1 - x0) * (yMax - y0) / (y1 - y0) ;
	y = yMax ;
      }
      else if (oc & OC_BOTTOM)
      {
	x = x0 + (x1 - x0) * (yMin - y0) / (y1 - y0) ;
	y = yMin ;
      }
      else if (oc & OC_RIGHT)
      {
	y = y0 + (y1 - y0) * (xMax - x0) / (x1 - x0) ;
	x = xMax ;
      }
      else 
      {
	y = y0 + (y1 - y0) * (xMin - x0) / (x1 - x0) ;
	x = xMin ;
      }
    }

    if (oc == oc0)
    {
      x0  = x ;
      y0  = y ;
      oc0 = outCode (x0, y0) ;
    }
    else
    {
      x1  = x ;
      y1  = y ;
      oc1 = outCode (x1, y1) ;
    }
  }

  *ix0 = x0 ;
  *iy0 = y0 ;
  *ix1 = x1 ;
  *iy1 = y1 ;

  return TRUE ;
}
