#!/bin/bash
UNAME=`uname -o`
if [ "$UNAME" == "Cygwin" ];then
echo `cygpath -w $1`;
else
echo $1;
fi
