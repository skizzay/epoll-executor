// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EXCEPTIONS_H__
#define EPOLLING_EXCEPTIONS_H__

#include <cerrno>
#include <system_error>
#include <type_traits>

namespace epolling {

inline void reset_errno() {
   errno = 0;
}


inline void throw_system_error(const char *what) {
   using std::make_error_code;
   auto error = make_error_code(static_cast<std::errc>(errno));
   reset_errno();
   throw std::system_error(error, what);
}


template<class F>
inline std::result_of_t<F()> safe(F f, const char *what) {
   reset_errno();
   auto result = f();
   if (0 > result) {
      throw_system_error(what);
   }
   return result;
}

}

#endif
