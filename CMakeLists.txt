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

	target_compile_features(voyageOpt PRIVATE cxx_std_17)
	#set(CMAKE_CXX_STANDARD 17)
	#set(CMAKE_CXX_STANDARD_REQUIRED ON)


	set(ENV{PKG_CONFIG_PATH} "$ENV{HOME}/usr/lib/pkgconfig:" $ENV{PKG_CONFIG_PATH})
	MESSAGE("PKG_CONFIG_PATH:" $ENV{PKG_CONFIG_PATH})
	find_package(PkgConfig)
	pkg_check_modules(HIREDIS hiredis)
	MESSAGE("HIREDIS_FOUND:" ${HIREDIS_FOUND})
	MESSAGE("HIREDIS_VERSION:" ${HIREDIS_VERSION})
	MESSAGE("HIREDIS_LIBRARIES:" ${HIREDIS_LIBRARIES})
	MESSAGE("HIREDIS_INCLUDE_DIRS:" ${HIREDIS_INCLUDE_DIRS})
	MESSAGE("HIREDIS_LIBRARY_DIRS:" ${HIREDIS_LIBRARY_DIRS})	
	INCLUDE_DIRECTORIES(${HIREDIS_INCLUDE_DIRS})
	LINK_DIRECTORIES(${HIREDIS_LIBRARY_DIRS})

	#find_package(redis++)
	pkg_check_modules(REDIS_PLUS_PLUS redis++)
	MESSAGE("REDIS_PLUS_PLUS_FOUND:" ${REDIS_PLUS_PLUS_FOUND})
	MESSAGE("REDIS_PLUS_PLUS_VERSION:" ${REDIS_PLUS_PLUS_VERSION})
 
	MESSAGE("REDIS_PLUS_PLUS_LIBRARIES:" ${REDIS_PLUS_PLUS_LIBRARIES})
	MESSAGE("REDIS_PLUS_PLUS_INCLUDE_DIRS:" ${REDIS_PLUS_PLUS_INCLUDE_DIRS})
	MESSAGE("REDIS_PLUS_PLUS_LIBRARY_DIRS:" ${REDIS_PLUS_PLUS_LIBRARY_DIRS})
	INCLUDE_DIRECTORIES(${REDIS_PLUS_PLUS_INCLUDE_DIRS})
	LINK_DIRECTORIES(${REDIS_PLUS_PLUS_LIBRARY_DIRS})

	#//pkg_check_modules(REDOX redox)
	#//MESSAGE("REDOX_FOUND:" ${REDOX_FOUND})
	#//MESSAGE("REDOX_VERSION:" ${REDOX_VERSION})
	#//MESSAGE("REDOX_LIBRARIES:" ${REDOX_LIBRARIES})
	#//MESSAGE("REDOX_INCLUDE_DIRS:" ${REDOX_INCLUDE_DIRS})
	#//MESSAGE("REDOX_LIBRARY_DIRS:" ${REDOX_LIBRARY_DIRS})
	#//INCLUDE_DIRECTORIES(${REDOX_INCLUDE_DIRS})
	#//LINK_DIRECTORIES(${REDOX_LIBRARY_DIRS})

	find_path(HIREDIS_HEADER hiredis)
	target_include_directories(voyageOpt PUBLIC ${HIREDIS_HEADER})

	find_library(HIREDIS_LIB hiredis)
	target_link_libraries(voyageOpt ${HIREDIS_LIB})

	# <------------ add redis-plus-plus dependency -------------->
	# NOTE: this should be *sw* NOT *redis++*
	find_path(REDIS_PLUS_PLUS_HEADER sw)
	target_include_directories(voyageOpt PUBLIC ${REDIS_PLUS_PLUS_HEADER})

	find_library(REDIS_PLUS_PLUS_LIB redis++)
	target_link_libraries(voyageOpt ${REDIS_PLUS_PLUS_LIB})	

	find_library(REDIS_PLUS_PLUS_LIB redis++)
	include_directories("/usr/include/gdal") 
	find_package( GDAL )  
	include_directories( ${GDAL_INCLUDE_DIRS} ) 
	find_package( ZLIB )
	include_directories( ${ZLIB_INCLUDE_DIRS} )
	#target_link_libraries(weatherTest ${ZLIB_LIBRARY} ${GDAL_LIBRARY} ${MY_LIB})  
	target_link_libraries(voyageOpt ${ZLIB_LIBRARY} ${GDAL_LIBRARY} ${REDIS_PLUS_PLUS_LIB})  
endif()
