# - Find Iberty
# This module finds libiberty.
#
# It sets the following variables:
#  IBERTY_LIBRARY     - The JSON-C library to link against.

FIND_LIBRARY( IBERTY_LIBRARY NAMES iberty_pic )

IF (NOT IBERTY_LIBRARY)
	FIND_LIBRARY( IBERTY_LIBRARY NAMES iberty)
	MESSAGE("IBERTY_LIBRARY is ${IBERTY_LIBRARY}")
	EXEC_PROGRAM( "nm ${IBERTY_LIBRARY} | grep -q _GLOBAL_OFFSET_TABLE_" RETURN_VALUE IBERTY_LIBRARY_TEST_RESULT)
	MESSAGE("grep result ${IBERTY_LIBRARY_TEST_RESULT}")
	IF ( "${IBERTY_LIBRARY_TEST_RESULT}" MATCHES "0" )
	ELSE()
		SET(IBERTY_LIBRARY FALSE)
	ENDIF()
ENDIF()

MESSAGE("IBERTY_LIBRARY is ${IBERTY_LIBRARY}")

IF (IBERTY_LIBRARY)

   # show which JSON-C was found only if not quiet
   MESSAGE( "Found libiberty: ${IBERTY_LIBRARY}")

   SET(IBERTY_FOUND TRUE)

ELSE (IBERTY_LIBRARY)

   IF ( IBERTY_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find libiberty. Try to install binutil-devel?")
   ELSE()
      MESSAGE(STATUS "Could not find libiberty; downloading binutils and building PIC libibierty.")
   ENDIF (IBERTY_FIND_REQUIRED)

ENDIF (IBERTY_LIBRARY)
