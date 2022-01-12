cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

enable_language(CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)					# don't let OPTION erase non-cache variables
set(CMAKE_CXX_EXTENSIONS OFF CACHE INTERNAL "" FORCE)	# disable compiler extensions
set(CMAKE_POSITION_INDEPENDENT_CODE ON)					# enable PIC
set(CMAKE_DEBUG_POSTFIX "-d")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)			# ensure CMAKE_BUILD_TYPE is not empty
endif()

# project parameters
set(project_name jukebox)
set(project_version "1.0.3")
set(project_version_name jukebox_version) # name of variable in generated header
set(cxx_standard cxx_std_20)
set(cmake_var_prefix JUKEBOX)

project(${project_name} VERSION "${project_version}")
set(is_root_project OFF)	# indicate if this is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
  message(STATUS "${PROJECT_NAME} is top level")
endif()

# includes
include(cmake/platform.cmake)

# cmake-utils
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
  cmake-utils
  GIT_REPOSITORY https://github.com/karnkaul/cmake-utils
  GIT_TAG v1.0.1
  GIT_SHALLOW ON
)
FetchContent_MakeAvailable(cmake-utils)
FetchContent_GetProperties(cmake-utils)

include("${cmake-utils_SOURCE_DIR}/cmake-utils.cmake")

# git operations
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  if(is_root_project)
    option(GIT_UPDATE_SUBMODULES "Init/Update submodules during generation" ${is_root_project})
    if(GIT_UPDATE_SUBMODULES)
      git_update_submodules(WARNING)	# update git submodules and warn on failure
    endif()
  endif()
endif()

# header text
if(is_root_project)
  set(build_config ${CMAKE_BUILD_TYPE})
  if(CMAKE_CONFIGURATION_TYPES)
    set(build_config "Multi-config")
  endif()
  message(STATUS "[${CMAKE_SYSTEM_PROCESSOR}] [${PLATFORM}] [${CMAKE_GENERATOR}] [${CMAKE_CXX_COMPILER_ID}] [${build_config}]")
endif()

# compile commands
if(is_root_project AND NOT "${CI_BUILD}")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(${cmake_var_prefix}_CONFIGURE_CI_SCRIPTS ON)
  message(STATUS "${PROJECT_NAME}: Exporting compile commands, configuring CI scripts")
endif()

# version
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/jk_version.hpp.in")
  message(STATUS "Configuring ${CMAKE_CURRENT_SOURCE_DIR}/src/jk_version.hpp.in")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/jk_version.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/jk_version.hpp")
endif()

# release
if(NOT "${CI_BUILD}" AND "${${cmake_var_prefix}_CONFIGURE_CI_SCRIPTS}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/release.yml.in")
  message(STATUS "Configuring ${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/release.yml")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/release.yml.in" "${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/release.yml")
endif()

# capo
set(CAPO_BUILD_EXAMPLE OFF)
set(CAPO_VALID_IF_INACTIVE OFF)
add_subdirectory(ext/capo)

# dibs
set(DIBS_BUILD_EXAMPLE OFF)
set(DIBS_INSTALL OFF)
add_subdirectory(ext/dibs)

# exe
add_executable(${PROJECT_NAME})
add_subdirectory(src)
target_source_group(TARGET ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE include src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:JK_DEBUG>)
target_link_libraries(${PROJECT_NAME} 
  PRIVATE 
    capo::capo
    dibs::dibs
    dibs::options
)
if(PLATFORM STREQUAL "Win64" AND NOT WIN64_GCC)
  if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
      /ENTRY:mainCRTStartup                              # Link to main() and not WinMain()
      /SUBSYSTEM:$<IF:$<CONFIG:Debug>,CONSOLE,WINDOWS>   # Spawn a console in Debug
    )
  else()
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,/SUBSYSTEM:$<IF:$<CONFIG:Debug>,CONSOLE,WINDOWS>,/ENTRY:mainCRTStartup)
  endif()
endif()

install(TARGETS ${PROJECT_NAME})
