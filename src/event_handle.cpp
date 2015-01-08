// vim: sw=3 ts=3 expandtab cindent
#include "event_handle.h"
#include "event_engine.h"
#include "event_service.h"
#include "bits/exceptions.h"
#include <iostream>
#include <unistd.h>

namespace epolling {

using std::move;
using std::swap;


event_handle::event_handle(native_handle_type h) :
   handle(h),
   mode_flags(mode::none)
{
}


event_handle::event_handle(event_handle &&other) :
   handle(other.handle),
   mode_flags(other.mode_flags),
   read_trigger(move(other.read_trigger)),
   write_trigger(move(other.write_trigger)),
   error_trigger(move(other.error_trigger)),
   event_loop(move(other.event_loop))
{
   other.handle = -1;
}


event_handle& event_handle::operator =(event_handle &&other) {
   if (this != &other) {
      event_handle temp{move(other)};
      swap(temp);
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


void event_handle::add_to(event_engine &e) {
   if (opened() && (engine() != e.shared_from_this())) {
      stop_monitoring();
      event_loop = e.shared_from_this();
      e.service->start_monitoring(*this);
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
   if (opened()) {
      stop_monitoring();
      (void)safe([this] { return ::close(native_handle()); }, "Failed to close handle.");
      handle = -1;
   }
}


void event_handle::stop_monitoring() {
   auto e = engine();
   if (e != nullptr) {
      e->service->stop_monitoring(*this);
   }
}


void event_handle::update_monitoring() {
   auto e = engine();
   if (e != nullptr) {
      e->service->update_monitoring(*this);
   }
}

}
