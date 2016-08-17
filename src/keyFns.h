/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * keys.c:
 *	Pesudo variables to reperesent our subset of keyboard keys
 *********************************************************************************
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
 *********************************************************************************
 */

extern int doKUp    (void *ptr) ;
extern int doKDown  (void *ptr) ;
extern int doKLeft  (void *ptr) ;
extern int doKRight (void *ptr) ;
extern int doKIns   (void *ptr) ;
extern int doKDel   (void *ptr) ;
extern int doKPgUp  (void *ptr) ;
extern int doKPgDn  (void *ptr) ;
extern int doKHome  (void *ptr) ;
extern int doKEnd   (void *ptr) ;

extern int doKF1    (void *ptr) ;
extern int doKF2    (void *ptr) ;
extern int doKF3    (void *ptr) ;
extern int doKF4    (void *ptr) ;
extern int doKF5    (void *ptr) ;
extern int doKF6    (void *ptr) ;
extern int doKF7    (void *ptr) ;
extern int doKF8    (void *ptr) ;
extern int doKF9    (void *ptr) ;
extern int doKF10   (void *ptr) ;
extern int doKF11   (void *ptr) ;
extern int doKF12   (void *ptr) ;
