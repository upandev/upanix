#	 Upanix - An x86 based Operating System
#	 Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
#
#  I am making my contributions/submissions to this project solely in
#  my personal capacity and am not conveying any rights to any
#  intellectual property of any third parties.
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
cmake_minimum_required(VERSION 3.15)
project(Upanix)

set(TARGET_BIN "upanix.elf")

set(CROSS_TOOL_PATH $ENV{HOME}/opt/cross)

set(UPANLIB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../upanlibs)
set(UPANLIB_OUT_DIR ${UPANLIB_SOURCE_DIR}/out)

set(CMAKE_CXX_COMPILER ${CROSS_TOOL_PATH}/bin/x86_64-elf-g++)
set(CMAKE_C_COMPILER ${CROSS_TOOL_PATH}/bin/x86_64-elf-gcc)
set(CMAKE_LINKER ${CROSS_TOOL_PATH}/bin/x86_64-elf-ld)
set(CMAKE_AR ${CROSS_TOOL_PATH}/bin/x86_64-elf-ar)

set(ENABLE_CXX_RT ON)
	
set(EXCEPTION_SUP " -fno-exceptions -fno-rtti")
if(ENABLE_CXX_RT)
	set(EXCEPTION_SUP " -fexceptions -frtti")
endif()

set(CMAKE_CXX_FLAGS " -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc -std=c++11 -nostdinc++ -fno-threadsafe-statics -fpermissive ${EXCEPTION_SUP}")
set(CMAKE_C_FLAGS " -std=c11 -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc")
set(CMAKE_EXE_LINKER_FLAGS " -T${PROJECT_SOURCE_DIR}/kernel.ld -O0 -nostdlib")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

#set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(${PROJECT_SOURCE_DIR}/bin)
include_directories(${PROJECT_SOURCE_DIR}/kernel)
include_directories(${PROJECT_SOURCE_DIR}/process)
include_directories(${PROJECT_SOURCE_DIR}/display)

include_directories(${PROJECT_SOURCE_DIR}/kernelprocs/console)
include_directories(${PROJECT_SOURCE_DIR}/kernelprocs/session)
include_directories(${PROJECT_SOURCE_DIR}/kernelprocs)

include_directories(${PROJECT_SOURCE_DIR}/drivers/floppy)
include_directories(${PROJECT_SOURCE_DIR}/drivers/ps2)
include_directories(${PROJECT_SOURCE_DIR}/drivers/keyboard)
include_directories(${PROJECT_SOURCE_DIR}/drivers/bus)
include_directories(${PROJECT_SOURCE_DIR}/drivers/ide)
include_directories(${PROJECT_SOURCE_DIR}/drivers/ide/vendorspec)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/uhci)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/ehci)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/xhci)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/devices)
include_directories(${PROJECT_SOURCE_DIR}/drivers/video)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net/protocol)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net/protocol/packets)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net/protocol/packets/recv)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net/protocol/packets/send)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net/e1000)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net/mac80211)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net/ath9k)
include_directories(${PROJECT_SOURCE_DIR}/drivers/)

include_directories(${PROJECT_SOURCE_DIR}/util)
include_directories(${PROJECT_SOURCE_DIR}/memory)
include_directories(${PROJECT_SOURCE_DIR}/io)
include_directories(${PROJECT_SOURCE_DIR}/filesystem)
include_directories(${PROJECT_SOURCE_DIR}/users)
include_directories(${PROJECT_SOURCE_DIR}/exeparser)
include_directories(${PROJECT_SOURCE_DIR}/syscall)
include_directories(${PROJECT_SOURCE_DIR}/resource)

include_directories(${UPANLIB_SOURCE_DIR}/libc/include)
include_directories(${UPANLIB_SOURCE_DIR}/libc/stdio)
include_directories(${UPANLIB_SOURCE_DIR}/libc/sysdeps/mos/common/)
include_directories(${UPANLIB_SOURCE_DIR}/libc/sysdeps/mos/common/bits)

include_directories(${UPANLIB_SOURCE_DIR}/libustd/infra)
include_directories(${UPANLIB_SOURCE_DIR}/libustd/algo)
include_directories(${UPANLIB_SOURCE_DIR}/libustd/ds)
include_directories(${UPANLIB_SOURCE_DIR}/libustd/mem)
include_directories(${UPANLIB_SOURCE_DIR}/libustd/mthread)

include_directories(${UPANLIB_SOURCE_DIR}/libm/include)
include_directories(${UPANLIB_SOURCE_DIR}/libm/include/bits)

include_directories(${PROJECT_SOURCE_DIR}/libmcpp/cal)
include_directories(${PROJECT_SOURCE_DIR}/libints)

include_directories(${UPANLIB_SOURCE_DIR}/libcxxrt)

include_directories(${UPANLIB_SOURCE_DIR}/libgccucore/zlib)

