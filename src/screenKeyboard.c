/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * screenKeyboard.c:
 *	Imprement the screen output, and keyboard input via the SDL
 *	graphics library.
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


#define	KEYBOARD_BUFFER_SIZE	 32
#define	KEYBOARD_BUFFER_MASK	 (KEYBOARD_BUFFER_SIZE - 1)

#define	DEFAULT_SCREEN_X	640
#define	DEFAULT_SCREEN_Y	480

#define	LOW_RES_PIXELS_WIDE		8
#define	LOW_RES_PIXELS_HIGH		8

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include <unistd.h>

#include "version.h"
#include "rtb.h"

#include "bool.h"
#include "clipper.h"
#include "spriteFns.h"
#include "screenKeyboard.h"

#include "font.h"

// Globals

SDL_Surface *myScreen ;

int bytesPerPixel = 0 ;
int  bitsPerPixel = 0 ;

// Locals

static       SDL_Thread     *keyboardThread ;
static const SDL_VideoInfo  *videoInfo ;
static       SDL_Rect      **videoModes ;
static       int             numVideoModes ;
static       int             savedKeypress = -1 ;

static int screenWidth  ;
static int screenHeight ;

static int doubleSizeFont = FALSE ;



int fontHeight = 10 ;
int fontWidth  = 8 ;

int  tWidth,  tHeight ;
int lgWidth, lgHeight ;
int hgWidth, hgHeight ;

static int hgHeight1, lgHeight1 ;

uint32_t fgTextColour ;
uint32_t bgTextColour ;
uint32_t plotColour ;

// Text cursor position

int cursorX, cursorY ;

// Misc

int lores  = FALSE ;
int hires  = FALSE ;

// Locals

int xOrigin = 0 ;
int yOrigin = 0 ;

// Functions:

void (*screenPutchar) (uint8_t c, int doUpdate) ;
void (*plotPoint)     (int x, int y) ;
void (*drawLine)      (register int x0, register int y0, register int x1, register int y1) ;


/*
 * stdR: stdG: stdB:
 *	Our standard colours - Based on the original CGA colous
 *********************************************************************************
 */

//  0: Black
//  1: Navy (Low Blue)
//  2: Green (Low Green)
//  3: Teal (Low Aqua/Cyan)
//  4: Maroon (Low Red)
//  5: Purple (Low Fuchsia/Magenta)
//  6: Olive (Low Yellow, Brown?)
//  7: Silver (Light Grey)
//  8: Grey
//  9: Blue
// 10: Lime (Green)
// 11: Aqua (Cyan)
// 12: Red
// 13: Pink (Fuchsia/Magenta)
// 14: Yellow
// 15: White

static uint8_t stdR [16] = {0x00,0x00,0x00,0x00,0x80,0x80,0x80,0xC0,0x80,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF} ;
static uint8_t stdG [16] = {0x00,0x00,0x80,0x80,0x00,0x00,0x80,0xC0,0x80,0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF} ;
static uint8_t stdB [16] = {0x00,0x80,0x00,0x80,0x00,0x80,0x00,0xC0,0x80,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF} ;

/*
 * Sprite handling
 *********************************************************************************
 */



/*
 * saveScreen:
 *	Save screen to a BMP file
 *********************************************************************************
 */

int saveScreen (char *filename)
{
  return SDL_SaveBMP (myScreen, filename) ;
}


/*
 * lockScreen: unlockScreen:
 *	Call the SDL lock/unlock code
 *********************************************************************************
 */

static void lockScreen (void)
{
  if (SDL_MUSTLOCK (myScreen)) 
    SDL_LockSurface (myScreen) ;
}

static void unlockScreen (void)
{
  if (SDL_MUSTLOCK (myScreen))
    SDL_UnlockSurface (myScreen) ;
}

/*
 * updateDisplay:
 *	Make sure the display is updated and visible
 *********************************************************************************
 */

void updateDisplay (void)
{
  updateSprites () ;
  SDL_Flip (myScreen) ;
}


/*
 * setTextCursor:
 *	Set the position on the text cursor.
 *	Top left is 0,0.
 *********************************************************************************
 */

void setTextCursor (int x, int y)
{
  if (x >= tWidth)	x = tWidth - 1 ;
  if (x < 0)		x = 0 ;

  if (y >= tHeight)	y = tHeight - 1 ;
  if (y < 0)		y = 0 ;

  cursorX = x ;
  cursorY = y ;
}


