#pragma once
#include <cstddef>
#include <initializer_list>

namespace neutron {

template <typename Elem, Elem...>
struct sequence;

template <typename Seq>
struct sequence_element;

template <typename Seq>
struct sequence_size;

template <typename, typename>
struct sequence_concat;

template <typename, auto>
struct sequence_append;

template <typename, typename>
struct sequence_merge;

template <std::size_t N, typename Ty = std::size_t>
struct integer_seq;

template <typename, typename>
struct sequence_remake;

template <typename Seq>
struct empty_sequence;

template <auto Lhs, auto Rhs>
struct less;

template <auto Lhs, auto Rhs>
struct equal;

template <auto Lhs, auto Rhs>
struct greater;

template <
    bool Cond, typename First, typename Second, template <typename, typename> typename Operator>
struct operate_if;

template <typename Seq, template <auto> typename Pr>
struct filt;

template <typename Seq, template <auto> typename Pr>
struct filt_not;

template <typename, template <auto, auto> typename Compare = less>
struct quick_sort;

template <typename Seq, template <typename...> typename Cnt = std::initializer_list>
struct as_container;

} // namespace neutron
