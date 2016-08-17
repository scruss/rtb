/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * graphics.c:
 *	Graphics procedures for RTB
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
#include <time.h>

#include <unistd.h>

#include "rtb.h"
#include "bomb.h"
#include "bool.h"
//#include "array.h"

#include "screenKeyboard.h"

#include "keywords.h"
#include "lines.h"
#include "symbolTable.h"
#include "shuntingYard.h"
#include "rpnEval.h"

#include "commands.h"
#include "goto.h"
#include "procFn.h"
#include "run.h"
#include "assign.h"

#include "graphicFns.h"

static int lastX = 0 ;
static int lastY = 0 ;

static double tAngle ;
static double turtleX, turtleY ;
static int    turtlePenDown ;

static int myPlotColour  = 15 ;

#define	NO_GR	"Graphics not initialised"

static int polyX [MAX_POLYGON_NODES] ;
static int polyY [MAX_POLYGON_NODES] ;
static int polyCount ;


/*
 * doUpdate:
 *	Flip the screen
 *********************************************************************************
 */

int doUpdate (void *ptr)
{
  updateDisplay () ;
  return TRUE ;
}


/*
 * doSaveScreen:
 *	Save the current screen as a BMP file - if possible
 *********************************************************************************
 */

int doSaveScreen (void *ptr)
{
  char *s ;
  int   ok ;

  if (!oneString ())
    return syntaxError ("SAVESCREEN: String expected") ;

  s = popS () ;

  ok = saveScreen (s) == 0 ;
  free (s) ;

  return ok ;
}



/*
 * doGr: doHgr:
 *	Clear the screen and initialise the gaphics systems
 *********************************************************************************
 */

int doGr (void *ptr)
{
  screenClear () ;
  setOrigin (0,0) ;
  lastX   = lastY   = 0 ;
  turtleX = lgWidth  / 2.0 ;
  turtleY = lgHeight / 2.0 ;
  tAngle  = 0.0 ;
  turtlePenDown = FALSE ;

  setupLoRes () ;

  return TRUE ;
}

int doHgr (void *ptr)
{
  doGr (ptr) ;

  turtleX = hgWidth  / 2.0 ;
  turtleY = hgHeight / 2.0 ;

  setupHiRes () ;

  return TRUE ;
}


/*
 * doGwidth: doGheight:
 *	Pseudo variables to represent the size of the graphics screen
 *********************************************************************************
 */

int doGwidth (void *ptr)
{
  if (ptr != NULL)
    return FALSE ;

  /**/ if (lores)
    pushN ((double)lgWidth) ;
  else if (hires)
    pushN ((double)hgWidth) ;
  else
    pushN (0.0) ;

  return TRUE ;
}

int doGheight (void *ptr)
{
  if (ptr != NULL)
    return FALSE ;

  /**/ if (lores)
    pushN ((double)lgHeight) ;
  else if (hires)
    pushN ((double)hgHeight) ;
  else
    pushN (0.0) ;

  return TRUE ;
}


/*
 * doColour:
 *	(Pseudo variable)
 *	Change graphics plot colour
 *********************************************************************************
 */

int doColour (void *ptr)
{
  if (ptr == NULL)		// Read
    pushN ((double)myPlotColour) ;
  else				// Write
  {
    myPlotColour = (int)rint (*(double *)ptr) ;
    setPlotColour (myPlotColour) ;
  }
  return TRUE ;
}


/*
 * doRgbColour:
 *	Set an RGB colour
 *********************************************************************************
 */

int doRgbColour (void *ptr)
{
  int b, g, r ;
  
  if (!threeNumbers ())
    return syntaxError ("rgbColour: Expected three numbers") ;

  b = (int)rint (popN ()) ;
  g = (int)rint (popN ()) ;
  r = (int)rint (popN ()) ;

  setRgbPlotColour (r, g, b) ;
  return TRUE ;
}


/*
 * doPlot:
 *	Basic primitive. Plot a point in the current colour
 *********************************************************************************
 */

int doPlot (void *ptr)
{
  int x, y ;

  if (!twoNumbers ())
    return syntaxError ("PLOT: Expected two numbers") ;
    
  if (lores || hires)
  {
    y = (int)rint (popN ()) ;
    x = (int)rint (popN ()) ;

    plotPoint (x, y) ;

    lastX = x ;
    lastY = y ;

    return TRUE ;
  }
  else
    return syntaxError (NO_GR) ;
}


/*
 * doHline
 :	Horizontal and Vertical lines
 *********************************************************************************
 */

int doHline (void *ptr)
{
  int x1, x2, y ;

  if (!threeNumbers ())
    return syntaxError ("HLINE: Expected three numbers") ;
  
  if (lores || hires)
  {
    y  = (int)rint (popN ()) ;
    x1 = (int)rint (popN ()) ;
    x2 = (int)rint (popN ()) ;

    drawLine (x1, y, x2, y) ;
    return TRUE ;
  }
  else
    return syntaxError (NO_GR) ;
}

