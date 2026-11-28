// IWYU pragma: private, include <neutron/fstream.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstddef>
    #include <iterator>
    #include <limits>
    #include <span>
    #include <stdexcept>
    #include <system_error>
    #include <type_traits>
    #include <utility>
#endif
#include "neutron/detail/fstream/file_mapping.hpp"
#include "neutron/detail/fstream/is_string.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/**
 * @brief Maps a file one window at a time.
 *
 * The file is split into windows of `window_size()` bytes; only the last one
 * may be shorter. At most one window is mapped at a time: accessing a window
 * unmaps the previous one, which invalidates every span obtained before.
 * Iterators therefore yield spans that must not outlive the next access.
 *
 * The behaviour is undefined if the file is truncated by someone else while a
 * window is mapped.
 *
 * @tparam ReadOnly Whether the content is mapped read-only.
 */
ATOM_EXPORT template <bool ReadOnly>
class file_window {
public:
    using element_type =
        std::conditional_t<ReadOnly, const std::byte, std::byte>;
    using value_type      = std::span<element_type>;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using offset_type     = internal::file_mapping::offset_type;

    class iterator;

    static constexpr size_type default_size = 16UL * 1024UL * 1024UL;

    static constexpr size_type npos = (std::numeric_limits<size_type>::max)();

    file_window() noexcept = default;

    /**
     * @brief Open an existing file.
     * @param wnd_size Rounded up to a multiple of the mapping granularity.
     * @throws std::invalid_argument `wnd_size` is zero or too large.
     * @throws std::system_error The file cannot be opened.
     */
    template <_file_path Path>
    requires ReadOnly
    explicit file_window(const Path& path, size_type wnd_size = default_size) {
        open(path, wnd_size);
    }

    /**
     * @brief Open a file for reading and writing.
     * @param wnd_size Rounded up to a multiple of the mapping granularity.
     * @throws std::invalid_argument `wnd_size` is zero or too large.
     * @throws std::system_error The file cannot be opened.
     */
    template <_file_path Path>
    requires(!ReadOnly)
    explicit file_window(
        const Path& path, size_type wnd_size = default_size,
        file_creation creation = file_creation::open_existing) {
        open(path, wnd_size, creation);
    }

    file_window(const file_window&)            = delete;
    file_window& operator=(const file_window&) = delete;

    file_window(file_window&& that) noexcept
        : file_(std::move(that.file_)),
          view_(std::exchange(that.view_, nullptr)),
          view_size_(std::exchange(that.view_size_, 0)),
          index_(std::exchange(that.index_, npos)),
          wnd_size_(std::exchange(that.wnd_size_, 0)) {}

    file_window& operator=(file_window&& that) noexcept {
        if (this != &that) {
            _unmap();
            file_      = std::move(that.file_);
            view_      = std::exchange(that.view_, nullptr);
            view_size_ = std::exchange(that.view_size_, 0);
            index_     = std::exchange(that.index_, npos);
            wnd_size_  = std::exchange(that.wnd_size_, 0);
        }
        return *this;
    }

    ~file_window() noexcept { _unmap(); }

    /// @copydoc file_window(const Path&, size_type)
    template <_file_path Path>
    requires ReadOnly
    void open(const Path& path, size_type wnd_size = default_size) {
        _open(_path_c_str(path), wnd_size, file_creation::open_existing);
    }

    /// @copydoc file_window(const Path&, size_type, file_creation)
    template <_file_path Path>
    requires(!ReadOnly)
    void open(
        const Path& path, size_type wnd_size = default_size,
        file_creation creation = file_creation::open_existing) {
        _open(_path_c_str(path), wnd_size, creation);
    }

    void close() noexcept {
        _unmap();
        file_.close();
        wnd_size_ = 0;
    }

    ATOM_ATTRS(ATOM_NODISCARD) bool is_open() const noexcept { return file_.is_open(); }

    /**
     * @brief Map window `n`, unmapping the current one.
     * @pre `n < size()`.
     * @throws std::system_error The window cannot be mapped; the current
     * window stays mapped.
     */
    ATOM_ATTRS(ATOM_NODISCARD) value_type operator[](size_type n) {
        ATOM_ASSERT(n < size());
        return _view(n);
    }

    /**
     * @brief Map window `n`, unmapping the current one.
     * @throws std::out_of_range `n` is not less than `size()`.
     * @throws std::system_error The window cannot be mapped; the current
     * window stays mapped.
     */
    ATOM_ATTRS(ATOM_NODISCARD) value_type at(size_type n) {
        if (n >= size()) [[unlikely]] {
            throw std::out_of_range("neutron::file_window: index out of range");
        }
        return (*this)[n];
    }

    /// @copydoc operator[](size_type)
    ATOM_ATTRS(ATOM_NODISCARD) value_type operator[](size_type n) const
    requires ReadOnly
    {
        return _view(n);
    }

