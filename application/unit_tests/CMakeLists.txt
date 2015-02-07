cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(unit_tests)

find_package(Threads REQUIRED)

add_executable(unit_tests
      main.cpp
      event_engine_tests.cpp
      fake_service.cpp
      fake_service.h
      notification_tests.cpp
      print_to.cpp
      print_to.h
      safe_tests.cpp
      #signal_manager_tests.cpp
   )

include_directories(/usr/src/gmock/include /usr/src/gmock/gtest/include)
target_link_libraries(unit_tests polling ${CMAKE_THREAD_LIBS_INIT} $ENV{HOME}/third-party/gmock/libgmock.a $ENV{HOME}/third-party/gmock/gtest/libgtest.a)

SETUP_TARGET_FOR_COVERAGE(cover_unit_tests
                          ${PROJECT_BINARY_DIR}/unit_tests
                          ${PROJECT_BINARY_DIR}/cover_unit_tests
                         )
