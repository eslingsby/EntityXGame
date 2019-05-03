cmake_minimum_required(VERSION 3.4)

project("libaudiodecoder" C CXX)

if (MSVC)
	add_library(
		"libaudiodecoder" STATIC
		"include/audiodecoder.h" "include/audiodecoderbase.h" "include/audiodecodermediafoundation.h"
		"src/audiodecoderbase.cpp" "src/audiodecodermediafoundation.cpp"
	)

	target_link_libraries("libaudiodecoder" "dxva2.lib")
	target_link_libraries("libaudiodecoder" "evr.lib")
	target_link_libraries("libaudiodecoder" "mf.lib")
	target_link_libraries("libaudiodecoder" "mfplat.lib")
	target_link_libraries("libaudiodecoder" "mfplay.lib")
	target_link_libraries("libaudiodecoder" "mfreadwrite.lib")
	target_link_libraries("libaudiodecoder" "mfuuid.lib")
endif(MSVC)

target_include_directories("libaudiodecoder" PUBLIC "include")