/*
 * screenClear:
 *	Clear and set the background colour
 *********************************************************************************
 */

void screenClear (void)
{
  lockScreen () ;
    SDL_FillRect (myScreen, NULL, bgTextColour) ;
  unlockScreen () ;

  setTextCursor (0,0) ;	// Top left
  updateDisplay () ;
}


/*
 * setTextBgColour: setTextFgColour: setPlotColour:
 *	Set the foregound and background colours for printing text and the
 *	colour to draw graphics in.
 *********************************************************************************
 */

void setTextBgColour (uint8_t bgCol)
  { bgTextColour = SDL_MapRGB (myScreen->format, stdR [bgCol], stdG [bgCol], stdB [bgCol]) ; }

void setTextFgColour (uint8_t fgCol)
  { fgTextColour = SDL_MapRGB (myScreen->format, stdR [fgCol], stdG [fgCol], stdB [fgCol]) ; }

void setPlotColour (uint8_t ptCol)
  { plotColour   = SDL_MapRGB (myScreen->format, stdR [ptCol], stdG [ptCol], stdB [ptCol]) ; }


/*
 * setRgbPlotColour:
 *	Set the graphics drawing colour explicitly
 *********************************************************************************
 */

void setRgbPlotColour (uint8_t r, uint8_t g, uint8_t b)
  { plotColour   = SDL_MapRGB (myScreen->format, r, g, b) ; }


/*
 * scrollScreen:
 *	Move it up by one lines worth of space, creating a new band of the
 *	background colour at the bottom
 *********************************************************************************
 */

static void scrollScreen (int lines)
{
  SDL_Rect scrollRect, boxRect ;

  scrollRect.x = 0 ;
  scrollRect.y = fontHeight ;
  scrollRect.w = screenWidth;
  scrollRect.h = screenHeight - fontHeight ;

  SDL_BlitSurface (myScreen, &scrollRect, myScreen, NULL) ;

  boxRect.x = 0 ;
  boxRect.y = screenHeight - fontHeight ;
  boxRect.w = screenWidth ;
  boxRect.h = fontHeight ;

  lockScreen () ;
    SDL_FillRect (myScreen, &boxRect, bgTextColour) ;
  unlockScreen () ;
}


/*
 * screenPutchar:
 *	Print a single character to the screen
 *********************************************************************************
 */

void screenPutchar16 (uint8_t c, int doUpdate)
{
  uint8_t *fontPtr = &font [c * fontHeight] ;
  uint8_t data ;

  uint16_t *screenPtr ;
  uint16_t *screenPtr1 ;

  register int y, y1 ;

// Specials

  if (c == 8)			// BS
  {
    if (cursorX == 0)
    {
      cursorX = tWidth - 1 ;
      if (cursorY != 0)
	--cursorY ;
    }
    else
      --cursorX ;
    return ;
  }

  if (c == '\r')		// CR
  {
    cursorX = 0 ;
    return ;
  }

  if (c == '\n')		// NL
  {
    cursorX = 0 ;
    if ((cursorY + 1) == tHeight)
      scrollScreen (1) ;
    else
      ++cursorY ;
    updateDisplay () ;		// Automatic update
    return ;
  }

// Can't print if we're offscreen

  if ((cursorX >= 0) && (cursorX < tWidth) && (cursorY >= 0) && (cursorY < tHeight))
  {

// Work out starting screen memory location

    screenPtr1 = myScreen->pixels + ( (cursorX * fontWidth * bytesPerPixel) +
		(cursorY * fontHeight * screenWidth * bytesPerPixel) ) ;

    lockScreen () ;
    if (doubleSizeFont)
    {
      fontPtr = &font [c * fontHeight / 2] ;
      for (y = 0 ; y < fontHeight ; y += 2)
      {
	data = *fontPtr++ ;
        for (y1 = 0 ; y1 < 2 ; ++y1)
	{
	  screenPtr = screenPtr1 + (y + y1) * screenWidth ;

	  if ((data & 0x80) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x80) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x40) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x40) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x20) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x20) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x10) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x10) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x08) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x08) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x04) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x04) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x02) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x02) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x01) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x01) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	}
      }
    }
    else
    {
      fontPtr = &font [c * fontHeight] ;
      for (y = 0 ; y < fontHeight ; ++y)
      {
	data = *fontPtr++ ;
	screenPtr = screenPtr1 + y * screenWidth ;

	if ((data & 0x80) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x40) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x20) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x10) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x08) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x04) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x02) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x01) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
      }
    }
    unlockScreen () ;
  }

  if (doUpdate)
    SDL_UpdateRect (myScreen, cursorX * fontWidth, cursorY * fontHeight, fontWidth, fontHeight) ;

  cursorX += 1 ;
  if (cursorX == tWidth)
  {
    cursorX = 0 ;
    if ((cursorY + 1) == tHeight)
    {
      scrollScreen (1) ;
      updateDisplay () ;
    }
    else
      ++cursorY ;
  }
}

