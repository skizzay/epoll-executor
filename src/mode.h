// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_MODE_H__
#define EPOLLING_MODE_H__

namespace epolling {

enum class mode : int {
   none  = 0x00,
   read  = 0x01,
   write = 0x02,
   urgent_read = 0x05,
   one_time = 0x08,
   read_write = read | write,
   urgent_read_write = urgent_read | write
};


inline constexpr mode operator |(mode l, mode r) {
   return static_cast<mode>(static_cast<int>(l) | static_cast<int>(r));
}


inline constexpr mode operator &(mode l, mode r) {
   return static_cast<mode>(static_cast<int>(l) & static_cast<int>(r));
}


inline constexpr mode operator ^(mode l, mode r) {
   return static_cast<mode>(static_cast<int>(l) ^ static_cast<int>(r));
}


inline constexpr mode operator ~(mode m) {
   return static_cast<mode>(~static_cast<int>(m));
}

}

#endif
