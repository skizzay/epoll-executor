cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(polling)

add_library(polling STATIC
      activation.h
      epoll_service.cpp
      epoll_service.h
      event_engine.h
      handle.h
      mode.h
      notification.cpp
      notification.h
      #signal_manager.cpp
      #signal_manager.h
      unique_handle.h
      bits/exceptions.h
   )
