// IWYU pragma: private, include <neutron/fstream.hpp>
#pragma once
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <system_error>
#include <utility>
#include "neutron/detail/utility/macros.hpp"
#include "neutron/detail/utility/system_error.hpp"

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#else
    #error "file mapping is not supported on this platform"
#endif

namespace neutron {

/**
 * @brief How a writable file mapping or stream opens its file.
 */
enum class file_creation : unsigned char {
    open_existing,  ///< The file must already exist.
    open_or_create, ///< Create the file if it does not exist.
    truncate,       ///< Create the file, or truncate an existing one to zero.
    create_new,     ///< Create the file; it must not exist yet.
};

namespace internal {

/**
 * @brief Throw `std::system_error` carrying `error`.
 *
 * Calls `std::abort()` instead when exceptions are disabled.
 */
[[noreturn]] inline void _throw_error(std::error_code error, const char* what) {
#if ATOM_ENABLE_EXCEPTIONS
    throw std::system_error(error, what);
#else
    (void)error;
    (void)what;
    std::abort();
#endif
}

/**
 * @brief An open file whose content can be mapped into memory.
 *
 * Views are mapped and unmapped by the owner; the file itself is closed on
 * destruction. Every operation has a `std::error_code` overload; the
 * exceptions-throwing ones are wrappers around them.
 */
class file_mapping {
public:
    using offset_type = std::uint64_t;

#if defined(_WIN32)
    static constexpr offset_type max_file_size =
        static_cast<offset_type>((std::numeric_limits<LONGLONG>::max)());
#else
    static constexpr offset_type max_file_size =
        static_cast<offset_type>((std::numeric_limits<off_t>::max)());
#endif

    file_mapping() noexcept = default;

    file_mapping(const file_mapping&)            = delete;
    file_mapping& operator=(const file_mapping&) = delete;

    file_mapping(file_mapping&& that) noexcept
        :
#if defined(_WIN32)
          file_(std::exchange(that.file_, INVALID_HANDLE_VALUE)),
          mapping_(std::exchange(that.mapping_, nullptr)),
#else
          fd_(std::exchange(that.fd_, -1)),
#endif
          size_(std::exchange(that.size_, 0)),
          writable_(std::exchange(that.writable_, false)) {
    }

    /// Views of this file must be unmapped first.
    file_mapping& operator=(file_mapping&& that) noexcept {
        if (this != &that) {
            close();
#if defined(_WIN32)
            file_    = std::exchange(that.file_, INVALID_HANDLE_VALUE);
            mapping_ = std::exchange(that.mapping_, nullptr);
#else
            fd_ = std::exchange(that.fd_, -1);
#endif
            size_     = std::exchange(that.size_, 0);
            writable_ = std::exchange(that.writable_, false);
        }
        return *this;
    }

    ~file_mapping() noexcept { close(); }

    /**
     * @brief Alignment required for the offset of a view.
     *
     * The page size on POSIX, the allocation granularity on Windows. Always a
     * power of two.
     */
    ATOM_NODISCARD static std::size_t granularity() noexcept {
        static const std::size_t value = _query_granularity();
        return value;
    }

