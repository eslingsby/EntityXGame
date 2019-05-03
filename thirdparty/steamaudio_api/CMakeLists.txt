cmake_minimum_required(VERSION 3.4)

project("phonon" C)

add_library("phonon" INTERFACE)

if (MSVC)
	if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
		target_link_libraries("phonon" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib/Windows/x64/phonon.lib")
	else()
		target_link_libraries("phonon" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib/Windows/x86/phonon.lib")
	endif()
endif()

target_include_directories("phonon" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

#set_property(TARGET "phonon" PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/Windows/x64/phonon.lib")

#set_target_properties("phonon" PROPERTIES LINKER_LANGUAGE C)

#target_link_libraries("phonon" "${CMAKE_CURRENT_SOURCE_DIR}/lib/Windows/x64/phonon.lib")