    /// @copydoc at(size_type)
    ATOM_ATTRS(ATOM_NODISCARD) value_type at(size_type n) const
    requires ReadOnly
    {
        if (n >= size()) [[unlikely]] {
            throw std::out_of_range("neutron::file_window: index out of range");
        }
        return _view(n);
    }

    /// @brief Map the window containing byte `offset` of the file.
    /// @pre `offset < file_size()`.
    ATOM_ATTRS(ATOM_NODISCARD) value_type window_at(offset_type offset) {
        ATOM_ASSERT(offset < file_size());
        return (*this)[static_cast<size_type>(offset / wnd_size_)];
    }

    /// @copydoc window_at(offset_type)
    ATOM_ATTRS(ATOM_NODISCARD) value_type window_at(offset_type offset) const
    requires ReadOnly
    {
        ATOM_ASSERT(offset < file_size());
        return _view(static_cast<size_type>(offset / wnd_size_));
    }

    /// @brief Iterate over the windows, mapping each on dereference.
    ATOM_ATTRS(ATOM_NODISCARD) iterator begin() noexcept { return iterator(this, 0); }

    /// @copydoc begin()
    ATOM_ATTRS(ATOM_NODISCARD) iterator end() noexcept { return iterator(this, size()); }

    /// @copydoc begin()
    ATOM_ATTRS(ATOM_NODISCARD) iterator begin() const noexcept
    requires ReadOnly
    {
        return iterator(this, 0);
    }

    /// @copydoc begin()
    ATOM_ATTRS(ATOM_NODISCARD) iterator end() const noexcept
    requires ReadOnly
    {
        return iterator(this, size());
    }

    /// @brief Number of windows.
    ATOM_ATTRS(ATOM_NODISCARD) size_type size() const noexcept {
        if (wnd_size_ == 0) {
            return 0;
        }
        const offset_type size = file_.size();
        const offset_type full = size / wnd_size_;
        return static_cast<size_type>(
            full + static_cast<offset_type>(size % wnd_size_ != 0));
    }

    ATOM_ATTRS(ATOM_NODISCARD) bool empty() const noexcept { return file_.size() == 0; }

    /// @brief Size of a full window in bytes.
    ATOM_ATTRS(ATOM_NODISCARD) size_type window_size() const noexcept { return wnd_size_; }

    /// @brief Size of the file in bytes.
    ATOM_ATTRS(ATOM_NODISCARD) offset_type file_size() const noexcept {
        return file_.size();
    }

    /// @brief Index of the mapped window, or `npos` if none is mapped.
    ATOM_ATTRS(ATOM_NODISCARD) size_type mapped_index() const noexcept { return index_; }

    /**
     * @brief Resize the file; a grown part reads as zeros.
     *
     * The current window is unmapped. The size is unchanged on failure.
     */
    void resize(offset_type size, std::error_code& error) noexcept
    requires(!ReadOnly)
    {
        ATOM_ASSERT(is_open());
        _unmap();
        file_.resize(size, error);
    }

    /**
     * @brief Map window `n` without throwing.
     * @param out The window, set on success.
     * @return False with `error` set: `n` is out of range, or the window
     * cannot be mapped and the current one stays mapped.
     */
    bool view(size_type n, value_type& out, std::error_code& error) const {
        error.clear();
        if (n >= size()) [[unlikely]] {
            error = std::make_error_code(std::errc::invalid_argument);
            return false;
        }
        if (n != index_) {
            const offset_type offset = static_cast<offset_type>(n) * wnd_size_;
            const offset_type rest   = file_.size() - offset;
            const size_type length =
                rest < wnd_size_ ? static_cast<size_type>(rest) : wnd_size_;
            // Map first, so that a failure leaves the current window intact.
            auto* const addr =
                static_cast<element_type*>(file_.map(offset, length, error));
            if (addr == nullptr) {
                return false;
            }
            _unmap();
            view_      = addr;
            view_size_ = length;
            index_     = n;
        }
        out = { view_, view_size_ };
        return true;
    }

    /// @copydoc resize(offset_type, std::error_code&)
    void flush(std::error_code& error) noexcept
    requires(!ReadOnly)
    {
        ATOM_ASSERT(is_open());
        file_.flush(view_, view_size_, true, error);
    }

    /// @copydoc resize(offset_type, std::error_code&)
    /// @throws std::system_error
    void resize(offset_type size)
    requires(!ReadOnly)
    {
        std::error_code error;
        resize(size, error);
        if (error) [[unlikely]] {
            internal::_throw_error(error, "file_window::resize");
        }
    }