    /**
     * @brief Open `path`, leaving the file closed on failure.
     * @param writable Request write access.
     * @param creation Whether the file must exist and whether it is truncated.
     */
    void open(
        const char* path, bool writable, file_creation creation,
        std::error_code& error) noexcept {
        error.clear();
        if (is_open()) [[unlikely]] {
            error = std::make_error_code(std::errc::device_or_resource_busy);
            return;
        }
#if defined(_WIN32)
        DWORD disposition = OPEN_EXISTING;
        if (creation == file_creation::open_or_create) {
            disposition = OPEN_ALWAYS;
        } else if (creation == file_creation::truncate) {
            disposition = CREATE_ALWAYS;
        } else if (creation == file_creation::create_new) {
            disposition = CREATE_NEW;
        }
        const DWORD access =
            writable ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
        const HANDLE file = ::CreateFileA(
            path, access,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            error.assign(last_system_error(), std::system_category());
            return;
        }
        LARGE_INTEGER size{};
        if (::GetFileSizeEx(file, &size) == FALSE) {
            const int code = last_system_error();
            ::CloseHandle(file);
            error.assign(code, std::system_category());
            return;
        }
        file_ = file;
        size_ = static_cast<offset_type>(size.QuadPart);
#else
        int flags = (writable ? O_RDWR : O_RDONLY) | O_CLOEXEC;
        if (creation == file_creation::open_or_create) {
            flags |= O_CREAT;
        } else if (creation == file_creation::truncate) {
            flags |= O_CREAT | O_TRUNC;
        } else if (creation == file_creation::create_new) {
            flags |= O_CREAT | O_EXCL;
        }
        constexpr mode_t mode =
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; // 0666

        int fd = -1;
        do {
            fd = ::open(path, flags, mode); // NOLINT(*-pro-type-vararg)
        } while (fd == -1 && errno == EINTR);
        if (fd == -1) {
            error.assign(errno, std::system_category());
            return;
        }
        struct stat buf{};
        if (::fstat(fd, &buf) == -1) {
            const int code = errno;
            ::close(fd);
            error.assign(code, std::system_category());
            return;
        }
        if (S_ISDIR(buf.st_mode)) {
            ::close(fd);
            error = std::make_error_code(std::errc::is_a_directory);
            return;
        }
        fd_   = fd;
        size_ = static_cast<offset_type>(buf.st_size);
#endif
        writable_ = writable;
    }

    /// @copydoc open(const char*, bool, file_creation, std::error_code&)
    /// @throws std::system_error
    void open(const char* path, bool writable, file_creation creation) {
        std::error_code error;
        open(path, writable, creation, error);
        if (error) [[unlikely]] {
            _throw_error(error, "file_mapping::open");
        }
    }

    /// Views of this file must be unmapped first.
    void close() noexcept {
#if defined(_WIN32)
        if (mapping_ != nullptr) {
            ::CloseHandle(mapping_);
            mapping_ = nullptr;
        }
        if (file_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(file_);
            file_ = INVALID_HANDLE_VALUE;
        }
#else
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
#endif
        size_     = 0;
        writable_ = false;
    }

    ATOM_NODISCARD bool is_open() const noexcept {
#if defined(_WIN32)
        return file_ != INVALID_HANDLE_VALUE;
#else
        return fd_ != -1;
#endif
    }

    ATOM_NODISCARD offset_type size() const noexcept { return size_; }

    /**
     * @brief Change the size of the file; a grown part reads as zeros.
     *
     * Views of this file must be unmapped first. The size is unchanged on
     * failure.
     */
    void resize(offset_type size, std::error_code& error) noexcept {
        error.clear();
        if (!is_open() || !writable_) [[unlikely]] {
            error = std::make_error_code(std::errc::bad_file_descriptor);
            return;
        }
        if (size > max_file_size) [[unlikely]] {
            error = std::make_error_code(std::errc::file_too_large);
            return;
        }
#if defined(_WIN32)
        // The mapping object has a fixed size; it is recreated on demand.
        if (mapping_ != nullptr) {
            ::CloseHandle(mapping_);
            mapping_ = nullptr;
        }
        FILE_END_OF_FILE_INFO info{};
        info.EndOfFile.QuadPart = static_cast<LONGLONG>(size);
        if (::SetFileInformationByHandle(
                file_, FileEndOfFileInfo, &info, sizeof(info)) == FALSE) {
            error.assign(last_system_error(), std::system_category());
            return;
        }
#else
        int result = -1;
        do {
            result = ::ftruncate(fd_, static_cast<off_t>(size));
        } while (result == -1 && errno == EINTR);
        if (result == -1) {
            error.assign(errno, std::system_category());
            return;
        }
#endif
        size_ = size;
    }

