// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_PRINT_TO_H__
#define EPOLLING_PRINT_TO_H__

#include <iosfwd>

namespace epolling {

class event_handle;
enum class mode : int;

void PrintTo(const event_handle &handle, std::ostream *os);
void PrintTo(mode flags, std::ostream *os);

}

#endif
