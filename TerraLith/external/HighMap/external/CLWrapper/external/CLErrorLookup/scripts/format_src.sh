#!/bin/bash

# directories to be formatted (recursive search)
DIRS="CLErrorLookup/include CLErrorLookup/src tests"
FORMAT_CMD="clang-format -style=file:scripts/clang_style -i"

# format C++
for D in ${DIRS}; do
    for F in `find ${D}/. -type f \( -iname \*.hpp -o -iname \*.cpp \)`; do
	echo ${F}
	${FORMAT_CMD} ${F}
    done
done


# format cmake files
cmake-format -i CMakeLists.txt CLErrorLookup/CMakeLists.txt

