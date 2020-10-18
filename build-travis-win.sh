#!/bin/bash
VERSION=$(cat VERSION)
CFLAGS="-g -Wall -DAPP_VERSION=\"\\\"$VERSION\\\"\""
LDFLAGS=  
CC=gcc
POPPLER_CFLAGS=$(pkg-config --cflags --static poppler-glib)
POPPLER_LIBS=$(pkg-config --libs poppler-glib)

echo $CFLAGS
echo "Building resconv"
$CC resconv.c -o resconv
echo "Building resources"
./resconv freeze.svg > res_gen.c
./resconv black.svg >> res_gen.c
./resconv open.svg >> res_gen.c
./resconv index.html.template >> res_gen.c
echo "Building res" 
$CC res.c -c $CFLAGS
echo "Building utils"
$CC utils.c -c $CFLAGS
echo "Building cli"
$CC cli.c -c $CFLAGS
echo "Building webslides"
$CC webslides.c -c $CFLAGS $POPPLER_CFLAGS
echo "Linking webslides"
$CC webslides.o cli.o utils.o res.o $LDFLAGS -o pdf-webslides $POPPLER_LIBS -lshlwapi
