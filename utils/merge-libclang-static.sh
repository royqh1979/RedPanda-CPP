#!/bin/bash
rm libclang_static.a
rm libclang.dll.a
rm libclang-cpp.dll.a
rm *.obj
for FILE in libclang*.a 
do
	echo $FILE
	rm -rf *.obj
	ar -x $FILE
	for OBJ in *.obj 
	do
		mv $OBJ "$FILE-$OBJ"
	done
	ar r libclang_static.a *.obj
done
rm -rf *.obj
