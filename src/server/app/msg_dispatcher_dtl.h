#pragma once

#include <memory>
#include <optional>
#include <server/domain/core/events.h>
#include <utility>

#include "common/wire/serder.h"

namespace dungeons::server::app::detail {

/**
 * @brief Безопасное приведение числового значения к целевому типу или enum.
 *
 * Если Target является enum, проверяет вхождение value в диапазон его базового типа.
 * Для обычных числовых типов выполняет прямое приведение.
 * @return std::optional с результатом или std::nullopt, если значение вне диапазона.
 */
template <typename Target, typename Source>
constexpr std::optional<Target> safeArgumentCast(Source value) {
    // Обработка Enum Class
    if constexpr (std::is_enum_v<Target>) {
        using Underlying = std::underlying_type_t<Target>;

        // Проверяем текущее значение uint64_t на диапазон базового типа энума
        if (!std::in_range<Underlying>(value)) {
            return std::nullopt;
        }
        auto underlying_value = static_cast<Underlying>(value);

        return static_cast<Target>(underlying_value);
    }
    // Обработка стандартных числовых типов (int, uint32_t и т.д.)
    else {
        return static_cast<Target>(value);
    }
}

///@brief Универсальный шаблон для создания событий с проверкой аргументов
template <typename EventType, typename... ArgTypes>
std::shared_ptr<core::Event> createEvent(const common::wire::MessageArgs& args) {
    constexpr size_t ExpectedSize = sizeof...(ArgTypes);

    if (args.size() != ExpectedSize) {
        return nullptr;
    }

    return [&]<size_t... Is>(std::index_sequence<Is...>) -> std::shared_ptr<core::Event> {
        // Получаем пакет из std::optional<ArgTypes>...
        auto casted_args = std::make_tuple(safeArgumentCast<ArgTypes>((args)[Is])...);

        // Проверяем, что ВСЕ optional внутри кортежа содержат значения.
        // Используем свертку && для std::get<Is>(casted_args).has_value()
        bool all_valid = (std::get<Is>(casted_args).has_value() && ...);

        if (!all_valid) {
            return nullptr;
        }

        // Если всё валидно, конструируем событие, доставая значения через оператор *
        return std::make_shared<EventType>((*std::get<Is>(casted_args))...);
    }(std::make_index_sequence<ExpectedSize>{});
}

}  // namespace dungeons::server::app::detail
