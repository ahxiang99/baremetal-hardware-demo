#pragma once

#include <concepts>
#include <cstdint>

class II2C {
   public:
    virtual ~II2C()                                                                                = default;
    virtual bool initialize()                                                                      = 0;
    virtual bool Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) = 0;
    virtual bool Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout)        = 0;
    virtual void handleEVInterrupt()                                                               = 0;
    virtual void handleERInterrupt()                                                               = 0;
};

template <typename T>
concept I2C_Device = requires(T s, uint16_t DevAddress, const uint8_t* pData, uint8_t* pData1, uint16_t len, uint32_t Timeout) {
    { s.Write(DevAddress, pData, len, Timeout) } -> std::convertible_to<bool>;
    { s.Read(DevAddress, pData1, len, Timeout) } -> std::convertible_to<bool>;
};

struct I2C_Ref {
    using i2c_write_fn     = bool (*)(void* ctx, uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout);
    using i2c_read_fn      = bool (*)(void* ctx, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);
    using i2c_mem_read_fn  = bool (*)(void* ctx, uint16_t DevAddress, uint8_t MemAddr, uint8_t* pData, uint16_t Size, uint32_t Timeout);
    using i2c_mem_write_fn = bool (*)(void* ctx, uint16_t DevAddress, uint8_t MemAddr, uint8_t* pData, uint16_t Size, uint32_t Timeout);

    void*            ctx{nullptr};
    i2c_write_fn     write_fn{nullptr};
    i2c_read_fn      read_fn{nullptr};
    i2c_mem_read_fn  mem_read_fn{nullptr};
    i2c_mem_write_fn mem_write_fn{nullptr};

   public:
    template <I2C_Device T>
    static I2C_Ref from(T& dev) {
        I2C_Ref ref;
        ref.ctx      = &dev;
        ref.write_fn = [](void* ctx, uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) { return static_cast<T*>(ctx)->Write(DevAddress, pData, Size, Timeout); };
        ref.read_fn  = [](void* ctx, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) { return static_cast<T*>(ctx)->Read(DevAddress, pData, Size, Timeout); };
        if constexpr (requires(T& t, uint16_t a, uint8_t b, uint8_t* c, uint16_t d, uint32_t e) {
                          { t.MemRead(a, b, c, d, e) } -> std::convertible_to<bool>;
                      }) {
            ref.mem_read_fn = [](void* ctx, uint16_t DevAddress, uint8_t MemAddr, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
                return static_cast<T*>(ctx)->MemRead(DevAddress, MemAddr, pData, Size, Timeout);
            };
        }
        if constexpr (requires(T& t, uint16_t a, uint8_t b, uint8_t* c, uint16_t d, uint32_t e) {
                          { t.MemWrite(a, b, c, d, e) } -> std::convertible_to<bool>;
                      }) {
            ref.mem_write_fn = [](void* ctx, uint16_t DevAddress, uint8_t MemAddr, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
                return static_cast<T*>(ctx)->MemWrite(DevAddress, MemAddr, pData, Size, Timeout);
            };
        }
        return ref;
    }

    bool Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) {
        return write_fn ? write_fn(ctx, DevAddress, pData, Size, Timeout) : false;
    }

    bool Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
        return read_fn ? read_fn(ctx, DevAddress, pData, Size, Timeout) : false;
    }

    bool MemRead(uint16_t DevAddress, uint8_t MemAddr, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
        return mem_read_fn ? mem_read_fn(ctx, DevAddress, MemAddr, pData, Size, Timeout) : false;
    }

    bool MemWrite(uint16_t DevAddress, uint8_t MemAddr, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
        return mem_write_fn ? mem_write_fn(ctx, DevAddress, MemAddr, pData, Size, Timeout) : false;
    }
};

struct I2CCommand {
    void (*fn)(void* ctx);  // function to call
    void* ctx;              // sensor object (this pointer)
};