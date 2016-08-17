/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * screenKeyboard.h:
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

// Keycodes

#define	RTB_KEY_F1	0x80
#define	RTB_KEY_F2	0x81
#define	RTB_KEY_F3	0x82
#define	RTB_KEY_F4	0x83
#define	RTB_KEY_F5	0x84
#define	RTB_KEY_F6	0x85
#define	RTB_KEY_F7	0x86
#define	RTB_KEY_F8	0x87
#define	RTB_KEY_F9	0x88
#define	RTB_KEY_F10	0x89
#define	RTB_KEY_F11	0x8A
#define	RTB_KEY_F12	0x8B

#define	RTB_KEY_INSERT	0x90
#define	RTB_KEY_DELETE	0x91
#define	RTB_KEY_HOME	0x92
#define	RTB_KEY_END	0x93
#define	RTB_KEY_PGUP	0x94
#define	RTB_KEY_PGDN	0x95

#define	RTB_KEY_LEFT	0xA0
#define	RTB_KEY_RIGHT	0xA1
#define	RTB_KEY_UP	0xA2
#define	RTB_KEY_DOWN	0xA3

#define	MAX_POLYGON_NODES	64

// Globals

extern SDL_Surface *myScreen ;

extern int          bytesPerPixel ;
extern int           bitsPerPixel ;

extern uint32_t     fgTextColour ;
extern uint32_t     bgTextColour ;
extern uint32_t     plotColour ;

extern int  tWidth,  tHeight ;
extern int lgWidth, lgHeight ;
extern int hgWidth, hgHeight ;

extern int cursorX, cursorY ;

extern int xOrigin, yOrigin ;

// Misc

extern int lores ;
extern int hires ;

// Functions

extern int  saveScreen         (char *filename) ;

extern void updateDisplay      (void) ;
extern void charUpdateDisplay  (void) ;

extern void setTextCursor      (int x, int y) ;
extern void screenClear        (void) ;

extern void setTextBgColour    (uint8_t bgCol) ;
extern void setTextFgColour    (uint8_t fgCol) ;
extern void setPlotColour      (uint8_t plotCol) ;
extern void setRgbPlotColour   (uint8_t r, uint8_t g, uint8_t b) ;

extern void (*screenPutchar)   (uint8_t c, int doUpdate) ;
extern void screenPuts         (char *s) ;
extern void screenPrintf       (char *message, ...) ;

extern void setGraphicsColour   (uint8_t plotCol) ;
extern void setOrigin           (int x, int y) ;
extern void (*plotPoint)        (int x, int y) ;
extern void (*drawLine)         (int x0, int y0, int x1, int y1) ;

extern void drawCircle          (int x, int y, int r, int fill) ;
extern void drawEllipse         (int cx, int cy, int xRadius, int yRadius, int fill) ;
extern void drawRectangle       (int x, int y, int w, int h, int fill) ;
extern void polygonFill         (int nodes, int polyX [MAX_POLYGON_NODES], int polyY [MAX_POLYGON_NODES]) ;

extern uint8_t keyboardGetchar  (void) ;
extern int     keyPressed       (void) ;

extern void setupLoRes          (void) ;
extern void setupHiRes          (void) ;

extern void openScreenKeyboard  (uint32_t sdlFlags) ;
extern void dumpVideoModes      (void) ;
extern void setupScreenKeyboard (uint32_t sdlFlags, int mode, int xSize, int ySize, int zSize, int doubleFont) ;
extern void closeScreenKeyboard (void) ;
