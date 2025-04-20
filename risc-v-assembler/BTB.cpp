#include "utils.hpp"

enum class PredictionState {
    STRONGLY_NOT_TAKEN,
    WEAKLY_NOT_TAKEN,
    WEAKLY_TAKEN,
    STRONGLY_TAKEN
};

// Pattern History Table class
class PHT {
private:
    map<uint32_t, PredictionState> predictionTable;
    const PredictionState initialPrediction = PredictionState::WEAKLY_TAKEN;

public:
    // Get prediction for a given PC
    bool getPrediction(uint32_t pc) {
        // Return true if prediction is taken (strongly or weakly), false otherwise
        switch (getCurrentState(pc)) {
            case PredictionState::STRONGLY_TAKEN: return true;
            case PredictionState::WEAKLY_TAKEN: return true;
            case PredictionState::WEAKLY_NOT_TAKEN: return false;
            case PredictionState::STRONGLY_NOT_TAKEN: return false;
            default: return false;
        }
    }

    bool isMisprediction(uint32_t pc, bool actualOutcome) {
        if (actualOutcome) {
            switch (getCurrentState(pc)) {
                case PredictionState::STRONGLY_TAKEN: return false;
                case PredictionState::WEAKLY_TAKEN: return false;
                case PredictionState::WEAKLY_NOT_TAKEN: return true;
                case PredictionState::STRONGLY_NOT_TAKEN: return true;
                default: break;
            }
        } else {
            switch (getCurrentState(pc)) {
                case PredictionState::STRONGLY_TAKEN: return true;
                case PredictionState::WEAKLY_TAKEN: return true;
                case PredictionState::WEAKLY_NOT_TAKEN: return false;
                case PredictionState::STRONGLY_NOT_TAKEN: return false;
                default: break;
            }
        }
    }

    // Update prediction based on actual branch outcome
    void updatePrediction(uint32_t pc, bool actualOutcome) {
        // 2-bit saturating counter state machine
        if (actualOutcome) {
            switch (getCurrentState(pc)) {
                case PredictionState::STRONGLY_TAKEN: break;
                case PredictionState::WEAKLY_TAKEN:
                    predictionTable[pc] = PredictionState::STRONGLY_TAKEN;
                    break;
                case PredictionState::WEAKLY_NOT_TAKEN:
                    predictionTable[pc] = PredictionState::WEAKLY_TAKEN;
                    break;
                case PredictionState::STRONGLY_NOT_TAKEN:
                    predictionTable[pc] = PredictionState::WEAKLY_NOT_TAKEN;
                    break;
                default: break;
            }
        } else {
            switch (getCurrentState(pc)) {
                case PredictionState::STRONGLY_TAKEN:
                    predictionTable[pc] = PredictionState::WEAKLY_TAKEN;
                    break;
                case PredictionState::WEAKLY_TAKEN:
                    predictionTable[pc] = PredictionState::WEAKLY_NOT_TAKEN;
                    break;
                case PredictionState::WEAKLY_NOT_TAKEN:
                    predictionTable[pc] = PredictionState::STRONGLY_NOT_TAKEN;
                    break;
                case PredictionState::STRONGLY_NOT_TAKEN: break;
                default: break;
            }
        }
    }

    PredictionState getCurrentState(uint32_t pc) {
        if (predictionTable.find(pc) == predictionTable.end()) predictionTable[pc] = initialPrediction;
        return predictionTable[pc];
    }

    // Print the entire PHT
    void printTable() {
        std::cout << "\n===== Pattern History Table =====\n";
        std::cout << std::setw(10) << "PC" << " | " << std::setw(15) << "State" << " | " << std::setw(12) << "Prediction" << std::endl;
        std::cout << "--------------------------------------\n";
        
        for (const auto& entry : predictionTable) {
            std::string stateStr;
            std::string predictionStr;
            
            switch (entry.second) {
                case PredictionState::STRONGLY_NOT_TAKEN:
                    stateStr = "00 (Strong NT)";
                    predictionStr = "Not Taken";
                    break;
                case PredictionState::WEAKLY_NOT_TAKEN:
                    stateStr = "01 (Weak NT)";
                    predictionStr = "Not Taken";
                    break;
                case PredictionState::WEAKLY_TAKEN:
                    stateStr = "10 (Weak T)";
                    predictionStr = "Taken";
                    break;
                case PredictionState::STRONGLY_TAKEN:
                    stateStr = "11 (Strong T)";
                    predictionStr = "Taken";
                    break;
                default:
                    stateStr = "Unknown";
                    predictionStr = "Unknown";
            }
            
            std::cout << "0x" << std::hex << std::setw(8) << entry.first << " | " 
                      << std::setw(15) << stateStr << " | " 
                      << std::setw(12) << predictionStr << std::endl;
        }
        std::cout << std::dec; // Reset to decimal output
    }
};

// Branch Target Buffer class
class BTB {
private:
    // Map PC to target address for taken branches
    std::map<uint32_t, pair<bool, uint32_t>> branchTargetBuffer;

public:
    // Check if a PC is in the BTB
    bool hasEntry(uint32_t pc) {
        return branchTargetBuffer.find(pc) != branchTargetBuffer.end();
    }

    bool isUnconditionalBranch(uint32_t pc) {
        return branchTargetBuffer[pc].first;
    }

    // Get target address for a given PC
    uint32_t getTargetAddress(uint32_t pc) {
        return branchTargetBuffer[pc].second;
    }

    // Update or add entry in BTB
    void updateEntry(uint32_t pc, uint32_t targetAddress, bool isUnconditional) {
        branchTargetBuffer[pc] = {isUnconditional, targetAddress};
    }

    // Print the entire BTB
    void printTable() {
        std::cout << "\n===== Branch Target Buffer =====\n";
        std::cout << std::setw(10) << "PC" << " | " << std::setw(15) << "Target Address" << std::endl;
        std::cout << "--------------------------------\n";
        
        for (const auto& entry : branchTargetBuffer) {
            std::cout << "0x" << std::hex << std::setw(8) << entry.first << " | " 
                      << "0x" << std::hex << std::setw(8) << entry.second.second << std::endl;
        }
        std::cout << std::dec; // Reset to decimal output
    }

};