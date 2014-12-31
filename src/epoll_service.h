/*vim:sw=3:ts=3:expandtab:cindent*/
#ifndef EPOLLING_EPOLL_SERVICE_H__
#define EPOLLING_EPOLL_SERVICE_H__

#include <experimental/executor>

namespace epolling {

class event_engine;

typedef int native_handle_type;

class epoll_service : public std::experimental::execution_context::service {
public:
   explicit epoll_service(event_engine &engine);
   virtual ~epoll_service() noexcept;

private:
   virtual void shutdown_service();

   native_handle_type epoll_fd;
};

}

#endif
