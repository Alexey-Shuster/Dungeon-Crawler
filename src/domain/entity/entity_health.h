#pragma once

#include <cstdint> // IWYU pragma: keep // uint32_t

namespace entity {

using HP = uint32_t;

class Health {
public:
    constexpr explicit Health(HP max_hp) : hp_{max_hp} {}

    void operator-=(HP hp) {
        hp_ < hp ? hp_ = 0 : hp_ -= hp;
    }

    operator bool() const noexcept {
        return hp_ != 0;
    }

    HP value() const noexcept {
        return hp_;
    }

private:
    HP hp_;
};

} // namespace entity