int doVline (void *ptr)
{
  int y1, y2, x ;
  
  if (!threeNumbers ())
    return syntaxError ("VLINE: Expected three numbers") ;
  
  if (lores || hires)
  {
    x  = (int)rint (popN ()) ;
    y1 = (int)rint (popN ()) ;
    y2 = (int)rint (popN ()) ;

    drawLine (x, y1, x, y2) ;
    return TRUE ;
  }
  else
    return syntaxError (NO_GR) ;
}


/*
 * doLine:
 *	General purpose line from x1,y1 to x2,y2
 *********************************************************************************
 */

int doLine (void *ptr)
{
  int x1, y1, x2, y2 ;
  
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!fourNumbers ())
    return syntaxError ("LINE: Expected four numbers") ;
  
  y2 = (int)rint (popN ()) ;
  x2 = (int)rint (popN ()) ;
  y1 = (int)rint (popN ()) ;
  x1 = (int)rint (popN ()) ;

  drawLine (x1, y1, x2, y2) ;

  lastX = x2 ;
  lastY = y2 ;

  return TRUE ;
}


/*
 * doLineTo:
 *	Draw a line from the last line end to our new x,y
 *********************************************************************************
 */

int doLineTo (void *ptr)
{
  int x, y ;
  
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!twoNumbers ())
    return syntaxError ("LINETO: Expected two numbers") ;
  
  y = (int)rint (popN ()) ;
  x = (int)rint (popN ()) ;

  drawLine (lastX, lastY, x, y) ;

  lastX = x ;
  lastY = y ;

  return TRUE ;
}


/*
 * polyStart:
 *	A polygon is when a parrot leaves home.
 *********************************************************************************
 */

int doPolyStart (void *ptr)
{
  if (lores || hires)
  {
    polyCount = 0 ;
    return TRUE ;
  }
  else
    return syntaxError (NO_GR) ;
}


/*
 * polyPlot:
 *	Poke the parrot
 *********************************************************************************
 */

int doPolyPlot (void *ptr)
{
  if (!twoNumbers ())
    return syntaxError ("POLYPLOT: Expected two numbers") ;
    
  if (lores || hires)
  {
    if (polyCount == MAX_POLYGON_NODES)
      return syntaxError ("Out of perches") ;

    polyY [polyCount] = (int)rint (popN ()) ;
    polyX [polyCount] = (int)rint (popN ()) ;

    ++polyCount ;

    return TRUE ;
  }
  else
    return syntaxError (NO_GR) ;
}

int doPolyEnd (void *ptr)
{
  if (lores || hires)
  {
    if (polyCount == 0)
      return syntaxError ("No polygon points") ;

    polygonFill (polyCount, polyX, polyY) ;
    return TRUE ;
  }
  else
    return syntaxError (NO_GR) ;
}


/*
 * doCircle:
 *	General purpose circle.
 *********************************************************************************
 */

int doCircle (void *ptr)
{
  int x, y, r, f ;
  
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!fourNumbers ())
    return syntaxError ("CIRCLE: Expected four numbers") ;
  
  f = (int)rint (popN ()) ;
  r = (int)rint (popN ()) ;
  y = (int)rint (popN ()) ;
  x = (int)rint (popN ()) ;

  drawCircle (x, y, r, f) ;
  return TRUE ;
}


/*
 * doEllipse:
 *	General purpose ellipse.
 *********************************************************************************
 */

int doEllipse (void *ptr)
{
  int cx, cy, xr, yr, f ;
  
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!fiveNumbers ())
    return syntaxError ("ELLIPSE: Expected five numbers") ;
  
  f  = (int)rint (popN ()) ;
  yr = (int)rint (popN ()) ;
  xr = (int)rint (popN ()) ;
  cy = (int)rint (popN ()) ;
  cx = (int)rint (popN ()) ;

  drawEllipse (cx, cy, xr, yr, f) ;
  return TRUE ;
}



/*
 * doRectangle:
 *	A rectangle is a spoilt days fishing.
 *********************************************************************************
 */

int doRectangle (void *ptr)
{
  int i, x, y, w, h, f ;
  
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!fiveNumbers ())
    return syntaxError ("RECT: Expected five numbers") ;
  
  f = (int)popN () ;
  h = (int)popN () ;
  w = (int)popN () ;
  y = (int)popN () ;
  x = (int)popN () ;

  if ((w != 0) && (h != 0))
  {
    if (f)
    {
      --w ;
      for (i = 0 ; i < h ; ++i)
	drawLine (x  ,y+i  , x+w,y+i) ;
    }
    else
    {
      --w ;
      --h ;
      drawLine (x  ,y  , x+w,y  ) ;
      drawLine (x+w,y  , x+w,y+h) ;
      drawLine (x+w,y+h, x  ,y+h) ;
      drawLine (x  ,y+h, x  ,y  ) ;
    }
  }
  return TRUE ;
}


