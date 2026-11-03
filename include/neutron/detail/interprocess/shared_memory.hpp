// IWYU pragma: private, include <neutron/interprocess.hpp>
#pragma once

#include <cerrno>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "neutron/detail/utility/completion_guard.hpp"
#include "neutron/detail/utility/macros.hpp"
#include "neutron/detail/utility/system_error.hpp"

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
#else
    #error "shared_memory is not supported on this platform"
#endif

namespace neutron {

inline constexpr struct read_only_t {
} read_only;
inline constexpr struct read_write_t {
} read_write;

/**
 * @brief A named memory region shared between processes.
 *
 * A read-write instance creates the region if it does not exist yet. Unless
 * `keep` is set, the name is removed when that instance is destroyed; existing
 * mappings stay valid until they are closed. A read-only instance never
 * removes the name.
 *
 * On Windows the region is destroyed once its last handle is closed, so
 * `keep` has no effect there.
 */
class shared_memory {
public:
    using size_type = std::size_t;

    /**
     * @brief Map an existing region for reading.
     * @throws std::invalid_argument `size` is zero.
     * @throws std::runtime_error The region is smaller than `size`.
     * @throws std::system_error The region cannot be opened or mapped.
     */
    shared_memory(std::string_view name, size_type size, read_only_t)
        : name_(_normalize(name)) {
        _check_size(size);
#if defined(_WIN32)
        handle_ = ::OpenFileMappingA(FILE_MAP_READ, FALSE, name_.c_str());
        if (handle_ == nullptr) {
            throw_last_system_error("OpenFileMapping");
        }
        auto guard = make_exception_guard([this]() noexcept {
            _close();
        });
        _map(FILE_MAP_READ, size);
        guard.dismiss();
#else
        const int fd = ::shm_open(name_.c_str(), O_RDONLY, 0);
        if (fd == -1) {
            throw_last_system_error("shm_open");
        }
        _map_fd(fd, size, PROT_READ);
#endif
    }

    /**
     * @brief Create or open a region for reading and writing.
     * @param keep Keep the name after this instance is destroyed (POSIX only).
     * @throws std::invalid_argument `size` is zero.
     * @throws std::runtime_error An existing region is smaller than `size`.
     * @throws std::system_error The region cannot be created or mapped.
     */
    shared_memory(
        std::string_view name, size_type size, read_write_t, bool keep = false)
        : name_(_normalize(name)) {
        _check_size(size);
#if defined(_WIN32)
        (void)keep;
        const auto high =
            static_cast<DWORD>(static_cast<unsigned long long>(size) >> 32U);
        const auto low = static_cast<DWORD>(size & 0xFFFFFFFFU);
        handle_        = ::CreateFileMappingA(
            INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, high, low,
            name_.c_str());
        if (handle_ == nullptr) {
            throw_last_system_error("CreateFileMapping");
        }
        auto guard = make_exception_guard([this]() noexcept {
            _close();
        });
        _map(FILE_MAP_ALL_ACCESS, size);
        guard.dismiss();
#else
        constexpr mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0644

        bool created = true;
        int fd = ::shm_open(name_.c_str(), O_RDWR | O_CREAT | O_EXCL, mode);
        if (fd == -1 && errno == EEXIST) {
            created = false;
            fd      = ::shm_open(name_.c_str(), O_RDWR, mode);
        }
        if (fd == -1) {
            throw_last_system_error("shm_open");
        }

        auto unlink_guard = make_exception_guard([this, created]() noexcept {
            if (created) {
                ::shm_unlink(name_.c_str());
            }
        });
        // Only a freshly created region is resized: macOS allows ftruncate
        // on a shared memory object just once.
        if (created && ::ftruncate(fd, static_cast<off_t>(size)) == -1) {
            const int error = errno;
            ::close(fd);
            throw_system_error(error, "ftruncate");
        }
        _map_fd(fd, size, PROT_READ | PROT_WRITE);
        unlink_guard.dismiss();

        unlink_ = !keep;
#endif
    }

    shared_memory(const shared_memory&)            = delete;
    shared_memory& operator=(const shared_memory&) = delete;

