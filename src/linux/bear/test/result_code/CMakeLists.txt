# This file is distributed under MIT-LICENSE. See COPYING for details.

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/run.sh
  @ONLY)

add_test(NAME result_code_check
  COMMAND ./run.sh)
