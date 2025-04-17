#ifndef CONTROL_H
#define CONTROL_H

#include "simulator.hpp"

class ControlCircuit {
    public:
        void IncrementClock() { clock += 1; }
        uint32_t CyclesExecuted() { return clock; }

        void UpdateControlSignals();
        void UpdateDecodeSignals();
        void UpdateExecuteSignals();
        void UpdateMemorySignals();
        void UpdateWritebackSignals();

        uint8_t MuxA = 0;
        uint8_t MuxB = 0;
        uint8_t ALU = 0;
        uint8_t MuxZ = 0;
        uint8_t MuxY = 0;
        uint8_t MuxMA = 0;
        uint8_t MuxMD = 0;
        uint8_t DemuxMD = 0;
        uint8_t MuxINC = 0;
        uint8_t MuxPC = 0;
        uint8_t EnableRegisterFile = 1;
        
    private:
        PipelinedSimulator simulator;
        uint32_t clock = 0;
};

#endif