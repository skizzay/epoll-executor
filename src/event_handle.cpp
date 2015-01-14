// vim: sw=3 ts=3 expandtab cindent
#include "event_handle.h"
#include "event_service.h"
#include "bits/exceptions.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

using std::cerr;
using std::error_code;
using std::exception;
using std::endl;
using std::make_tuple;
using std::move;
using std::size_t;
using std::experimental::string_view;
using std::swap;
using std::terminate;
using std::tuple;

namespace {

using namespace epolling;

inline int get_flags(native_handle_type handle, error_code &ec) {
   return safe([handle] { return ::fcntl(handle, F_GETFL); }, ec);
}

inline void set_flags(native_handle_type handle, int flags, error_code &ec) {
   (void)safe([handle, flags] { return ::fcntl(handle, F_SETFL, flags); }, ec);
}

inline bool is_blocking_flag_set(int flags) {
   return 0 == (flags & O_NONBLOCK);
}

}


namespace epolling {

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
   catch (const exception &e) {
      cerr << e.what() << endl;
      terminate();
   }
}


tuple<size_t, error_code> event_handle::bytes_available() const noexcept {
   size_t num_bytes = 0;
   error_code error;
   (void)safe([this, &num_bytes] { return ::ioctl(native_handle(), FIONREAD, &num_bytes); }, error);
   return make_tuple(num_bytes, error);
}


error_code event_handle::make_blocking() noexcept {
   error_code error;
   int flags = get_flags(native_handle(), error);
   if (!error && !is_blocking_flag_set(flags)) {
      set_flags(native_handle(), flags & (~O_NONBLOCK), error);
   }
   return error;
}


error_code event_handle::make_non_blocking() noexcept {
   error_code error;
   int flags = get_flags(native_handle(), error);
   if (!error && is_blocking_flag_set(flags)) {
      set_flags(native_handle(), flags | O_NONBLOCK, error);
   }
   return error;
}


tuple<bool, error_code> event_handle::is_blocking() const noexcept {
   error_code error;
   bool result = is_blocking_flag_set(get_flags(native_handle(), error));
   return make_tuple(result, error);
}


tuple<int, error_code> event_handle::read(void *data, size_t num_bytes) noexcept {
   error_code error;
   int result = safe([=] { return ::read(native_handle(), data, num_bytes); }, error);
   return make_tuple(result, error);
}


tuple<int, error_code> event_handle::write(const void *data, size_t num_bytes) noexcept {
   error_code error;
   int result = safe([=] { return ::write(native_handle(), data, num_bytes); }, error);
   return make_tuple(result, error);
}


error_code event_handle::open(native_handle_type new_handle) noexcept {
   error_code error;
   if (opened()) {
      error = close();
   }

   if (!error) {
      handle = safe([new_handle] { return new_handle; }, error);
   }

   return error;
}


error_code event_handle::close() noexcept {
   error_code error;
   if (opened()) {
      (void)safe([this] { return ::close(native_handle()); }, error);
      handle = -1;
   }
   return error;
}

}