void screenPutchar32 (uint8_t c, int doUpdate)
{
  uint8_t *fontPtr = &font [c * fontHeight] ;
  uint8_t data ;

  uint32_t *screenPtr ;
  uint32_t *screenPtr1 ;

  register int y, y1 ;

// Specials

  if (c == 8)			// BS
  {
    if (cursorX == 0)
    {
      cursorX = tWidth - 1 ;
      if (cursorY != 0)
	--cursorY ;
    }
    else
      --cursorX ;
    return ;
  }

  if (c == '\r')		// CR
  {
    cursorX = 0 ;
    return ;
  }

  if (c == '\n')		// NL
  {
    cursorX = 0 ;
    if ((cursorY + 1) == tHeight)
      scrollScreen (1) ;
    else
      ++cursorY ;
    updateDisplay () ;		// Automatic update
    return ;
  }

// Can't print if we're offscreen

  if ((cursorX >= 0) && (cursorX < tWidth) && (cursorY >= 0) && (cursorY < tHeight))
  {

// Work out starting screen memory location

    screenPtr1 = myScreen->pixels + ( (cursorX * fontWidth * bytesPerPixel) +
		(cursorY * fontHeight * screenWidth * bytesPerPixel) ) ;

    lockScreen () ;
    if (doubleSizeFont)
    {
      fontPtr = &font [c * fontHeight / 2] ;
      for (y = 0 ; y < fontHeight ; y += 2)
      {
	data = *fontPtr++ ;
        for (y1 = 0 ; y1 < 2 ; ++y1)
	{
	  screenPtr = screenPtr1 + (y + y1) * screenWidth ;

	  if ((data & 0x80) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x80) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x40) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x40) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x20) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x20) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x10) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x10) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x08) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x08) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x04) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x04) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x02) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x02) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x01) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	  if ((data & 0x01) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	}
      }
    }
    else
    {
      fontPtr = &font [c * fontHeight] ;
      for (y = 0 ; y < fontHeight ; ++y)
      {
	data = *fontPtr++ ;
	screenPtr = screenPtr1 + y * screenWidth ;

	if ((data & 0x80) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x40) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x20) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x10) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x08) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x04) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x02) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
	if ((data & 0x01) == 0) *screenPtr = bgTextColour ; else *screenPtr = fgTextColour ; ++screenPtr ;
      }
    }
    unlockScreen () ;
  }

  if (doUpdate)
    SDL_UpdateRect (myScreen, cursorX * fontWidth, cursorY * fontHeight, fontWidth, fontHeight) ;

  cursorX += 1 ;
  if (cursorX == tWidth)
  {
    cursorX = 0 ;
    if ((cursorY + 1) == tHeight)
    {
      scrollScreen (1) ;
      updateDisplay () ;
    }
    else
      ++cursorY ;
  }
}


/*
 * screenPuts:
 *	Print a string to the screen
 *********************************************************************************
 */

void screenPuts (char *s)
{
  while (*s)
    screenPutchar (*s++, FALSE) ;
}


/*
 * screenPrintf:
 *	Emulate printf to the screen
 *********************************************************************************
 */

void screenPrintf (char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  va_start (argp, message) ;
    vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  screenPuts (buffer) ;
}


/*
 * setOrigin:
 *	Set the graphics origin (0,0) point.
 *********************************************************************************
 */

void setOrigin (int x, int y)
{
  xOrigin = x ;
  yOrigin = y ;
}


/*
 * plotPoint:
 *	Plot a single point on the screen
 *********************************************************************************
 */

void plotPointLo (register int x, register int y)
{
  SDL_Rect blob ;

  x += xOrigin ;
  y += yOrigin ;
  y  = lgHeight1 - y ;

  if ((x >= 0) && (x < lgWidth) && (y >= 0) && (y < lgHeight))
  {
    blob.x = x * LOW_RES_PIXELS_WIDE ; blob.w = LOW_RES_PIXELS_WIDE ;
    blob.y = y * LOW_RES_PIXELS_HIGH ; blob.h = LOW_RES_PIXELS_HIGH ;
    SDL_FillRect (myScreen, &blob, plotColour) ;
  }
}

