function("download_extract_tar" folder url)
	file(DOWNLOAD "${url}" "${CMAKE_CURRENT_SOURCE_DIR}/${folder}.tar.gz")

	execute_process(
		COMMAND "${CMAKE_COMMAND}" -E tar xzf "${folder}.tar.gz"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)

	file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/${folder}.tar.gz")
endfunction("download_extract_tar")

function("add_external_tar" folder url cmake)
	if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${folder}")
		download_extract_tar("${folder}" "${url}")
	endif()
	
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${folder}/${cmake}")
endfunction("add_external_tar")

function("download_file" file url)
	if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
		file(DOWNLOAD "${url}" "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
	endif()
endfunction("download_file")