    /**
     * @brief Write every modified window back to the file.
     * @throws std::system_error
     */
    void flush()
    requires(!ReadOnly)
    {
        ATOM_ASSERT(is_open());
        file_.flush(view_, view_size_, true);
    }

private:
    void _open(const char* path, size_type wnd_size, file_creation creation) {
        const size_type rounded = _round_window(wnd_size);
        close();
        file_.open(path, !ReadOnly, creation);
        wnd_size_ = rounded;
    }

    static size_type _round_window(size_type wnd_size) {
        const size_type mask = internal::file_mapping::granularity() - 1;
        if (wnd_size == 0 || wnd_size > npos - mask) [[unlikely]] {
            throw std::invalid_argument(
                "neutron::file_window: invalid window size");
        }
        return (wnd_size + mask) & ~mask;
    }

    /// Map `n`, or keep the current window when it is already the one.
    ATOM_ATTRS(ATOM_NODISCARD) value_type _view(size_type n) const {
        if (n != index_) [[unlikely]] {
            _switch(n);
        }
        return { view_, view_size_ };
    }

    void _switch(size_type n) const {
        const offset_type offset = static_cast<offset_type>(n) * wnd_size_;
        const offset_type rest   = file_.size() - offset;
        const size_type length =
            rest < wnd_size_ ? static_cast<size_type>(rest) : wnd_size_;
        // Map first, so that a failure leaves the current window intact.
        auto* const view =
            static_cast<element_type*>(file_.map(offset, length));
        _unmap();
        view_      = view;
        view_size_ = length;
        index_     = n;
    }

    void _unmap() const noexcept {
        if (view_ != nullptr) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            internal::file_mapping::unmap(
                const_cast<std::byte*>(view_), view_size_);
            view_      = nullptr;
            view_size_ = 0;
            index_     = npos;
        }
    }

    internal::file_mapping file_;
    // The mapping is a cache of the file content: mapping another window is
    // not observable through a const read-only view, so it stays mutable.
    mutable element_type* view_  = nullptr;
    mutable size_type view_size_ = 0;
    mutable size_type index_     = npos;
    size_type wnd_size_          = 0;
};

/**
 * @brief Random access iterator over the windows of a `file_window`.
 *
 * Dereferencing maps the window, see `file_window`.
 */
template <bool ReadOnly>
class file_window<ReadOnly>::iterator {
public:
    using iterator_concept  = std::random_access_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type        = file_window::value_type;
    using difference_type   = file_window::difference_type;
    using reference         = value_type;
    using pointer           = void;

    iterator() noexcept = default;

    ATOM_ATTRS(ATOM_NODISCARD) reference operator*() const { return (*owner_)[index_]; }

    ATOM_ATTRS(ATOM_NODISCARD) reference operator[](difference_type n) const {
        return (*owner_)[_advance(index_, n)];
    }

    iterator& operator++() noexcept {
        ++index_;
        return *this;
    }

    iterator operator++(int) noexcept {
        auto tmp = *this;
        ++index_;
        return tmp;
    }

    iterator& operator--() noexcept {
        --index_;
        return *this;
    }

    iterator operator--(int) noexcept {
        auto tmp = *this;
        --index_;
        return tmp;
    }

    iterator& operator+=(difference_type n) noexcept {
        index_ = _advance(index_, n);
        return *this;
    }

    iterator& operator-=(difference_type n) noexcept {
        index_ = _advance(index_, -n);
        return *this;
    }

    ATOM_ATTRS(ATOM_NODISCARD) friend iterator
        operator+(iterator it, difference_type n) noexcept {
        return it += n;
    }

    ATOM_ATTRS(ATOM_NODISCARD) friend iterator
        operator+(difference_type n, iterator it) noexcept {
        return it += n;
    }

    ATOM_ATTRS(ATOM_NODISCARD) friend iterator
        operator-(iterator it, difference_type n) noexcept {
        return it -= n;
    }

    ATOM_ATTRS(ATOM_NODISCARD) friend difference_type
        operator-(const iterator& lhs, const iterator& rhs) noexcept {
        return static_cast<difference_type>(lhs.index_ - rhs.index_);
    }

    ATOM_ATTRS(ATOM_NODISCARD) friend bool
        operator==(const iterator& lhs, const iterator& rhs) noexcept {
        return lhs.index_ == rhs.index_;
    }

    ATOM_ATTRS(ATOM_NODISCARD) friend auto
        operator<=>(const iterator& lhs, const iterator& rhs) noexcept {
        return lhs.index_ <=> rhs.index_;
    }

private:
    friend class file_window;

    using _owner_pointer =
        std::conditional_t<ReadOnly, const file_window*, file_window*>;

    iterator(_owner_pointer owner, size_type index) noexcept
        : owner_(owner), index_(index) {}

    static size_type _advance(size_type index, difference_type n) noexcept {
        return index + static_cast<size_type>(n);
    }

    _owner_pointer owner_ = nullptr;
    size_type index_      = 0;
};

} // namespace neutron
