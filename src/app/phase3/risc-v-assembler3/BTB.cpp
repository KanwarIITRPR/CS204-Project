#include <iostream>
#include <iomanip>
#include <map>
#include <cassert>
#include <vector>
#include <cstdint>
#include <string>
#include <random>
#include <ctime>

// Pattern History Table class
class PHT {
private:
    // Map to store PC to prediction state (00, 01, 10, 11)
    // 00 = Strongly Not Taken, 01 = Weakly Not Taken
    // 10 = Weakly Taken, 11 = Strongly Taken
    std::map<uint32_t, uint8_t> predictionTable;
    static const uint8_t STRONGLY_NOT_TAKEN = 0;  // 00
    static const uint8_t WEAKLY_NOT_TAKEN = 1;    // 01
    static const uint8_t WEAKLY_TAKEN = 2;        // 10
    static const uint8_t STRONGLY_TAKEN = 3;      // 11

public:
    PHT() {
        // Initialize an empty prediction table
    }



    // Get prediction for a given PC
    bool getPrediction(uint32_t pc) {
        // If PC not in table, initialize with Weakly Taken (10)
        if (predictionTable.find(pc) == predictionTable.end()) {
            predictionTable[pc] = WEAKLY_TAKEN;
        }
        
        // Return true if prediction is taken (10 or 11), false otherwise
        return (predictionTable[pc] >= WEAKLY_TAKEN);
    }

    // Update prediction based on actual branch outcome
    void updatePrediction(uint32_t pc, bool actualOutcome) {
        // If PC not in table, initialize with Weakly Taken
        if (predictionTable.find(pc) == predictionTable.end()) {
            predictionTable[pc] = WEAKLY_TAKEN;
        }
        
        uint8_t currentState = predictionTable[pc];
        
        // 2-bit saturating counter state machine
        if (actualOutcome) {
            // Branch was taken
            if (currentState < STRONGLY_TAKEN) {
                // Increment state (move toward Strongly Taken)
                predictionTable[pc] = currentState + 1;
            }
        } else {
            // Branch was not taken
            if (currentState > STRONGLY_NOT_TAKEN) {
                // Decrement state (move toward Strongly Not Taken)
                predictionTable[pc] = currentState - 1;
            }
        }
    }

    // Add this to your PHT class
    uint8_t getCurrentState(uint32_t pc) {
        if (predictionTable.find(pc) == predictionTable.end()) {
            return WEAKLY_TAKEN; // Default state
        }
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
                case STRONGLY_NOT_TAKEN:
                    stateStr = "00 (Strong NT)";
                    predictionStr = "Not Taken";
                    break;
                case WEAKLY_NOT_TAKEN:
                    stateStr = "01 (Weak NT)";
                    predictionStr = "Not Taken";
                    break;
                case WEAKLY_TAKEN:
                    stateStr = "10 (Weak T)";
                    predictionStr = "Taken";
                    break;
                case STRONGLY_TAKEN:
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

    // Clear the PHT
    void clearTable() {
        predictionTable.clear();
    }
};

// Branch Target Buffer class
class BTB {
private:
    // Map PC to target address for taken branches
    std::map<uint32_t, uint32_t> branchTargetBuffer;

public:
    BTB() {
        // Initialize an empty BTB
    }

    // Check if a PC is in the BTB
    bool hasEntry(uint32_t pc) {
        return branchTargetBuffer.find(pc) != branchTargetBuffer.end();
    }

    // Get target address for a given PC
    uint32_t getTargetAddress(uint32_t pc) {
        if (hasEntry(pc)) {
            return branchTargetBuffer[pc];
        }
        return 0; // Return 0 if not found (caller should check hasEntry first)
    }

    // Update or add entry in BTB
    void updateEntry(uint32_t pc, uint32_t targetAddress) {
        branchTargetBuffer[pc] = targetAddress;
    }

    // Print the entire BTB
    void printTable() {
        std::cout << "\n===== Branch Target Buffer =====\n";
        std::cout << std::setw(10) << "PC" << " | " << std::setw(15) << "Target Address" << std::endl;
        std::cout << "--------------------------------\n";
        
        for (const auto& entry : branchTargetBuffer) {
            std::cout << "0x" << std::hex << std::setw(8) << entry.first << " | " 
                      << "0x" << std::hex << std::setw(8) << entry.second << std::endl;
        }
        std::cout << std::dec; // Reset to decimal output
    }

