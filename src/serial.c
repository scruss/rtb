/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * serial.c:
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <errno.h>

#include "bool.h"
#include "serial.h"

// Locals

static int serialPortFD [MAX_SERIAL_PORTS] ;


/*
 * getFd:
 *	Return our internal mapping of handle to file descriptor
 *********************************************************************************
 */

static int getFd (int handle)
{
  if ((handle < 0) || (handle >= MAX_SERIAL_PORTS))
    return -1 ;
  else
    return serialPortFD [handle] ;
}


/*
 * serialOpen:
 *	Open and initialise the serial port, setting all the right
 *	port parameters - or as many as are required - hopefully!
 *********************************************************************************
 */

int serialOpen (char *device, int baud)
{
  struct termios options ;
  int handle = -1 ;
  int status = 0 ;
  int i, fd ;
  speed_t mybod ;

#ifdef	DEBUG
  printf ("openSerialPort: <%s> baud: $d\n", device, baud) ;
#endif

  switch (baud)
  {
    case     50:	mybod =     B50 ; break ;
    case     75:	mybod =     B75 ; break ;
    case    110:	mybod =    B110 ; break ;
    case    134:	mybod =    B134 ; break ;
    case    150:	mybod =    B150 ; break ;
    case    200:	mybod =    B200 ; break ;
    case    300:	mybod =    B300 ; break ;
    case    600:	mybod =    B600 ; break ;
    case   1200:	mybod =   B1200 ; break ;
    case   1800:	mybod =   B1800 ; break ;
    case   2400:	mybod =   B2400 ; break ;
    case   9600:	mybod =   B9600 ; break ;
    case  19200:	mybod =  B19200 ; break ;
    case  38400:	mybod =  B38400 ; break ;
    case  57600:	mybod =  B57600 ; break ;
    case 115200:	mybod = B115200 ; break ;
    case 230400:	mybod = B230400 ; break ;

    default:
      return -2 ;
  }

  if ((fd = open (device, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) == -1)
    return -1 ;

  fcntl (fd, F_SETFL, O_RDWR) ;

// Find an empty slot

  for (i = 0 ; i < MAX_SERIAL_PORTS ; ++i)
    if (serialPortFD [i] == -1)
    {
      serialPortFD [i] = fd ;
      handle = i ;
      break ;
    }

  if (handle == -1)	// Too many serial ports open! (Realy? Wow...)
  {
    close (fd) ;
    return -1 ;
  }

  serialPortFD [handle] = fd ;

// Get and modify current options:

  tcgetattr (fd, &options) ;

  cfmakeraw (&options) ;

  cfsetispeed (&options, mybod) ;
  cfsetospeed (&options, mybod) ;

  options.c_cflag |= (CLOCAL | CREAD) ;
  options.c_cflag &= ~PARENB ;
  options.c_cflag &= ~CSTOPB ;
  options.c_cflag &= ~CSIZE ;
  options.c_cflag |= CS8 ;
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG) ;
  options.c_oflag &= ~OPOST ;

  options.c_cc [VMIN]  =  0 ;
  options.c_cc [VTIME] = 60 ;	// Six seconds (10 deciseconds)

  tcsetattr (fd, TCSANOW, &options) ;

  ioctl (fd, TIOCMGET, &status);

  status |= TIOCM_DTR ;
  status |= TIOCM_RTS ;

  ioctl (fd, TIOCMSET, &status);

  usleep (100000) ;

  return handle ;
}


/*
 * serialClose:
 *	Release the serial port
 *********************************************************************************
 */

void serialClose (int handle)
{
  int fd ;

  if ((fd = getFd (handle)) == -1)
    return ;

  close (fd) ;
  serialPortFD [handle] = -1 ;
}


/*
 * serialCloseAll:
 *	Close all serial ports - used at end of program run
 *********************************************************************************
 */

void serialCloseAll (void)
{
  int i ;

  for (i = 0 ; i < MAX_SERIAL_PORTS ; ++i)
    serialClose (i) ;
}


/*
 * serialPutchar:
 *	Send a single character to the serial port
 *********************************************************************************
 */

void serialPutchar (int handle, uint8_t c)
{
  int fd ;

  if ((fd = getFd (handle)) == -1)
    return ;

  write (fd, &c, 1) ;
}


/*
 * serialPuts:
 *	Send a string to the serial port
 *********************************************************************************
 */

void serialPuts (int handle, char *s)
{
  int fd ;

  if ((fd = getFd (handle)) == -1)
    return ;

  write (fd, s, strlen (s)) ;
}

/*
 * serialDataAvail:
 *	Return the number of bytes of data avalable to be read in the serial port
 *********************************************************************************
 */

int serialDataAvail (int handle)
{
  int fd, result ;

  if ((fd = getFd (handle)) == -1)
    return -1 ;

  if (ioctl (fd, FIONREAD, &result) == -1)
    return -1 ;

  return result ;
}


/*
 * serialGetchar:
 *	Get a single character from the serial device.
 *	Note: Zero is a valid character and this function will time-out after
 *	6 seconds.
 *********************************************************************************
 */

int serialGetchar (int handle)
{
  int fd ;
  uint8_t x ;

  if ((fd = getFd (handle)) == -1)
    return -1 ;

  if (read (fd, &x, 1) != 1)
    return -1 ;

  return ((int)x) & 0xFF ;
}


/*
 * serialInit:
 *	Setup our serial subsystem
 *********************************************************************************
 */

void setupSerial (void)
{
  int i ;

  for (i = 0 ; i < MAX_SERIAL_PORTS ; ++i)
    serialPortFD [i] = -1 ;
}
