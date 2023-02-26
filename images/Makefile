#	 Upanix - An x86 based Operating System
#	 Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
#
# I am making my contributions/submissions to this project solely in
# my personal capacity and am not conveying any rights to any
# intellectual property of any third parties.
#																			 
#	 This program is free software: you can redistribute it and/or modify
#	 it under the terms of the GNU General Public License as published by
#	 the Free Software Foundation, either version 3 of the License, or
#	 (at your option) any later version.
#																			 
#	 This program is distributed in the hope that it will be useful,
#	 but WITHOUT ANY WARRANTY; without even the implied warranty of
#	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	 GNU General Public License for more details.
#																			 
#	 You should have received a copy of the GNU General Public License
#	 along with this program.  If not, see <http://www.gnu.org/licenses/
LD = ${HOME}/opt/cross/i686-elf/bin/ld

IMAGE_MOUSE_CURSOR_BMP = mouse_cursor1.bmp
IMAGE_MOUSE_CURSOR_BMP_OBJ = mouse_cursor1.o
build: ${IMAGE_MOUSE_CURSOR_BMP_OBJ}
${IMAGE_MOUSE_CURSOR_BMP_OBJ}: ${IMAGE_MOUSE_CURSOR_BMP}
	${LD} -r -b binary -o ${IMAGE_MOUSE_CURSOR_BMP_OBJ} ${IMAGE_MOUSE_CURSOR_BMP}

IMAGE_MOUSE_CURSOR_PNG = mouse_cursor2.png
IMAGE_MOUSE_CURSOR_PNG_OBJ = mouse_cursor2.o
build: ${IMAGE_MOUSE_CURSOR_PNG_OBJ}
${IMAGE_MOUSE_CURSOR_PNG_OBJ}: ${IMAGE_MOUSE_CURSOR_PNG}
	${LD} -r -b binary -o ${IMAGE_MOUSE_CURSOR_PNG_OBJ} ${IMAGE_MOUSE_CURSOR_PNG}

IMAGE_TEST = test.png
IMAGE_TEST_OBJ = test.o
build: ${IMAGE_TEST_OBJ}
${IMAGE_TEST_OBJ}: ${IMAGE_TEST}
	${LD} -r -b binary -o ${IMAGE_TEST_OBJ} ${IMAGE_TEST}

#readelf -s ${IMAGE_MOUSE_CURSOR_OBJ}

clean:
	rm -f *.o

distclean:
	rm -f *.o
