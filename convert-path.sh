#!/bin/bash
UNAME=`uname`
if [ "$UNAME" == "Darwin" ];then
echo $1;
else
UNAME=`uname -o`
if [ "$UNAME" == "Cygwin" ];then
echo `cygpath -w $1`;
else
echo $1;
fi
fi
