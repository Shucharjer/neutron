#pragma once
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/metafn/definition.hpp"

namespace neutron::execution {

template <typename Env, typename QueryTag, typename... Args>
concept _has_query = requires(const Env& env, Args&&... args) {
    env.query(QueryTag{}, std::forward<Args>(args)...);
};

template <typename QueryTag, typename Args, typename... Envs>
struct _first_env; // first queryable env, fe

template <
    typename QueryTag, typename... Args, typename CurrE, typename... Others>
requires _has_query<CurrE, QueryTag, Args...>
struct _first_env<QueryTag, type_list<Args...>, CurrE, Others...> {
    using type = CurrE;
};

template <
    typename QueryTag, typename... Args, typename CurrE, typename... Others>
requires(!_has_query<CurrE, QueryTag, Args...>)
struct _first_env<
    QueryTag, type_list<Args...>, CurrE, Others...> { // first queryable env, fe
    using type = _first_env<QueryTag, Others...>;
};

template <typename QueryTag, typename Args, typename... Envs>
using _first_env_t = typename _first_env<QueryTag, Args, Envs...>::type;

template <queryable Env>
struct _env_base {
    Env _env;
};

template <queryable... Envs>
struct env : _env_base<Envs>... {
    template <typename QueryTag, typename... Args>
    requires(_has_query<Envs, QueryTag, Args...> || ...)
    constexpr auto query(QueryTag qry, Args&&... args) const {
        using first_env_t = _first_env_t<QueryTag, type_list<Args...>, Envs...>;
        const auto& env   = static_cast<const first_env_t&>(*this);
        return env.query(qry, std::forward<Args>(args)...);
    }
};

template <class... Envs>
env(Envs...) -> env<std::unwrap_reference_t<Envs>...>;

} // namespace neutron::execution
