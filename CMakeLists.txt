cmake_minimum_required(VERSION 3.14)

project(xcalbuild VERSION 0.2.0)
enable_testing()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif(MSVC)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_USE_MULTITHREADED ON)
find_package(ZLIB REQUIRED)
find_library(LIBARCHIVE_LIBRARY archive)
find_package(Boost 1.73.0 REQUIRED COMPONENTS filesystem program_options log date_time iostreams)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Threads)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (WIN32)
set(CMAKE_CXX_FLAGS "/EHa /W4 /DWIN32_LEAN_AND_MEAN")
else()
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-system-headers")
set(CMAKE_CXX_FLAGS_DEBUG "-g --coverage")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto")
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/.git/HEAD")
  file(READ "${PROJECT_SOURCE_DIR}/.git/HEAD"
    PROJECT_SOURCE_VERSION)
  if("${PROJECT_SOURCE_VERSION}" MATCHES "^ref:")
    string(REGEX REPLACE "^ref: *([^ \n\r]*).*" "\\1"
      PROJECT_GIT_REF "${PROJECT_SOURCE_VERSION}")
    file(READ "${PROJECT_SOURCE_DIR}/.git/${PROJECT_GIT_REF}"
      PROJECT_SOURCE_VERSION)
  endif()
  string(STRIP "${PROJECT_SOURCE_VERSION}"
    PROJECT_SOURCE_VERSION)
endif()

if (UNIX)
# check if Doxygen is installed
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if (DOXYGEN_FOUND)
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_GENERATE_TREEVIEW YES)
  set(DOXYGEN_OUTPUT_DIRECTORY docs/)
  set(DOXYGEN_PROJECT_BRIEF "Xcalibyte build monitor.")
  set(DOXYGEN_PROJECT_NUMBER ${CIO_VERSION}${CIO_LAST})
  set(DOXYGEN_QUIET NO)
  set(DOXYGEN_RECURSIVE YES)
  set(DOXYGEN_STRICT_PROTO_MATCHING YES)
  set(DOXYGEN_STRIP_FROM_PATH ${PROJECT_BINARY_DIR}/src/ ${CMAKE_CURRENT_SOURCE_DIR})
  set(DOXYGEN_WARN_AS_ERROR NO)
  set(DOXYGEN_PROJECT_NAME ${CMAKE_PROJECT_NAME})
  set(DOXYGEN_EXCLUDE_PATTERNS
    */.git/*
    */CMakeFiles/*
    */scripts
    */tests
    */src/include/taskflow
    CMakeLists.txt)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")

  doxygen_add_docs(docs
      ${PROJECT_SOURCE_DIR}/src
      ${PROJECT_SOURCE_DIR}/docs
      ${PROJECT_SOURCE_DIR}/README.md
        COMMENT "Generating API documentation with Doxygen"
  )
  add_custom_command(TARGET docs
      POST_BUILD
      COMMAND cp -R ${PROJECT_SOURCE_DIR}/docs/files ${PROJECT_BINARY_DIR}/docs/html/
  )

else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
endif (UNIX)

add_subdirectory(src)
add_subdirectory(profiles)
add_subdirectory(tests)

install(DIRECTORY
    DESTINATION bin)
install(TARGETS
    xcalbuild
    profile-tester
    RUNTIME DESTINATION bin)

if (APPLE)
    message("Please make sure you have installed bear in your apple system")
elseif (UNIX)
    install(TARGETS
        unix-tracer
        RUNTIME DESTINATION bin)
elseif (WIN32)
    install(TARGETS
        win-tracer
        RUNTIME DESTINATION bin)
endif ()

if (UNIX AND NOT APPLE)
    install(FILES ${CMAKE_BINARY_DIR}/src/linux/bear/src/libtracer-preload32.so DESTINATION lib32 RENAME libtracer-preload.so)
    install(FILES ${CMAKE_BINARY_DIR}/src/linux/bear/src/libtracer-preload64.so DESTINATION lib64 RENAME libtracer-preload.so)
endif ()

install(FILES
    ${CMAKE_SOURCE_DIR}/src/linux/config
    DESTINATION .)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/profiles DESTINATION .
    PATTERN "CMakeLists.txt" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include DESTINATION .
    PATTERN "CMakeLists.txt" EXCLUDE
)

# A pseudo target to install before run test.
ADD_TEST(00_Install ${CMAKE_BUILD_TOOL} install)
