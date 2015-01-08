// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_HANDLE_H__
#define EPOLLING_EVENT_HANDLE_H__

#include <memory>
#include <system_error>

namespace epolling {

enum class mode : int {
   none  = 0x00,
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
   explicit event_handle(native_handle_type handle);
   event_handle(const event_handle &) = delete;
   event_handle(event_handle &&);
   ~event_handle() noexcept;

   event_handle& operator =(const event_handle &) = delete;
   event_handle& operator =(event_handle &&);

   inline native_handle_type native_handle() const {
      return handle;
   }

   inline std::shared_ptr<event_engine> engine() const {
      return event_loop.lock();
   }

   void add_to(event_engine &e);

   inline bool opened() const {
      return !closed();
   }
   inline bool closed() const {
      return 0 > native_handle();
   }

   int read(void *data, std::size_t num_bytes);
   int write(const void *data, std::size_t num_bytes);
   void close();

   template<class Handler>
   inline void on_read(Handler &&handler, bool urgent=false) {
      using std::forward;
      using std::make_unique;
      read_trigger = make_unique<basic_event_trigger<Handler>>(forward<Handler>(handler));
      reinterpret_cast<int&>(mode_flags) |= static_cast<int>(urgent ? mode::urgent_read : mode::read);
      update_monitoring();
   }
   inline void clear_read() {
      clear_trigger(read_trigger, mode::urgent_read);
   }
   inline void on_read() {
      execute(read_trigger);
   }

   template<class Handler>
   inline void on_write(Handler &&handler) {
      using std::forward;
      using std::make_unique;
      write_trigger = make_unique<basic_event_trigger<Handler>>(forward<Handler>(handler));
      reinterpret_cast<int&>(mode_flags) |= static_cast<int>(mode::write);
      update_monitoring();
   }
   inline void clear_write() {
      clear_trigger(write_trigger, mode::write);
   }
   inline void on_write() {
      execute(write_trigger);
   }

   template<class Handler>
   inline void on_error(Handler &&handler) {
      using std::forward;
      using std::make_unique;
      error_trigger = make_unique<basic_event_trigger<Handler>>(forward<Handler>(handler));
      update_monitoring();
   }
   inline void clear_error() {
      clear_trigger(error_trigger, mode{});
   }
   inline void on_error() {
      execute(error_trigger);
   }

   inline mode flags() const {
      return mode_flags;
   }

   inline void swap(event_handle &other) {
      using std::swap;

      swap(handle, other.handle);
      swap(mode_flags, other.mode_flags);
      swap(read_trigger, other.read_trigger);
      swap(write_trigger, other.write_trigger);
      swap(error_trigger, other.error_trigger);
      swap(event_loop, other.event_loop);
   }

private:
   static inline void execute(std::unique_ptr<event_trigger> &trigger) {
      if (trigger != nullptr) {
         trigger->execute();
      }
   }
   inline void clear_trigger(std::unique_ptr<event_trigger> &trigger, mode mask) {
      if (trigger != nullptr) {
         reinterpret_cast<int&>(mode_flags) &= ~static_cast<int>(mask);
         update_monitoring();
         trigger.reset();
      }
   }
   void stop_monitoring();
   void update_monitoring();

   native_handle_type handle;
   mode mode_flags;
   std::unique_ptr<event_trigger> read_trigger;
   std::unique_ptr<event_trigger> write_trigger;
   std::unique_ptr<event_trigger> error_trigger;
   std::weak_ptr<event_engine> event_loop;
};

}

#endif
