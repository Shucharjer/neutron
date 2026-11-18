// IWYU pragma: private, include <neutron/fstream.hpp>
#pragma once
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <utility>
#include "neutron/detail/fstream/file_mapping.hpp"
#include "neutron/detail/fstream/is_string.hpp"
#include "neutron/detail/utility/completion_guard.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/**
 * @brief Maps a whole file into memory as a contiguous range of bytes.
 *
 * Writes through a writable map go directly to the file. The behaviour is
 * undefined if the file is truncated by someone else while it is mapped.
 *
 * @tparam ReadOnly Whether the content is mapped read-only.
 */
template <bool ReadOnly>
class file_map {
    using offset_type = internal::file_mapping::offset_type;

public:
    using element_type =
        std::conditional_t<ReadOnly, const std::byte, std::byte>;
    using value_type      = std::byte;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer         = element_type*;
    using const_pointer   = const std::byte*;
    using reference       = element_type&;
    using const_reference = const std::byte&;
    using iterator        = pointer;
    using const_iterator  = const_pointer;

    file_map() noexcept = default;

    /**
     * @brief Map an existing file.
     * @throws std::system_error The file cannot be opened or mapped.
     */
    template <_file_path Path>
    requires ReadOnly
    explicit file_map(const Path& path) {
        open(path);
    }

    /**
     * @brief Map a file for reading and writing.
     * @throws std::system_error The file cannot be opened or mapped.
     */
    template <_file_path Path>
    requires(!ReadOnly)
    explicit file_map(
        const Path& path,
        file_creation creation = file_creation::open_existing) {
        open(path, creation);
    }

    file_map(const file_map&)            = delete;
    file_map& operator=(const file_map&) = delete;

    file_map(file_map&& that) noexcept
        : file_(std::move(that.file_)),
          data_(std::exchange(that.data_, nullptr)) {}

    file_map& operator=(file_map&& that) noexcept {
        if (this != &that) {
            _unmap();
            file_ = std::move(that.file_);
            data_ = std::exchange(that.data_, nullptr);
        }
        return *this;
    }

    ~file_map() noexcept { _unmap(); }

    /// @copydoc file_map(const Path&)
    template <_file_path Path>
    requires ReadOnly
    void open(const Path& path) {
        _open(_path_c_str(path), file_creation::open_existing);
    }

    /// @copydoc file_map(const Path&, file_creation)
    template <_file_path Path>
    requires(!ReadOnly)
    void open(
        const Path& path,
        file_creation creation = file_creation::open_existing) {
        _open(_path_c_str(path), creation);
    }

    void close() noexcept {
        _unmap();
        file_.close();
    }

    ATOM_NODISCARD bool is_open() const noexcept { return file_.is_open(); }

    /**
     * @brief Resize the file and remap it; a grown part reads as zeros.
     *
     * Pointers into the map are invalidated. The size is unchanged if the file
     * cannot be resized; the map is closed if it cannot be remapped.
     * @throws std::system_error
     */
    void resize(size_type size)
    requires(!ReadOnly)
    {
        ATOM_ASSERT(is_open());
        if (size == this->size()) {
            return;
        }
        _unmap();
        auto guard = make_exception_guard([this]() noexcept {
            file_.close();
        });
        std::error_code error;
        file_.resize(size, error);
        _map();
        guard.dismiss();
        if (error) [[unlikely]] {
            internal::_throw_error(error, "file_map::resize");
        }
    }

    /**
     * @brief Write the modified content back to the file.
     * @throws std::system_error
     */
    void flush()
    requires(!ReadOnly)
    {
        ATOM_ASSERT(is_open());
        file_.flush(data_, size(), false);
    }

    ATOM_NODISCARD pointer data() noexcept { return data_; }
    ATOM_NODISCARD const_pointer data() const noexcept { return data_; }

    /// @brief Size of the file in bytes.
    ATOM_NODISCARD size_type size() const noexcept {
        return static_cast<size_type>(file_.size());
    }

    ATOM_NODISCARD bool empty() const noexcept { return size() == 0; }

    ATOM_NODISCARD reference operator[](size_type n) noexcept {
        ATOM_ASSERT(n < size());
        return data_[n];
    }

    ATOM_NODISCARD const_reference operator[](size_type n) const noexcept {
        ATOM_ASSERT(n < size());
        return data_[n];
    }

    /// @throws std::out_of_range `n` is not less than `size()`.
    ATOM_NODISCARD reference at(size_type n) {
        _check_index(n);
        return data_[n];
    }

    /// @throws std::out_of_range `n` is not less than `size()`.
    ATOM_NODISCARD const_reference at(size_type n) const {
        _check_index(n);
        return data_[n];
    }

    ATOM_NODISCARD iterator begin() noexcept { return data_; }
    ATOM_NODISCARD const_iterator begin() const noexcept { return data_; }
    ATOM_NODISCARD const_iterator cbegin() const noexcept { return data_; }

    ATOM_NODISCARD iterator end() noexcept { return data_ + size(); }
    ATOM_NODISCARD const_iterator end() const noexcept {
        return data_ + size();
    }
    ATOM_NODISCARD const_iterator cend() const noexcept { return end(); }

private:
    void _open(const char* path, file_creation creation) {
        close();
        file_.open(path, !ReadOnly, creation);
        auto guard = make_exception_guard([this]() noexcept {
            file_.close();
        });
        _map();
        guard.dismiss();
    }

    void _map() {
        const offset_type size = file_.size();
        if constexpr (sizeof(size_type) < sizeof(offset_type)) {
            if (size > std::numeric_limits<size_type>::max()) [[unlikely]] {
                internal::_throw_error(
                    std::make_error_code(std::errc::value_too_large),
                    "file_map");
            }
        }
        if (size != 0) {
            data_ = static_cast<pointer>(
                file_.map(0, static_cast<size_type>(size)));
        }
    }

    void _unmap() noexcept {
        if (data_ != nullptr) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            internal::file_mapping::unmap(
                const_cast<std::byte*>(data_), size());
            data_ = nullptr;
        }
    }

    void _check_index(size_type n) const {
        if (n >= size()) [[unlikely]] {
            throw std::out_of_range("neutron::file_map: index out of range");
        }
    }

    internal::file_mapping file_;
    pointer data_ = nullptr;
};

} // namespace neutron
