// vim: sw=3 ts=3 expandtab cindent
#include "notification.h"
#include "event_engine.h"
#include "bits/exceptions.h"
#include <limits>
#include <unistd.h>
#include <sys/eventfd.h>

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
   handle.open(create_native_handle(how_to_behave, initial_value), "Failed to create eventfd");
   e.start_monitoring(handle, mode::urgent_read);
}


void notification::set(uint64_t value) {
   (void)handle.write(&value, sizeof(value), "Failed to set event notification");
}

}
