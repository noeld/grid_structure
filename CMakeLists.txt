cmake_minimum_required(VERSION 3.20)

project(ogl2 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(fmt CONFIG REQUIRED)

message(STATUS "glfw-libraries ${GLFW_LIBRARIES}")
message(STATUS "OPENGL_LIBRARIES ${OPENGL_LIBRARIES}")

include_directories(${OPENGL_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS})

add_executable(ogl2 ogl2.cpp)
target_link_libraries(ogl2
    ${OPENGL_LIBRARIES}
    glfw
    ${GLEW_LIBRARIES}
    fmt::fmt
    )
target_compile_options(ogl2 PRIVATE  "-mavx2")