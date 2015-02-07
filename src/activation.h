// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_ACTIVATION_H__
#define EPOLLING_ACTIVATION_H__

#include "handle.h"
#include <cassert>
#include <map>
#include <mutex>

namespace epolling {

template<class Signature> struct basic_activation;
template<class Signature, class NativeHandleType, class Table=std::map<NativeHandleType, basic_activation<Signature>>, class Mutex=std::mutex> class basic_activation_store;

template<class R, class ... Args>
struct basic_activation<R(Args...)> final {
   typedef R (*callback_type)(void *, Args...);
   typedef R result_type;

   callback_type callback;
   void *object;

   inline R execute(Args && ... args) {
      using std::forward;

      assert(callback != nullptr);
      return (*callback)(object, forward<Args>(args)...);
   }
};


template<class R, class ... Args, class NativeHandleType, class Table, class Mutex>
class basic_activation_store<R(Args...), NativeHandleType, Table, Mutex> final {
public:
   using activation_type = basic_activation<R(Args...)>;
   using callback_type = typename activation_type::callback_type;
   using size_type = typename Table::size_type;

   template<class T, R (T::*OnActivation)(Args...), class U>
   static inline activation_type create_activation(U &object) noexcept {
      struct trampoline final {
         static inline R jump(void *pointer, Args ... args) {
            assert(pointer != nullptr);
            U *object = static_cast<U*>(pointer);
            return (object->*OnActivation)(args...);
         }
      };

      return create_activation<&trampoline::jump>(&object);
   }

   template<callback_type OnActivation>
   static inline activation_type create_activation(void *object=nullptr) noexcept {
      return {OnActivation, object};
   }

   // NOTE: An activation must be associated for h.  Otherwise, behavior is undefined.
   template<class Tag, NativeHandleType Invalid>
   inline activation_type& get(const handle<Tag, NativeHandleType, Invalid> &h) {
      std::lock_guard<Mutex> l{m};
      return activations[h];
   }

   template<class Tag, NativeHandleType Invalid>
   inline void deactivate(const handle<Tag, NativeHandleType, Invalid> &h) {
      std:: lock_guard<Mutex> l{m};
      (void)activations.erase(h);
      (void)l;
   }

   constexpr inline size_type size() const {
      return activations.size();
   }

   constexpr inline size_type max_size() const {
      return activations.max_size();
   }

private:
   Mutex m;
   Table activations;
};

}

#endif
