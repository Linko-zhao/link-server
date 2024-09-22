#ifndef __LINKO_NONCOPYABLE_H__
#define __LINKO_NONCOPYABLE_H__

namespace linko {

class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

}

#endif
