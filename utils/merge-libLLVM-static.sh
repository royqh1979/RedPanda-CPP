#!/bin/bash
rm libLLVM_static.a
rm *.obj
for FILE in libLLVM*.a 
do
	PREFIX=`echo "$FILE" | sed s/^libLLVM//g | sed s/\.a//g`
	echo $PREFIX
	echo $FILE
	rm -rf *.obj
	ar -x $FILE
	for OBJ in *.obj 
	do
		OBJ_NAME=`echo "$OBJ" | sed s/\.cpp//g`
		mv $OBJ "$PREFIX-$OBJ_NAME"
	done
	ar r libLLVM_static.a *.obj
done
rm -rf *.obj