static void plotPoint16hi (register int x, register int y)
{
  x += xOrigin ;
  y += yOrigin ;
  y  = hgHeight1 - y ;

  if ((x >= 0) && (x < hgWidth) && (y >= 0) && (y < hgHeight))
    *((uint16_t *)myScreen->pixels + y * hgWidth + x)  = (uint16_t)plotColour ;
}

static void plotPoint32hi (register int x, register int y)
{
  x += xOrigin ;
  y += yOrigin ;
  y  = hgHeight1 - y ;

  if ((x >= 0) && (x < hgWidth) && (y >= 0) && (y < hgHeight))
    *((uint32_t *)myScreen->pixels + y * hgWidth + x)  = plotColour ;
}


/*
 * drawLine:
 *	Draw a line on the screen
 *	This is Bresenham's line algorithm
 *********************************************************************************
 */

static void drawLineLo (int x0, int y0, int x1, int y1)
{
  SDL_Rect blob ;

  int dx, dy ;
  int sx, sy ;
  int err, e2 ;

  if (!lineClip (&x0, &y0, &x1, &y1))
    return ;

  dx = abs (x1 - x0) ;
  dy = abs (y1 - y0) ;

  sx = (x0 < x1) ? 1 : -1 ;
  sy = (y0 < y1) ? 1 : -1 ;

  err = dx - dy ;
 
  for (;;)
  {
    blob.x = x0 * LOW_RES_PIXELS_WIDE ; blob.w = LOW_RES_PIXELS_WIDE ;
    blob.y = y0 * LOW_RES_PIXELS_HIGH ; blob.h = LOW_RES_PIXELS_HIGH ;
    SDL_FillRect (myScreen, &blob, plotColour) ;

    if ((x0 == x1) && (y0 == y1))
      break ;

    e2 = 2 * err ;

    if (e2 > -dy)
    {
      err -= dy ;
      x0  += sx ;
    }

    if (e2 < dx)
    {
      err += dx ;
      y0  += sy ;
    }
  }
}

static void drawLine16hi (int x0, int y0, int x1, int y1)
{
  int dx, dy ;
  int sx, sy ;
  int err, e2 ;

  if (!lineClip (&x0, &y0, &x1, &y1))
    return ;

  dx = abs (x1 - x0) ;
  dy = abs (y1 - y0) ;

  sx = (x0 < x1) ? 1 : -1 ;
  sy = (y0 < y1) ? 1 : -1 ;

  err = dx - dy ;
 
  for (;;)
  {
    *((uint16_t *)myScreen->pixels + y0 * hgWidth + x0)  = (uint16_t)plotColour ;

    if ((x0 == x1) && (y0 == y1))
      break ;

    e2 = 2 * err ;

    if (e2 > -dy)
    {
      err -= dy ;
      x0  += sx ;
    }

    if (e2 < dx)
    {
      err += dx ;
      y0  += sy ;
    }
  }
}

static void drawLine32hi (int x0, int y0, int x1, int y1)
{
  int dx, dy ;
  int sx, sy ;
  int err, e2 ;

  if (!lineClip (&x0, &y0, &x1, &y1))
    return ;

  dx = abs (x1 - x0) ;
  dy = abs (y1 - y0) ;

  sx = (x0 < x1) ? 1 : -1 ;
  sy = (y0 < y1) ? 1 : -1 ;

  err = dx - dy ;
 
  for (;;)
  {
    *((uint32_t *)myScreen->pixels + y0 * hgWidth + x0)  = plotColour ;

    if ((x0 == x1) && (y0 == y1))
      break ;

    e2 = 2 * err ;

    if (e2 > -dy)
    {
      err -= dy ;
      x0  += sx ;
    }

    if (e2 < dx)
    {
      err += dx ;
      y0  += sy ;
    }
  }
}


/*
 * drawCircle:
 *	This is the midpoint circle algorithm.
 *********************************************************************************
 */

