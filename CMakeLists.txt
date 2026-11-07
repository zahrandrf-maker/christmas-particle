cmake_minimum_required(VERSION 3.21)
project(BRChristmasParticles VERSION 0.1.0 LANGUAGES CXX)
if(NOT WIN32)
  message(FATAL_ERROR "BR Christmas Particles currently supports Windows only")
endif()
include(FetchContent)
set(GLEW_USE_STATIC_LIBS TRUE)
find_package(GLEW REQUIRED)
set(FFGL_BUILD_EXAMPLE_PLUGINS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(ffgl
  GIT_REPOSITORY https://github.com/resolume/ffgl.git
  GIT_TAG 5b6e07840277df9ec35a228b9eb067636316350f
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(ffgl)
add_library(BRChristmasParticles SHARED
  src/BRChristmasParticles.cpp src/BRChristmasParticles.h FFGLPlugin.def)
target_compile_features(BRChristmasParticles PRIVATE cxx_std_17)
target_compile_definitions(BRChristmasParticles PRIVATE NOMINMAX)
target_link_libraries(BRChristmasParticles PRIVATE ffgl::sdk GLEW::GLEW gdiplus ole32 comdlg32 opengl32)
target_include_directories(BRChristmasParticles PRIVATE ${ffgl_SOURCE_DIR}/source/lib)
set_target_properties(BRChristmasParticles PROPERTIES
  OUTPUT_NAME "BR Christmas Particles" PREFIX "" SUFFIX ".dll"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugin"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/plugin")
if(MSVC)
  target_compile_options(BRChristmasParticles PRIVATE /W4 /permissive- /utf-8 /UWIN32_LEAN_AND_MEAN /UNOMETAFILE /UNOGDI)
  target_link_options(BRChristmasParticles PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/FFGLPlugin.def")
endif()

