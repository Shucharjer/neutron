#pragma once
#include <concepts>

namespace neutron {

template <template <typename> typename>
struct _check_type_alias_exists; // just for type deduction here

template <typename Token>
concept unstoppable_token = requires (const Token tok) {
    typename _check_type_alias_exists<Token::template callback_type>;
    { tok.stop_requested() } noexcept -> std::same_as<bool>;
    { tok.stop_possible() } noexcept -> std::same_as<bool>;
    { Token(tok) } noexcept;
} && std::copyable<Token> && std::equality_comparable<Token> &&
std::swappable<Token>;

}