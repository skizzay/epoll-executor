// vim: sw=3 ts=3 expandtab cindent
#include "event_service.h"

namespace epolling {

event_service::event_service(std::experimental::execution_context &e) :
   std::experimental::execution_context::service(e)
{
}

}
