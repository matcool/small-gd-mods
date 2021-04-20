#pragma once
#include <cstdint>

template <typename R, typename T>
inline R cast(T value) { return reinterpret_cast<R>(value); }

inline auto follow(std::uintptr_t addr) { return *cast<std::uintptr_t*>(addr); }
inline auto follow(void* addr) { return *cast<void**>(addr); }