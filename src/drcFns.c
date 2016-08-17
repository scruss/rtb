/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * drcFns.c:
 *	Functions to handle the Drogon Remote Control protocol over a serial port
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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <math.h>

#include "bomb.h"

#include "bool.h"
#include "keywords.h"
#include "symbolTable.h"

#include "shuntingYard.h"
#include "rpnEval.h"

#include "rtb.h"
#include "serial.h"

#include "drcFns.h"

// Local data


/*
 * doDrcOpen:
 *	Open a Drogon Remote-Control channel
 *********************************************************************************
 */

int doDrcOpen (void *ptr)
{
  char *deviceName ;
  int   handle, tries, ok ;
  time_t then ;
  int    x ;

  if (!oneString ())
    return syntaxError ("drcOpen: Expected a string") ;

  deviceName = popS () ;

// Try DRC serial protocol

  if ((handle = serialOpen (deviceName, 115200)) == -1)
  {
    free (deviceName) ;
    return syntaxError ("drcOpen: Device open failed") ;
  }

  usleep (100000) ;
  while (serialDataAvail (handle))
    (void)serialGetchar (handle) ;

  ok = FALSE ;
  for (tries = 1 ; tries < 5 ; ++tries)
  {
    serialPutchar (handle, '@') ;
    then = time (NULL) + 2 ;
    while (time (NULL) < then)
      if (serialDataAvail (handle))
      {
	x = serialGetchar (handle) ;
	if (x == '@')
	{
	  ok = TRUE ;
	  break ;
	}
      }
    if (ok)
      break ;
  }

  free (deviceName) ;

  if (!ok)
  {
    serialClose (handle) ;
    return syntaxError ("drcOpen: No DRC device detected") ;
  }

  pushN ((double)handle) ;
  return TRUE ;
}


/*
 * doDrcClose:
 *	Close a DRC device
 *********************************************************************************
 */

int doDrcClose (void *ptr)
{
  int handle ;

  if (!oneNumber ())
    return syntaxError ("drcClose: Expected a number") ;

  handle = (int)rint (popN ()) ;

  serialClose (handle) ;

  return TRUE ;
}


/*
 * doDrcPinMode:
 *	Set the mode of a digital pin to input, output or PWM
 *********************************************************************************
 */

int doDrcPinMode (void *ptr)
{
  int mode, pin, handle ;

  if (!threeNumbers ())
    return syntaxError ("PinMode: Expected three numbers") ;

  mode   = (int)rint (popN ()) ;
  pin    = (int)rint (popN ()) ;
  handle = (int)rint (popN ()) ;

  /**/ if (mode == 0)
    serialPutchar (handle, 'i') ;	// Input
  else if (mode == 2)
    serialPutchar (handle, 'p') ;	// PWM
  else
    serialPutchar (handle, 'o') ;	// Default to output

  serialPutchar (handle, pin) ;

  return TRUE ;
}

/*
 * doDrcPullUpDn:
 *	Set the pin pull up/down modes (if supported!)
 *********************************************************************************
 */

int doDrcPullUpDn (void *ptr)
{
  int mode, pin, handle ;

  mode   = (int)rint (popN ()) ;
  pin    = (int)rint (popN ()) ;
  handle = (int)rint (popN ()) ;

  return TRUE ;
}



/*
 * doDrcDigitalRead:
 *	Read a pin value
 *********************************************************************************
 */

int doDrcDigitalRead (void *ptr)
{
  int pin, handle, value ;

  if (!twoNumbers ())
    return syntaxError ("digitalRead: Expected two numbers") ;

  pin    = (int)rint (popN ()) ;
  handle = (int)rint (popN ()) ;

  serialPutchar (handle, 'r') ;	// Send read command
  serialPutchar (handle, pin) ;
  value = serialGetchar (handle) == '0' ? 0 : 1 ;

  pushN ((double)value) ;

  return TRUE ;
}


int doDrcAnalogRead (void *ptr)
{
  int pin, handle ;
  int vHi, vLo ;

  if (!twoNumbers ())
    return syntaxError ("analogRead: Expected two numbers") ;

  pin    = (int)rint (popN ()) ;
  handle = (int)rint (popN ()) ;

  serialPutchar (handle, 'a') ;	// Send read command
  serialPutchar (handle, pin) ;
  vHi = serialGetchar (handle) ;
  vLo = serialGetchar (handle) ;

  pushN ((double)((vHi << 8) | vLo)) ;

  return TRUE ;
}


/*
 * doDrcDigitalWrite:
 *	Emutlate the wiring digitalWrite command
 *********************************************************************************
 */

int doDrcDigitalWrite (void *ptr)
{
  int val, pin, handle ;

  if (!threeNumbers ())
    return syntaxError ("digitalWrite: Expected three numbers") ;

  val    = (int)rint (popN ()) ;
  pin    = (int)rint (popN ()) ;
  handle = (int)rint (popN ()) ;

  serialPutchar (handle, val == 0 ? '0' : '1') ;
  serialPutchar (handle, pin) ;

  return TRUE ;
}

/*
 * doDrcPwmWrite:
 *	Emutlate the wiring analogWrite command
 *********************************************************************************
 */

int doDrcPwmWrite (void *ptr)
{
  int val, pin, handle ;

  if (!threeNumbers ())
    return syntaxError ("pwmWrite: Expected three numbers") ;

  val    = (int)rint (popN ()) ;
  pin    = (int)rint (popN ()) ;
  handle = (int)rint (popN ()) ;

  serialPutchar (handle, 'v') ;
  serialPutchar (handle, pin) ;
  serialPutchar (handle, val) ;

  return TRUE ;
}
