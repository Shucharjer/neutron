// IWYU pragma: private, include <neutron/interprocess.hpp>
#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <neutron/utility.hpp>
#include "neutron/detail/macros.hpp"

#if defined(__linux) || defined(__linux__)
    #include <fcntl.h>
    #include <sys/file.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>

#elif defined(_WIN32)

#else

#endif

namespace neutron {

inline constexpr struct read_only_t {
} read_only;
inline constexpr struct read_write_t {
} read_write;

class shared_memory {
public:
    using size_type = std::size_t;

    shared_memory(std::string_view name, std::size_t size, read_only_t);
    shared_memory(
        std::string_view name, std::size_t size, read_write_t,
        bool keep = false);

    shared_memory(const shared_memory&)            = delete;
    shared_memory& operator=(const shared_memory&) = delete;

    shared_memory(shared_memory&&) noexcept;
    shared_memory& operator=(shared_memory&&) noexcept;

    ~shared_memory() noexcept;

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
    bool keep_        = false;
    int fd_           = -1;
    void* addr_       = nullptr;
    std::size_t size_ = 0;
    std::string name_;
};

inline void _throw_last_system_error() {
    throw std::system_error(std::error_code(errno, std::system_category()));
}
inline void _throw_last_system_error(const char* what) {
    throw std::system_error(
        std::error_code(errno, std::system_category()), what);
}
inline void _throw_last_system_error(const std::string& what) {
    throw std::system_error(
        std::error_code(errno, std::system_category()), what);
}

#if defined(__linux) || defined(__linux__)

shared_memory::shared_memory(
    std::string_view name, std::size_t size, read_only_t)
    : fd_(shm_open(name.data(), O_RDONLY, 0)) // NOLINT
{
    if (fd_ == -1) {
        _throw_last_system_error("shm_open error");
    }

    auto guard = make_exception_guard([name]() noexcept {
        shm_unlink(name.data()); // NOLINT
    });

    struct stat buf; // NOLINT
    if (fstat(fd_, &buf) == -1) {
        _throw_last_system_error("fstat error");
    }

    if (buf.st_size != size) {
        throw std::runtime_error("shm size not match");
    }

    addr_ = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd_, 0);
    if (addr_ == MAP_FAILED) {
        _throw_last_system_error("mmap error");
    }

    size_ = size;
    auto memguard =
        make_exception_guard([this]() noexcept { munmap(addr_, size_); });

    name_ = name;

    guard.dismiss();
    memguard.dismiss();
}

shared_memory::shared_memory(
    std::string_view name, std::size_t size, read_write_t, bool keep)
    : fd_(shm_open(
          name.data(),                          // NOLINT
          O_RDWR | O_CREAT,
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH // 0644
          )) {
    if (fd_ == -1) {
        _throw_last_system_error("shm_open error");
    }

    auto guard = make_exception_guard([name]() noexcept {
        shm_unlink(name.data()); // NOLINT
    });

    struct stat buf; // NOLINT
    if (fstat(fd_, &buf) == -1) {
        _throw_last_system_error("fstat error");
    }

    if (buf.st_size != size) {
        if (ftruncate(fd_, static_cast<off_t>(size)) == -1) {
            _throw_last_system_error();
        }
    }

    addr_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (addr_ == MAP_FAILED) {
        _throw_last_system_error("mmap error");
    }

    size_ = size;
    auto memguard =
        make_exception_guard([this]() noexcept { munmap(addr_, size_); });

    name_ = name;

    guard.dismiss();
    memguard.dismiss();
}

shared_memory::~shared_memory() noexcept {
    if (fd_ != -1) {
        munmap(addr_, size_);
        if (!keep_) {
            shm_unlink(name_.c_str());
        }
    }
}

#elif defined(_WIN32) || defined(_WIN64)
#else
#endif

} // namespace neutron
