/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * spriteFns.c:
 *	Sprite Graphics procedures for RTB
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
#include <errno.h>

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

#include "spriteFns.h"

#define	SS_OFF		0
#define	SS_FIRST_PLOT	1
#define	SS_ONSCREEN	2
#define	SS_DEL		3

struct spriteStruct
{
  SDL_Surface *sprite ;
  SDL_Surface *background ;
  int x, y ;
  int newX, newY ;
//  int w, h ;
  int state ;
} ;


// Locals

static struct spriteStruct sprites [MAX_SPRITES] ;
static int numSprites = 0 ;

/*
 * doLoadSprite:
 *	Load a sprite from disk into memory
 *********************************************************************************
 */

int doLoadSprite (void *ptr)
{
  char *s ;
  int   ok ;
  SDL_Surface *tmp, *sprite ;
  uint32_t colourKey ;
  struct spriteStruct *sp ;

  if (!oneString ())
    return syntaxError ("LoadSprite: String expected") ;

  s = popS () ;

  if (numSprites == MAX_SPRITES)
    return syntaxError ("LoadSprite: Too many sprites") ;

  ok = (tmp = SDL_LoadBMP (s)) != NULL ;
  free (s) ;

  if (!ok)
    return syntaxError ("LoadSprite: Unable to load: %s", strerror (errno)) ;

  sp = &sprites [numSprites] ;

  sprite = SDL_DisplayFormat (tmp) ;

  SDL_FreeSurface (tmp) ;

  colourKey = SDL_MapRGB (myScreen->format, 254, 254, 254) ;
  SDL_SetColorKey (sprite, SDL_SRCCOLORKEY | SDL_RLEACCEL, colourKey) ;

  sp->sprite     = sprite ;
  sp->state      = SS_OFF ;
  sp->x          = sp->y = sp->newX = sp->newY = 0 ;
  sp->background = SDL_CreateRGBSurface (SDL_HWSURFACE, sprite->w, sprite->h, bitsPerPixel, 0,0,0,0) ;

//printf ("    screen: 0x%08X\n", myScreen->flags) ;
//printf ("    sprite: 0x%08X\n", sp->sprite->flags) ;
//printf ("background: 0x%08X\n", sp->background->flags) ;

  pushN ((double)numSprites) ;

  ++numSprites ;
  return TRUE ;
}


/*
 * doPlotSprite:
 *	Put a sprite on the screen
 *	Except that we don't. All we're doing here is updating the position and
 *	marking it such that it needs to be updated.
 *********************************************************************************
 */

int doPlotSprite (void *ptr)
{
  struct spriteStruct *sp ;
  int x, y, s ;

  if (!threeNumbers ())
    return syntaxError ("PlotSprite: 3 numbers expected") ;

  y = (int)rint (popN ()) ;
  x = (int)rint (popN ()) ;
  s = (int)rint (popN ()) ;

  sp = &sprites [s] ;

/*
  x += xOrigin ;
  y += yOrigin ;

  if (lores)
    y  = lgHeight - 1 - y + sp->h ;
  else
    y  = hgHeight - 1 - y + sp->h ;
*/

  sp->newX = x ;
  sp->newY = y ;

  if (sp->state == SS_OFF)
    sp->state = SS_FIRST_PLOT ;

  return TRUE ;
}


/*
 * doDelSprite:
 *	Delete a sprite from the screen on the next update.
 *********************************************************************************
 */

int doDelSprite (void *ptr)
{
  int s ;

  if (!oneNumber ())
    return syntaxError ("DelSprite: Number expected") ;

  s = (int)rint (popN ()) ;
  
  if (sprites [s].sprite == NULL)
    return syntaxError ("DelSprite: Not a sprite index") ;

  sprites [s].state = SS_DEL ;

  return TRUE ;
}


/*
 * updateSprites:
 *	Called from the main update code: It scans through the sprites,
 *	Looking for active ones, and if so, then it draws them, taking
 *	care of what's left on-screen...
 *********************************************************************************
 */

void updateSprites (void)
{
  struct SDL_Rect spRect ;
  struct spriteStruct *sp ;
  int s ;

// First pass. Restore the backgrounds of all sprites, effectively
//	wiping them off the screen

  sp = &sprites [0] ;
  for (s = 0 ; s < numSprites ; ++s, ++sp)
  {
    if ((sp->state == SS_ONSCREEN) || (sp->state == SS_DEL))
    {
      spRect.w = sp->sprite->w ;
      spRect.h = sp->sprite->h ;

      spRect.x = sp->x ;
      spRect.y = hgHeight - 1 - sp->y - sp->sprite->h ;
      SDL_BlitSurface (sp->background, NULL,  myScreen, &spRect) ;
      if (sp->state == SS_DEL)
	sp->state = SS_OFF ;
    }
  }

// Second pass:
//	Save the backgound of the ones we're going to plot

  sp = &sprites [0] ;
  for (s = 0 ; s < numSprites ; ++s, ++sp)
  {
    if ((sp->state == SS_FIRST_PLOT) || (sp->state == SS_ONSCREEN))
    {
      spRect.w = sp->sprite->w ;
      spRect.h = sp->sprite->h ;

      spRect.x = sp->newX ;
      spRect.y = hgHeight - 1 - sp->newY - sp->sprite->h ;
      SDL_BlitSurface (myScreen, &spRect, sp->background, NULL) ;
    }
  }

// Third pass:
//	Plot the ones that need plotting

  sp = &sprites [0] ;
  for (s = 0 ; s < numSprites ; ++s, ++sp)
  {
    if ((sp->state == SS_FIRST_PLOT) || (sp->state == SS_ONSCREEN))
    {
      spRect.w = sp->sprite->w ;
      spRect.h = sp->sprite->h ;

      spRect.x = sp->newX ;
      spRect.y = hgHeight - 1 - sp->newY - sp->sprite->h ;
      SDL_BlitSurface (sp->sprite, NULL,  myScreen, &spRect) ;

      sp->state = SS_ONSCREEN ;
      sp->x = sp->newX ;
      sp->y = sp->newY ;
    }
  }

}


/*
 * deleteSprites:
 *	Delete all sprites from memory. Called on program end, clear, etc.
 *********************************************************************************
 */

void deleteSprites (void)
{
  struct spriteStruct *sp ;
  int i ;

  sp = &sprites [0] ;
  for (i = 0 ; i < numSprites ; ++i, ++sp)
    sp->state = SS_DEL ;

  updateSprites () ;

  sp = &sprites [0] ;
  for (i = 0 ; i < numSprites ; ++i, ++sp)
  {
    SDL_FreeSurface (sp->sprite) ;
    SDL_FreeSurface (sp->background) ;
  }

  numSprites = 0 ;
}



/*
 * setupSprites:
 *	Initialise stuff that needs initialising
 *********************************************************************************
 */

void setupSprites (void)
{
  int i ;

  for (i = 0 ; i < MAX_SPRITES ; ++i)
    sprites [i].sprite = NULL ;
}
