// vim: sw=3 ts=3 expandtab cindent
#include "event_handle.h"
#include "event_service.h"
#include "bits/exceptions.h"
#include <iostream>
#include <unistd.h>

namespace epolling {

using std::move;
using std::swap;


event_handle::event_handle(event_handle &&other) :
   handle(other.handle),
   trigger(move(other.trigger))
{
   other.handle = -1;
}


event_handle& event_handle::operator =(event_handle &&other) {
   if (this != &other) {
      event_handle{move(other)}.swap(*this);
   }
   return *this;
}


event_handle::~event_handle() noexcept {
   try {
      close();
   }
   catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      std::terminate();
   }
}


int event_handle::read(void *data, std::size_t num_bytes, std::experimental::string_view what) {
   return safe([=] { return ::read(native_handle(), data, num_bytes); }, what);
}


int event_handle::read(void *data, std::size_t num_bytes, std::error_code &ec) noexcept {
   return safe([=] { return ::read(native_handle(), data, num_bytes); }, ec);
}


int event_handle::write(const void *data, std::size_t num_bytes, std::experimental::string_view what) {
   return safe([=] { return ::write(native_handle(), data, num_bytes); }, what);
}


int event_handle::write(const void *data, std::size_t num_bytes, std::error_code &ec) noexcept {
   return safe([=] { return ::write(native_handle(), data, num_bytes); }, ec);
}


void event_handle::open(native_handle_type new_handle, std::experimental::string_view what) {
   if (opened()) {
      close("Failed to close handle during open");
   }
   handle = safe([new_handle] { return new_handle; }, what);
}


void event_handle::open(native_handle_type new_handle, std::error_code &ec) {
   if (opened()) {
      close(ec);
   }
   else {
      ec.clear();
   }

   if (!ec) {
      handle = safe([new_handle] { return new_handle; }, ec);
   }
}


void event_handle::close(std::experimental::string_view what) {
   if (opened()) {
      (void)safe([this] { return ::close(native_handle()); }, what);
      handle = -1;
   }
}


void event_handle::close(std::error_code &ec) noexcept {
   if (opened()) {
      (void)safe([this] { return ::close(native_handle()); }, ec);
      handle = -1;
   }
}

}