include_directories(${UPANLIB_SOURCE_DIR}/libupanui)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/frame)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/uicomponent)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/uicomponent/button)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/uicomponent/scroller)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/uiobject)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/uiobject/core)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/uiobject/core/layout)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/uiobject/canvas)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/uiobject/image)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/events)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/text/default)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/text/usfn)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/text)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/image)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/image/bmp)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/image/png)
include_directories(${UPANLIB_SOURCE_DIR}/libupanui/interop/includes)

add_custom_target(osutils COMMAND make WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/osutils)
add_custom_target(boot COMMAND make boot_elf WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/boot)
add_custom_target(mem COMMAND make WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/memory)
add_custom_target(interrupt_handlers COMMAND make WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libints)

execute_process(COMMAND bash ${PROJECT_SOURCE_DIR}/upanix_src_files.sh WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE UPANIX_SRCS)
string(REPLACE "\n" ";" UPANIX_SRCS ${UPANIX_SRCS})

execute_process(COMMAND bash -c "${CMAKE_CXX_COMPILER} -print-file-name=crtbegin.o" OUTPUT_VARIABLE CRTBEGIN_OBJ)
string(REPLACE "\n" "" CRTBEGIN_OBJ ${CRTBEGIN_OBJ})
execute_process(COMMAND bash -c "${CMAKE_CXX_COMPILER} -print-file-name=crtend.o" OUTPUT_VARIABLE CRTEND_OBJ)
string(REPLACE "\n" "" CRTEND_OBJ ${CRTEND_OBJ})

set(CRTINIT_OBJ "${PROJECT_SOURCE_DIR}/osutils/crti.o ${CRTBEGIN_OBJ}")
set(CRTFINI_OBJ "${CRTEND_OBJ} ${PROJECT_SOURCE_DIR}/osutils/crtn.o")

set(OTHER_OBJS ${PROJECT_SOURCE_DIR}/memory/Mem.o ${PROJECT_SOURCE_DIR}/libints/TimerInterruptHandler.o)

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} ${CMAKE_EXE_LINKER_FLAGS} ${PROJECT_SOURCE_DIR}/boot/fpu_init.obj ${CRTINIT_OBJ} <OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${CRTFINI_OBJ}")

#LIBMCPP
execute_process(COMMAND bash -c "cd ${PROJECT_SOURCE_DIR}; find ./libmcpp -name '*.cpp' | grep -v CMakeFile" OUTPUT_VARIABLE MCPP_SRC)
string(REPLACE "\n" ";" MCPP_SRC ${MCPP_SRC})
add_library(mcpp STATIC "${MCPP_SRC}")
add_custom_command(TARGET mcpp POST_BUILD COMMAND ${CROSS_TOOL_PATH}/bin/x86_64-elf-ranlib $<TARGET_FILE:mcpp>)

#LIBINTS
execute_process(COMMAND bash -c "cd ${PROJECT_SOURCE_DIR}; find ./libints -name '*.cpp' | grep -v CMakeFile" OUTPUT_VARIABLE INTS_SRC)
string(REPLACE "\n" ";" INTS_SRC ${INTS_SRC})
add_library(ints STATIC "${INTS_SRC}")
add_custom_command(TARGET ints POST_BUILD COMMAND ${CROSS_TOOL_PATH}/bin/x86_64-elf-ranlib $<TARGET_FILE:ints>)
target_compile_options(ints PRIVATE -mgeneral-regs-only -mno-red-zone)

#UPANIX
add_executable(${TARGET_BIN} ${OTHER_OBJS} ${UPANIX_SRCS})

add_dependencies(${TARGET_BIN} mcpp)
add_dependencies(${TARGET_BIN} osutils)
add_dependencies(${TARGET_BIN} boot)
add_dependencies(${TARGET_BIN} mem)
add_dependencies(${TARGET_BIN} interrupt_handlers)

target_link_libraries(${TARGET_BIN} ${UPANLIB_OUT_DIR}/libcxxrt.a)
target_link_libraries(${TARGET_BIN} mcpp)
target_link_libraries(${TARGET_BIN} ints)
target_link_libraries(${TARGET_BIN} ${UPANLIB_OUT_DIR}/libgcceh.a)
target_link_libraries(${TARGET_BIN} ${UPANLIB_OUT_DIR}/libustd.a)
target_link_libraries(${TARGET_BIN} ${UPANLIB_OUT_DIR}/libupanui_interop.a)
target_link_libraries(${TARGET_BIN} ${UPANLIB_OUT_DIR}/libm.a)
target_link_libraries(${TARGET_BIN} ${UPANLIB_OUT_DIR}/libc.a)
target_link_libraries(${TARGET_BIN} ${UPANLIB_OUT_DIR}/libgccucore.a)

set_source_files_properties(${OTHER_OBJS} PROPERTIES EXTERNAL_OBJECT true GENERATED true)

