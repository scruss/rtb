# rtb
Gordon Henderson's “Return to Basic” interpreter — GPL 3 version of 2013-01-27

Gordon no longer provides source for his interpreter, so this is an
archive of the last known GPL version retrieved from his server. Many
thanks to John of the
[allbasic.info](http://www.allbasic.info/forum/index.php?topic=223.msg4768#msg4768
"allbasic.info") forum for providing the files.

## Building

Compiles successfully on both Ubuntu 16.04 x86_64 and Raspbian Jessie armhf. Requires cmake-qt-gui and libsdl-dev.

1. Start `cmake-gui` from the *src* folder
2. *Browse Source …* to the current folder
3. *Browse Build …* to the same folder
4. Choose *Configure*
5. On the **SDL_LIBRARY** entry, add `-lm` at the end of the line after `-lpthread`
6. Choose *Generate*
7. From a terminal, enter `make` from the *src* folder

The `rtb` binary will be built in the *src* folder.

## Licence

GPL 3.

## Copyright

© 2012-2013, Gordon Henderson — https://projects.drogon.net/
