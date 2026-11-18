#pragma once
#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include "neutron/detail/utility/macros.hpp"

namespace neutron_test {

/**
 * @file
 * @brief A uniquely named file in the temporary directory.
 *
 * The file itself is not created; it is removed again when the object is
 * destroyed, whether or not it exists at that point.
 */
class temp_file {
public:
    explicit temp_file(const char* tag) {
        static std::atomic<unsigned long long> counter{ 0 };
        const auto stamp = static_cast<unsigned long long>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        std::string name = "neutron_";
        name.append(tag);
        name.push_back('_');
        name.append(std::to_string(stamp));
        name.push_back('_');
        name.append(
            std::to_string(counter.fetch_add(1, std::memory_order_relaxed)));
        path_ = std::filesystem::temp_directory_path() / name;
    }

    temp_file(const temp_file&)            = delete;
    temp_file& operator=(const temp_file&) = delete;

    ~temp_file() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    ATOM_NODISCARD const std::filesystem::path& path() const noexcept {
        return path_;
    }

    /// @brief The path as a string, accepted by the neutron file classes.
    ATOM_NODISCARD std::string string() const { return path_.string(); }

private:
    std::filesystem::path path_;
};

} // namespace neutron_test
