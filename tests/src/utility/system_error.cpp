#include <cerrno>
#include <cstring>
#include <system_error>
#include <neutron/utility.hpp>

#if defined(_WIN32)
    #include <windows.h>
#endif

using namespace neutron;

namespace {

template <typename Fn>
bool throws_code(Fn&& fn, int code, const char* what) {
    ATOM_TRY { fn(); }
    ATOM_CATCH(const std::system_error& error) {
        return error.code().value() == code &&
               error.code().category() == std::system_category() &&
               std::strstr(error.what(), what) != nullptr;
    }
    return false;
}

bool test_throw_system_error() {
    return throws_code(
        [] {
            throw_system_error(EINVAL, "explicit");
        },
        EINVAL, "explicit");
}

bool test_throw_last_system_error() {
#if defined(_WIN32)
    constexpr int code = ERROR_FILE_NOT_FOUND;
    ::SetLastError(code);
#else
    constexpr int code = ENOENT;
    errno              = code;
#endif
    if (last_system_error() != code) {
        return false;
    }
    return throws_code(
        [] {
            throw_last_system_error("last");
        },
        code, "last");
}

} // namespace

int main() {
    if (!test_throw_system_error()) {
        return 1;
    }
    if (!test_throw_last_system_error()) {
        return 2;
    }
    return 0;
}
