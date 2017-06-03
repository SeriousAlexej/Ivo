if(WIN32)
	set(glm_ROOT "" CACHE PATH "Path to glm root directory.")
endif(WIN32)

set(glm_FIND_DIRS
 ${glm_ROOT}
 $ENV{glm_ROOT}
 /usr/local
 /usr
)

find_path(glm_INCLUDE_DIRS
 NAMES "glm/glm.hpp"
 PATH_SUFFIXES "include"
 PATHS ${glm_FIND_DIRS}
)

if (glm_INCLUDE_DIRS)
  SET(glm_FOUND TRUE)
ENDIF (glm_INCLUDE_DIRS)

if (glm_FOUND)
  if (NOT glm_FIND_QUIETLY)
	message(STATUS "Found OpenGL Mathematics library: ${glm_INCLUDE_DIRS}")
  endif (NOT glm_FIND_QUIETLY)
else (glm_FOUND)
  if (glm_FIND_REQUIRED)
	message(FATAL_ERROR "Could not find OpenGL Mathematics library")
  endif (glm_FIND_REQUIRED)
endif (glm_FOUND)
