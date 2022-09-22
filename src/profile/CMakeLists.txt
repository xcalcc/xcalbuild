add_library(toolchain-profile OBJECT
    validation.cpp
    parsed-work-item.cpp
    option.cpp
    tool-profile.cpp
    toolchain-profile.cpp
)

add_executable(profile-tester
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:toolchain-profile>
    profile-tester.cpp
)

target_link_libraries(profile-tester PRIVATE
    ${Boost_LIBRARIES}
    nlohmann_json nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS profile-tester
    RUNTIME DESTINATION bin)

add_executable(toolchain-profile-test
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:toolchain-profile>
    toolchain-profile.test.cpp
)

target_link_libraries(toolchain-profile-test PRIVATE
    ${Boost_LIBRARIES}
    nlohmann_json nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT})

add_test(NAME test_toolchain_profile COMMAND toolchain-profile-test)
