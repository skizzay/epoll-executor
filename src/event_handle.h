// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_HANDLE_H__
#define EPOLLING_EVENT_HANDLE_H__

#include <memory>
#include <system_error>

namespace epolling {

enum class mode {
   read  = 0x01,
   write = 0x02,
   urgent_read = 0x05,
   one_time = 0x08,
   read_write = read | write,
   urgent_read_write = urgent_read | write
};

class event_engine;

typedef int native_handle_type;

class event_handle final {
   class event_trigger {
   public:
      virtual void execute() = 0;
   };

   template<class F>
   class basic_event_trigger final : public event_trigger {
      F callback;

   public:
      explicit inline basic_event_trigger(F &&f) :
         event_trigger(),
         callback(std::forward<F>(f))
      {
      }

      virtual void execute() final override {
         (void)callback();
      }
   };

public:
   event_handle(event_engine &engine, native_handle_type handle, mode flags);
   event_handle(const event_handle &) = delete;
   event_handle(event_handle &&);
   ~event_handle() noexcept;

   event_handle& operator =(const event_handle &) = delete;
   event_handle& operator =(event_handle &&);

   inline native_handle_type native_handle() const {
      return handle;
   }

   inline event_engine & engine() {
      return event_loop;
   }

   int read(void *data, std::size_t num_bytes);
   int write(const void *data, std::size_t num_bytes);
   void close();

   template<class Handler>
   inline void on_read(Handler &&handler) {
      using std::forward;
      read_trigger = std::make_unique<basic_event_trigger<Handler>>(forward<Handler>(handler));
   }
   inline void on_read() {
      execute(read_trigger);
   }

   template<class Handler>
   inline void on_write(Handler &&handler) {
      using std::forward;
      write_trigger = std::make_unique<basic_event_trigger<Handler>>(forward<Handler>(handler));
   }
   inline void on_write() {
      execute(write_trigger);
   }

   template<class Handler>
   inline void on_error(Handler &&handler) {
      using std::forward;
      error_trigger = std::make_unique<basic_event_trigger<Handler>>(forward<Handler>(handler));
   }
   inline void on_error() {
      execute(error_trigger);
   }

private:
   inline void execute(std::unique_ptr<event_trigger> &trigger) {
      if (trigger != nullptr) {
         trigger->execute();
      }
   }
   void stop_monitoring();

   event_engine &event_loop;
   native_handle_type handle;
   std::unique_ptr<event_trigger> read_trigger;
   std::unique_ptr<event_trigger> write_trigger;
   std::unique_ptr<event_trigger> error_trigger;
};

}

#endif
