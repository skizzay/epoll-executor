// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_HANDLE_H__
#define EPOLLING_EVENT_HANDLE_H__
#include <system_error>

namespace epolling {

class event_engine;

typedef int native_handle_type;

class event_handle {
public:
   virtual ~event_handle() noexcept;

   inline native_handle_type native_handle() const {
      return handle;
   }

protected:
   explicit event_handle(event_engine &e);
   void register_handle(native_handle_type new_handle);
   void close_handle();

private:
   event_engine &engine;
   native_handle_type handle;
};

}

#endif
