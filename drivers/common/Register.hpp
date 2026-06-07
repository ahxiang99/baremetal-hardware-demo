#pragma once
#include <cstdint>

// ============================================================
//  Register<addr, T> — a zero-cost hardware register wrapper
//  Target: STM32F401XE (Cortex-M4)
// ============================================================
//
//  RULES:
//  - No heap, no virtual, no RTTI
//  - Must compile to a single load/store instruction
//  - addr is baked in at compile time — never stored as a member
//
//  YOUR CHALLENGES are marked with: // ??? <-- YOUR CODE HERE

enum class Access { ReadWrite, ReadOnly, WriteOnly };

template <uintptr_t addr, typename T, Access access = Access::ReadWrite>
class Register {
   public:
    // ----------------------------------------------------------
    // CHALLENGE 1:
    // Return a reference to the volatile hardware register.
    // Hint: cast addr to a pointer of the right type, then dereference.
    // What type should the pointer be? Think about WHY volatile matters here.
    // ----------------------------------------------------------
    static auto& ref() {
        return *(reinterpret_cast<volatile T*>(addr));
    }

    // ----------------------------------------------------------
    // CHALLENGE 2:
    // Read the current value of the register.
    // ----------------------------------------------------------
    static T read() {
        static_assert(access != Access::WriteOnly, "Access is Write Only");
        return static_cast<T>(ref());
    }

    // ----------------------------------------------------------
    // CHALLENGE 3:
    // Write a value to the register (full overwrite).
    // ----------------------------------------------------------
    static void write(T value) {
        static_assert(access != Access::ReadOnly, "Access is Read Only");
        ref() = value;
    }

    // ----------------------------------------------------------
    // CHALLENGE 4:
    // Set specific bits without touching others.
    // Example: set_bits(0b11 << 10) must only affect bits 10 and 11.
    // Hint: this is a classic read-modify-write. One line.
    // ----------------------------------------------------------
    static void set_bits(T mask) {
        static_assert(access != Access::ReadOnly, "Access must have Write Access");
        ref() |= mask;
    }

    // ----------------------------------------------------------
    // CHALLENGE 5 (bonus):
    // Clear specific bits without touching others.
    // Mirror of set_bits — how do you zero out only the masked bits?
    // ----------------------------------------------------------
    static void clear_bits(T mask) {
        static_assert(access != Access::ReadOnly, "Access must have Write Access");
        ref() &= ~mask;
    }
};

// ----------------------------------------------------------
// Usage example (do NOT modify — this is your test target):
//
//   // GPIOA base = 0x40020000, MODER register offset = 0x00
//   using GPIOA_MODER = Register<0x40020000, uint32_t>;
//
//   GPIOA_MODER::write(0);           // zero out MODER
//   GPIOA_MODER::set_bits(1 << 10);  // set bit 10
//   GPIOA_MODER::clear_bits(1 << 10); // clear bit 10
//   uint32_t val = GPIOA_MODER::read();
// ----------------------------------------------------------