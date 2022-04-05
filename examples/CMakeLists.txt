link_directories(${LIBRARY_OUTPUT_PATH})

set(EXAMPLES_LIST
	test_stream
	handler_tutorial
	timer_tutorial
	spdlog_tutorial
	gtest_tutorial
	server
)

set(ES_LIB es gtest pthread ${LIBRT})

foreach(src ${EXAMPLES_LIST})
	string(REPLACE "-" ";" arr ${src})
	list(GET arr -1 bin_name)
	add_executable(${bin_name} ${src}.cc)
	target_link_libraries(${bin_name} ${ES_LIB})
endforeach()
