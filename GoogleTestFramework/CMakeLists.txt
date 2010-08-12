set( GTEST_SRCS src/gtest-all.cc src/gtest_main.cc )
include_directories( include )
include_directories( . )

## Compiler flags
if(CMAKE_COMPILER_IS_GNUCXX)
    set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -Wall -Wextra")
endif()

add_library( gtest_main ${GTEST_SRCS} )
