// IWYU pragma: private, include <neutron/fstream.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <algorithm>
    #include <bit>
    #include <cstddef>
    #include <filesystem>
    #include <ios>
    #include <limits>
    #include <streambuf>
    #include <string>
    #include <system_error>
    #include <type_traits>
    #include <utility>
#endif
#include "neutron/detail/fstream/file_mapping.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/// @brief A character type that can be stored as its object representation.
template <typename CharT>
concept _mappable_char =
    std::is_trivially_copyable_v<CharT> && std::has_single_bit(sizeof(CharT));

/**
 * @brief A `std::basic_filebuf` replacement that accesses the file through a
 * window-wise memory mapping.
 *
 * At most one window of the file is mapped at a time, and it serves as both
 * the get and the put area, so reading and writing copy straight from and to
 * the file. As for `std::basic_filebuf`, reading and writing share a single
 * position. Characters are stored as their object representation; the
 * `std::codecvt` facet of the locale is not used.
 *
 * A writable file grows a window at a time while writing. `sync()` and
 * `close()` truncate it to the written size, so frequent flushing while
 * appending costs a remapping each time.
 *
 * Every operation reports failures as `std::basic_filebuf` does and never
 * throws. The behaviour is undefined if the file is truncated by someone else
 * while it is open.
 */
ATOM_EXPORT template <typename CharT, typename Traits = std::char_traits<CharT>>
requires _mappable_char<CharT>
class basic_fmbuf : public std::basic_streambuf<CharT, Traits> {
    using _streambuf = std::basic_streambuf<CharT, Traits>;
    using _offset    = internal::file_mapping::offset_type;

    /// Largest position, in characters.
    static constexpr _offset _max_length =
        internal::file_mapping::max_file_size / sizeof(CharT);

public:
    using char_type   = CharT;
    using traits_type = Traits;
    using int_type    = typename Traits::int_type;
    using pos_type    = typename Traits::pos_type;
    using off_type    = typename Traits::off_type;
    using size_type   = std::size_t;

    static constexpr size_type default_window_size = 16UL * 1024UL * 1024UL;

    basic_fmbuf() = default;

    basic_fmbuf(const basic_fmbuf&)            = delete;
    basic_fmbuf& operator=(const basic_fmbuf&) = delete;

    /// The mapping is taken over, so the buffer pointers stay valid.
    basic_fmbuf(basic_fmbuf&& that) noexcept
        : _streambuf(that), file_(std::move(that.file_)),
          view_(std::exchange(that.view_, nullptr)),
          view_length_(std::exchange(that.view_length_, 0)),
          window_length_(std::exchange(that.window_length_, 0)),
          base_(std::exchange(that.base_, 0)),
          pos_(std::exchange(that.pos_, 0)),
          size_(std::exchange(that.size_, 0)),
          opened_size_(std::exchange(that.opened_size_, 0)),
          mode_(std::exchange(that.mode_, std::ios_base::openmode{})) {
        that._reset_areas();
    }

    /// Closes the current file first, ignoring errors.
    basic_fmbuf& operator=(basic_fmbuf&& that) noexcept {
        if (this != &that) {
            close();
            _streambuf::operator=(that);
            file_          = std::move(that.file_);
            view_          = std::exchange(that.view_, nullptr);
            view_length_   = std::exchange(that.view_length_, 0);
            window_length_ = std::exchange(that.window_length_, 0);
            base_          = std::exchange(that.base_, 0);
            pos_           = std::exchange(that.pos_, 0);
            size_          = std::exchange(that.size_, 0);
            opened_size_   = std::exchange(that.opened_size_, 0);
            mode_ = std::exchange(that.mode_, std::ios_base::openmode{});
            that._reset_areas();
        }
        return *this;
    }

    /// Closes the file, ignoring errors.
    ~basic_fmbuf() override { close(); }

    void swap(basic_fmbuf& that) noexcept {
        _streambuf::swap(that);
        std::swap(file_, that.file_);
        std::swap(view_, that.view_);
        std::swap(view_length_, that.view_length_);
        std::swap(window_length_, that.window_length_);
        std::swap(base_, that.base_);
        std::swap(pos_, that.pos_);
        std::swap(size_, that.size_);
        std::swap(opened_size_, that.opened_size_);
        std::swap(mode_, that.mode_);
    }

    friend void swap(basic_fmbuf& lhs, basic_fmbuf& rhs) noexcept {
        lhs.swap(rhs);
    }

    ATOM_ATTRS(ATOM_NODISCARD) bool is_open() const noexcept { return file_.is_open(); }

