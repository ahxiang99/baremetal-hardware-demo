#pragma once

#include "IUart.hpp"

class UartBase : public IUart {
   protected:
    UartState tx_state_;
    UartState rx_state_;
    UartError error_;

   public:
    UartBase() : tx_state_(UartState::Reset), rx_state_(UartState::Reset), error_(UartError::None) {}

    UartError getError() const {
        return error_;
    }

   protected:
    void SetError(UartError&& error) {
        error_ = error;
    }
};