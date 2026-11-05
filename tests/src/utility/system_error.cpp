#include <cerrno>
#include <cstring>
#include <optional>
#include <system_error>
#include <neutron/utility.hpp>
#include "require.hpp"

#if defined(_WIN32)
    #include <windows.h>
#endif

using namespace neutron;

namespace {

/// Runs `fn` and returns the std::system_error it throws, if any.
template <typename Fn>
std::optional<std::system_error> caught_system_error(Fn&& fn) {
    ATOM_TRY { fn(); }
    ATOM_CATCH(const std::system_error& error) { return error; }
    return std::nullopt;
}

/// Error code stored as the platform's last error.
#if defined(_WIN32)
constexpr int platform_code = ERROR_FILE_NOT_FOUND;
#else
constexpr int platform_code = ENOENT;
#endif

void set_last_error(int code) noexcept {
#if defined(_WIN32)
    ::SetLastError(static_cast<DWORD>(code));
#else
    errno = code;
#endif
}

void test_throw_system_error_carries_code_and_message() {
    auto error = caught_system_error([] {
        throw_system_error(EINVAL, "explicit");
    });

    require_or_return(error.has_value(), );
    require(error->code().value() == EINVAL);
    require(error->code().category() == std::system_category());
    require(std::strstr(error->what(), "explicit") != nullptr);
}

void test_last_system_error_reads_the_platform_error() {
    set_last_error(platform_code);

    require(last_system_error() == platform_code);
}

void test_throw_last_system_error_carries_code_and_message() {
    set_last_error(platform_code);

    auto error = caught_system_error([] {
        throw_last_system_error("last");
    });

    require_or_return(error.has_value(), );
    require(error->code().value() == platform_code);
    require(error->code().category() == std::system_category());
    require(std::strstr(error->what(), "last") != nullptr);
}

} // namespace

int main() {
    test_throw_system_error_carries_code_and_message();
    test_last_system_error_reads_the_platform_error();
    test_throw_last_system_error_carries_code_and_message();
    return neutron_test::result();
}