void drawCircle (int x, int y, int r, int fill)
{
  int ddF_x = 1 ;
  int ddF_y = -2 * r ;

  int f = 1 - r ;
  int x1 = 0 ;
  int y1 = r ;
 
  if (fill)
  {
    drawLine (x, y + r, x, y - r) ;
    drawLine (x + r, y, x - r, y) ;
  }
  else
  {
    plotPoint (x, y + r) ;
    plotPoint (x, y - r) ;
    plotPoint (x + r, y) ;
    plotPoint (x - r, y) ;
  }
 
  while (x1 < y1)
  {
    // ddF_x == 2 * x1 + 1 ;
    // ddF_y == -2 * y1 ;
    // f == x1*x1 + y1*y1 - r*r + 2*x1 - y1 + 1 ;
    if (f >= 0) 
    {
      y1-- ;
      ddF_y += 2 ;
      f += ddF_y ;
    }
    x1++ ;
    ddF_x += 2 ;
    f += ddF_x ;    
    if (fill)
    {
      drawLine (x + x1, y + y1, x - x1, y + y1) ;
      drawLine (x + x1, y - y1, x - x1, y - y1) ;
      drawLine (x + y1, y + x1, x - y1, y + x1) ;
      drawLine (x + y1, y - x1, x - y1, y - x1) ;
    }
    else
    {
      plotPoint (x + x1, y + y1) ; plotPoint (x - x1, y + y1) ;
      plotPoint (x + x1, y - y1) ; plotPoint (x - x1, y - y1) ;
      plotPoint (x + y1, y + x1) ; plotPoint (x - y1, y + x1) ;
      plotPoint (x + y1, y - x1) ; plotPoint (x - y1, y - x1) ;
    }
  }
}


/*
 * drawEllipse:
 *	Fast ellipse drawing algorithm by 
 *      John Kennedy
 *	Mathematics Department
 *	Santa Monica College
 *	1900 Pico Blvd.
 *	Santa Monica, CA 90405
 *	jrkennedy6@gmail.com
 *********************************************************************************
 */


static void plot4ellipsePoints (int cx, int cy, int x, int y, int fill)
{
  if (fill)
  {
    drawLine (cx + x, cy + y, cx - x, cy + y) ;
    drawLine (cx - x, cy - y, cx + x, cy - y) ;
  }
  else
  {
    plotPoint (cx + x, cy + y) ;
    plotPoint (cx - x, cy + y) ;
    plotPoint (cx - x, cy - y) ;
    plotPoint (cx + x, cy - y) ;
  }
}

void drawEllipse (int cx, int cy, int xRadius, int yRadius, int fill)
{
  int x, y ;
  int xChange, yChange, ellipseError ;
  int twoAsquare, twoBsquare ;
  int stoppingX, stoppingY ;

  twoAsquare = 2 * xRadius * xRadius ;
  twoBsquare = 2 * yRadius * yRadius ;

  x = xRadius ;
  y = 0 ;

  xChange = yRadius * yRadius * (1 - 2 * xRadius) ;
  yChange = xRadius * xRadius ;

  ellipseError = 0 ;
  stoppingX    = twoBsquare * xRadius ;
  stoppingY    = 0 ;

  while (stoppingX >= stoppingY)	// 1st set of points
  {
    plot4ellipsePoints (cx, cy, x, y, fill) ;
    ++y ;
    stoppingY    += twoAsquare ;
    ellipseError += yChange ;
    yChange      += twoAsquare ;

    if ((2 * ellipseError + xChange) > 0 )
    {
      --x ;
      stoppingX    -= twoBsquare ;
      ellipseError += xChange ;
      xChange      += twoBsquare ;
    }
  }

  x = 0 ;
  y = yRadius ;

  xChange = yRadius * yRadius ;
  yChange = xRadius * xRadius * (1 - 2 * yRadius) ;

  ellipseError = 0 ;
  stoppingX    = 0 ;
  stoppingY    = twoAsquare * yRadius ;

  while (stoppingX <= stoppingY)	//2nd set of points
  {
    plot4ellipsePoints (cx, cy, x, y, fill) ;
    ++x ;
    stoppingX    += twoBsquare ;
    ellipseError += xChange ;
    xChange      += twoBsquare ;

    if ((2 * ellipseError + yChange) > 0 )
    {
      --y ;
      stoppingY -= twoAsquare ;
      ellipseError += yChange ;
      yChange += twoAsquare ;
    }
  }
}



/*
 * polygonFill:
 *	I was looking for a general purpose triangle fill routine and found this.
 *	It's by Darel Rex Finley - see http://alienryderflex.com/polygon_fill/
 *
 *	I think it's not quite perfect, so I'll actually draw the lines in-order
 *	to make sure the outline is there - sometimes it's missing a pixel...
 *	... and even then it's still not perfect, but it'll do for now...
 *
 */

