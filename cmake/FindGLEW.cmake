# Copyright (c) 2007 NVIDIA Corporation
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
#
# Try to find GLEW library and include pathi. Once done this will define:
# GLEW_FOUND
# GLEW_INCLUDE_DIR
# GLEW_LIBRARY

IF (WIN32)
    FIND_PATH( GLEW_INCLUDE_DIR GL/glew.h
        $ENV{PROGRAMFILES}/GLEW/include
        ${PROJECT_SOURCE_DIR}/src/nvgl/glew/include
        DOC "The directory where GL/glew.h resides")
    FIND_LIBRARY( GLEW_LIBRARY
        NAMES glew GLEW glew32 glew32s
        PATHS
        $ENV{PROGRAMFILES}/GLEW/lib
        ${PROJECT_SOURCE_DIR}/src/nvgl/glew/bin
        ${PROJECT_SOURCE_DIR}/src/nvgl/glew/lib
        DOC "The GLEW library")
ELSE (WIN32)
    FIND_PATH( GLEW_INCLUDE_DIR GL/glew.h
        /usr/include
        /usr/local/include
        /sw/include
        /opt/local/include
        DOC "The directory where GL/glew.h resides")
    FIND_LIBRARY( GLEW_LIBRARY
        NAMES GLEW glew
        PATHS
        /usr/lib64
        /usr/lib
        /usr/local/lib64
        /usr/local/lib
        /sw/lib
        /opt/local/lib
        DOC "The GLEW library")
ENDIF (WIN32)

IF (GLEW_INCLUDE_DIR)
    SET( GLEW_FOUND 1 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
ELSE (GLEW_INCLUDE_DIR)
    SET( GLEW_FOUND 0 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
ENDIF (GLEW_INCLUDE_DIR)

MARK_AS_ADVANCED( GLEW_FOUND )
