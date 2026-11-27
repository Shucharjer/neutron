// IWYU pragma: private, include <neutron/fstream.hpp>
#pragma once
#include <filesystem>
#include <ios>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include "neutron/detail/fstream/fmbuf.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/**
 * @brief A `std::basic_ofstream` replacement writing through a
 * `basic_fmbuf`.
 *
 * Behaves like `std::basic_ofstream`: `std::ios_base::out` is always added to
 * the open mode, and failures set `failbit` instead of throwing unless
 * `exceptions()` asks for it.
 *
 * @code
 * neutron::ofmstream stream("data.txt");
 * stream << "neutron " << 42 << '\n';
 * @endcode
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
requires _mappable_char<CharT>
class basic_ofmstream : public std::basic_ostream<CharT, Traits> {
    using _ostream = std::basic_ostream<CharT, Traits>;
    using _buffer  = basic_fmbuf<CharT, Traits>;

public:
    using char_type   = CharT;
    using traits_type = Traits;
    using int_type    = typename Traits::int_type;
    using pos_type    = typename Traits::pos_type;
    using off_type    = typename Traits::off_type;
    using size_type   = typename _buffer::size_type;

    static constexpr size_type default_window_size =
        _buffer::default_window_size;

    basic_ofmstream() : _ostream(std::addressof(buf_)) {}

    /// @param wnd_size See `basic_fmbuf::open`.
    explicit basic_ofmstream(
        const char* path, std::ios_base::openmode mode = std::ios_base::out,
        size_type wnd_size = default_window_size)
        : _ostream(std::addressof(buf_)) {
        open(path, mode, wnd_size);
    }

    /// @copydoc basic_ofmstream(const char*, std::ios_base::openmode,
    /// size_type)
    explicit basic_ofmstream(
        const std::string& path,
        std::ios_base::openmode mode = std::ios_base::out,
        size_type wnd_size           = default_window_size)
        : basic_ofmstream(path.c_str(), mode, wnd_size) {}

    /// @copydoc basic_ofmstream(const char*, std::ios_base::openmode,
    /// size_type)
    explicit basic_ofmstream(
        const std::filesystem::path& path,
        std::ios_base::openmode mode = std::ios_base::out,
        size_type wnd_size           = default_window_size)
        : _ostream(std::addressof(buf_)) {
        open(path, mode, wnd_size);
    }

    basic_ofmstream(const basic_ofmstream&)            = delete;
    basic_ofmstream& operator=(const basic_ofmstream&) = delete;

    basic_ofmstream(basic_ofmstream&& that) noexcept
        // The base only takes the state of `basic_ios`, not the buffer.
        : _ostream(std::move(that)),
          buf_(std::move(that.buf_)) { // NOLINT(bugprone-use-after-move)
        _ostream::set_rdbuf(std::addressof(buf_));
    }

    basic_ofmstream& operator=(basic_ofmstream&& that) noexcept {
        _ostream::operator=(std::move(that));
        buf_ = std::move(that.buf_); // NOLINT(bugprone-use-after-move)
        return *this;
    }

    ~basic_ofmstream() override = default;

    void swap(basic_ofmstream& that) noexcept {
        _ostream::swap(that);
        buf_.swap(that.buf_);
    }

    friend void swap(basic_ofmstream& lhs, basic_ofmstream& rhs) noexcept {
        lhs.swap(rhs);
    }

    ATOM_ATTRS(ATOM_NODISCARD) _buffer* rdbuf() const noexcept {
        return std::addressof(buf_);
    }

    ATOM_ATTRS(ATOM_NODISCARD) bool is_open() const noexcept { return buf_.is_open(); }

    /// @brief Open a file; clears the state on success, sets `failbit`
    /// otherwise.
    void open(
        const char* path, std::ios_base::openmode mode = std::ios_base::out,
        size_type wnd_size = default_window_size) {
        _check(buf_.open(path, mode | std::ios_base::out, wnd_size));
    }

    /// @copydoc open(const char*, std::ios_base::openmode, size_type)
    void open(
        const std::string& path,
        std::ios_base::openmode mode = std::ios_base::out,
        size_type wnd_size           = default_window_size) {
        open(path.c_str(), mode, wnd_size);
    }

    /// @copydoc open(const char*, std::ios_base::openmode, size_type)
    void open(
        const std::filesystem::path& path,
        std::ios_base::openmode mode = std::ios_base::out,
        size_type wnd_size           = default_window_size) {
        _check(buf_.open(path, mode | std::ios_base::out, wnd_size));
    }

    /// @brief Close the file; sets `failbit` on failure.
    void close() {
        if (buf_.close() == nullptr) {
            this->setstate(std::ios_base::failbit);
        }
    }

    /// @copydoc basic_fmbuf::window_size
    ATOM_ATTRS(ATOM_NODISCARD) size_type window_size() const noexcept {
        return buf_.window_size();
    }

private:
    void _check(const _buffer* opened) {
        if (opened != nullptr) {
            this->clear();
        } else {
            this->setstate(std::ios_base::failbit);
        }
    }

    // Mutable since `rdbuf() const` hands out a modifiable buffer, as for
    // `std::basic_ofstream`.
    mutable _buffer buf_;
};

using ofmstream  = basic_ofmstream<char>;
using wofmstream = basic_ofmstream<wchar_t>;

} // namespace neutron
