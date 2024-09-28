cmake_minimum_required (VERSION 3.15)

project ("MiTurboFix"
	VERSION 0.0.7
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

FetchContent_Declare(
	ktsignal
	GIT_REPOSITORY https://github.com/kin4stat/ktsignal.git
	GIT_TAG f08a19d9a00ad5284871bf44178d835da7b26ebf
)
FetchContent_Declare(
	kthook
	GIT_REPOSITORY https://github.com/kin4stat/kthook.git
	GIT_TAG 21ca70481273d1efcc98ca49063178e46277009b
)
FetchContent_Declare(
	RakHook
	GIT_REPOSITORY https://github.com/povargek/RakHook.git
	GIT_TAG 7685236c9bb929d741b73d9a5f6d390c8cad6e7f
)
FetchContent_MakeAvailable(ktsignal kthook rakhook)

add_library(${PROJECT_NAME} SHARED)
target_link_libraries(${PROJECT_NAME} PUBLIC kthook rakhook)
target_sources(${PROJECT_NAME} PRIVATE
	src/main.cpp
	src/Plugin.cpp
	src/PluginRPC.cpp
	src/RPCEnumerations.h
	src/misc.h
	src/resource.h
	src/Resource.rc
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME ${PROJECT_NAME}
	SUFFIX ".asi"
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
