// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_HANDLE_H__
#define EPOLLING_HANDLE_H__

#include <utility>

namespace epolling {

#if EPOLLING_IS_LINUX
typedef int native_handle_type;
template<class Tag, class Impl=int, Impl Invalid=Impl{-1}> class handle;
#endif

template<class T, class I, I i>
constexpr inline void swap(handle<T, I, i>& l, handle<T, I, i> &r) noexcept;

template<class Tag, class Impl, Impl Invalid>
class handle final {
   Impl value;

public:
   constexpr inline handle() noexcept :
      handle(Invalid)
   {
   }

   constexpr inline handle(Impl v) noexcept :
      value(v)
   {
   }

   inline handle(handle &&other) noexcept :
      value(other.value)
   {
      other.value = Invalid;
   }
   inline handle& operator =(handle &&other) noexcept {
      using std::move;

      if (this != &other) {
         value = move(other.value);
         other.value = Invalid;
      }
      return *this;
   }

   inline handle(const handle &) noexcept = default;
   inline handle& operator =(const handle &) noexcept = default;

   constexpr inline operator Impl() const noexcept {
      return value;
   }

   constexpr inline void swap(handle &r) noexcept {
      using std::swap;

      swap(value, r.value);
   }

   constexpr inline bool valid() const noexcept {
      return value != Invalid;
   }
};


template<class T, class I, I i>
constexpr inline void swap(handle<T, I, i>& l, handle<T, I, i> &r) noexcept {
   l.swap(r);
}


template<class T, class I, I i>
constexpr inline bool operator==(const handle<T, I, i> &l, const handle<T, I, i> &r) noexcept {
   return static_cast<I>(l) == static_cast<I>(r);
}


template<class T, class I, I i>
constexpr inline bool operator!=(const handle<T, I, i> &l, const handle<T, I, i> &r) noexcept {
   return static_cast<I>(l) != static_cast<I>(r);
}


template<class T, class I, I i>
constexpr inline bool operator< (const handle<T, I, i> &l, const handle<T, I, i> &r) noexcept {
   return static_cast<I>(l) < static_cast<I>(r);
}


template<class T, class I, I i>
constexpr inline bool operator<=(const handle<T, I, i> &l, const handle<T, I, i> &r) noexcept {
   return static_cast<I>(l) <= static_cast<I>(r);
}


template<class T, class I, I i>
constexpr inline bool operator> (const handle<T, I, i> &l, const handle<T, I, i> &r) noexcept {
   return static_cast<I>(l) > static_cast<I>(r);
}


template<class T, class I, I i>
constexpr inline bool operator>=(const handle<T, I, i> &l, const handle<T, I, i> &r) noexcept {
   return static_cast<I>(l) >= static_cast<I>(r);
}

}

#endif
