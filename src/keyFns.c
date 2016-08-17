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

#include <SDL/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "rtb.h"
#include "screenKeyboard.h"
#include "bool.h"
#include "rpnEval.h"
#include "numFns.h"

#include "keyFns.h"


/*
 * doKey:
 *********************************************************************************
 */

static int doKey (void *ptr, int key)
{
  if (ptr != NULL)
    return FALSE ;
  pushN ((double)key) ;
  return TRUE ;
}

int doKUp    (void *ptr) { return doKey (ptr,  RTB_KEY_UP) ;     }
int doKDown  (void *ptr) { return doKey (ptr,  RTB_KEY_DOWN) ;   }
int doKLeft  (void *ptr) { return doKey (ptr,  RTB_KEY_LEFT) ;   }
int doKRight (void *ptr) { return doKey (ptr,  RTB_KEY_RIGHT) ;  }
int doKIns   (void *ptr) { return doKey (ptr,  RTB_KEY_INSERT) ; }
int doKDel   (void *ptr) { return doKey (ptr,  RTB_KEY_DELETE) ; }
int doKPgUp  (void *ptr) { return doKey (ptr,  RTB_KEY_PGUP) ;   }
int doKPgDn  (void *ptr) { return doKey (ptr,  RTB_KEY_PGDN) ;   }
int doKHome  (void *ptr) { return doKey (ptr,  RTB_KEY_HOME) ;   }
int doKEnd   (void *ptr) { return doKey (ptr,  RTB_KEY_END) ;    }

int doKF1    (void *ptr) { return doKey (ptr,  RTB_KEY_F1) ;     }
int doKF2    (void *ptr) { return doKey (ptr,  RTB_KEY_F2) ;     }
int doKF3    (void *ptr) { return doKey (ptr,  RTB_KEY_F3) ;     }
int doKF4    (void *ptr) { return doKey (ptr,  RTB_KEY_F4) ;     }
int doKF5    (void *ptr) { return doKey (ptr,  RTB_KEY_F5) ;     }
int doKF6    (void *ptr) { return doKey (ptr,  RTB_KEY_F6) ;     }
int doKF7    (void *ptr) { return doKey (ptr,  RTB_KEY_F7) ;     }
int doKF8    (void *ptr) { return doKey (ptr,  RTB_KEY_F8) ;     }
int doKF9    (void *ptr) { return doKey (ptr,  RTB_KEY_F9) ;     }
int doKF10   (void *ptr) { return doKey (ptr,  RTB_KEY_F10) ;    }
int doKF11   (void *ptr) { return doKey (ptr,  RTB_KEY_F11) ;    }
int doKF12   (void *ptr) { return doKey (ptr,  RTB_KEY_F12) ;    }
