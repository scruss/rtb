/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * graphics.h:
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

// Functions

extern int doUpdate     (void *ptr) ;

extern int doSaveScreen (void *ptr) ;

extern int doGr         (void *ptr) ;
extern int doHgr        (void *ptr) ;

extern int doGheight    (void *ptr) ;
extern int doGwidth     (void *ptr) ;
extern int doOrigin     (void *ptr) ;

extern int doColour     (void *ptr) ;
extern int doRgbColour  (void *ptr) ;

extern int doPlot       (void *ptr) ;
extern int doHline      (void *ptr) ;
extern int doVline      (void *ptr) ;
extern int doLine       (void *ptr) ;
extern int doLineTo     (void *ptr) ;
extern int doRectangle  (void *ptr) ;
extern int doTriangle   (void *ptr) ;
extern int doCircle     (void *ptr) ;
extern int doEllipse    (void *ptr) ;

extern int doPolyStart  (void *ptr) ;
extern int doPolyPlot   (void *ptr) ;
extern int doPolyEnd    (void *ptr) ;

extern int doPenDown    (void *ptr) ;
extern int doPenUp      (void *ptr) ;
extern int doMove       (void *ptr) ;
extern int doMoveTo     (void *ptr) ;
extern int doLeft       (void *ptr) ;
extern int doRight      (void *ptr) ;
extern int doTangle     (void *ptr) ;
