// vim: sw=3 ts=3 expandtab cindent
#include "notification.h"
#include "event_engine.h"
#include "bits/exceptions.h"
#include <limits>
#include <unistd.h>
#include <sys/eventfd.h>

using std::get;
using std::system_error;

namespace epolling {

void notification::set(uint64_t value) {
#if 0
   auto error = std::get<std::error_code>(handle.write(&value, sizeof(value)));
   if (error) {
      throw system_error(error, "Failed to write notification to file descriptor");
   }
#else
   ::write(event_fd.get_handle(), &value, sizeof(value));
#endif
}


native_handle_type notification::create_native_handle(behavior b, uint64_t initial_value) {
#if EPOLLING_IS_LINUX
   return safe([=] {
         return ::eventfd(static_cast<int>(initial_value),
                          EFD_CLOEXEC | EFD_NONBLOCK | (b == behavior::semaphore ? EFD_SEMAPHORE : 0));
      }, "Failed to create event file descriptor.");
#endif
}


void notification::on_activation(mode activation_flags) {
   (void) activation_flags;
   ::read(event_fd.get_handle(), &last_read_value, sizeof(last_read_value));
}

}
