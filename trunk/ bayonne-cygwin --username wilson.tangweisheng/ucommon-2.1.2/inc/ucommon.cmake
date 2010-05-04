# Copyright (C) 2009 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

if (NOT UCOMMON_LIBS AND NOT UCOMMON_FLAGS)
	include(CheckCCompilerFlag)

	if(CMAKE_COMPILER_IS_GNUCXX)
		set(UCOMMON_VISIBILITY_FLAG "-fvisibility=hidden")
		if(MINGW OR MSYS)
			set(CHECK_FLAGS -Wno-long-long -mthreads -fvisibility-inlines-hidden)
		else()
			set(CHECK_FLAGS -Wno-long-long -pthread -mt -fvisibility-inlines-hidden)
		endif()
	endif()

	if(BUILD_STATIC)
		set(UCOMMON_FLAGS ${UCOMMON_FLAGS} -DUCOMMON_STATIC)
	endif()

	# see if we are building with or without std c++ libraries...
	if (BUILD_STDLIB)
		# for now we assume only newer libstdc++ library
		set(UCOMMON_FLAGS ${UCOMMON_FLAGS} -DNEW_STDLIB)
	else()
		if(CMAKE_COMPILER_IS_GNUCXX)
			set(CHECK_FLAGS ${CHECK_FLAGS} -fno-exceptions -fno-rtti -fno-enforce-eh-specs)
			if(MINGW OR MSYS)
				set(UCOMMON_LIBS ${UCOMMON_LIBS} -nodefaultlibs -nostdinc++ msvcrt)
			else()
				set(UCOMMON_LIBS ${UCOMMON_LIBS} -nodefaultlibs -nostdinc++)
			endif()
		endif()	
	endif()

	# check final for compiler flags
	foreach(flag ${CHECK_FLAGS})
		check_c_compiler_flag(${flag} CHECK_${flag})
		if(CHECK_${flag})
			set(UCOMMON_FLAGS ${UCOMMON_FLAGS} "${flag}")
		endif()
	endforeach()

	# visibility support for linking reduction (gcc >4.1 only so far...)

	if(UCOMMON_VISIBILITY_FLAG)
		check_c_compiler_flag(${UCOMMON_VISIBILITY_FLAG} CHECK_VISIBILITY)
	endif()

	if(CHECK_VISIBILITY)
		set(UCOMMON_FLAGS ${UCOMMON_FLAGS} ${UCOMMON_VISIBILITY_FLAG} -DUCOMMON_VISIBILITY=1)
	else()
		set(UCOMMON_FLAGS ${UCOMMON_FLAGS} -DUCOMMON_VISIBILITY=0)
	endif()

endif()