void polygonFill (int polyCorners, int mpX [MAX_POLYGON_NODES], int mpY [MAX_POLYGON_NODES])
{
  int nodeX [MAX_POLYGON_NODES] ;
  int nodes, py, i, j, swap ;

  int top, bot, left, right ;

  double polyX [MAX_POLYGON_NODES], polyY [MAX_POLYGON_NODES] ;

  if (polyCorners < 3)	// Duh!
    return ;

  for (i = 1 ; i < polyCorners ; ++i)
    drawLine (mpX [i - 1], mpY [i - 1], mpX [i], mpY [i]) ;

  for (i = 0 ; i < polyCorners ; ++i)
  {
    polyX [i] = (double)mpX [i] ;
    polyY [i] = (double)mpY [i] ;
  }

  bot = left = 0 ;

  if (lores)
  {
    top   = lgHeight - 1 ;
    right = lgWidth  - 1 ;
  }
  else
  {
    top   = hgHeight - 1 ;
    right = hgWidth  - 1 ;
  }

//  Loop through the rows of the image.

    for (py = bot ; py < top ; ++py)
    {

//  Build a list of nodes.

      nodes = 0 ;
      j = polyCorners - 1 ;
      for (i = 0 ; i < polyCorners ; i++)
      {
	if (polyY [i] < (double)py && polyY [j] >= (double)py ||
		polyY [j] < (double)py && polyY [i] >= (double)py)
	{
	  nodeX [nodes++] = (int)(polyX [i] + ((double)py - polyY [i]) / (polyY [j] - polyY[i]) *
		(polyX [j] - polyX [i]));
	}
	j = i ;
      }

//  Sort the nodes, via a simple “Bubble” sort.

      i = 0 ;
      while (i < (nodes - 1))
      {
	if (nodeX [i] > nodeX [i + 1])
	{
	  swap = nodeX [i] ; nodeX [i] = nodeX [i + 1] ; nodeX [i + 1] = swap ;
	  if (i)
	    --i ;
	}
	else
	  ++i ;
      }

  //  Fill the pixels between node pairs.
  for (i = 0 ; i < nodes ; i += 2)
  {
    if (nodeX [i  ] >= right)
      break ;
    if (nodeX [i+1] >  left )
    {
      if (nodeX [i  ] <  left ) nodeX [i  ] = left ;
      if (nodeX [i+1] >  right) nodeX [i+1] = right ;
      for (j = nodeX [i] ; j < nodeX [i+1] ; j++)
	plotPoint (j, py) ;
     }
    }
  } 
}



/*
 * _keyboardGetchar:
 *	Internal routine to get a character from the keyboard input and
 *	apply our own spedial key encoding.
 *********************************************************************************
 */

static int _keyboardGetchar (SDL_KeyboardEvent *key)
{
//  SDL_KeyboardEvent *key = &savedEvent.key ;

  if (key->keysym.unicode >= 128)
    return -1 ;

  if (key->keysym.unicode != 0)
    return key->keysym.unicode ;

  switch (key->keysym.sym)
  {
    case SDLK_LEFT:
      return RTB_KEY_LEFT ;

    case SDLK_RIGHT:
      return RTB_KEY_RIGHT ;

    case SDLK_UP:
      return RTB_KEY_UP ;

    case SDLK_DOWN:
      return RTB_KEY_DOWN ;

    case SDLK_HOME:
      return RTB_KEY_HOME ;

    case SDLK_END:
      return RTB_KEY_END ;

    case SDLK_PAGEUP:
      return RTB_KEY_PGUP ;

    case SDLK_PAGEDOWN:
      return RTB_KEY_PGDN ;

    default:
      return -1 ;
  }
}
 

/*
 * keyPressed:
 *	Return an indication of any data avalable to be read from the keyboard
 *********************************************************************************
 */

int keyPressed (void)
{
  SDL_Event event ;

  if (savedKeypress != -1)
    return TRUE ;

  if (SDL_PollEvent (&event) == 0)
    return FALSE ;

  if (event.type != SDL_KEYDOWN)
    return FALSE ;

  savedKeypress = _keyboardGetchar (&event.key) ;

  if (savedKeypress == 27)
    escapePressed = TRUE ;

  return savedKeypress != -1 ;
}


/*
 * keyboardGetchar:
 *	Wait for, and read a single character from the keyboard
 *********************************************************************************
 */

uint8_t keyboardGetchar (void)
{
  uint8_t key ;

  while (!keyPressed ())
    usleep (10000) ;

  key = savedKeypress ;
  savedKeypress = -1 ;
  return key ;
}


