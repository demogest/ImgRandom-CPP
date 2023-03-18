cmake_minimum_required(VERSION 3.18)
project(cppRandom)

set(CMAKE_CXX_STANDARD 14)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(nlohmann-json_IMPLICIT_CONVERSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(Boost COMPONENTS system REQUIRED filesystem REQUIRED)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif ()
add_executable(cppRandom main.cpp)

target_link_libraries(cppRandom ${Boost_LIBRARIES} Crow::Crow nlohmann_json::nlohmann_json)