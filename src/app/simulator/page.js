"use client";
import { useState, useEffect, useRef } from "react";
import MemoryTab from "../memory/page";
import Knob from "../burger/page";

export default function SimulatorTab({ editorCode, outputData, setOutputData, memoryData, setMemoryData, isAssembled, setIsAssembled }) {
  const [consoleOutput, setConsoleOutput] = useState("console output");
  const [activeTab, setActiveTab] = useState("registers");
  const [displayMode, setDisplayMode] = useState("hex");
  const [showBurger, setShowBurger] = useState(false);

  // Track the current PC and instruction in each pipeline stage
  const [pipelineState, setPipelineState] = useState({
    Fetch: null,
    Decode: null,
    Execute: null,
    Memory: null,
    Writeback: null
  });
  const [pipelineStageText, setPipelineStageText] = useState({
    Fetch: "",
    Decode: "",
    Execute: "",
    Memory: "",
    Writeback: ""
  });
  
  const initialRegisters = [
    "0x00000000", "0x00000000", "0x7FFFFFDC", "0x10000000", // x0 - x3
    "0x00000000", "0x00000000", "0x00000000", "0x00000000", // x4 - x7
    "0x00000000", "0x00000000", "0x00000001", "0x7FFFFFDC", // x8 - x11
    ...Array(20).fill("0x00000000"), // x12 - x31
  ];
  const [registers, setRegisters] = useState([]);
  const registersContainerRef = useRef(null);

  const cancelRunRef = useRef(false);
  const consoleOutputRef = useRef(null);

  const registerNames = [
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3",
    "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4",
    "t5", "t6"
  ];

  const updateRegisterState = (newRegisters) => {
    console.log("New Registers: ", newRegisters);
    
    if (!newRegisters || typeof newRegisters !== "object") {
      console.error("Register data is undefined or invalid");
      return;
    }
    
    // Create a new array with the updated values
    const updatedRegisters = Array(32).fill("0x00000000");
    
    // Fill in the values from the fetched registers
    Object.entries(newRegisters).forEach(([key, value]) => {
      // Extract register number (e.g., "x5" -> 5)
      const regNum = parseInt(key.replace("x", ""), 10);
      
      if (!isNaN(regNum) && regNum >= 0 && regNum < 32) {
        // Format value according to display mode
        let formattedValue;
        
        if (displayMode === 'hex') {
          // For hex mode, ensure it's a proper hex string
          formattedValue = typeof value === 'number' 
            ? `0x${value.toString(16).padStart(8, '0')}`
            : value.startsWith('0x') ? value : `0x${value}`;
        } else if (displayMode === 'decimal') {
          // For decimal mode, convert to number
          formattedValue = typeof value === 'number'
            ? value
            : parseInt(value.replace(/^0x/, ''), 16);
        } else {
          // For unsigned mode
          formattedValue = typeof value === 'number'
            ? value.toString()
            : BigInt(`0x${value.replace(/^0x/, '')}`).toString();
        }
        
        updatedRegisters[regNum] = formattedValue;
      }
    });
    
    // Update the state with the new register values
    setRegisters(updatedRegisters);
  };
  
  const updateRegisters = async () => {
    try {
      const regResponse = await fetch("http://127.0.0.1:5000/registers");
      const regData = await regResponse.json();
      
      console.log("Register response:", regData);
  
      if (regData.error) {
        console.error("Register fetch error:", regData.error);
        return;
      }
  
      // Process the register values
      updateRegisterState(regData.registers || {});
    } catch (err) {
      console.error("Error fetching register data:", err);
    }
  };
   
  const Convertor = (value, displayMode) => {
    try {
      // Handle the case where value is already a number
      if (typeof value === 'number') {
        if (displayMode === 'hex') {
          // Convert number to hex string
          return '0x' + value.toString(16).padStart(8, '0');
        }
        // Already a number and display mode is decimal, return as is
        return value;
      }
      
      // Handle string values (hex format)
      if (typeof value === 'string') {
        // Remove any '0x' prefix and whitespace
        const cleanHex = value.trim().replace(/^0x/i, ''); 
        
        if (displayMode === 'decimal') {
          // For very large hex values
          try {
            return BigInt('0x' + cleanHex).toString();
          } catch {
            // Fallback for normal-sized values
            return parseInt(cleanHex, 16);
          }
        } else {
          // Ensure proper hex format with 0x prefix
          return '0x' + cleanHex.padStart(8, '0');
        }
      }
      
      // For any other type, return as is
      return value;
    } catch (error) {
      console.error("Conversion error for value:", value, error);
      return value; // Return original on error
    }
  };

  // Extract MC from pipeline stage output
  const extractMC = (stageText) => {
    const mcMatch = stageText.match(/Completed: 0x([0-9A-Fa-f]*)/);
    return mcMatch ? '0x'+mcMatch[1] : null;
  };

  const handleRun = async () => {
    try {
      cancelRunRef.current = false;
      setConsoleOutput(""); // Clear console before printing new log
  
      // Reset pipeline stages for each new run
      setPipelineStageText({
        Fetch: "",
        Decode: "",
        Execute: "",
        Memory: "",
        Writeback: ""
      });
  
      // Reset pipeline state for each new run
      setPipelineState({
        Fetch: null,
        Decode: null,
        Execute: null,
        Memory: null,
        Writeback: null
      });
  
      const response = await fetch("http://127.0.0.1:5000/read-log");
      const data = await response.json();
  
      if (data.error) {
        setConsoleOutput(`❌ Error: ${data.error}`);
        return;
      }
  
      // Initial register fetch
      await updateRegisters();
  
      // Process and visualize the log
      const lines = data.lines;
      for (let i = 0; i < lines.length; i++) {
        if (cancelRunRef.current) break;
  
        const line = lines[i];
        await new Promise(resolve => setTimeout(resolve, 150)); // Add small delay
  
        setConsoleOutput(prev => prev + line);
  
        const stageMatch = line.match(/^(Fetch|Decode|Execute|Memory Access|Writeback) Completed:/);
        if (stageMatch) {
          const stageKey = stageMatch[1];
          const mappedKey = {
            "Fetch": "Fetch",
            "Decode": "Decode",
            "Execute": "Execute",
            "Memory Access": "Memory",
            "Writeback": "Writeback"
          }[stageKey];
  
          // Reset pipeline state only for the current instruction's stage
          setPipelineStageText(prev => ({
            ...prev,
            [mappedKey]: line
          }));
  
          // Extract MC for this stage
          const mc = extractMC(line);
  
          // Update pipeline state with the current PC for the current instruction
          if (mc) {
            setPipelineState(prev => ({
              ...prev,
              [mappedKey]: mc
            }));
          }
  
          // Update registers after each pipeline stage
          await updateRegisters();
        }
      }
    } catch (err) {
      setConsoleOutput(`❌ Failed to fetch log.txt: ${err.message}`);
    }
  };
  
  const handleAssemble = async () => {
    try {
        setConsoleOutput("");
        setShowBurger(true); 
        console.log("Sending request to assemble...");
        console.log("Code:", editorCode);

        const response = await fetch("http://127.0.0.1:5000/assemble", {
            method: "POST",   
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ code: editorCode }),
        });

        if (!response.ok) {
            const errorText = await response.text();
            throw new Error(`HTTP error! Status: ${response.status}, Message: ${errorText}`);
        }

        const data = await response.json();
        console.log("Server Response:", data);

        if (data.error) {
            alert("Assembler Error: " + data.error);
            return;
        }
        console.log("Data:",data);
        let newMemoryData = new Map(memoryData);
        let newOutputData = [];

        Object.entries(data[0].machine_code).forEach(([pc, instrObj]) => {
            let pcFormatted = `0x${pc.toUpperCase()}`;
            let instr = instrObj.machine_code.toUpperCase();
            let basicInstr = instrObj.basic_code || "N/A";
            let byteArray = [
                instr.slice(6, 8),
                instr.slice(4, 6),
                instr.slice(2, 4),
                instr.slice(0, 2),
            ];
            newMemoryData.set(pcFormatted, byteArray);
            newOutputData.push({
                pc: pcFormatted,
                machineCode: instr,
                basicCode: basicInstr,
            });
        });

        if (data[1].memory_data) {
            Object.entries(data[1].memory_data).forEach(([addr, value]) => {
                let addrFormatted = `0x${addr.toUpperCase()}`;
                let byteArray = [
                  value.slice(6, 8),
                  value.slice(4, 6),
                  value.slice(2, 4),
                  value.slice(0, 2),
                ];
                newMemoryData.set(addrFormatted, byteArray);
            });
        }

        setMemoryData(newMemoryData);
        setOutputData(newOutputData);
        setIsAssembled(true);
        
        // Reset registers to initial values when assembling
        setRegisters(initialRegisters.map(hex => Convertor(hex, displayMode)));
      } catch (error) {
        console.error("Assembly request failed:", error);
      }
    };

  const handleDisplayChange = (e) => {
    setDisplayMode(e.target.value);
  };

  useEffect(() => {
    setRegisters(initialRegisters.map(hex => Convertor(hex, displayMode)));
  }, [displayMode]);

  useEffect(() => {
    if (consoleOutputRef.current) {
      consoleOutputRef.current.scrollTop = consoleOutputRef.current.scrollHeight;
    }
  }, [consoleOutput]);

  const handleReassemble = () => {
    cancelRunRef.current = true;
    setConsoleOutput(""); // Clear console output
    setPipelineStageText({
      Fetch: "",
      Decode: "",
      Execute: "",
      Memory: "",
      Writeback: "",
    }); // Reset pipeline stages
    setPipelineState({
      Fetch: null,
      Decode: null,
      Execute: null,
      Memory: null,
      Writeback: null
    });
    
    // Reset registers to initial values when reassembling
    setRegisters(initialRegisters.map(hex => Convertor(hex, displayMode)));
    
    setIsAssembled(false); // Reset assembly state
    handleAssemble(); // Re-assemble from the editor
  };

  // Determine the pipeline stage for a given PC
  const getPipelineStageForMC = (mc) => {
    if (pipelineState.Fetch === mc) return "Fetch";
    if (pipelineState.Decode === mc) return "Decode";
    if (pipelineState.Execute === mc) return "Execute";
    if (pipelineState.Memory === mc) return "Memory";
    if (pipelineState.Writeback === mc) return "Writeback";
    return null;
  };
  
  // Get row background color based on pipeline stage
  const getRowBackground = (mc) => {
    const stage = getPipelineStageForMC(mc);
    if (!stage) return "";
    
    // High contrast styles for each pipeline stage
    const stageStyles = {
      "Fetch": "bg-blue-500 text-white",
      "Decode": "bg-green-500 text-white",
      "Execute": "bg-yellow-500 text-black font-bold",
      "Memory": "bg-purple-500 text-white",
      "Writeback": "bg-red-500 text-white"
    };
    
    return stageStyles[stage] || "";
  };

  // Register Display Component for better organization
  const RegistersDisplay = () => (
    <div 
      ref={registersContainerRef}
      className="mt-2 flex-1 overflow-y-auto border border-gray-600 p-2 max-h-[60vh] scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-gray-900"
    >
      <div className="grid grid-cols-1 md:grid-cols-2 gap-2">
        {registers.map((value, index) => (
          <div key={index} className="flex items-center mb-2">
            <span className="w-20 text-yellow-400">{registerNames[index]} (x{index})</span>
            <input
              className="w-full bg-gray-800 text-white px-2 py-1 border border-gray-600 rounded"
              value={value}
              readOnly
            />
          </div>
        ))}
      </div>
    </div>
  );

  return (
    <div className="p-2 text-white h-screen flex flex-col bg-gray-950">
      {/* Top Control Bar */}
      <div className="flex justify-between items-center mb-3 bg-gray-800 p-3 rounded-lg shadow-lg border border-gray-700">
        {/* Left side - Assemble & Control Buttons */}
        <div className="flex gap-3">
          {isAssembled ? (
            <>
              <button 
                className="bg-green-600 hover:bg-green-700 px-4 py-2 rounded-md font-semibold text-white flex items-center gap-2 shadow-md transition-all duration-150"
                onClick={handleRun}
              >
                <span className="text-lg">▶</span> Run Simulation
              </button>
              <button
                className="bg-amber-600 hover:bg-amber-700 px-4 py-2 rounded-md font-semibold text-white flex items-center gap-2 shadow-md transition-all duration-150"
                onClick={handleReassemble} 
              >
                <span className="text-lg">🔄</span> Re-assemble
              </button>
            </>
          ) : (
            <button
              className="bg-blue-600 hover:bg-blue-700 px-5 py-2 font-semibold rounded-md text-white flex items-center gap-2 shadow-md transition-all duration-150"
              onClick={handleAssemble}
            >
              <span className="text-lg">🛠</span> Assemble & Simulate
            </button>
          )}
        </div>

        {/* Right side - Display Controls */}
        <div className="flex items-center gap-3">
          <span className="text-sm text-gray-300">Display Format:</span>
          <select
            className="bg-gray-700 text-white px-3 py-2 text-sm rounded-md border border-gray-600 shadow-md"
            value={displayMode}
            onChange={handleDisplayChange}
          >
            <option value="hex">Hexadecimal</option>
            <option value="decimal">Decimal</option>
            <option value="unsigned">Unsigned</option>
          </select>
        </div>
      </div>

      {/* Pipeline Stages Visualization - Enhanced */}
      <div className="grid grid-cols-5 gap-2 mb-3">
        {["Fetch", "Decode", "Execute", "Memory", "Writeback"].map((stage, idx) => {
          const stageColors = [
            "from-blue-600 to-blue-800", 
            "from-green-600 to-green-800", 
            "from-yellow-600 to-yellow-800", 
            "from-purple-600 to-purple-800", 
            "from-red-600 to-red-800"
          ];
          
          return (
            <div
              key={stage}
              className={`p-3 rounded-lg shadow-lg text-white overflow-hidden bg-gradient-to-br ${stageColors[idx]} border border-gray-700`}
            >
              <h4 className="font-bold text-sm mb-1 flex items-center">
                <span className="mr-1">{["🔍", "🧩", "⚙️", "💾", "✏️"][idx]}</span>
                {stage}
              </h4>
              <p className="text-xs whitespace-nowrap overflow-hidden text-ellipsis opacity-90">
                {pipelineStageText[stage] || "Waiting..."}
              </p>
            </div>
          );
        })}
      </div>

      {/* Main Content Grid */}
      <div className="grid grid-cols-2 gap-3 flex-grow overflow-hidden">
        {/* Left Section - Machine Code Table */}
        <div className="bg-gray-900 rounded-lg shadow-lg border border-gray-700 flex flex-col overflow-hidden">
          <h2 className="text-sm font-semibold text-blue-400 p-2 border-b border-gray-700 bg-gray-800">
            <span className="mr-2">📋</span>Machine Code Output
          </h2>
          
          {/* Scrollable Table */}
          <div className="overflow-y-auto flex-grow scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-gray-900">
            <table className="w-full border-collapse text-sm">
              <thead>
                <tr className="bg-gray-800 text-gray-200 sticky top-0">
                  <th className="px-3 py-2 border-b border-gray-700 text-left">PC</th>
                  <th className="px-3 py-2 border-b border-gray-700 text-left">Machine Code</th>
                  <th className="px-3 py-2 border-b border-gray-700 text-left">Basic Code</th>
                </tr>
              </thead>
              <tbody>
              {outputData.length > 0 ? (
                outputData.map((row, idx) => {
                  const stageStyle = getRowBackground(row.machineCode);
                  return (
                    <tr 
                      key={idx} 
                      className={`hover:bg-gray-800 transition-colors duration-150 ${stageStyle || 'border-b border-gray-800'}`}
                    >
                      <td className="px-3 py-2">{row.pc}</td>
                      <td className="px-3 py-2 font-mono">{row.machineCode}</td>
                      <td className="px-3 py-2">{row.basicCode}</td>
                    </tr>
                  );
                })
              ) : (
                <tr>
                  <td colSpan="3" className="px-3 py-8 text-gray-400 text-center">
                    No output available. Click "Assemble & Simulate" to start.
                  </td>
                </tr>
              )}
              </tbody>
            </table>
          </div>
        </div>

        {/* Right Section - Registers & Memory Tabs */}
        <div className="bg-gray-900 rounded-lg shadow-lg border border-gray-700 flex flex-col overflow-hidden">
          {/* Tabs Navigation */}
          <div className="flex border-b border-gray-700 bg-gray-800">
            <button
              onClick={() => setActiveTab("registers")}
              className={`flex-1 py-2 px-4 text-sm font-medium transition-colors duration-150 flex items-center justify-center gap-2
                ${activeTab === "registers" 
                  ? "text-yellow-400 border-b-2 border-yellow-400" 
                  : "text-gray-300 hover:bg-gray-700"}`}
            >
              <span>🗂</span> Registers
            </button>
            <button
              onClick={() => setActiveTab("memory")}
              className={`flex-1 py-2 px-4 text-sm font-medium transition-colors duration-150 flex items-center justify-center gap-2
                ${activeTab === "memory" 
                  ? "text-blue-400 border-b-2 border-blue-400" 
                  : "text-gray-300 hover:bg-gray-700"}`}
            >
              <span>💾</span> Memory
            </button>
          </div>

          {/* Tab Content */}
          <div className="flex-grow overflow-hidden">
            {activeTab === "registers" && <RegistersDisplay />}
            {activeTab === "memory" && <MemoryTab displayMode={displayMode} memoryData={memoryData} />}
          </div>
        </div>
      </div>

      {/* Console Output */}
      <div className="border-t border-gray-600 p-2 mt-2 bg-gray-900 rounded-lg shadow-lg">
        <div className="flex justify-between items-center mb-1">
          <h3 className="text-yellow-400 text-sm font-semibold">💬 Console</h3>
          <button className="text-red-400 text-xs hover:underline" onClick={() => setConsoleOutput("")}>
            Clear
          </button>
        </div>
        <div 
          ref={consoleOutputRef}
          className="bg-gray-800 p-2 text-green-400 rounded h-40 overflow-y-auto text-xs whitespace-pre-wrap"
        >
          {consoleOutput || "No output yet"}
        </div>
      </div>

      {/* Burger Menu */}
      <div className="fixed top-4 left-4 z-50 transition-all duration-300 ease-in-out">
        {showBurger && (
          <div className="animate-fade-in-up">
            <Knob />
          </div>
        )}
      </div>
    </div>
  );
}