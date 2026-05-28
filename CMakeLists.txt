cmake_minimum_required(VERSION 3.20)

project(ProyectoRiegoBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB BACKEND_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/backend/src/*.cpp"
)

add_executable(riego_backend ${BACKEND_SOURCES})
target_include_directories(riego_backend PRIVATE "${CMAKE_SOURCE_DIR}/backend/include")

if(MSVC)
  target_compile_options(riego_backend PRIVATE /W4)
else()
  target_compile_options(riego_backend PRIVATE -O2 -Wall -Wextra -pthread)
endif()

find_package(Threads REQUIRED)
target_link_libraries(riego_backend PRIVATE Threads::Threads)

if(WIN32)
  target_link_libraries(riego_backend PRIVATE ws2_32)
endif()

set_target_properties(riego_backend PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/backend/bin"
)

if(CMAKE_CONFIGURATION_TYPES)
  foreach(CONFIG_NAME IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${CONFIG_NAME}" CONFIG_NAME_UPPER)
    set_target_properties(riego_backend PROPERTIES
      "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_NAME_UPPER}" "${CMAKE_SOURCE_DIR}/backend/bin"
    )
  endforeach()
endif()
