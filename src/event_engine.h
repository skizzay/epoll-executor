// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_ENGINE_H__
#define EPOLLING_EVENT_ENGINE_H__

#include "activation.h"
#include <atomic>
#include <chrono>
#include <experimental/executor>
#include <memory>

namespace epolling {

class notification;
class signal_manager;
enum class mode : int;


template<class EventService, template<class> class Delete=std::default_delete>
class event_engine : public std::experimental::execution_context,
                     public std::enable_shared_from_this<event_engine<EventService, Delete>> {
   friend class signal_manager;

public:
   using duration_type = std::chrono::nanoseconds;
   using time_point = std::chrono::steady_clock::time_point;

   explicit event_engine(std::size_t mepp);
   event_engine() = delete;

   virtual ~event_engine() noexcept final override;

   template<class DurationType=duration_type>
   inline std::error_code run(DurationType timeout=DurationType{-1});

   template<class DurationType=duration_type>
   inline bool poll(DurationType timeout=DurationType{-1});

   template<class DurationType=duration_type>
   inline bool poll_one(DurationType timeout=DurationType{-1});

   inline void stop(std::error_code reason);

   template<class T, void (T::*OnActivation)(mode), class Tag, class Impl, Impl Invalid, class U>
   inline void start_monitoring(const handle<Tag, Impl, Invalid> &h, mode flags, U &object);

   template<class Tag, class Impl, Impl Invalid>
   inline void update_monitoring(const handle<Tag, Impl, Invalid> &h, mode flags);

   template<class Tag, class Impl, Impl Invalid>
   inline void stop_monitoring(handle<Tag, Impl, Invalid> &h);

   inline void quit();
   inline bool running() const;
   inline const time_point &time() const;

private:
   void set_signal_manager(const signal_manager *manager);
   void do_stop(EventService *srvc, std::error_code reason);
   void wait_until_not_running();

   template<class DurationType>
   bool do_poll(EventService *srvc, std::size_t max_events_to_poll, DurationType timeout);

   template<class DurationType>
   void do_run(EventService *srvc, DurationType timeout);

   std::error_code stop_reason;
   std::atomic<EventService*> service;
   std::unique_ptr<notification, Delete<notification>> wakeup;
   const std::size_t max_events_per_poll;
   std::atomic<bool> exit_flag;
   std::atomic<bool> quitting;
   std::atomic<std::ptrdiff_t> execution_count;
   time_point cached_now;
};

}


#include "event_engine.hpp"

#endif
