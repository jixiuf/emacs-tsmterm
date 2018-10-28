cmake_minimum_required(VERSION 3.0.2)

project(tsmterm C)

add_library(tsmterm-module MODULE tsmterm-module.c utf8.c elisp.c)
set_property(TARGET tsmterm-module PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(tsmterm-module PROPERTIES PREFIX "")
set_target_properties(tsmterm-module PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Link with libtsm
target_link_libraries(tsmterm-module tsm)

# Custom run command for testing
add_custom_target(run
  COMMAND emacs -Q -L ${CMAKE_SOURCE_DIR} --eval "\\(require \\'tsmterm\\)" --eval "\\(tsmterm\\)"
  DEPENDS tsmterm-module
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
