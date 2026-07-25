#include <cstddef>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

struct member_gettible {
    int val;
    template <std::size_t I>
    auto& get() {
        return val;
    }
    template <std::size_t I>
    const auto& get() const {
        return val;
    }
};
struct nothrow_member_gettible {
    int val;
    template <std::size_t I>
    auto& get() noexcept {
        return val;
    }
    template <std::size_t I>
    const auto& get() const noexcept {
        return val;
    }
};

namespace test_case {
struct adl_gettible {
    int val;
};

template <std::size_t I>
int& get(adl_gettible& obj) {
    return obj.val;
}

template <std::size_t I>
const int& get(const adl_gettible& obj) {
    return obj.val;
}

struct nothrow_adl_gettible {
    int val;
};

template <std::size_t I>
int& get(nothrow_adl_gettible& obj) noexcept {
    return obj.val;
}

template <std::size_t I>
const int& get(const nothrow_adl_gettible& obj) noexcept {
    return obj.val;
}

} // namespace test_case

int main() {

    // member function get
    {
        { require_or_return(get<0>(::member_gettible{ 8 }) == 8, 1); }
        static_assert(!_get::_has_nothrow_member_get<::member_gettible, 0>);

        { require_or_return(get<0>(::nothrow_member_gettible{ 8 }) == 8, 1); }
        static_assert(
            _get::_has_nothrow_member_get<::nothrow_member_gettible, 0>);
    }

    // adl get
    {
        { require_or_return(get<0>(::test_case::adl_gettible{ 8 }) == 8, 1); }
        static_assert(_get::_has_adl_get<::test_case::adl_gettible, 0>);
        static_assert(
            !_get::_has_nothrow_adl_get<::test_case::adl_gettible, 0>);

        {
            require_or_return(
                get<0>(::test_case::nothrow_adl_gettible{ 8 }) == 8, 1);
        }
        static_assert(
            _get::_has_nothrow_adl_get<::test_case::nothrow_adl_gettible, 0>);
    }

    // std get
    {}

    // reflection get
    {}

    return 0;
}
