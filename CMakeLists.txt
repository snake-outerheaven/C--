cmake_minimum_required(VERSION 3.10)

project(HelloWorld)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(hello_world exercise/exercise_1_hello_world/hello_world.cpp)
add_executable(variables exercise/exercise_2-variables/variables.cpp)
add_executable(variables2 exercise/exercise_2-variables/variables2.cpp)
add_executable(variables3 exercise/exercise_2-variables/variables3.cpp)
add_executable(break exercise/exercise_3_control/break.cpp)
add_executable(for_loop exercise/exercise_3_control/for.cpp)
add_executable(while_loop exercise/exercise_3_control/while.cpp)
add_executable(function exercise/exercise_4_function/function.cpp)
add_executable(function2 exercise/exercise_4_function/function2.cpp)
add_executable(pointer exercise/exercise_4_function/pointer.cpp)