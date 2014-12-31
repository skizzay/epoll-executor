/*vim:sw=3:ts=3:expandtab:cindent*/
#include "epoll_service.h"
#include "event_engine.h"
#include <iostream>
#include <system_error>
#include <sys/epoll.h>
#include <unistd.h>

using std::make_error_code;

namespace {

inline void throw_system_error(const char *what) {
   auto error = make_error_code(static_cast<std::errc>(errno));
   throw std::system_error(error, what);
}

}


namespace epolling {

epoll_service::epoll_service(event_engine &engine) :
   std::experimental::execution_context::service(engine),
   epoll_fd(::epoll_create1(EPOLL_CLOEXEC))
{
   if (epoll_fd < 0) {
      throw_system_error("Failed to create epoll file descriptor.");
   }
}


epoll_service::~epoll_service() noexcept {
   try {
      shutdown_service();
   }
   catch (const std::exception &e) {
      std::cerr << "Failed to shutdown service. " << e.what() << std::endl;
      abort();
   }
}


void epoll_service::shutdown_service() {
   if (epoll_fd > 0) {
      if (::close(epoll_fd) == 0) {
         epoll_fd = -1;
      }
      else {
         throw_system_error("Failed to close epoll file descriptor.");
      }
   }
}

}
