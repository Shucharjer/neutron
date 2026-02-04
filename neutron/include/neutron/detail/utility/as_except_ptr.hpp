#pragma once

#include <concepts>
#include <exception>
#include <system_error>
#include <type_traits>
namespace neutron {

template <typename Error>
constexpr decltype(auto) as_except_ptr(Error&& error) noexcept {
    if constexpr (std::same_as<std::exception_ptr, std::decay_t<Error>>) {
        return std::forward<Error>(error);
    } else if constexpr (std::same_as<std::error_code, std::decay_t<Error>>) {
        return std::make_exception_ptr(std::system_error(error));
    } else {
        return std::make_exception_ptr(std::forward<Error>(error));
    }
}

} // namespace neutron
