cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(polling)

add_library(polling STATIC
      epoll_service.cpp
      epoll_service.h
      event_handle.cpp
      event_handle.h
      event_engine.cpp
      event_engine.h
      signal_manager.cpp
      signal_manager.h
   )
