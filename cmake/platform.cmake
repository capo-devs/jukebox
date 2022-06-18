set(WIN64_MSBUILD 0 CACHE INTERNAL "" FORCE)
set(WIN64_CLANG 0 CACHE INTERNAL "" FORCE)
set(WIN64_VCXX 0 CACHE INTERNAL "" FORCE)
set(WIN64_GCC 0 CACHE INTERNAL "" FORCE)
set(LINUX_CLANG 0 CACHE INTERNAL "" FORCE)
set(LINUX_GCC 0 CACHE INTERNAL "" FORCE)
set(MSVC_RUNTIME 0 CACHE INTERNAL "" FORCE)
set(GCC_RUNTIME 0 CACHE INTERNAL "" FORCE)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Enforce x64
  set(CMAKE_VS_PLATFORM_NAME "x64" CACHE STRING "" FORCE)

  if(NOT CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    message(FATAL_ERROR "Only x64 builds are supported!")
  endif()

  set(PLATFORM "Win64" CACHE INTERNAL "" FORCE)

  if(CMAKE_GENERATOR MATCHES "^(Visual Studio)")
    set(WIN64_MSBUILD 1)
    set(MSVC_RUNTIME 1)
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(WIN64_CLANG 1)

    if(MSVC)
      # Ref: https://gitlab.kitware.com/cmake/cmake/issues/17808
      set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-imsvc" CACHE STRING "" FORCE)
    endif()

    set(MSVC_RUNTIME 1)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(WIN64_VCXX 1)
    set(MSVC_RUNTIME 1)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(WIN64_GCC 1)
    set(GCC_RUNTIME 1)
  else()
    message("\tWARNING: Unsupported compiler [${CMAKE_CXX_COMPILER_ID}], expect build warnings/errors!")
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(GCC_RUNTIME 1)
  set(PLATFORM "Linux" CACHE INTERNAL "" FORCE)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(LINUX_CLANG 1)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(LINUX_GCC 1)
  else()
    message("\tWARNING: Unsupported compiler [${CMAKE_CXX_COMPILER_ID}], expect build warnings/errors!")
  endif()
else()
  message(WARNING "Unsupported system [${CMAKE_SYSTEM_NAME}]!")
endif()

if(PLATFORM STREQUAL "Win64")
  # Embed debug info in builds
  string(REPLACE "ZI" "Z7" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "Zi" "Z7" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "ZI" "Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
  string(REPLACE "Zi" "Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif()
