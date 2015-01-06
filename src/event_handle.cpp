// vim: sw=3 ts=3 expandtab cindent
#include "event_handle.h"
#include "event_engine.h"
#include "bits/exceptions.h"
#include <iostream>
#include <unistd.h>

using std::swap;


namespace epolling {

event_handle::event_handle(event_engine &e, native_handle_type h, mode flags) :
   event_loop(e),
   handle(safe([handle=h] { return handle; }, "Cannot register invalid handle."))
{
   e.start_monitoring(*this, flags);
}


event_handle::event_handle(event_handle &&other) :
   event_loop(other.engine()),
   handle(-1)
{
   swap(handle, other.handle);
}


event_handle& event_handle::operator =(event_handle &&other) {
   if (this != &other) {
      close();
      swap(handle, other.handle);
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


int event_handle::read(void *data, std::size_t num_bytes) {
   return safe([=] { return ::read(native_handle(), data, num_bytes); },
               "Failed to read from handle.");
}


int event_handle::write(const void *data, std::size_t num_bytes) {
   return safe([=] { return ::write(native_handle(), data, num_bytes); },
               "Failed to write to handle.");
}


void event_handle::close() {
   if (0 < native_handle()) {
      stop_monitoring();
      (void)safe([this] { return ::close(native_handle()); }, "Failed to close handle.");
      handle = -1;
   }
}


void event_handle::stop_monitoring() {
   engine().stop_monitoring(*this);
}

}
