cmake_minimum_required(VERSION 2.8)



add_compile_options(-Wall -Wextra -Wpedantic)
#include_directories(modules/)

add_executable(sample main.cpp)

#target_compile_features(sample PRIVATE cxx_lambda_init_captures)