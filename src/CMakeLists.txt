# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

project(flutter_sarec_library VERSION 0.0.1)

add_library(flutter_sarec SHARED
  "flutter_sarec.cc"
)

set_target_properties(flutter_sarec PROPERTIES
  PUBLIC_HEADER flutter_sarec.h
  OUTPUT_NAME "flutter_sarec"
)

target_compile_definitions(flutter_sarec PUBLIC DART_SHARED_LIB)
