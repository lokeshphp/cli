# CMakeList.txt : CMake project for weatherTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("voyageOpt")

add_definitions(-DUNICODE) 
add_definitions(-D_UNICODE)

# Add source to this project's executable.
add_executable (voyageOpt "weatherTest.cpp" "weatherTest.h" "voyageOpt.cpp" "test.cpp")

# TODO: Add tests and install targets if needed.


if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION) 
	#include_directories(C:/OSGeo4W64/apps/gdal-dev/include)  
	# include_directories(C:/OSGeo4W64/apps/gdal-dev/lib)
	# include_directories(C:\Users\User\zlib_tmp\zlib-1.2.12\build\Release)  
	find_library(MY_LIB legacy_stdio_definitions.lib)
	find_package(legacy_stdio_definitions.lib REQUIRED)
	if(NOT MY_LIB)
		message(FATAL_ERROR "legacy... not found")
	endif()
	if(MY_LIB)
		message(FATAL_ERROR "legacy... found. YES") 
	endif()
	link_libraries(legacy_stdio_definitions.lib) 
	target_link_libraries(voyageOpt) # C:\Users\User\zlib_tmp\zlib-1.2.12\build\Release\zlib.lib)# C:/OSGeo4W64/apps/gdal-dev/lib/gdal_i.lib)
#	target_link_libraries(weatherTest legacy_stdio_definitions.lib C:/OSGeo4W64/apps/gdal-dev/lib/gdal_i.lib C:\Users\User\zlib_tmp\zlib-1.2.3-bin\zlib1.dll)
#	target_link_libraries(weatherTest C:/OSGeo4W64/apps/gdal-dev/lib/gdal_i.lib)  
else()
	#find_library(MY_LIB legacy_stdio_definitions.lib)
	include_directories("/usr/include/gdal") 
	find_package( GDAL )  
	include_directories( ${GDAL_INCLUDE_DIRS} ) 
	find_package( ZLIB )
	include_directories( ${ZLIB_INCLUDE_DIRS} )
	#target_link_libraries(weatherTest ${ZLIB_LIBRARY} ${GDAL_LIBRARY} ${MY_LIB})  
	target_link_libraries(voyageOpt ${ZLIB_LIBRARY} ${GDAL_LIBRARY})  
endif()
