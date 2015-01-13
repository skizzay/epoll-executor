// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EXCEPTIONS_H__
#define EPOLLING_EXCEPTIONS_H__

#include <cerrno>
#include <experimental/string_view>
#include <system_error>
#include <type_traits>

namespace epolling {


namespace details_ {

struct errno_context {
   inline errno_context() noexcept {
      clear();
   }

   inline ~errno_context() noexcept {
      clear();
   }

   inline std::error_code create_error_code() noexcept {
      using std::make_error_code;
      return make_error_code(static_cast<std::errc>(errno));
   }

   inline void clear() noexcept {
      errno = 0;
   }
};

}


template<class F>
inline std::result_of_t<F()> safe(F f, std::experimental::string_view what) {
   details_::errno_context context;
   auto result = f();
   if (0 > result) {
      throw std::system_error(context.create_error_code(), std::string{what});
   }
   return result;
}


template<class F>
inline std::result_of_t<F()> safe(F f, std::error_code &ec) noexcept {
   details_::errno_context context;
   ec.clear();
   int result = -1;
   try {
      result = f();
      if (0 > result) {
         ec = context.create_error_code();
      }
   }
   catch (const std::system_error &e) {
      ec = e.code();
   }
   catch (...) {
      ec = context.create_error_code();
   }
   return result;
}

}

#endif
