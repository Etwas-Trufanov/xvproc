#pragma once
#include <cstdlib>

namespace memory {
    class ram {
        private:
            int m[4096];
        public:
            int get_from_memory(std::size_t address) {
                if (address > 4095 or address < 0) {
                    return 0;
                }
                return m[address];
            }

            void set_to_memory(std::size_t address, int value) {
                if (!(address > 4095 or address < 0)) {
                    m[address] = value;
                }
            }

    };
}
