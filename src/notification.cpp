// vim: sw=3 ts=3 expandtab cindent
#include "notification.h"
#include "event_engine.h"
#include "bits/exceptions.h"
#include <limits>
#include <unistd.h>
#include <sys/eventfd.h>

using std::get;
using std::system_error;

namespace {

inline auto create_native_handle(epolling::notification::behavior behavior, uint64_t initial_value) {
   return ::eventfd(static_cast<int>(initial_value),
                    EFD_CLOEXEC | EFD_NONBLOCK | (behavior == epolling::notification::behavior::semaphore ? EFD_SEMAPHORE : 0));
}

}

namespace epolling {

notification::notification(event_engine &e, behavior how_to_behave, uint64_t initial_value) :
   last_read_value(std::numeric_limits<uint64_t>::max()),
   handle([=] (auto) { handle.read(&last_read_value, sizeof(last_read_value)); })
{
   auto error = handle.open(create_native_handle(how_to_behave, initial_value));
   if (error) {
      throw system_error(error, "Failed to create event file descriptor");
   }
   else {
      e.start_monitoring(handle, mode::urgent_read);
   }
}


void notification::set(uint64_t value) {
   auto error = std::get<std::error_code>(handle.write(&value, sizeof(value)));
   if (error) {
      throw system_error(error, "Failed to write notification to file descriptor");
   }
}

}
