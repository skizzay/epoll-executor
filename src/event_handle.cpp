// vim: sw=3 ts=3 expandtab cindent
#include "event_handle.h"
#include "event_engine.h"
#include <iostream>
#include <system_error>
#include <unistd.h>

namespace {

inline void throw_system_error(const char *what) throw(std::system_error) {
   using std::make_error_code;
   auto error = make_error_code(static_cast<std::errc>(errno));
   throw std::system_error(error, what);
}

}

namespace epolling {

event_handle::event_handle(event_engine &e) :
   engine(e),
   handle(-1)
{
}


event_handle::~event_handle() noexcept {
   try {
      close_handle();
   }
   catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      std::terminate();
   }
}

void event_handle::register_handle(native_handle_type new_handle) {
   if (handle != new_handle) {
      engine.start_monitoring(*this);
      close_handle();
      handle = new_handle;
   }
}


void event_handle::close_handle() {
   if (0 > handle) {
      if (0 > ::close(handle)) {
         throw_system_error("Failed to close handle.");
      }
      engine.stop_monitoring(*this);
      handle = -1;
   }
}

}
