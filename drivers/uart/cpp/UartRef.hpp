#pragma once

#include "UartConcepts.hpp"
#include "low-level/uart_registers.h"

struct UartRef {
    void* obj{nullptr};
    bool (*send_fn)(void*, const uint8_t*, size_t){nullptr};
    bool (*receive_fn)(void*, uint8_t*, size_t){nullptr};
    USART_TypeDef* (*get_fn)(void*);

    template <UartDevice Uart>
    static UartRef from(Uart& uart) {
        return {&uart, [](void* ctx, const uint8_t* pData, size_t len) { return static_cast<Uart*>(ctx)->send(pData, len); },
                [](void* ctx, uint8_t* pData, size_t len) { return static_cast<Uart*>(ctx)->receive(pData, len); }, [](void* ctx) { return static_cast<Uart*>(ctx)->rawInstance(); }};
    }

    bool send(const uint8_t* data, size_t len) {
        return send_fn ? send_fn(obj, data, len) : false;
    }

    bool receive(uint8_t* buf, size_t len) {
        return receive_fn ? receive_fn(obj, buf, len) : false;
    }

    USART_TypeDef* rawInstance() const {
        return get_fn ? get_fn(obj) : nullptr;
    }
};