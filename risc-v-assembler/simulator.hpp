#include "utils.cpp"
#include "main.cpp"

#define TEXT_SEGMENT_END "END-OF-TEXT-SEGMENT"
#define DATA_SEGMENT_END "END-OF-DATA-SEGMENT"

enum Stage {
    QUEUED,
    FETCH,
    DECODE,
    EXECUTE,
    MEMORY_ACCESS,
    WRITEBACK,
    COMMITTED
};

struct InterStageRegisters {
    uint32_t RA, RB;
    uint32_t RM;
    uint32_t RY, RZ;
};