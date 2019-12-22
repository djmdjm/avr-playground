#!/bin/sh

# This script downloads and builds a ncurses library with the extensions
# we need for fakeui.

set -xe

test -f ncurses-6.1.tar.gz || \
	wget https://invisible-mirror.net/archives/ncurses/ncurses-6.1.tar.gz

tar zxf ncurses-6.1.tar.gz
cd ncurses-6.1

./configure \
    --without-ada \
    --without-cxx-shared \
    --without-shared \
    --enable-colorfgbg \
    --enable-hard-tabs \
    --enable-overwrite \
    --enable-xmc-glitch \
    --disable-stripping \
    --disable-wattr-macros \
    --with-ospeed=unsigned \
    --with-terminfo-dirs=/lib/terminfo:/usr/share/terminfo \
    --with-termlib=tinfo \
    --with-ticlib=tic \
    --with-xterm-kbs=DEL \
    --enable-ext-mouse

make -j8

