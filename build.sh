#!/bin/bash
CC_PARMS=" --std=gnu99 -g -lcrypt -lgcrypt -Wall -DINTERACTIVE_MODE"
mkdir -p obj 
mkdir -p bin
cd src
for i in *.c
do
	if (( "$(stat $i -c %Y 2> /dev/null)"1 < "$(stat ../obj/${i}.o -c %Y 2> /dev/null)"1 )) 
	then
		echo Omitting $i
	else
		echo CC $i ...
		gcc -c $i -o ../obj/${i}.o $CC_PARMS
	fi
done

echo LINKING

cd ..

gcc obj/* -o bin/ftpmod -lcrypt
