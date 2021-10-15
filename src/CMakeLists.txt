target_sources(${PROJECT_NAME} PRIVATE
  jk_common.hpp
  main.cpp
)

add_subdirectory(gui)
add_subdirectory(misc)
add_subdirectory(win)
add_subdirectory(vk)
