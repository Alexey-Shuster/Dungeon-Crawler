#pragma once

#include <format>

#include "strong_id.h"

template <typename Tag, typename T>
struct std::formatter<dungeons::common::types::StrongId<Tag, T>> {  // NOLINT

    // Метод parse принимает спецификаторы формата (например, {:d} или {:x})
    // просто возвращаем указатель на конец строки формата
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    // Метод format определяет, КАК именно объект превратится в строку
    auto format(const dungeons::common::types::StrongId<Tag, T>& id, std::format_context& ctx) const {
        // внутреннее значение форматируем как обычное число
        return std::format_to(ctx.out(), "{}", id.get());
    }
};
