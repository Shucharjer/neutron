#pragma once
#include <concepts>
#include <string_view>

namespace neutron {

template <typename StringView>
concept string_view_like = std::convertible_to<StringView, std::string_view>;

}
