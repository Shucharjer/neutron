// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <ranges>
#include <tuple>
#include <type_traits>
#include "neutron/detail/concepts/fixed_array.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/reflection/refl.hpp"
#include "neutron/detail/tuple/shared_tuple.hpp"

namespace neutron {

template <typename Format, typename T>
struct serialize_t;

template <typename Format, typename T>
struct deserialize_t;

template <typename Format, typename T>
constexpr auto serialize(Format& format, const T& data) noexcept(
    noexcept(serialize_t<Format, T>()(format, data))) {
    return serialize_t<Format, T>()(format, data);
}

template <typename Format, typename T>
constexpr auto deserialize(Format& format, T& data) noexcept(
    noexcept(deserialize_t<Format, T>()(format, data))) {
    return deserialize_t<Format, T>()(format, data);
}

template <std::size_t Size>
struct _bitbuf_impl {
    std::byte* data;
};

template <>
class _bitbuf_impl<static_cast<std::size_t>(-1)> {
public:
private:
    void* data_;
    std::size_t length_;
};

template <std::size_t Size>
_bitbuf_impl<Size> bitbuf(char (&)[Size]) {
    return _bitbuf_impl<Size>{};
}

template <std::size_t Size>
_bitbuf_impl<Size> bitbuf(unsigned char (&)[Size]) {
    return _bitbuf_impl<Size>{};
}

template <std::size_t Size>
_bitbuf_impl<Size> bitbuf(std::byte (&)[Size]) {
    return _bitbuf_impl<Size>{};
}

template <std::size_t Size>
_bitbuf_impl<Size> bitbuf(std::array<char, Size>&) {
    return _bitbuf_impl<Size>{};
}

template <std::size_t Size>
_bitbuf_impl<Size> bitbuf(std::array<unsigned char, Size>&) {
    return _bitbuf_impl<Size>{};
}

template <std::size_t Size>
_bitbuf_impl<Size> bitbuf(std::array<std::byte, Size>&) {
    return _bitbuf_impl<Size>{};
}

class bitstream {
    friend struct _bitviewer;

public:
    constexpr bitstream() noexcept = default;

    constexpr bitstream(std::byte* byte, std::size_t length) noexcept
        : byte_(byte), length_(length) {}

    ATOM_NODISCARD constexpr void* data() noexcept { return byte_; }
    ATOM_NODISCARD constexpr const void* data() const noexcept { return byte_; }

private:
    std::byte* byte_    = nullptr;
    std::size_t length_ = 0;
};

struct _bitviewer {
    ATOM_NODISCARD static constexpr auto data(bitstream& bs) noexcept
        -> std::byte*& {
        return bs.byte_;
    }

    ATOM_NODISCARD static constexpr auto data(const bitstream& bs) noexcept
        -> const std::byte* {
        return bs.byte_;
    }

    ATOM_NODISCARD static constexpr auto length(bitstream& bs) noexcept
        -> std::size_t& {
        return bs.length_;
    }

    ATOM_NODISCARD static constexpr auto length(const bitstream& bs) noexcept
        -> std::size_t {
        return bs.length_;
    }
};

#if ATOM_HAS_REFLECTION

#else

template <std::integral T>
struct serialize_t<bitstream, T> {
    void operator()(bitstream& bs, const T& val) const noexcept {
        *reinterpret_cast<T*>(_bitviewer::data(bs)) = val;
        _bitviewer::data(bs) += sizeof(T);
        _bitviewer::length(bs) -= sizeof(T);
    }
};

template <std::integral T>
struct deserialize_t<bitstream, T> {
    void operator()(bitstream& bs, T& val) const noexcept {
        auto& data = _bitviewer::data(bs);
        val        = *reinterpret_cast<T*>(data);
        data += sizeof(T);
        _bitviewer::length(bs) -= sizeof(T);
    }
};

template <std::floating_point T>
struct serialize_t<bitstream, T> {
    void operator()(bitstream& bs, const T& val) const noexcept {
        *reinterpret_cast<T*>(_bitviewer::data(bs)) = val;
        _bitviewer::data(bs) += sizeof(T);
        _bitviewer::length(bs) -= sizeof(T);
    }
};

template <std::floating_point T>
struct deserialize_t<bitstream, T> {
    void operator()(bitstream& bs, T& val) const noexcept {
        auto& data = _bitviewer::data(bs);
        val        = *reinterpret_cast<T*>(data);
        data += sizeof(T);
        _bitviewer::length(bs) -= sizeof(T);
    }
};

template <fixed_array T>
struct serialize_t<bitstream, T> {
    void operator()(bitstream& bs, const T& val) const noexcept {
        std::ranges::for_each(
            val, [&bs](const auto& elem) { serialize(bs, elem); });
    }
};

template <fixed_array T>
struct deserialize_t<bitstream, T> {
    void operator()(bitstream& bs, T& val) const noexcept {
        std::ranges::for_each(
            val, [&bs](auto& elem) { deserialize(bs, elem); });
    }
};

template <typename... Args>
struct serialize_t<bitstream, std::tuple<Args...>> {
    void operator()(
        bitstream& bs, const std::tuple<Args...>& val) const noexcept {
        std::apply(
            [&bs](const auto&... elem) { (serialize(bs, elem), ...); }, val);
    }
};

template <typename... Args>
struct deserialize_t<bitstream, std::tuple<Args...>> {
    void operator()(bitstream& bs, std::tuple<Args...>& val) const noexcept {
        std::apply([&bs](auto&... elem) { (deserialize(bs, elem), ...); }, val);
    }
};

template <typename... Args>
struct serialize_t<bitstream, shared_tuple<Args...>> {
    void operator()(
        bitstream& bs, const shared_tuple<Args...>& val) const noexcept {
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (serialize(bs, val.template get<I>()), ...);
        }(std::index_sequence_for<Args...>{});
    }
};