/*
 * setupLoRes: setupHiRes:
 *	Do any steps needed to initialise either graphics mode
 *********************************************************************************
 */

void setupLoRes (void)
{
  lores = TRUE ;
  hires = FALSE ;

  drawLine  = drawLineLo ;
  plotPoint = plotPointLo ;
}

void setupHiRes (void)
{
  lores = FALSE ;
  hires = TRUE  ;

  if (bitsPerPixel == 16)
  {
    drawLine  = drawLine16hi ;
    plotPoint = plotPoint16hi ;
  }
  else
  {
    drawLine  = drawLine32hi ;
    plotPoint = plotPoint32hi ;
  }
}


/*
 * dumpVideoModes:
 *	Lets see what the hardware can handle
 *********************************************************************************
 */

void dumpVideoModes (void)
{
  int i ;

  if (videoModes == (SDL_Rect **)0)
      printf ("No modes available!\n") ;
  else if (videoModes == (SDL_Rect**)-1)
    printf ("All resolutions available.\n") ;
  else
  {
    printf ("Available Modes:\n") ;
    for (i=0 ; videoModes [i] ; ++i)
      printf ("%2d: %5d x %4d\n", i, videoModes [i]->w, videoModes [i]->h) ;
  }

  printf ("Video Info:\n") ;
  printf ("  hw_available: %s\n",  videoInfo->hw_available? "Yes" : " No") ;
  printf ("  wm_available: %s\n",  videoInfo->wm_available? "Yes" : " No") ;
  printf ("  blit_hw:      %s,",   videoInfo->blit_hw ?     "Yes" : " No") ;
  printf ("  blit_hw_CC:   %s,",   videoInfo->blit_hw_CC ?  "Yes" : " No") ;
  printf ("  blit_hw_A:    %s\n",  videoInfo->blit_hw_A ?   "Yes" : " No") ;
  printf ("  blit_sw:      %s,",   videoInfo->blit_sw ?     "Yes" : " No") ;
  printf ("  blit_sw_CC:   %s,",   videoInfo->blit_sw_CC ?  "Yes" : " No") ;
  printf ("  blit_sw_A:    %s\n",  videoInfo->blit_sw_A ?   "Yes" : " No") ;
  printf ("  blit_fill:    %s\n",  videoInfo->blit_fill ?   "Yes" : " No") ;
  printf ("  video_mem:    %4d\n", videoInfo->video_mem) ;
  printf ("  current_w:    %4d\n", videoInfo->current_w) ;
  printf ("  current_h:    %4d\n", videoInfo->current_h) ;
  printf ("PixelFormat:\n") ;
  printf ("  BitsPerPixel: %d, BytesPerPixel: %d\n", videoInfo->vfmt->BitsPerPixel, videoInfo->vfmt->BytesPerPixel) ;

}


/*
 * openScreenKeyboard:
 *	Kick things off
 *********************************************************************************
 */

void openScreenKeyboard (uint32_t sdlFlags)
{
  if (SDL_Init (SDL_INIT_VIDEO) < 0)
  {
    fprintf( stderr, "Could not initialise Screen and Keyboard systems: %s\n", SDL_GetError ()) ;
    exit (1) ;
  }

// Grab the full-screen video modes and use then for running under X
//	as well as full-screen mode. If nothing else, it'll stop
//	us having to actually type in a resolution

  videoInfo  = SDL_GetVideoInfo () ;
  videoModes = SDL_ListModes (NULL, sdlFlags) ;

  if (videoModes == (SDL_Rect**)0)
  {
    fprintf (stderr, "No modes available. Unable to run.\n") ;
    exit (1) ;
  }

  if (videoModes == (SDL_Rect**)-1)
  {
    numVideoModes = 0 ;
    return ;
  }

  while (videoModes [numVideoModes++])
    ;
}


/*
 * setupScreenKeyboard:
 *	Initialise our SDL screen and keyboard handlers
 *
 *	For screen resolution or BPP, we use the system values if running on
 *	the console, or if rrunning under X we use the defaults (640x480)
 *	but the user suppled ones will over ride it. For BPP, we're only
 *	catering for 16 or 32.
 *********************************************************************************
 */

