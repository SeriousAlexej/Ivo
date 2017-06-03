if(WIN32)
	set(assimp_ROOT "" CACHE PATH "Path to Assimp root directory.")

	if(MSVC12)
		set(assimp_MSVC_VERSION "vc120")
	elseif(MSVC14)	
		set(assimp_MSVC_VERSION "vc140")
	endif(MSVC12)
	
	if(MSVC12 OR MSVC14)
		set(assimp_LIB_NAMES "assimp-${assimp_MSVC_VERSION}-mt.lib")
	else()
		set(assimp_LIB_NAMES "assimp" "assimp.dll")
	endif()
	
else(WIN32)

	set(assimp_LIB_NAMES "assimp")
	
endif(WIN32)

set(assimp_FIND_DIRS
 ${assimp_ROOT}
 $ENV{assimp_ROOT}
 /usr/local
 /usr
)

find_path(
  assimp_INCLUDE_DIRS
  NAMES "assimp/postprocess.h" "assimp/scene.h" "assimp/version.h" "assimp/config.h" "assimp/cimport.h"
  PATH_SUFFIXES "include"
  PATHS ${assimp_FIND_DIRS}
)

find_library(
  assimp_LIBRARIES
  NAMES ${assimp_LIB_NAMES}
  PATH_SUFFIXES "lib64" "lib" "code" "build/code"
  PATHS ${assimp_FIND_DIRS}
)

if (assimp_INCLUDE_DIRS AND assimp_LIBRARIES)
  SET(assimp_FOUND TRUE)
ENDIF (assimp_INCLUDE_DIRS AND assimp_LIBRARIES)

if (assimp_FOUND)
  if (NOT assimp_FIND_QUIETLY)
	message(STATUS "Found asset importer library: ${assimp_LIBRARIES}")
  endif (NOT assimp_FIND_QUIETLY)
else (assimp_FOUND)
  if (assimp_FIND_REQUIRED)
	message(FATAL_ERROR "Could not find asset importer library")
  endif (assimp_FIND_REQUIRED)
endif (assimp_FOUND)