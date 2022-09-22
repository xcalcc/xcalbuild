if (UNIX AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
  add_test(NAME test_auto_detect_profile
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test.sh "${CMAKE_BINARY_DIR}/install/bin/xcalbuild" "${CMAKE_CURRENT_SOURCE_DIR}"
  )
endif()