#pragma once
#include <array>
#include <cstddef>
#include <memory>
#include <meta>
#include <vector>

namespace neutron {

template <typename T, size_t Count>
using struct_of_arrays = [:[] {
    using namespace std::meta;
    struct _impl;
    consteval {
        auto context                  = access_context::current();
        std::vector<info> old_members = nonstatic_data_members_of(^^T, context);
        std::vector<info> new_members;
        for (info member : old_members) {
            auto array_type = substitute(
                ^^std::array, { type_of(member), reflect_constant(Count) });
            auto member_desc =
                data_member_spec(array_type, { .name = identifier_of(member) });
            new_members.emplace_back(member_desc);
        }
        define_aggregate(^^_impl, new_members);
    }
    return ^^_impl;
}():];

template <typename T, typename Alloc = std::allocator<T>>
using struct_of_vectors = [:[] {
    using namespace std::meta;
    struct _impl;
    consteval {
        auto alloc_info     = ^^Alloc;
        auto alloc_template = template_of(alloc_info);
        auto alloc_args     = template_arguments_of(alloc_info);

        auto context                  = access_context::current();
        std::vector<info> old_members = nonstatic_data_members_of(^^T, context);
        std::vector<info> new_members;
        for (info member : old_members) {
            std::vector<info> arg_list;
            arg_list.emplace_back(type_of(member));
            arg_list.insert(
                arg_list.end(), alloc_args.begin() + 1, alloc_args.end());
            auto alloc = substitute(alloc_template, arg_list);
            auto array_type =
                substitute(^^std::vector, { type_of(member), alloc });
            auto member_desc =
                data_member_spec(array_type, { .name = identifier_of(member) });
            new_members.emplace_back(member_desc);
        }
        define_aggregate(^^_impl, new_members);
    }
    return ^^_impl;
}():];

} // namespace neutron
