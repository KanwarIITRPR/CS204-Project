#ifndef IAG_H
#define IAG_H

#include "simulator.hpp"

class IAG {
    private:
        
    public:
        const int INSTRUCTION_SIZE = 4;
        uint32_t PC, buffer_PC;
        void UpdateBuffer();

        void UpdatePC();
};

#endif