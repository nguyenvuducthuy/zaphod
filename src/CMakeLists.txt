cmake_minimum_required (VERSION 3.1)
project (zaphod)

# For statically linking embree
add_definitions(-DEMBREE_STATIC_LIB)
add_definitions(-DSFML_STATIC)
  
include_directories(${CMAKE_CURRENT_LIST_DIR})

include_directories(${INC_DIRS})

file(GLOB_RECURSE SRC
    "*.h"
    "*.cpp"
)

add_executable(zaphod_exe ${SRC})
target_link_libraries(zaphod_exe ${LIBS})
SET_TARGET_PROPERTIES ( zaphod_exe PROPERTIES OUTPUT_NAME zaphod)
