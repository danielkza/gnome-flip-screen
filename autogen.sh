#!/bin/sh

if ! [ -f "src/gnome-flip-screen.c" ]; then
  echo "Missing src folder in current directory"
  exit 1
fi

autoreconf -i

echo "Run ./configure && make to build"
