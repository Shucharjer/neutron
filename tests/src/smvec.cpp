// Basic coverage for neutron::smvec
#include <memory>
#include <string>
#include <vector>
#include <neutron/print.hpp>
#include <neutron/smvec.hpp>
#include "require.hpp"

using namespace neutron;

// helpers
template <typename T, size_t N>
static void fill_iota(smvec<T, N>& v, size_t n) {
    v.clear();
    for (size_t i = 0; i < n; ++i)
        v.push_back(static_cast<T>(i));
}

static void test_constructors() {
    // default
    {
        smvec<int, 4> smv;
        require(smv.size() == 0);
        require(smv.capacity() == 4);
    }

    // initializer_list prefers element-list
    {
        smvec<int, 4> smv{ 3, 4, 4 };
        require(smv.size() == 3);
        require(smv[0] == 3);
        require(smv[1] == 4);
        require(smv[2] == 4);
    }

    // size constructor (value-init)
    {
        smvec<int, 8> smv(5);
        require(smv.size() == 5);
        int sum = 0;
        for (auto x : smv)
            sum += x;
        require(sum == 0);
    }

    // size + value constructor
    {
        smvec<int, 4> smv(6, 7);
        require(smv.size() == 6);
        for (auto x : smv)
            require(x == 7);
        require(smv.capacity() >= 6);
    }

    // range constructor
    {
        std::vector<int> base{ 1, 2, 3, 4, 5 };
        smvec<int, 16> smv(base.begin(), base.end());
        require(smv.size() == base.size());
        for (size_t i = 0; i < base.size(); ++i)
            require(smv[i] == base[i]);
    }
}

static void test_push_pop_resize_reserve() {
    smvec<int, 4> v;
    require(v.size() == 0);
    require(v.capacity() == 4);

    // push up to SBO
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    require(v.size() == 4);
    require(v.front() == 1);
    require(v.back() == 4);

    // trigger growth (move out of SBO)
    v.push_back(5);
    require(v.size() == 5);
    require(v.capacity() >= 8); // doubled from SBO
    require(v[0] == 1 && v[4] == 5);

    // pop
    v.pop_back();
    require(v.size() == 4);
    require(v.back() == 4);

    // reserve larger
    v.reserve(64);
    require(v.capacity() >= 64);
    require(v.size() == 4);
    require(v[0] == 1 && v[3] == 4);

    // resize grow default-init
    v.resize(10);
    require(v.size() == 10);
    require(v[0] == 1 && v[3] == 4);

    // resize grow with value
    v.resize(12, 9);
    require(v.size() == 12);
    require(v[10] == 9 && v[11] == 9);

    // shrink
    v.resize(2);
    require(v.size() == 2);
    require(v.back() == 2);
}

static void test_insert_erase_assign() {
    smvec<int, 4> v;
    fill_iota(v, 5); // [0,1,2,3,4]

    // insert single at begin
    v.insert(
        static_cast<smvec<int, 4>::const_iterator>(v.begin()),
        42); // [42,0,1,2,3,4]
    require(v.size() == 6);
    require(v.front() == 42);

    // insert count in middle
    v.insert(
        static_cast<smvec<int, 4>::const_iterator>(v.begin() + 3), 2,
        7); // [42,0,1,7,7,2,3,4]
    require(v.size() == 8);
    require(v[3] == 7 && v[4] == 7);

    // insert range at end
    int more[3] = { 9, 8, 6 };
    v.insert(
        static_cast<smvec<int, 4>::const_iterator>(v.end()), std::begin(more),
        std::end(more));
    require(v.size() == 11);
    require(v.back() == 6);

    // insert ilist somewhere
    v.insert(
        static_cast<smvec<int, 4>::const_iterator>(v.begin() + 2), { 1, 1, 1 });
    require(v.size() == 14);
    require(v[2] == 1 && v[3] == 1 && v[4] == 1);

    // erase single
    v.erase(static_cast<smvec<int, 4>::const_iterator>(v.begin()));
    require(v.size() == 13);

    // erase range
    auto it = v.erase(
        static_cast<smvec<int, 4>::const_iterator>(v.begin() + 1),
        static_cast<smvec<int, 4>::const_iterator>(v.begin() + 4));
    require(static_cast<size_t>(it.base() - v.data()) == 1);
    require(v.size() == 10);

    // assign count
    v.assign(3, 5);
    require(v.size() == 3);
    for (auto x : v)
        require(x == 5);

    // assign range
    std::vector<int> base{ 7, 7, 8, 8, 9 };
    v.assign(base.begin(), base.end());
    require(v.size() == base.size());
    for (size_t i = 0; i < base.size(); ++i)
        require(v[i] == base[i]);

    // assign ilist
    v.assign({ 1, 2, 3, 4, 5, 6, 7 });
    require(v.size() == 7);
    require(v.front() == 1 && v.back() == 7);
}

static void test_copy_move_swap_shrink() {
    // copy/move on SBO case
    smvec<int, 8> a{ 1, 2, 3, 4 };
    smvec<int, 8> b = a; // copy
    require(b.size() == a.size() && b[0] == 1 && b[3] == 4);

    smvec<int, 8> c = std::move(a); // move
    require(c.size() == 4 && c[0] == 1 && c[3] == 4);

    // copy/move on heap case
    smvec<int, 4> x;
    fill_iota(x, 16);
    smvec<int, 4> y = x; // copy
    require(y.size() == 16 && y[0] == 0 && y[15] == 15);

    smvec<int, 4> z = std::move(x); // move
    require(z.size() == 16 && z[5] == 5 && z[15] == 15);

    // swap
    smvec<int, 4> s1{ 1, 2 };
    smvec<int, 4> s2{ 7, 8, 9, 10, 11 };
    s1.swap(s2);
    require(s1.size() == 5 && s1[0] == 7 && s1.back() == 11);
    require(s2.size() == 2 && s2[0] == 1 && s2[1] == 2);

    // shrink_to_fit (heap -> exact or SBO)
    smvec<int, 4> w;
    fill_iota(w, 20);
    require(w.capacity() >= 20);
    w.resize(3);
    w.shrink_to_fit();
    // when size <= Count, it should be SBO
    require(w.size() == 3);
    require(w.capacity() == 4);

    fill_iota(w, 10);
    w.resize(7);
    const auto old_cap = w.capacity();
    w.shrink_to_fit();
    require(w.capacity() <= old_cap);
    require(w.capacity() >= w.size());
}

static void test_iterators_and_emplace() {
    smvec<std::string, 4> v;
    v.emplace_back("hello");
    v.emplace_back(3, 'x');
    require(v.size() == 2);
    require(v[0] == "hello");
    require(v[1] == std::string("xxx"));

    size_t count = 0;
    for (auto it = v.begin(); it != v.end(); ++it) {
        ++count;
    }
    require(count == v.size());
}

int main() {
    test_constructors();
    println("constructors ok");
    test_push_pop_resize_reserve();
    println("push pop resize reserve ok");
    test_insert_erase_assign();
    println("insert erase assign ok");
    test_copy_move_swap_shrink();
    println("copy move swap shrink ok");
    test_iterators_and_emplace();
    println("iterator emplace ok");
    return 0;
}