    // Clear the BTB
    void clearTable() {
        branchTargetBuffer.clear();
    }
};

// Branch Prediction Unit - combines PHT and BTB
class BranchPredictionUnit {
private:
    PHT pht;
    BTB btb;
    bool debugEnabled;

public:
    BranchPredictionUnit(bool debug = false) : debugEnabled(debug) {}

    // Get prediction for a branch
    bool predictBranch(uint32_t pc) {
        return pht.getPrediction(pc);
    }

    // Get target address if branch is predicted taken
    uint32_t getTargetAddress(uint32_t pc) {
        return btb.getTargetAddress(pc);
    }

    // Update after branch execution with actual results
    void updateAfterExecution(uint32_t pc, bool actualTaken, uint32_t actualTarget) {
        // Update PHT with actual outcome
        pht.updatePrediction(pc, actualTaken);
        
        // If branch was taken, update BTB with target
        if (actualTaken) {
            btb.updateEntry(pc, actualTarget);
        }
        
        // Print details if debug is enabled
        if (debugEnabled) {
            printDetails(pc, actualTaken, actualTarget);
        }
    }

    // Enable or disable debug output
    void setDebugMode(bool enable) {
        debugEnabled = enable;
    }

    // Print details of BPU for current cycle
    void printDetails(uint32_t pc, bool wasTaken, uint32_t targetAddress) {
        std::cout << "\n----- Branch Prediction Unit Status -----\n";
        std::cout << "PC: 0x" << std::hex << pc << std::dec << std::endl;
        std::cout << "Branch was " << (wasTaken ? "TAKEN" : "NOT TAKEN") << std::endl;
        
        if (wasTaken) {
            std::cout << "Target Address: 0x" << std::hex << targetAddress << std::dec << std::endl;
        }
        
        // Print PHT and BTB tables
        pht.printTable();
        btb.printTable();
        std::cout << "---------------------------------------\n";
    }

    // Print full tables (can be called at any time)
    void printTables() {
        pht.printTable();
        btb.printTable();
    }
    
    // Clear all tables
    void clearTables() {
        pht.clearTable();
        btb.clearTable();
    }
};


struct BranchTestCase {
    uint32_t pc;
    bool taken;
    uint32_t targetAddress;
    
