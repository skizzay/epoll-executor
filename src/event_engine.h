// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_ENGINE_H__
#define EPOLLING_EVENT_ENGINE_H__

#include <experimental/executor>

namespace epolling {

class event_handle;

class event_engine : public std::experimental::execution_context {
public:
   void start_monitoring(event_handle &handle) {
      (void)handle;
   }

   void stop_monitoring(event_handle &handle) {
      (void)handle;
   }
};

}

#endif
