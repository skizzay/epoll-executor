// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_PRINT_TO_H__
#define EPOLLING_PRINT_TO_H__

#include "handle.h"
#include <ostream>

namespace epolling {

class event_handle;
enum class mode : int;

template<class Tag, class Impl, Impl Invalid>
void PrintTo(const handle<Tag, Impl, Invalid> &h, std::ostream *os) {
   *os << "handle{" << static_cast<Impl>(h) << '}';
}

void PrintTo(mode flags, std::ostream *os);

}

#endif
