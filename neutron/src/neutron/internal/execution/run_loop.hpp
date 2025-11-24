#pragma once
#include "./fwd.hpp"

namespace neutron::execution {

class run_loop {
    class run_loop_scheduler;
    class run_loop_sender;
    class run_loop_opstate_base;
    template <typename Rcvr>
    class run_loop_opstate;

    run_loop_opstate_base* _pop_front();
    void _push_back(run_loop_opstate_base*);

public:
    run_loop() noexcept                  = default;
    run_loop(const run_loop&)            = delete;
    run_loop(run_loop&&)                 = delete;
    run_loop& operator=(const run_loop&) = delete;
    run_loop& operator=(run_loop&&)      = delete;
    ~run_loop();

    run_loop_scheduler get_scheduler();
    void run();
    void finish();
};

} // namespace neutron::execution
