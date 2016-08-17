/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * piFns.c:
 *	Functions to handle the Raspberry Pi's GPIO pins
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

#ifdef	RASPBERRY_PI
#warning "Raspberry Pi"
#  include <wiringPi.h>
#endif


/*
 * doPinMode:
 *	Set the mode of a digital pin to input, output or PWM
 *********************************************************************************
 */

int doPinMode (void *ptr)
{
#ifdef	RASPBERRY_PI
  int mode, pin ;

  if (!twoNumbers ())
    return syntaxError ("PinMode: Expected two numbers") ;

  mode   = (int)rint (popN ()) ;
  pin    = (int)rint (popN ()) ;

  pinMode (pin, mode) ;

  return TRUE ;
#else
  return syntaxError ("Not running on a Raspberry Pi") ;
#endif
}


/*
 * doPullUpDn:
 *	Set the pin pull up/down modes (if supported!)
 *********************************************************************************
 */

int doPullUpDn (void *ptr)
{
#ifdef	RASPBERRY_PI
  int mode, pin ;

  mode   = (int)rint (popN ()) ;
  pin    = (int)rint (popN ()) ;

  pullUpDnControl (pin, mode) ;

  return TRUE ;
#else
  return syntaxError ("Not running on a Raspberry Pi") ;
#endif
}


/*
 * doDigitalRead:
 *	Read a pin value
 *********************************************************************************
 */

int doDigitalRead (void *ptr)
{
#ifdef	RASPBERRY_PI
  int pin, value ;

  if (!oneNumber ())
    return syntaxError ("digitalRead: Expected a number") ;

  pin   = (int)rint (popN ()) ;
  value = digitalRead (pin) ;

  pushN ((double)value) ;

  return TRUE ;
#else
  return syntaxError ("Not running on a Raspberry Pi") ;
#endif
}


/*
 * doDigitalWrite:
 *	Write a pin with the value
 *********************************************************************************
 */

int doDigitalWrite (void *ptr)
{
#ifdef	RASPBERRY_PI
  int val, pin ;

  if (!twoNumbers ())
    return syntaxError ("digitalWrite: Expected two numbers") ;

  val    = (int)rint (popN ()) ;
  pin    = (int)rint (popN ()) ;

  digitalWrite (pin, val) ;

  return TRUE ;
#else
  return syntaxError ("Not running on a Raspberry Pi") ;
#endif
}
