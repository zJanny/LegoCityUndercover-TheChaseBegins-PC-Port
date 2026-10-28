execute_process(COMMAND ${CMAKE_COMMAND} -DROOT_DIR=${ROOT_DIR} -P ${GUARD}
  RESULT_VARIABLE result ERROR_VARIABLE output OUTPUT_VARIABLE standard)
if(result EQUAL 0)
  message(FATAL_ERROR "qol-separation accepted ${ROOT_DIR}, expected a rejection naming ${EXPECT}")
endif()
string(FIND "${output}" "${EXPECT}" position)
if(position EQUAL -1)
  message(FATAL_ERROR "qol-separation rejected ${ROOT_DIR} without naming ${EXPECT}:\n${output}")
endif()