/*
 * doTriangle:
 *	A triangle goes ping.
 *********************************************************************************
 */

int doTriangle (void *ptr)
{
  int f, x1, x2, x3, y1, y2, y3 ;
  
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!sevenNumbers ())
    return syntaxError ("TRIANGLE: Expected seven numbers") ;
  
  f  = (int)popN () ;
  y3 = (int)popN () ;
  x3 = (int)popN () ;
  y2 = (int)popN () ;
  x2 = (int)popN () ;
  y1 = (int)popN () ;
  x1 = (int)popN () ;

  if (f)	// Filled
  {
    polyX [0] = x1 ; polyY [0] = y1 ;
    polyX [1] = x2 ; polyY [1] = y2 ;
    polyX [2] = x3 ; polyY [2] = y3 ;
    polygonFill (3, polyX, polyY) ;
  }
  else
  {
    drawLine (x1 ,y1  , x2, y2) ;
    drawLine (x2 ,y2  , x3, y3) ;
    drawLine (x3 ,y3  , x1, y1) ;
  }

  return TRUE ;
}



/*
 * doOrigin:
 *	Set the graphics origin
 *********************************************************************************
 */

int doOrigin (void *ptr)
{
  int x, y ;
  
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!twoNumbers ())
    return syntaxError ("ORIGIN: Expected two numbers") ;
  
  y = (int)rint (popN ()) ;
  x = (int)rint (popN ()) ;

  if (lores)
  {
    if ((x >= lgWidth) || (x < 0))
      return syntaxError ("ORIGIN: X is off-screen") ;

    if ((y >= lgHeight) || (y < 0))
      return syntaxError ("ORIGIN: Y is off-screen") ;
  }
  else
  {
    if ((x >= hgWidth) || (x < 0))
      return syntaxError ("ORIGIN: X is off-screen") ;

    if ((y >= hgHeight) || (y < 0))
      return syntaxError ("ORIGIN: Y is off-screen") ;
  }

  setOrigin (x, y) ;
  return TRUE ;
}


/*
 * doPenDown: doPenUp:
 *	Change the state of the turtles pen
 *********************************************************************************
 */

int doPenDown (void *ptr)
{
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  turtlePenDown = TRUE ;
  return TRUE ;
}

int doPenUp (void *ptr)
{
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  turtlePenDown = FALSE ;
  return TRUE ;
}


/*
 * doMove:
 *	Move the turtle
 *********************************************************************************
 */

int doMove (void *ptr)
{
  double newX, newY ;
  double d ;

  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!oneNumber ())
    return syntaxError ("MOVE: Expected a number") ;

  d = popN () ;

  newX = turtleX + d * sin (angleConversion * tAngle) ;
  newY = turtleY + d * cos (angleConversion * tAngle) ;

  if (turtlePenDown)
    drawLine ((int)rint (turtleX), (int)rint (turtleY), (int)rint (newX), (int)rint (newY)) ;

  turtleX = newX ;
  turtleY = newY ;

  return TRUE ;
}


/*
 * doMoveTo:
 *	Move the turtle an an absolute location
 *********************************************************************************
 */

int doMoveTo (void *ptr)
{
  double newX, newY ;

  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!twoNumbers ())
    return syntaxError ("MOVETO: Expected two numbers") ;

  newY = popN () ;
  newX = popN () ;

  if (turtlePenDown)
    drawLine ((int)rint (turtleX), (int)rint (turtleY), (int)rint (newX), (int)rint (newY)) ;

  turtleX = newX ;
  turtleY = newY ;

  return TRUE ;
}


/*
 * doLeft: doRight:
 *	Turn the turtle
 *********************************************************************************
 */

int doLeft (void *ptr)
{
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!oneNumber ())
    return syntaxError ("LEFT: Expected a number") ;

  tAngle -= popN () ;
  return TRUE ;
}

int doRight (void *ptr)
{
  if (! (lores || hires))
    return syntaxError (NO_GR) ;

  if (!oneNumber ())
    return syntaxError ("RIGHT: Expected a number") ;

  tAngle += popN () ;
  return TRUE ;
}


/*
 * doTangle:
 *	Set the turtle angle to an absolute value
 *	PseudoVariable - R/W
 *********************************************************************************
 */

int doTangle (void *ptr)
{
  if (ptr == NULL)		// Read
    pushN ((double)tAngle) ;
  else				// Write
    tAngle = *(double *)ptr ;
  return TRUE ;
}
