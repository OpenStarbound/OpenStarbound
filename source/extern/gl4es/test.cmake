# Check if all required arguments are provided
macro(argument_required argument failstring)
	if (NOT ${argument})
		message(FATAL_ERROR ${failstring})
	endif (NOT ${argument})
endmacro(argument_required)

argument_required(LIBRARY_FOLDER "All tests require to have a library output folder.")
argument_required(TESTS_DIRECTORY "All tests require to have a test folder.")
argument_required(TEST_FILENAME "All tests require a trace filename to be reprinted.")
argument_required(CALLS "All tests require a call count.")

# Enable different GLES version tests
if (GLES_FORCED)
	set(GLES${GLES_FORCED}_ENABLED ON)
else (GLES_FORCED)
	set(GLES1_ENABLED ON)
	set(GLES2_ENABLED ON)
endif (GLES_FORCED)

# Special case fo pixels tolerance
if (TOLERANCE)
	set(TOLERANCE_GLES1 ${TOLERANCE})
	set(TOLERANCE_GLES2 ${TOLERANCE})
endif (TOLERANCE)

# Use the built library
set(ENV{LD_LIBRARY_PATH} ${LIBRARY_FOLDER}:$ENV{LD_LIBRARY_PATH})

macro(run_test GLES)
	if (GLES${GLES}_ENABLED)
		argument_required(TOLERANCE_GLES${GLES} "All tests require a pixel tolerance for GLES ${GLES}.")

		set(ENV{LIBGL_ES} ${GLES})

		message(STATUS "Starting test in GLES ${GLES}...")
		execute_process(
			COMMAND ${TESTS_DIRECTORY}/test.sh
			${TEST_FILENAME}
			${CALLS}
			${TOLERANCE_GLES${GLES}}
			${EXTRACT_RANGE}
			ERROR_VARIABLE TEST_ERROR
			OUTPUT_VARIABLE TEST_OUTPUT
			WORKING_DIRECTORY ${TESTS_DIRECTORY}
		)
		message(STATUS "Ran test.\nError: ${TEST_ERROR}\nOutput: ${TEST_OUTPUT}")
		
		if (TEST_OUTPUT)
			set(ERROR ${ERROR} ${GLES})
		endif (TEST_OUTPUT)
	endif (GLES${GLES}_ENABLED)
endmacro(run_test)

run_test(1)
run_test(2)

if (ERROR)
	message(FATAL_ERROR "Test(s) failed while using GLES ${ERROR}")
endif (ERROR)

message(STATUS "Success.")