    /**
     * @brief Open a file, following `std::basic_filebuf::open`.
     *
     * `mode` must be one of the combinations accepted by
     * `std::basic_filebuf::open`, including `std::ios_base::noreplace` where
     * it is available.
     * @param wnd_size Window size in bytes, rounded up to a multiple of the
     * mapping granularity.
     * @return `this`, or null if the buffer is already open, `mode` or
     * `wnd_size` is invalid, or the file cannot be opened.
     */
    basic_fmbuf* open(
        const char* path, std::ios_base::openmode mode,
        size_type wnd_size = default_window_size) {
        if (is_open()) {
            return nullptr;
        }
        file_creation creation{};
        const std::ios_base::openmode access = _parse_mode(mode, creation);
        const size_type window_length        = _window_length(wnd_size);
        if (access == std::ios_base::openmode{} || window_length == 0) {
            return nullptr;
        }
        std::error_code error;
        file_.open(path, _has(access, std::ios_base::out), creation, error);
        if (error) {
            return nullptr;
        }
        window_length_ = window_length;
        opened_size_   = file_.size();
        size_          = opened_size_ / sizeof(char_type);
        pos_           = _has(mode, std::ios_base::ate) ? size_ : 0;
        mode_          = access;
        return this;
    }

    /// @copydoc open(const char*, std::ios_base::openmode, size_type)
    basic_fmbuf* open(
        const std::string& path, std::ios_base::openmode mode,
        size_type wnd_size = default_window_size) {
        return open(path.c_str(), mode, wnd_size);
    }

    /// @copydoc open(const char*, std::ios_base::openmode, size_type)
    basic_fmbuf* open(
        const std::filesystem::path& path, std::ios_base::openmode mode,
        size_type wnd_size = default_window_size) {
        if constexpr (std::is_same_v<std::filesystem::path::value_type, char>) {
            return open(path.c_str(), mode, wnd_size);
        } else {
            return open(path.string(), mode, wnd_size);
        }
    }

    /**
     * @brief Close the file.
     *
     * A writable file is truncated to the written size first. The file is
     * closed even if that fails.
     * @return `this`, or null if no file is open or the truncation fails.
     */
    basic_fmbuf* close() noexcept {
        if (!is_open()) {
            return nullptr;
        }
        const _offset bytes = _content_bytes();
        _idle();
        _unmap();
        std::error_code error;
        if (_writable() && file_.size() != bytes) {
            file_.resize(bytes, error);
        }
        file_.close();
        window_length_ = 0;
        base_ = pos_ = size_ = opened_size_ = 0;
        mode_                               = std::ios_base::openmode{};
        return error ? nullptr : this;
    }

    /// @brief Size of a window in bytes, zero if no file is open.
    ATOM_ATTRS(ATOM_NODISCARD) size_type window_size() const noexcept {
        return window_length_ * sizeof(char_type);
    }

protected:
    std::streamsize showmanyc() override {
        if (!_readable()) {
            return -1;
        }
        const _offset pos = _tell();
        const _offset end = _end();
        if (pos >= end) {
            return -1;
        }
        constexpr auto max = (std::numeric_limits<std::streamsize>::max)();
        return end - pos > static_cast<_offset>(max)
                   ? max
                   : static_cast<std::streamsize>(end - pos);
    }

    int_type underflow() override {
        if (!_readable()) {
            return traits_type::eof();
        }
        if (this->gptr() != nullptr && this->gptr() < this->egptr()) {
            return traits_type::to_int_type(*this->gptr());
        }
        const _offset pos = _tell();
        _idle();
        if (pos >= size_ || !_read_at(pos)) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(*this->gptr());
    }

    /// Moves back across windows; a character differing from the content is
    /// not put back, as that would modify the file.
    int_type pbackfail(int_type ch) override {
        if (!_readable()) {
            return traits_type::eof();
        }
        const _offset pos = _tell();
        if (pos == 0 || pos > _end()) {
            return traits_type::eof();
        }
        _idle();
        if (!_read_at(pos - 1)) {
            return traits_type::eof();
        }
        if (!traits_type::eq_int_type(ch, traits_type::eof()) &&
            !traits_type::eq(traits_type::to_char_type(ch), *this->gptr())) {
            this->gbump(1);
            return traits_type::eof();
        }
        return traits_type::not_eof(ch);
    }

    int_type overflow(int_type ch) override {
        if (!_writable()) {
            return traits_type::eof();
        }
        if (traits_type::eq_int_type(ch, traits_type::eof())) {
            return traits_type::not_eof(ch);
        }
        if (this->pptr() == nullptr || this->pptr() >= this->epptr()) {
            const _offset pos = _appending() ? _end() : _tell();
            _idle();
            if (!_write_at(pos)) {
                return traits_type::eof();
            }
        }
        *this->pptr() = traits_type::to_char_type(ch);
        this->pbump(1);
        return ch;
    }

