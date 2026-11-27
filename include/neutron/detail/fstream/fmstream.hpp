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
 * @brief A `std::basic_fstream` replacement reading and writing through a
 * `basic_fmbuf`.
 *
 * Behaves like `std::basic_fstream`: the open mode is used as given, and
 * failures set `failbit` instead of throwing unless `exceptions()` asks for
 * it.
 *
 * @code
 * neutron::fmstream stream("data.txt", std::ios::in | std::ios::out);
 * int value = 0;
 * stream >> value;
 * stream.seekp(0);
 * stream << value + 1;
 * @endcode
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
requires _mappable_char<CharT>
class basic_fmstream : public std::basic_iostream<CharT, Traits> {
    using _iostream = std::basic_iostream<CharT, Traits>;
    using _buffer   = basic_fmbuf<CharT, Traits>;

public:
    using char_type   = CharT;
    using traits_type = Traits;
    using int_type    = typename Traits::int_type;
    using pos_type    = typename Traits::pos_type;
    using off_type    = typename Traits::off_type;
    using size_type   = typename _buffer::size_type;

    static constexpr size_type default_window_size =
        _buffer::default_window_size;

    basic_fmstream() : _iostream(std::addressof(buf_)) {}

    /// @param wnd_size See `basic_fmbuf::open`.
    explicit basic_fmstream(
        const char* path,
        std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
        size_type wnd_size           = default_window_size)
        : _iostream(std::addressof(buf_)) {
        open(path, mode, wnd_size);
    }

    /// @copydoc basic_fmstream(const char*, std::ios_base::openmode, size_type)
    explicit basic_fmstream(
        const std::string& path,
        std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
        size_type wnd_size           = default_window_size)
        : basic_fmstream(path.c_str(), mode, wnd_size) {}

    /// @copydoc basic_fmstream(const char*, std::ios_base::openmode, size_type)
    explicit basic_fmstream(
        const std::filesystem::path& path,
        std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
        size_type wnd_size           = default_window_size)
        : _iostream(std::addressof(buf_)) {
        open(path, mode, wnd_size);
    }

    basic_fmstream(const basic_fmstream&)            = delete;
    basic_fmstream& operator=(const basic_fmstream&) = delete;

    basic_fmstream(basic_fmstream&& that) noexcept
        // The base only takes the state of `basic_ios`, not the buffer.
        : _iostream(std::move(that)),
          buf_(std::move(that.buf_)) { // NOLINT(bugprone-use-after-move)
        _iostream::set_rdbuf(std::addressof(buf_));
    }

    basic_fmstream& operator=(basic_fmstream&& that) noexcept {
        _iostream::operator=(std::move(that));
        buf_ = std::move(that.buf_); // NOLINT(bugprone-use-after-move)
        return *this;
    }

    ~basic_fmstream() override = default;

    void swap(basic_fmstream& that) noexcept {
        _iostream::swap(that);
        buf_.swap(that.buf_);
    }

    friend void swap(basic_fmstream& lhs, basic_fmstream& rhs) noexcept {
        lhs.swap(rhs);
    }

    ATOM_ATTRS(ATOM_NODISCARD) _buffer* rdbuf() const noexcept {
        return std::addressof(buf_);
    }

    ATOM_ATTRS(ATOM_NODISCARD) bool is_open() const noexcept { return buf_.is_open(); }

    /// @brief Open a file; clears the state on success, sets `failbit`
    /// otherwise.
    void open(
        const char* path,
        std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
        size_type wnd_size           = default_window_size) {
        _check(buf_.open(path, mode, wnd_size));
    }

    /// @copydoc open(const char*, std::ios_base::openmode, size_type)
    void open(
        const std::string& path,
        std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
        size_type wnd_size           = default_window_size) {
        open(path.c_str(), mode, wnd_size);
    }

    /// @copydoc open(const char*, std::ios_base::openmode, size_type)
    void open(
        const std::filesystem::path& path,
        std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
        size_type wnd_size           = default_window_size) {
        _check(buf_.open(path, mode, wnd_size));
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
    // `std::basic_fstream`.
    mutable _buffer buf_;
};

using fmstream  = basic_fmstream<char>;
using wfmstream = basic_fmstream<wchar_t>;

} // namespace neutron
