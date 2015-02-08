// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_SIGNAL_HANDLE_H__
#define EPOLLING_SIGNAL_HANDLE_H__

#include "handle.h"
#include <sys/signalfd.h>

namespace epolling {

struct signal_tag;
typedef handle<signal_tag, int, 0> signal_handle;

struct signal_tag {
   constexpr static inline signal_handle get_handle(const ::signalfd_siginfo &siginfo) {
      return {siginfo.ssi_errno};
   }
};

}

#endif
