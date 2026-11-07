cmake_minimum_required(VERSION 3.16)
project(ai_assisted_grade LANGUAGES CXX)
add_executable(ai_grade src/main.cpp)
target_compile_features(ai_grade PRIVATE cxx_std_17)
set_target_properties(ai_grade PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
if(WIN32)
  target_link_libraries(ai_grade PRIVATE shell32)
endif()
if(MSVC)
  target_compile_options(ai_grade PRIVATE /W4 /utf-8 /EHsc)
  set_property(TARGET ai_grade PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  target_compile_options(ai_grade PRIVATE -Wall -Wextra -Wpedantic)
endif()
enable_testing()
add_test(NAME business_self_test COMMAND ai_grade --self-test)
