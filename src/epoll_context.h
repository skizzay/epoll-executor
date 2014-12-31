/* vim: sw=3 ts=3 expandtab cindent */
#ifndef EPOLLING_EPOLL_CONTEXT_H__
#define EPOLLING_EPOLL_CONTEXT_H__

#include <experimental/executor>

namespace epolling {

namespace executors = std::experimental;

class epoll_service : public executors::execution_context:service {
public:
   virtual ~epoll_service() = default;

private:
   virtual void shutdown_service();
};

}

#endif
