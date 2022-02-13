find_library(CPPUNIT_LIBRARY cppunit)
find_path(CPPUNIT_INCLUDE_DIR cppunit/extensions/HelperMacros.h)
if(NOT CPPUNIT_LIBRARY OR NOT CPPUNIT_INCLUDE_DIR)
  message(SEND_ERROR "Could not find cppunit for tests")
endif()

add_executable(test_main main.cpp)
target_include_directories(test_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_main ${CPPUNIT_LIBRARY} cppbor)
add_test(NAME main_test COMMAND test_main
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_sources
  )
