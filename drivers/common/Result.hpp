#pragma once

#include <variant>

struct Unit {};  // or use std::monostate

enum class Err : uint8_t {
    None,
    InvalidParam,
    Timeout,
    Busy,
    HwFault,
    NotInitialized,
    NullInstance,

    /* Gpio Related */
    InvalidPort,
    InvalidPinMask,
    AfOnNonAltFn
};

template <typename T = Unit, typename E = Err>
struct Result {
   private:
    struct Ok {
        T v;
    };
    struct Err {
        E e;
    };
    std::variant<Ok, Err> data;

    explicit Result(Ok o) : data(std::move(o)) {}
    explicit Result(Err e) : data(std::move(e)) {}

   public:
    static Result success(T val) {
        return Result{Ok{std::move(val)}};
    }
    static Result fail(E err) {
        return Result{Err{std::move(err)}};
    }

    bool isOk() const {
        return std::holds_alternative<Ok>(data);
    }

    T& value() {
        return std::get<Ok>(data).v;
    }
    const T& value() const {
        return std::get<Ok>(data).v;
    }
    E& error() {
        return std::get<Err>(data).e;
    }
    const E& error() const {
        return std::get<Err>(data).e;
    }
};

inline Result<> Ok() {
    return Result<>::success(Unit{});
}
inline Result<> Fail(Err e) {
    return Result<>::fail(e);
}

#define TRY(expr)                                    \
    do {                                             \
        if (auto r_ = (expr); !r_.isOk()) return r_; \
    } while (0)
