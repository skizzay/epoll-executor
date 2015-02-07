// vim: sw=3 ts=3 expandtab cindent
#include "print_to.h"
#include "event_handle.h"
#include "mode.h"
#include <ostream>
#include <experimental/string_view>
#include <vector>

using std::experimental::string_view;
using std::vector;

namespace epolling {

void PrintTo(const event_handle &handle, std::ostream *os) {
   *os << "event_handle(" << handle.native_handle() << ')';
}


void PrintTo(mode flags, std::ostream *os) {
   vector<string_view> out_flags;

   if ((flags & mode::urgent_read) != mode::none) {
      out_flags.emplace_back("urgent read");
   }
   else if ((flags & mode::read) != mode::none) {
      out_flags.emplace_back("read");
   }

   if ((flags & mode::write) != mode::none) {
      out_flags.emplace_back("write");
   }

   if ((flags & mode::one_time) != mode::none) {
      out_flags.emplace_back("one time");
   }

   if (flags == mode::none) {
      out_flags.emplace_back("none");
   }

   bool first = true;
   *os << "mode(";
   for (auto flag : out_flags) {
      if (!first) {
         *os << '|';
      }
      *os << flag;
      first = false;
   }
   *os << ')';
}

}
