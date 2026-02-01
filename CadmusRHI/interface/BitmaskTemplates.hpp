#pragma once

#include <concepts>
#include <type_traits>

template <typename T>
concept BitmaskEnum = std::is_enum_v<T> and requires(T e) {
    // A look for enable_bitmask_operator_or to  enable  this operator
    enable_bitmask_operators(e);
};

template <BitmaskEnum T>
constexpr T operator&(T lhs, T rhs)
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <BitmaskEnum T>
constexpr T operator^(T lhs, T rhs)
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
}

template <BitmaskEnum T>
constexpr T operator~(T v)
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(~static_cast<U>(v));
}

template <BitmaskEnum T>
constexpr T operator|(T lhs, T rhs)
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <BitmaskEnum T>
constexpr T &operator|=(T &lhs, T rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

template <BitmaskEnum T>
constexpr T &operator&=(T &lhs, T rhs)
{
    lhs = lhs & rhs;
    return lhs;
}