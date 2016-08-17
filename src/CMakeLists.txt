cmake_minimum_required (VERSION 2.6) 
project (rtb) 

set (CMAKE_COLOR_MAKEFILE OFF)

set ( CMAKE_C_FLAGS "-Wall -O3" )

FIND_PACKAGE( SDL REQUIRED )

IF(SDL_FOUND)
  MESSAGE("Found SDL ${SDL_INCLUDE_DIR}")
ELSE(SDL_FOUND)
  MESSAGE("Could not find SDL")
ENDIF(SDL_FOUND)

set(rtb_SOURCE_FILES
    rtb.c 
    hash.c 
    readline.c 
    bomb.c 
    keywords.c
    array.c
    screenKeyboard.c 
    clipper.c						
    tokenize.c 
    tokenize1.c 
    tokenize2.c					
    parseInput.c 
    lines.c 
    renumber.c 
    load.c					
    symbolTable.c 
    rpnEval.c 
    shuntingYard.c					 
    commands.c 
    run.c 
    list.c							
    numFns.c 
    strFns.c 
    relFns.c 
    procedures.c 
    goto.c 
    procFn.c 
    assign.c	 
    colourFns.c 
    graphicFns.c 
    spriteFns.c 
    keyFns.c				
    switch.c 
    cycle.c 
    textFns.c						
    serial.c 
    serialFns.c 
    drcFns.c 
    fileFns.c					
    piFns.c 
    nesFns.c
)

set(rtb_LINK_LIBRARIES
     ${SDL_LIBRARY}
)

include_directories(
     ${SDL_INCLUDE_DIR}/../
)


add_executable (rtb ${rtb_SOURCE_FILES}) 

if(${APPLE})
    add_definitions(-D_APPLE_) 
endif(${APPLE})

if(${RASPBERRYPI})
    set(rtb_LINK_LIBRARIES wiringPi ${rtb_LINK_LIBRARIES})
    add_definitions(-DRASPBERRY_PI)
endif(${RASPBERRYPI})

if(${RASPBERRYPI})
install (TARGETS
	rtb
	RUNTIME DESTINATION /usr/local/bin
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
	GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
)
else(${RASPBERRYPI})
install (TARGETS
	rtb
	RUNTIME DESTINATION /usr/local/bin
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
	GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
endif(${RASPBERRYPI})

install (FILES
	rtb.1
	DESTINATION /usr/local/man/man1
	PERMISSIONS	OWNER_READ OWNER_WRITE
			GROUP_READ GROUP_WRITE
			WORLD_READ
)


target_link_libraries(rtb ${rtb_LINK_LIBRARIES})
