#include <neutron/poly.hpp>
#include <neutron/print.hpp>
#include <neutron/value_list.hpp>

using namespace neutron;

struct Object {
    template <typename Base>
    struct interface : Base {
        void foo() const { this->template invoke<0, Object>(); }
    };

    template <typename Impl>
    using impl = value_list<&Impl::foo>;
};

int main() {
    struct _impl {
        void foo() const { println("called foo() in _impl"); }
    };
    poly<Object> p{ _impl{} };
    p->foo();

    struct _ex_impl : _impl {
        void foo() const { println("called foo() in _ex_impl"); }
    };
    poly<Object> ep{ _ex_impl{} };
    ep->foo();
}
