#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>
#include <neutron/interprocess.hpp>

using namespace neutron;

int main() {

    {
        shared_memory shm("__test", 32, read_write);
        const char text[] = "this is a string\n";
        strcpy(static_cast<char*>(shm.data()), text); // NOLINT
        std::ifstream is("/dev/shm/__test");
        std::string line;
        std::getline(is, line);
        std::string_view tmp =
            std::string_view{ text, sizeof(text) - 2 }; // NOLINT
        if (line != tmp) {
            return 1;
        }
    }

    {
        shared_memory shm("__test", 32, read_write);
        std::uint32_t* pu32 = static_cast<std::uint32_t*>(shm.data());
        pu32[7]             = 42;
        shared_memory rshm("__test", 32, read_only);
        if (static_cast<const std::uint32_t*>(rshm.data())[7] != 42) {
            return 1;
        }
    }

    return 0;
}
