cmake_minimum_required(VERSION 3.16)
project(school_manual LANGUAGES CXX)
add_executable(school_manual main.cpp)
set_target_properties(school_manual PROPERTIES
  OUTPUT_NAME manual_grade
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS OFF
)
target_compile_features(school_manual PRIVATE cxx_std_17)
if(WIN32)
  target_link_libraries(school_manual PRIVATE shell32)
endif()
if(MSVC)
  target_compile_options(school_manual PRIVATE /utf-8 /W4)
else()
  target_compile_options(school_manual PRIVATE -Wall -Wextra -Wpedantic)
endif()
enable_testing()
add_test(NAME business_self_test COMMAND school_manual --self-test)
