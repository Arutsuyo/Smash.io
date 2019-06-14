cmake_minimum_required(VERSION 2.8.12.1)

PROJECT(SSBM)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Debug / Release (default)
IF(CMAKE_BUILD_TYPE MATCHES Debug) # Debug
	message("Debug build.")
	add_definitions(-DDEBUG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -g")
	
	# Controller Printing
	if(NOT CTRL_OUTPUT)
		if(DEFINED CTRL_OUTPUT)
			set(CTRL_OUTPUT false CACHE BOOLEAN false)
			message("Silencing controller printing. Print with: -DCTRL_OUTPUT=true")
			add_definitions(-DCTRL_OUTPUT=false)
		else()
			set(CTRL_OUTPUT true CACHE BOOLEAN true)
			message("CTRL_OUTPUT not defined.")
			message("Printing Controller output. Silence with: -DCTRL_OUTPUT=false")
			add_definitions(-DCTRL_OUTPUT=false)
		endif()
	else()
		set(CTRL_OUTPUT true CACHE BOOLEAN true)
		message("Printing Controller output. Silence with: -DCTRL_OUTPUT=false")
		add_definitions(-DCTRL_OUTPUT=true)
	endif()
	
	# Tensor Error Printing
	if(NOT TENSOR_ERR_PRINT)
		if(DEFINED TENSOR_ERR_PRINT)
			set(TENSOR_ERR_PRINT false CACHE BOOLEAN false)
			message("Silencing Tensor error printing. Print with: -DTENSOR_ERR_PRINT=true")
			add_definitions(-DTENSOR_ERR_PRINT=false)
		else()
			set(TENSOR_ERR_PRINT true CACHE BOOLEAN true)
			message("TENSOR_ERR_PRINT not defined.")
			message("Printing Tensor erro output. Silence with: -DTENSOR_ERR_PRINT=false")
			add_definitions(-DTENSOR_ERR_PRINT=false)
		endif()
	else()
		set(TENSOR_ERR_PRINT true CACHE BOOLEAN true)
		message("Printing Tensor erro output. Silence with: -DTENSOR_ERR_PRINT=false")
		add_definitions(-DTENSOR_ERR_PRINT=true)
	endif()

	# Memory Printing
	if(NOT MEMORY_OUT)
		if(DEFINED MEMORY_OUT)
			set(MEMORY_OUT false CACHE BOOLEAN false)
			message("Silencing Memory scan printing. Print with: -DMEMORY_OUT=true")
			add_definitions(-DMEMORY_OUT=false)
		else()
			set(MEMORY_OUT true CACHE BOOLEAN true)
			message("MEMORY_OUT not defined.")
			message("Printing Memory scan output. Silence with: -DMEMORY_OUT=false")
			add_definitions(-DMEMORY_OUT=false)
		endif()
	else()
		set(MEMORY_OUT true CACHE BOOLEAN true)
		message("Printing Memory scan output. Silence with: -DMEMORY_OUT=false")
		add_definitions(-DMEMORY_OUT=true)
	endif()

ELSE() # Release
	message("Release build.")
	message("If you want to build in debug, run with cmake with: -DCMAKE_BUILD_TYPE=Debug")
	add_definitions(-DCTRL_OUTPUT=false)
	add_definitions(-DTENSOR_ERR_PRINT=false)
	add_definitions(-DMEMORY_OUT=false)
ENDIF()

option(CTRL_OUTPUT "Feature description" $(CTRL_OUTPUT))

SET (CMAKE_SHARED_LINKER_FLAGS)
IF(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DSTBI_MSC_SECURE_CRT)
endif()

FILE(GLOB SOURCE_SOURCE ${CMAKE_SOURCE_DIR}/Src/*.cpp)
FILE(GLOB SOURCE_HEADERS ${CMAKE_SOURCE_DIR}/Src/*.h)

IF(WIN32)
	SOURCE_GROUP("Sources" FILES${SOURCE_SOURCE})
	SOURCE_GROUP("Headers" FILES${SOURCE_HEADERS})
endif()

SET(SOURCES ${SOURCE_SOURCE})
SET(HEADERS ${SOURCE_HEADERS})

include_directories(${CMAKE_SOURCE_DIR}/include)
IF(WIN32)
	LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/Win32)
ELSE()
	LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
ENDIF()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
IF(WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
ENDIF()
add_executable(SSBM ${SOURCES} ${HEADERS})
target_link_libraries(SSBM Threads::Threads)

configure_file(
	"${CMAKE_SOURCE_DIR}/Python/client_secret.json"
	"${CMAKE_BINARY_DIR}/client_secret.json"
	COPYONLY)
configure_file(
	"${CMAKE_SOURCE_DIR}/Python/trainer.py"
	"${CMAKE_BINARY_DIR}/trainer.py" 
	COPYONLY)
file(COPY "${CMAKE_SOURCE_DIR}/Version" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY "${CMAKE_SOURCE_DIR}/Files" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY "${CMAKE_SOURCE_DIR}/AI" DESTINATION ${CMAKE_BINARY_DIR})