    /**
     * @brief Map `length` bytes starting at `offset`.
     * @return The view, or null with `error` set.
     * @pre `offset` is a multiple of `granularity()`, `length` is nonzero and
     * the range lies within the file.
     */
    ATOM_NODISCARD void*
        map(offset_type offset, std::size_t length,
            std::error_code& error) const noexcept {
        error.clear();
        if (!is_open() || length == 0 || offset % granularity() != 0 ||
            offset > size_ || length > size_ - offset) [[unlikely]] {
            error = std::make_error_code(std::errc::invalid_argument);
            return nullptr;
        }
#if defined(_WIN32)
        if (mapping_ == nullptr) {
            mapping_ = ::CreateFileMappingA(
                file_, nullptr, writable_ ? PAGE_READWRITE : PAGE_READONLY, 0,
                0, nullptr);
            if (mapping_ == nullptr) {
                error.assign(last_system_error(), std::system_category());
                return nullptr;
            }
        }
        void* const addr = ::MapViewOfFile(
            mapping_, writable_ ? FILE_MAP_WRITE : FILE_MAP_READ,
            static_cast<DWORD>(offset >> 32U),
            static_cast<DWORD>(offset & 0xFFFFFFFFU), length);
        if (addr == nullptr) {
            error.assign(last_system_error(), std::system_category());
            return nullptr;
        }
        return addr;
#else
        const int prot   = writable_ ? (PROT_READ | PROT_WRITE) : PROT_READ;
        void* const addr = ::mmap(
            nullptr, length, prot, MAP_SHARED, fd_, static_cast<off_t>(offset));
        if (addr == MAP_FAILED) {
            error.assign(errno, std::system_category());
            return nullptr;
        }
        return addr;
#endif
    }

    /// @copydoc map(offset_type, std::size_t, std::error_code&) const
    /// @throws std::system_error
    ATOM_NODISCARD void* map(offset_type offset, std::size_t length) const {
        std::error_code error;
        void* const addr = map(offset, length, error);
        if (error) [[unlikely]] {
            _throw_error(error, "file_mapping::map");
        }
        return addr;
    }

    static void unmap(void* addr, std::size_t length) noexcept {
#if defined(_WIN32)
        (void)length;
        ::UnmapViewOfFile(addr);
#else
        ::munmap(addr, length);
#endif
    }

    /**
     * @brief Write the modified pages of a view back to the file.
     * @param addr The view, may be null.
     * @param sync_file Also flush pages of views that are already unmapped.
     */
    void flush(
        void* addr, std::size_t length, bool sync_file,
        std::error_code& error) const noexcept {
        error.clear();
#if defined(_WIN32)
        (void)sync_file;
        if (addr != nullptr && ::FlushViewOfFile(addr, length) == FALSE) {
            error.assign(last_system_error(), std::system_category());
            return;
        }
        // FlushViewOfFile does not wait for the data to reach the disk.
        if (::FlushFileBuffers(file_) == FALSE) {
            error.assign(last_system_error(), std::system_category());
        }
#else
        if (addr != nullptr && ::msync(addr, length, MS_SYNC) == -1) {
            error.assign(errno, std::system_category());
            return;
        }
        if (sync_file && ::fsync(fd_) == -1) {
            error.assign(errno, std::system_category());
        }
#endif
    }

    /// @copydoc flush(void*, std::size_t, bool, std::error_code&) const
    /// @throws std::system_error
    void flush(void* addr, std::size_t length, bool sync_file) const {
        std::error_code error;
        flush(addr, length, sync_file, error);
        if (error) [[unlikely]] {
            _throw_error(error, "file_mapping::flush");
        }
    }

private:
    static std::size_t _query_granularity() noexcept {
#if defined(_WIN32)
        SYSTEM_INFO info{};
        ::GetSystemInfo(&info);
        const auto value =
            static_cast<std::size_t>(info.dwAllocationGranularity);
#else
        const auto value = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
#endif
        ATOM_ASSERT(std::has_single_bit(value));
        return value;
    }

#if defined(_WIN32)
    HANDLE file_ = INVALID_HANDLE_VALUE;
    // Created on first use, so a const mapping can still be read.
    mutable HANDLE mapping_ = nullptr;
#else
    int fd_ = -1;
#endif
    offset_type size_ = 0;
    bool writable_    = false;
};

} // namespace internal

} // namespace neutron