    /// Positions may lie after the end; `which` is ignored, as there is a
    /// single position.
    pos_type seekoff(
        off_type off, std::ios_base::seekdir dir,
        std::ios_base::openmode /*which*/) override {
        if (!is_open()) {
            return _bad_pos();
        }
        _offset origin = 0;
        if (dir == std::ios_base::cur) {
            origin = _tell();
            if (off == 0) {
                return pos_type(static_cast<off_type>(origin));
            }
        } else if (dir == std::ios_base::end) {
            origin = _end();
        }
        _offset target = 0;
        if (off < 0) {
            // Avoids negating the minimum value.
            const auto back = static_cast<_offset>(-(off + 1)) + 1;
            if (back > origin) {
                return _bad_pos();
            }
            target = origin - back;
        } else {
            const auto forward = static_cast<_offset>(off);
            if (forward > _max_length - origin) {
                return _bad_pos();
            }
            target = origin + forward;
        }
        _seek(target);
        return pos_type(static_cast<off_type>(target));
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
        return seekoff(static_cast<off_type>(pos), std::ios_base::beg, which);
    }

    /// Truncates a writable file to the written size; the content itself is
    /// already in the file.
    int sync() override {
        if (!_writable()) {
            return 0;
        }
        const _offset bytes = _content_bytes();
        if (file_.size() == bytes) {
            return 0;
        }
        _idle();
        _unmap();
        std::error_code error;
        file_.resize(bytes, error);
        return error ? -1 : 0;
    }

private:
    ATOM_ATTRS(ATOM_NODISCARD) static constexpr bool _has(
        std::ios_base::openmode mode, std::ios_base::openmode flag) noexcept {
        return (mode & flag) != std::ios_base::openmode{};
    }

    /**
     * Follows the table of `std::basic_filebuf::open`.
     * @return The access, a combination of `in`, `out` and `app`, or zero if
     * `mode` is invalid.
     */
    static std::ios_base::openmode _parse_mode(
        std::ios_base::openmode mode, file_creation& creation) noexcept {
        using ios                 = std::ios_base;
        const ios::openmode flags = mode & ~(ios::ate | ios::binary);
        if (flags == ios::in) {
            creation = file_creation::open_existing;
            return ios::in;
        }
        if (flags == ios::out || flags == (ios::out | ios::trunc)) {
            creation = file_creation::truncate;
            return ios::out;
        }
        if (flags == ios::app || flags == (ios::out | ios::app)) {
            creation = file_creation::open_or_create;
            return ios::out | ios::app;
        }
        if (flags == (ios::in | ios::out)) {
            creation = file_creation::open_existing;
            return ios::in | ios::out;
        }
        if (flags == (ios::in | ios::out | ios::trunc)) {
            creation = file_creation::truncate;
            return ios::in | ios::out;
        }
        if (flags == (ios::in | ios::app) ||
            flags == (ios::in | ios::out | ios::app)) {
            creation = file_creation::open_or_create;
            return ios::in | ios::out | ios::app;
        }
#ifdef __cpp_lib_ios_noreplace
        creation = file_creation::create_new;
        if (flags == (ios::out | ios::noreplace) ||
            flags == (ios::out | ios::trunc | ios::noreplace)) {
            return ios::out;
        }
        if (flags == (ios::in | ios::out | ios::trunc | ios::noreplace)) {
            return ios::in | ios::out;
        }
#endif
        return ios::openmode{};
    }

    /// @return Characters in a window, or zero if `wnd_size` is invalid.
    static size_type _window_length(size_type wnd_size) noexcept {
        const size_type mask = internal::file_mapping::granularity() - 1;
        if (wnd_size == 0 ||
            wnd_size > (std::numeric_limits<size_type>::max)() - mask) {
            return 0;
        }
        const size_type bytes = (wnd_size + mask) & ~mask;
        return bytes % sizeof(char_type) == 0 ? bytes / sizeof(char_type) : 0;
    }

    ATOM_ATTRS(ATOM_NODISCARD) static pos_type _bad_pos() noexcept {
        return pos_type(static_cast<off_type>(-1));
    }

    ATOM_ATTRS(ATOM_NODISCARD) bool _readable() const noexcept {
        return _has(mode_, std::ios_base::in);
    }

    ATOM_ATTRS(ATOM_NODISCARD) bool _writable() const noexcept {
        return _has(mode_, std::ios_base::out);
    }

    ATOM_ATTRS(ATOM_NODISCARD) bool _appending() const noexcept {
        return _has(mode_, std::ios_base::app);
    }

    /// The put area is set while writing, the get area while reading;
    /// otherwise the position is `pos_`.
    ATOM_ATTRS(ATOM_NODISCARD) _offset _tell() const noexcept {
        if (this->pptr() != nullptr) {
            return base_ + static_cast<_offset>(this->pptr() - view_);
        }
        if (this->gptr() != nullptr) {
            return base_ + static_cast<_offset>(this->gptr() - view_);
        }
        return pos_;
    }