template <typename... Args>
struct deserialize_t<bitstream, shared_tuple<Args...>> {
    void operator()(bitstream& bs, shared_tuple<Args...>& val) const noexcept {
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (deserialize(bs, val.template get<I>()), ...);
        }(std::index_sequence_for<Args...>{});
    }
};

template <std::ranges::contiguous_range Contiguous>
requires(!fixed_array<Contiguous> && std::ranges::sized_range<Contiguous>)
struct serialize_t<bitstream, Contiguous> {
    using tp = std::ranges::range_value_t<Contiguous>;
    void operator()(bitstream& bs, const Contiguous& range) const noexcept {
        const auto count = std::ranges::size(range);
        serialize(bs, count);
        if constexpr (std::is_trivially_copyable_v<tp>) {
            std::memcpy(
                _bitviewer::data(bs), std::ranges::data(range),
                sizeof(tp) * count);
            _bitviewer::data(bs) += sizeof(tp) * count;
            _bitviewer::length(bs) -= sizeof(tp) * count;
        } else {
            std::ranges::for_each(
                range, [&bs](const auto& val) { serialize(bs, val); });
        }
    }
};

template <std::ranges::contiguous_range Contiguous>
requires(!fixed_array<Contiguous> && std::ranges::sized_range<Contiguous>)
struct deserialize_t<bitstream, Contiguous> {
    using tp = std::ranges::range_value_t<Contiguous>;
    void operator()(bitstream& bs, Contiguous& range) const noexcept {
        std::size_t count = 0;
        deserialize(bs, count);
        range.resize(count);
        if constexpr (std::is_trivially_copyable_v<tp>) {
            std::memcpy(
                std::ranges::data(range), _bitviewer::data(bs),
                sizeof(tp) * count);
            _bitviewer::data(bs) += sizeof(tp) * count;
            _bitviewer::length(bs) -= sizeof(tp) * count;
        } else {
            std::ranges::for_each(
                range, [&bs](auto& val) { deserialize(bs, val); });
        }
    }
};

template <std::ranges::contiguous_range Contiguous>
requires(!std::ranges::sized_range<Contiguous>)
struct serialize_t<bitstream, Contiguous> {
    void operator()(bitstream& bs, const Contiguous& range) const noexcept {
        auto& data  = _bitviewer::data(bs);
        auto* count = reinterpret_cast<std::size_t*>(data);
        data += sizeof(std::size_t);
        _bitviewer::length(bs) -= sizeof(std::size_t);
        *count = 0;

        std::ranges::for_each(range, [&bs, count](const auto& val) {
            serialize(bs, val);
            ++(*count);
        });
    }
};

template <_refl_legacy::aggregate T>
requires(!fixed_array<T>)
struct serialize_t<bitstream, T> {
    void operator()(bitstream& bs, const T& val) const noexcept {
        if constexpr (member_count_of<T>() != 0) {
            auto tup = _refl_legacy::object_to_tuple_view(val);
            std::apply(
                [&bs](const auto&... val) { (serialize(bs, val), ...); }, tup);
        }
    }
};

template <_refl_legacy::aggregate T>
requires(!fixed_array<T>)
struct deserialize_t<bitstream, T> {
    void operator()(bitstream& bs, T& val) const noexcept {
        if constexpr (member_count_of<T>() != 0) {
            auto tup = _refl_legacy::object_to_tuple_view(val);
            std::apply(
                [&bs](auto&... elems) { (deserialize(bs, elems), ...); }, tup);
        }
    }
};

#endif

} // namespace neutron