    BranchTestCase(uint32_t pc, bool taken, uint32_t targetAddress) 
        : pc(pc), taken(taken), targetAddress(targetAddress) {}
};

void runTests() {
    std::cout << "Running BPU test cases...\n";
    
    BranchPredictionUnit bpu(false); // Start with debug off
    std::vector<BranchTestCase> testCases;
    
    // Test Case 1: Pattern of always taken branches
    std::cout << "\n=== Test Case 1: Always Taken Pattern ===\n";
    for (int i = 0; i < 10; i++) {
        BranchTestCase tc(0x1000, true, 0x2000);
        testCases.push_back(tc);
    }
    
    // Test Case 2: Pattern of never taken branches
    std::cout << "\n=== Test Case 2: Never Taken Pattern ===\n";
    for (int i = 0; i < 10; i++) {
        BranchTestCase tc(0x2000, false, 0);
        testCases.push_back(tc);
    }
    
    // Test Case 3: Alternating pattern (taken, not taken)
    std::cout << "\n=== Test Case 3: Alternating Pattern ===\n";
    for (int i = 0; i < 10; i++) {
        BranchTestCase tc(0x3000, i % 2 == 0, i % 2 == 0 ? 0x4000 : 0);
        testCases.push_back(tc);
    }
    
    // Test Case 4: Complex pattern (taken, taken, not taken)
    std::cout << "\n=== Test Case 4: Complex Pattern ===\n";
    for (int i = 0; i < 10; i++) {
        BranchTestCase tc(0x4000, i % 3 != 2, i % 3 != 2 ? 0x5000 : 0);
        testCases.push_back(tc);
    }
    
    // Test Case 5: Multiple branch addresses
    std::cout << "\n=== Test Case 5: Multiple Branch Addresses ===\n";
    for (int i = 0; i < 10; i++) {
        uint32_t pc = 0x5000 + (i % 3) * 0x100;
        BranchTestCase tc(pc, i % 2 == 0, i % 2 == 0 ? 0x6000 : 0);
        testCases.push_back(tc);
    }
    
    // Test Case 6: Random pattern
    std::cout << "\n=== Test Case 6: Random Pattern ===\n";
    std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<int> dist(0, 1);
    for (int i = 0; i < 10; i++) {
        bool taken = dist(rng) == 1;
        BranchTestCase tc(0x7000, taken, taken ? 0x8000 : 0);
        testCases.push_back(tc);
    }
    
    // Execute all test cases
    int correctPredictions = 0;
    int totalPredictions = 0;
    
    for (const auto& tc : testCases) {
        // Get prediction before execution
        bool prediction = bpu.predictBranch(tc.pc);
        
        // Only count if we've seen this PC before
        if (totalPredictions > 0) {
            if (prediction == tc.taken) {
                correctPredictions++;
            }
            totalPredictions++;
        } else {
            totalPredictions++;
        }
        
        // Update BPU with actual results
        bpu.updateAfterExecution(tc.pc, tc.taken, tc.targetAddress);
    }
    
    // Print results and tables
    std::cout << "\n=== Test Results ===\n";
    std::cout << "Total branches: " << totalPredictions << std::endl;
    std::cout << "Correct predictions: " << correctPredictions << std::endl;
    std::cout << "Prediction accuracy: " << std::fixed << std::setprecision(2) 
              << (static_cast<float>(correctPredictions) / (totalPredictions - 1)) * 100 << "%" << std::endl;
    
    // Enable debug and print final tables
    bpu.setDebugMode(true);
    bpu.printTables();
    
    // Corner case tests
    std::cout << "\n=== Corner Case Tests ===\n";
    
    // Corner Case 1: Same PC with changing target
    BranchPredictionUnit bpu2(true);
    std::cout << "\nCorner Case 1: Same PC with changing target\n";
    bpu2.updateAfterExecution(0xA000, true, 0xB000);
    bpu2.updateAfterExecution(0xA000, true, 0xC000); // Target changes
    
    // Corner Case 2: State transitions test for 2-bit counter
    BranchPredictionUnit bpu3(true);
    std::cout << "\nCorner Case 2: 2-bit counter state transitions\n";
    
    // Starting at Weakly Taken (10)
    std::cout << "Starting at default Weakly Taken state\n";
    bool prediction = bpu3.predictBranch(0xD000);
    assert(prediction == true); // Should be taken
    
    // Move to Strongly Taken (11)
    bpu3.updateAfterExecution(0xD000, true, 0xE000);
    prediction = bpu3.predictBranch(0xD000);
    assert(prediction == true); // Should still be taken
    
    // Stay at Strongly Taken (11)
    bpu3.updateAfterExecution(0xD000, true, 0xE000);
    prediction = bpu3.predictBranch(0xD000);
    assert(prediction == true); // Should still be taken
    
    // Move to Weakly Taken (10)
    bpu3.updateAfterExecution(0xD000, false, 0);
    prediction = bpu3.predictBranch(0xD000);
    assert(prediction == true); // Should still be taken
    
    // Move to Weakly Not Taken (01)
    bpu3.updateAfterExecution(0xD000, false, 0);
    prediction = bpu3.predictBranch(0xD000);
    assert(prediction == false); // Should not be taken
    
    // Move to Strongly Not Taken (00)
    bpu3.updateAfterExecution(0xD000, false, 0);
    prediction = bpu3.predictBranch(0xD000);
    assert(prediction == false); // Should not be taken
    
    // Stay at Strongly Not Taken (00)
    bpu3.updateAfterExecution(0xD000, false, 0);
    prediction = bpu3.predictBranch(0xD000);
    assert(prediction == false); // Should not be taken
    
    std::cout << "All corner cases tested successfully!\n";


// Test Case: Long biased pattern with sudden change
std::cout << "\n=== Test Case 7: Long Biased Pattern with Sudden Change ===\n";
bpu.clearTables();

// Train with 20 taken branches
for (int i = 0; i < 20; i++) {
    bpu.updateAfterExecution(0xF000, true, 0xF100);
}

// Now suddenly switch to not taken and check predictions
std::cout << "Predictions after pattern change:\n";
for (int i = 0; i < 5; i++) {
    bool prediction = bpu.predictBranch(0xF000);
    std::cout << "  Iteration " << i+1 << ": " << (prediction ? "Taken" : "Not Taken") << std::endl;
    bpu.updateAfterExecution(0xF000, false, 0);
}

// Test Case: Complex pattern recognition (TTNT pattern)
std::cout << "\n=== Test Case 8: Complex Pattern Recognition ===\n";
bpu.clearTables();

// Create a TTNT pattern and repeat it
bool pattern[4] = {true, true, false, true}; // TTNT pattern
std::cout << "Using TTNT pattern, checking if predictor learns it:\n";

for (int i = 0; i < 4; i++) { // First train with one complete pattern
    bpu.updateAfterExecution(0xE000, pattern[i], pattern[i] ? 0xE100 : 0);
}

// Now check predictions for next iterations
for (int i = 0; i < 8; i++) {
    int patternIndex = i % 4;
    bool prediction = bpu.predictBranch(0xE000);
    bool actual = pattern[patternIndex];
    std::cout << "  Iteration " << i+1 << ": Predicted " 
              << (prediction ? "Taken" : "Not Taken") 
              << ", Actual " << (actual ? "Taken" : "Not Taken") 
              << (prediction == actual ? " ✓" : " ✗") << std::endl;
    bpu.updateAfterExecution(0xE000, actual, actual ? 0xE100 : 0);
}

// Test Case: Branch aliasing
std::cout << "\n=== Test Case 9: Branch Aliasing ===\n";
bpu.clearTables();

// Create two branches with different patterns
uint32_t pc1 = 0xD000;
uint32_t pc2 = 0xD100;

// Train branch 1 to be always taken
std::cout << "Training branch 1 (0xD000) to be always taken\n";
for (int i = 0; i < 5; i++) {
    bpu.updateAfterExecution(pc1, true, 0xD500);
}

// Train branch 2 to be never taken
std::cout << "Training branch 2 (0xD100) to be never taken\n";
for (int i = 0; i < 5; i++) {
    bpu.updateAfterExecution(pc2, false, 0);
}

// Now alternate between them and check predictions
std::cout << "Checking predictions when alternating between branches:\n";
for (int i = 0; i < 5; i++) {
    bool pred1 = bpu.predictBranch(pc1);
    bool pred2 = bpu.predictBranch(pc2);
    std::cout << "  Iteration " << i+1 
              << ": Branch 1 prediction: " << (pred1 ? "Taken" : "Not Taken") 
              << ", Branch 2 prediction: " << (pred2 ? "Not Taken" : "Taken") 
              << (pred1 && !pred2 ? " ✓" : " ✗") << std::endl;
    
    bpu.updateAfterExecution(pc1, true, 0xD500);
    bpu.updateAfterExecution(pc2, false, 0);
}

// Test Case: Hysteresis behavior
std::cout << "\n=== Test Case 10: Hysteresis Behavior ===\n";
bpu.clearTables();
uint32_t pc = 0xB000;

// Start with default (Weakly Taken)
std::cout << "Starting at default state (should be Weakly Taken)\n";
 
prediction = bpu.predictBranch(pc);
std::cout << "  Initial prediction: " << (prediction ? "Taken" : "Not Taken") << std::endl;

// Test hysteresis with pattern: T T T N T N T N N T
bool pattern2[10] = {true, true, true, false, true, false, true, false, false, true};
std::string states[4] = {"Strongly Not Taken", "Weakly Not Taken", "Weakly Taken", "Strongly Taken"};

std::cout << "Testing with pattern: T T T N T N T N N T\n";
for (int i = 0; i < 10; i++) {
    prediction = bpu.predictBranch(pc);
    std::cout << "  Step " << i+1 << ": Predicted " 
              << (prediction ? "Taken" : "Not Taken") 
              << ", Actual " << (pattern2[i] ? "Taken" : "Not Taken");
    
    // Get current state for display (this requires adding a getter in your PHT class)
    // If you don't want to modify the PHT class, you can comment this part out
    // uint8_t state = pht.getCurrentState(pc);
    // std::cout << " (State: " << states[state] << ")";
    
    std::cout << std::endl;
    bpu.updateAfterExecution(pc, pattern2[i], pattern2[i] ? 0xB100 : 0);
}

}

int main() {
    runTests();
    return 0;
}