    shared_memory(shared_memory&& that) noexcept
        : addr_(std::exchange(that.addr_, nullptr)),
          size_(std::exchange(that.size_, 0)), name_(std::move(that.name_)),
#if defined(_WIN32)
          handle_(std::exchange(that.handle_, nullptr))
#else
          unlink_(std::exchange(that.unlink_, false))
#endif
    {
    }

    shared_memory& operator=(shared_memory&& that) noexcept {
        if (this != &that) {
            _close();
            addr_ = std::exchange(that.addr_, nullptr);
            size_ = std::exchange(that.size_, 0);
            name_ = std::move(that.name_);
#if defined(_WIN32)
            handle_ = std::exchange(that.handle_, nullptr);
#else
            unlink_ = std::exchange(that.unlink_, false);
#endif
        }
        return *this;
    }

    ~shared_memory() noexcept { _close(); }

    ATOM_NODISCARD void* data() noexcept { return addr_; }
    ATOM_NODISCARD const void* data() const noexcept { return addr_; }

    ATOM_NODISCARD size_type size() const noexcept { return size_; }

    ATOM_NODISCARD auto span() noexcept -> std::span<std::byte> {
        return { static_cast<std::byte*>(addr_), size_ };
    }
    ATOM_NODISCARD auto span() const noexcept -> std::span<const std::byte> {
        return { static_cast<const std::byte*>(addr_), size_ };
    }

private:
    static void _check_size(size_type size) {
        if (size == 0) [[unlikely]] {
            throw std::invalid_argument("shared_memory: size must be nonzero");
        }
    }

    static std::string _normalize(std::string_view name) {
#if defined(_WIN32)
        return std::string(name);
#else
        // POSIX only specifies the behaviour of names with a leading slash.
        if (name.starts_with('/')) {
            return std::string(name);
        }
        std::string result;
        result.reserve(name.size() + 1);
        result.push_back('/');
        result.append(name);
        return result;
#endif
    }

#if defined(_WIN32)
    void _map(DWORD access, size_type size) {
        addr_ = ::MapViewOfFile(handle_, access, 0, 0, 0);
        if (addr_ == nullptr) {
            throw_last_system_error("MapViewOfFile");
        }
        MEMORY_BASIC_INFORMATION info{};
        if (::VirtualQuery(addr_, &info, sizeof(info)) == 0) {
            throw_last_system_error("VirtualQuery");
        }
        if (info.RegionSize < size) {
            throw std::runtime_error("shared_memory: size mismatch");
        }
        size_ = size;
    }
#else
    /// Takes ownership of `fd`; it is always closed before returning.
    void _map_fd(int fd, size_type size, int prot) {
        struct stat buf{};
        if (::fstat(fd, &buf) == -1) {
            const int error = errno;
            ::close(fd);
            throw_system_error(error, "fstat");
        }
        // macOS rounds the object size up to a page boundary.
        if (static_cast<std::make_unsigned_t<off_t>>(buf.st_size) < size) {
            ::close(fd);
            throw std::runtime_error("shared_memory: size mismatch");
        }

        void* const addr = ::mmap(nullptr, size, prot, MAP_SHARED, fd, 0);
        const int error  = errno;
        ::close(fd); // the mapping keeps the object alive
        if (addr == MAP_FAILED) {
            throw_system_error(error, "mmap");
        }
        addr_ = addr;
        size_ = size;
    }
#endif

    void _close() noexcept {
#if defined(_WIN32)
        if (addr_ != nullptr) {
            ::UnmapViewOfFile(addr_);
        }
        if (handle_ != nullptr) {
            ::CloseHandle(handle_);
        }
        handle_ = nullptr;
#else
        if (addr_ != nullptr) {
            ::munmap(addr_, size_);
        }
        if (unlink_) {
            ::shm_unlink(name_.c_str());
        }
        unlink_ = false;
#endif
        addr_ = nullptr;
        size_ = 0;
    }

    void* addr_     = nullptr;
    size_type size_ = 0;
    std::string name_;
#if defined(_WIN32)
    HANDLE handle_ = nullptr;
#else
    bool unlink_ = false;
#endif
};

} // namespace neutron