    /// End of the content, in characters.
    ATOM_ATTRS(ATOM_NODISCARD) _offset _end() const noexcept {
        return this->pptr() != nullptr ? (std::max)(size_, _tell()) : size_;
    }

    /// Size the file has when closed; trailing bytes of the opened file that
    /// do not form a character are kept.
    ATOM_ATTRS(ATOM_NODISCARD) _offset _content_bytes() const noexcept {
        return (std::max)(_end() * sizeof(char_type), opened_size_);
    }

    void _reset_areas() noexcept {
        this->setg(nullptr, nullptr, nullptr);
        this->setp(nullptr, nullptr);
    }

    /// Leave the get and the put area, keeping the position and the size.
    void _idle() noexcept {
        const _offset pos = _tell();
        size_             = _end();
        pos_              = pos;
        _reset_areas();
    }

    void _unmap() noexcept {
        if (view_ != nullptr) {
            internal::file_mapping::unmap(
                view_, view_length_ * sizeof(char_type));
            view_        = nullptr;
            view_length_ = 0;
        }
    }

    /// Map `length` characters from `base`; the current window stays mapped
    /// on failure.
    bool _map(_offset base, size_type length) noexcept {
        std::error_code error;
        void* const addr = file_.map(
            base * sizeof(char_type), length * sizeof(char_type), error);
        if (addr == nullptr) {
            return false;
        }
        _unmap();
        view_        = static_cast<char_type*>(addr);
        view_length_ = length;
        base_        = base;
        return true;
    }

    ATOM_ATTRS(ATOM_NODISCARD) _offset _window_base(_offset pos) const noexcept {
        return pos - (pos % window_length_);
    }

    /// Set the get area at `pos`.
    /// @pre The buffer is idle and `pos < size_`.
    bool _read_at(_offset pos) noexcept {
        const _offset base = _window_base(pos);
        if (view_ == nullptr || base_ != base) {
            const _offset rest = (file_.size() / sizeof(char_type)) - base;
            if (!_map(
                    base, static_cast<size_type>(std::min(
                              static_cast<_offset>(window_length_), rest)))) {
                return false;
            }
        }
        const _offset valid =
            (std::min)(static_cast<_offset>(view_length_), size_ - base);
        this->setg(
            view_, view_ + static_cast<size_type>(pos - base),
            view_ + static_cast<size_type>(valid));
        return true;
    }

    /// Set the put area at `pos`, growing the file to cover the window.
    /// @pre The buffer is idle.
    bool _write_at(_offset pos) noexcept {
        if (pos >= _max_length) [[unlikely]] {
            return false;
        }
        const _offset base     = _window_base(pos);
        const size_type length = static_cast<size_type>(
            (std::min)(static_cast<_offset>(window_length_),
                       _max_length - base));
        if (view_ == nullptr || base_ != base || view_length_ != length) {
            const _offset need = (base + length) * sizeof(char_type);
            if (file_.size() < need) {
                // The file cannot be resized while a window is mapped.
                _unmap();
                std::error_code error;
                file_.resize(need, error);
                if (error) {
                    return false;
                }
            }
            if (!_map(base, length)) {
                return false;
            }
        }
        this->setp(
            view_ + static_cast<size_type>(pos - base), view_ + view_length_);
        return true;
    }

    /// Move to `target`, keeping the current area when it covers `target`.
    void _seek(_offset target) noexcept {
        if (this->gptr() != nullptr && target >= base_ &&
            target - base_ <= static_cast<_offset>(this->egptr() - view_)) {
            this->setg(
                view_, view_ + static_cast<size_type>(target - base_),
                this->egptr());
            return;
        }
        // Appending moves to the end on the next write.
        if (this->pptr() != nullptr && !_appending() && target >= base_ &&
            target - base_ <= static_cast<_offset>(view_length_)) {
            size_ = _end();
            this->setp(
                view_ + static_cast<size_type>(target - base_),
                view_ + view_length_);
            return;
        }
        _idle();
        pos_ = target;
    }

    internal::file_mapping file_;
    /// Mapped window, or null when none is.
    char_type* view_ = nullptr;
    /// Characters in the mapped window.
    size_type view_length_ = 0;
    /// Characters in a full window.
    size_type window_length_ = 0;
    /// Position of `view_`.
    _offset base_ = 0;
    /// Position while neither area is set.
    _offset pos_ = 0;
    /// Characters of content, not counting writes into the put area.
    _offset size_ = 0;
    /// Size of the file when opened, in bytes.
    _offset opened_size_ = 0;
    /// A combination of `in`, `out` and `app`, zero when closed.
    std::ios_base::openmode mode_{};
};

ATOM_EXPORT using fmbuf  = basic_fmbuf<char>;
ATOM_EXPORT using wfmbuf = basic_fmbuf<wchar_t>;

} // namespace neutron
