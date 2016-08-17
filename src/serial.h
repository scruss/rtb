/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * serial.h:
 *	Handle a serial port
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

#define	MAX_SERIAL_PORTS	8

extern int   serialOpen      (char *device, int baud) ;
extern void  serialClose     (int handle) ;
extern void  serialCloseAll  (void) ;
extern void  serialPutchar   (int handle, uint8_t c) ;
extern void  serialPuts      (int handle, char *s) ;
extern int   serialDataAvail (int handle) ;
extern int   serialGetchar   (int handle) ;

extern void  setupSerial     (void) ;
