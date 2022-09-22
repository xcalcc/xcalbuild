if (UNIX AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
  add_test(NAME test_profile_tester
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test.sh "${CMAKE_BINARY_DIR}/install/bin/profile-tester" "${CMAKE_CURRENT_SOURCE_DIR}"
  )
endif()