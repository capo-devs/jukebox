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
set(project_version "0.1.0.0")
set(project_version_name jukebox_version) # name of variable in generated header
set(cxx_standard cxx_std_20)

project(${project_name} VERSION "${project_version}")
set(is_root_project OFF)	# indicate if this is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
  message(STATUS "${PROJECT_NAME} is top level")
endif()

# includes
include(cmake/platform.cmake)
include(cmake/utils.cmake)

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

# compile commands
if(is_root_project AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  message(STATUS "Exporting compile commands")
endif()

# version
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/jk_version.hpp.in")
  message(STATUS "Configuring jk_version.hpp")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/jk_version.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/jk_version.hpp")
endif()

# appveyor
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.appveyor.yml.in")
  message(STATUS "Configuring appveyor.yml")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/.appveyor.yml.in" "${CMAKE_CURRENT_SOURCE_DIR}/.appveyor.yml")
endif()

# capo
set(CAPO_BUILD_EXAMPLE OFF)
add_subdirectory(ext/capo)

# vulkan-hpp
add_subdirectory(ext/vulkan_hpp)

# vk-bootstrap
set(VK_BOOTSTRAP_VULKAN_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/vulkan_hpp/include")
add_subdirectory(ext/vk_bootstrap)

# glfw
set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
add_subdirectory(ext/glfw)

# imgui
add_subdirectory(ext/dear_imgui)

# exe
add_executable(${PROJECT_NAME})
add_subdirectory(src)
target_source_group(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE include src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:JK_DEBUG>)
target_link_libraries(${PROJECT_NAME} 
  PRIVATE 
    vk-hpp::vk-hpp
    vk-bootstrap::vk-bootstrap
    capo::capo
    capo::options
    dear_imgui::dear_imgui
)
