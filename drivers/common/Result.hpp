#pragma once

#include <variant>

template <typename T, typename E>
struct Result {
    std::variant<T, E> data;

    static Result      success(T val) {
        return {val};
    }
    static Result fail(E err) {
        return {err};
    }

    bool isOk() const {
        return std::holds_alternative<T>(data);
    }

    T& value() const {
        return std::get<T>(data);
    }

    E& error() const {
        return std::get<E>(data);
    }
};