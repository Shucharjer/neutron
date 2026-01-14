#include <neutron/small_vector.hpp>

using namespace neutron;

void test_constructors() {

    // default
    { constexpr small_vector<int, 4> smv; }

    { small_vector<int, 4> smv{ 3, 4, 4 }; }
}

int main() { 
    test_constructors();
    
    return 0; }
