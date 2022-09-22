FOREACH(tool cc1 as ar ld)
    add_test(NAME test_${tool}_profile
             COMMAND ${CMAKE_BINARY_DIR}/install/bin/profile-tester --
                     -p ${CMAKE_CURRENT_SOURCE_DIR}/${tool}.json
                     -t ${CMAKE_CURRENT_SOURCE_DIR}/${tool}.test.json)
ENDFOREACH()
