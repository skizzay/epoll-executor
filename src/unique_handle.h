// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_UNIQUE_HANDLE_H__
#define EPOLLING_UNIQUE_HANDLE_H__

#include "handle.h"
#include <tuple>

namespace epolling {

template<class HandleType, class Closer> class unique_handle;

template<class Tag, class Impl, Impl I, class Closer>
class unique_handle<handle<Tag, Impl, I>, Closer> final {
public:
   typedef handle<Tag, Impl, I> handle_type;
   typedef std::decay_t<Closer> closing_type;
   typedef Impl native_handle_type;

   explicit constexpr inline unique_handle(handle_type h) noexcept :
      data(h, closing_type{})
   {
      static_assert(!std::is_pointer<closing_type>::value,
                    "Constructed unique_handle without a closing method.");
   }

   constexpr inline unique_handle(handle_type h,
                                  std::conditional_t<std::is_reference<closing_type>::value, closing_type, const closing_type &> c) noexcept :
      data(h, c)
   {
   }

   constexpr inline unique_handle(handle_type h, std::remove_reference_t<closing_type> &&c) noexcept :
      data(h, c)
   {
      static_assert(!std::is_reference<closing_type>::value,
                    "rvalue closing_type must be bound to reference.");
   }

   constexpr inline unique_handle(unique_handle &&h) noexcept :
      data(h.release(), h.get_deleter())
   {
   }

   unique_handle(const unique_handle &) = delete;
   unique_handle & operator =(const unique_handle &) = delete;

   inline ~unique_handle() noexcept {
      reset();
   }

   inline void reset(handle_type h={}) noexcept {
      using std::get;
      using std::move;

      if (get_handle().valid()) {
         get_closer()(get_handle());
      }

      get<0>(data) = move(h);
   }

   inline handle_type release() noexcept {
      using std::get;

      handle_type h{get_handle()};
      get<1>(data) = handle_type{};

      return h;
   }

   inline const handle_type & get_handle() const noexcept {
      using std::get;

      return get<0>(data);
   }

   inline const closing_type & get_closer() const noexcept {
      using std::get;

      return get<1>(data);
   }

   inline closing_type & get_closer() noexcept {
      using std::get;

      return get<1>(data);
   }

private:
   typedef std::tuple<handle_type, closing_type> storage_type;
   storage_type data;
};

}

#endif