void setupScreenKeyboard (uint32_t sdlFlags, int mode, int xSize, int ySize, int zSize, int doubleFont)
{
  char msg [1024] ;

  int systemX, systemY, systemZ ;

  systemX = videoInfo->current_w ;
  systemY = videoInfo->current_h ;
  systemZ = videoInfo->vfmt->BitsPerPixel ;

  printf ("Screen/Keyboard initialisation:\n") ;

  doubleSizeFont = doubleFont ;

  if (doubleFont)
  {
    fontWidth  *= 2 ;
    fontHeight *= 2 ;
    printf ("  Double size font selected\n") ;
  }
  else
    printf ("  Standard font selected\n") ;

  if (mode == -1)
  {
    if (videoInfo->wm_available)	// Running under X?
    {
      screenWidth  = DEFAULT_SCREEN_X ;
      screenHeight = DEFAULT_SCREEN_Y ;
    }
    else				// No. Use the system defaults.
    {
      screenWidth  = systemX ;
      screenHeight = systemY ;
    }
  }
  else if ((mode >= 0) && (mode < numVideoModes))
  {
    printf ("  Starting in mode: %d:\n", mode) ;
    screenWidth  =  videoModes [mode]->w ;
    screenHeight =  videoModes [mode]->h ;
  }
  else
  {
    printf ("  Invalid mode. Defaulting to %d x %d\n", DEFAULT_SCREEN_X, DEFAULT_SCREEN_Y) ;
    screenWidth  = DEFAULT_SCREEN_X ;
    screenHeight = DEFAULT_SCREEN_Y ;
  }

// User override?

  if ((xSize + ySize) != 0)
  {
    screenWidth  = xSize ;
    screenHeight = ySize ;
    printf ("  *> User forcing screen size to: %d x %d\n", xSize, ySize) ;
  }
  else
    printf ("  Screen resolution of %d x %d selected.\n", screenWidth, screenHeight) ;

  bitsPerPixel = systemZ ;
  if ((systemZ == 8) || (systemZ == 24))
  {
    bitsPerPixel = 32 ;
    printf ("  *> System reported %d bpp - unsupported. Forced to %d\n", systemZ, bitsPerPixel) ;
  }

  if (zSize != 0)
  {
    bitsPerPixel = zSize ;
    printf ("  *> User forcing BPP to %d\n", zSize) ;
  }
  else
    printf ("  Screen Bits Per Pixel set to: %d\n", bitsPerPixel) ;

  bytesPerPixel = bitsPerPixel / 8 ;

  if (bitsPerPixel == 16)
    screenPutchar = screenPutchar16 ;
  else
    screenPutchar = screenPutchar32 ;

  myScreen = SDL_SetVideoMode (screenWidth, screenHeight, bitsPerPixel, sdlFlags) ;

  if (myScreen == NULL)
  {
    fprintf( stderr, "Unable to open the screen: %s\n", SDL_GetError ()) ;
    exit (-1) ;
  }

  SDL_EnableUNICODE   (1) ;
  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL) ;
  if (initialFilename == NULL)
    sprintf (msg, "Return to BASIC | Version %s.%s.%s",      VERSION, MAJ_VERSION, SUB_VERSION) ;
  else
    sprintf (msg, "Return to BASIC | Version %s.%s.%s | %s", VERSION, MAJ_VERSION, SUB_VERSION, initialFilename) ;
  SDL_WM_SetCaption (msg, "RtB") ;

  SDL_ShowCursor      (SDL_DISABLE) ;

   tWidth  = screenWidth  / fontWidth ;
   tHeight = screenHeight / fontHeight ;
  lgWidth  = screenWidth  / LOW_RES_PIXELS_WIDE ;
  lgHeight = screenHeight / LOW_RES_PIXELS_HIGH ;
  hgWidth  = screenWidth ;
  hgHeight = screenHeight ;

  lgHeight1 = lgHeight - 1 ;
  hgHeight1 = hgHeight - 1 ;

  lores = hires = FALSE ;

  setTextFgColour (15) ;		// White on Black
  setTextBgColour (0) ;			// ...Black
  screenClear     () ;			// Clear to Black
  setPlotColour   (15) ;		// White

  if (initialFilename == NULL)
  {
    screenPuts    ("\n\n\n") ;
    screenPrintf  ("Return to BASIC. Version %s.%s.%s\n\nReady\n\n", VERSION, MAJ_VERSION, SUB_VERSION) ;
    updateDisplay () ;
  }
}


/*
 * closeScreenKeyboard:
 *	Tidy up the SDL stuff
 *********************************************************************************
 */

void closeScreenKeyboard (void)
{
  SDL_KillThread (keyboardThread) ;
  SDL_Quit () ;
}
