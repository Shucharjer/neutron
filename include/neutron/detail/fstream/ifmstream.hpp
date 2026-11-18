// IWYU pragma: private, include <neutron/fstream.hpp>
#pragma once
#include <filesystem>
#include <ios>
#include <istream>
#include <memory>
#include <string>
#include <utility>
#include "neutron/detail/fstream/fmbuf.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/**
 * @brief A `std::basic_ifstream` replacement reading through a
 * `basic_fmbuf`.
 *
 * Behaves like `std::basic_ifstream`: `std::ios_base::in` is always added to
 * the open mode, and failures set `failbit` instead of throwing unless
 * `exceptions()` asks for it. Opened with `in` alone, the file is mapped
 * read-only.
 *
 * @code
 * neutron::ifmstream stream("data.txt");
 * std::string line;
 * while (std::getline(stream, line)) {
 *     // use line
 * }
 * @endcode
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
requires _mappable_char<CharT>
class basic_ifmstream : public std::basic_istream<CharT, Traits> {
    using _istream = std::basic_istream<CharT, Traits>;
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

    basic_ifmstream() : _istream(std::addressof(buf_)) {}

    /// @param wnd_size See `basic_fmbuf::open`.
    explicit basic_ifmstream(
        const char* path, std::ios_base::openmode mode = std::ios_base::in,
        size_type wnd_size = default_window_size)
        : _istream(std::addressof(buf_)) {
        open(path, mode, wnd_size);
    }

    /// @copydoc basic_ifmstream(const char*, std::ios_base::openmode,
    /// size_type)
    explicit basic_ifmstream(
        const std::string& path,
        std::ios_base::openmode mode = std::ios_base::in,
        size_type wnd_size           = default_window_size)
        : basic_ifmstream(path.c_str(), mode, wnd_size) {}

    /// @copydoc basic_ifmstream(const char*, std::ios_base::openmode,
    /// size_type)
    explicit basic_ifmstream(
        const std::filesystem::path& path,
        std::ios_base::openmode mode = std::ios_base::in,
        size_type wnd_size           = default_window_size)
        : _istream(std::addressof(buf_)) {
        open(path, mode, wnd_size);
    }

    basic_ifmstream(const basic_ifmstream&)            = delete;
    basic_ifmstream& operator=(const basic_ifmstream&) = delete;

    basic_ifmstream(basic_ifmstream&& that) noexcept
        // The base only takes the state of `basic_ios`, not the buffer.
        : _istream(std::move(that)),
          buf_(std::move(that.buf_)) { // NOLINT(bugprone-use-after-move)
        _istream::set_rdbuf(std::addressof(buf_));
    }

    basic_ifmstream& operator=(basic_ifmstream&& that) noexcept {
        _istream::operator=(std::move(that));
        buf_ = std::move(that.buf_); // NOLINT(bugprone-use-after-move)
        return *this;
    }

    ~basic_ifmstream() override = default;

    void swap(basic_ifmstream& that) noexcept {
        _istream::swap(that);
        buf_.swap(that.buf_);
    }

    friend void swap(basic_ifmstream& lhs, basic_ifmstream& rhs) noexcept {
        lhs.swap(rhs);
    }

    ATOM_NODISCARD _buffer* rdbuf() const noexcept {
        return std::addressof(buf_);
    }

    ATOM_NODISCARD bool is_open() const noexcept { return buf_.is_open(); }

    /// @brief Open a file; clears the state on success, sets `failbit`
    /// otherwise.
    void open(
        const char* path, std::ios_base::openmode mode = std::ios_base::in,
        size_type wnd_size = default_window_size) {
        _check(buf_.open(path, mode | std::ios_base::in, wnd_size));
    }

    /// @copydoc open(const char*, std::ios_base::openmode, size_type)
    void open(
        const std::string& path,
        std::ios_base::openmode mode = std::ios_base::in,
        size_type wnd_size           = default_window_size) {
        open(path.c_str(), mode, wnd_size);
    }

    /// @copydoc open(const char*, std::ios_base::openmode, size_type)
    void open(
        const std::filesystem::path& path,
        std::ios_base::openmode mode = std::ios_base::in,
        size_type wnd_size           = default_window_size) {
        _check(buf_.open(path, mode | std::ios_base::in, wnd_size));
    }

    /// @brief Close the file; sets `failbit` on failure.
    void close() {
        if (buf_.close() == nullptr) {
            this->setstate(std::ios_base::failbit);
        }
    }

    /// @copydoc basic_fmbuf::window_size
    ATOM_NODISCARD size_type window_size() const noexcept {
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
    // `std::basic_ifstream`.
    mutable _buffer buf_;
};

using ifmstream  = basic_ifmstream<char>;
using wifmstream = basic_ifmstream<wchar_t>;

} // namespace neutron
