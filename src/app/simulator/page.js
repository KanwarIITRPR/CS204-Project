"use client";
import { useState, useEffect, useRef } from "react";
import MemoryTab from "../memory/page";

export default function SimulatorTab({ editorCode, outputData, setOutputData, memoryData, setMemoryData, isAssembled, setIsAssembled }) {
  const [consoleOutput, setConsoleOutput] = useState("console output");
  const [activeTab, setActiveTab] = useState("registers");
  const [displayMode, setDisplayMode] = useState("hex");
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

  const cancelRunRef = useRef(false); // Ref to track if the run is cancelled
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
  
    // Convert and preserve keys (x0, x1, ...)
    const updatedRegisters = Object.entries(newRegisters)
      .sort((a, b) => {
        // Sort x0 to x31
        const getIndex = (key) => parseInt(key.replace("x", ""));
        return getIndex(a[0]) - getIndex(b[0]);
      })
      .map(([key, value]) => {
        try {
          const converted = Convertor(value, displayMode);
          return {
            name: key,
            value: converted[displayMode] || value  // extract the correct string
          };
        } catch (error) {
          console.error(`Error converting register ${key}:`, error);
          return { name: key, value };
        }
      });
      ;
  
    setRegisters(updatedRegisters); // Expected format: [{ name: 'x0', value: ... }, ...]
  };
  
  const updateRegisters = async () => {
    try {
      const regResponse = await fetch("http://127.0.0.1:5000/registers");
      const regData = await regResponse.json();
  
      if (regData.error) {
        console.error("Register fetch error:", regData.error);
        return;
      }
  
      // Parse all register values from hex (e.g., "0x1A" -> 26)
      const processedRegisters = Object.entries(regData.registers || {}).reduce((acc, [key, value]) => {
        try {
          const raw = value.replace(/^0x/, "");
          acc[key] = parseInt(raw, 16);
        } catch {
          console.warn(`Invalid hex for ${key}: ${value}`);
          acc[key] = value;
        }
        return acc;
      }, {});
  
      updateRegisterState(processedRegisters);
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
      // await updateRegisters();
  
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
          // await updateRegisters();
        }
      }
    } catch (err) {
      setConsoleOutput(`❌ Failed to fetch log.txt: ${err.message}`);
    }
  };
  
  
  const handleAssemble = async () => {
    try {
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
    setIsAssembled(false); // Reset assembly state
    handleAssemble(); // Re-assemble from the editor
  };

  // Determine the pipeline stage for a given PC
  const getPipelineStageForMC = (mc) => {
    // console.log('0x' + mc, pipelineState);
    
    
    // Check for each pipeline stage and return the stage where the MC is located
    if (pipelineState.Fetch === mc) return "Fetch";
    if (pipelineState.Decode === mc) return "Decode";
    if (pipelineState.Execute === mc) return "Execute";
    if (pipelineState.Memory === mc) return "Memory";
    if (pipelineState.Writeback === mc) return "Writeback";
  
    // Return null if no stage is found for the given MC
    return null;
  };
  

  // Get row background color based on pipeline stage
  const getRowBackground = (mc) => {
    const stage = getPipelineStageForMC(mc);
    // console.log("stage", stage);
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

  return (
    <div className="p-4 text-white h-screen flex flex-col">
    {/* Assemble & Control Buttons */}
    <div className="flex gap-2 self-center mb-6">
      {isAssembled ? (
        <>
        <button className="bg-green-600 px-4 py-2 rounded cursor-pointer" onClick={handleRun}>
        Run
        </button>
        {/* <button className="bg-gray-500 px-4 py-2 rounded">Step</button>
        <button className="bg-gray-700 px-4 py-2 rounded text-gray-400" disabled>Prev</button>
        <button className="bg-gray-700 px-4 py-2 rounded text-gray-400" disabled>Reset</button> */}
        <button
              className="bg-yellow-600 px-4 py-2 rounded"
              onClick={handleReassemble} 
            >
              Re-assemble
            </button>
        </>
      ) : (
        <button
          className="bg-green-500 px-6 py-3 text-lg font-semibold rounded-lg cursor-pointer hover:bg-green-600 transition"
          onClick={handleAssemble}
        >
          🛠 Assemble & Simulate
        </button>
      )}
    </div>

      {/* Main Content Wrapper */}
      <div className="flex w-full h-[70vh] gap-6">
        {/* Left Section - Machine Code Table */}
        <div className="w-1/2 bg-gray-900 p-4 rounded-lg shadow-lg border border-gray-700">
          <h2 className="text-lg font-semibold text-yellow-400 mb-3 text-center">Machine Code Output</h2>
          
          {/* ✅ Scrollable Table Wrapper */}
          <div className="overflow-y-auto max-h-[50vh] border border-gray-700 rounded-lg">
            <table className="w-full border-collapse border text-sm">
              <thead>
                <tr className="bg-gray-700 text-white sticky top-0">
                  <th className="border px-3 py-2">PC</th>
                  <th className="border px-3 py-2">Machine Code</th>
                  <th className="border px-3 py-2">Basic Code</th>
                </tr>
              </thead>
              <tbody>
              {outputData.length > 0 ? (
              outputData.map((row, idx) => {
                const stageStyle = getRowBackground(row.machineCode);
                return (
                  <tr 
                    key={idx} 
                    className={`text-center ${stageStyle}`}
                  >
                    <td className="border px-3 py-2">{row.pc}</td>
                    <td className="border px-3 py-2">{row.machineCode}</td>
                    <td className="border px-3 py-2">{row.basicCode}</td>
                  </tr>
                );
              })
            ) : (
              <tr className="text-center">
                <td colSpan="3" className="border px-3 py-2 text-gray-400">No output yet</td>
              </tr>
            )}
              </tbody>
            </table>
          </div>
        </div>

        {/* Right Section - Registers & Memory */}
        <div className="w-1/2 bg-gray-900 p-4 rounded-lg shadow-lg border border-gray-700 flex flex-col">
          {/* Tabs - Registers & Memory */}
          <div className="flex justify-center gap-10 text-green-400 border-b pb-2">
            <button
              onClick={() => setActiveTab("registers")}
              className={`hover:underline text-lg font-semibold transition ${activeTab === "registers" ? "text-yellow-400" : ""}`}
            >
              🗂 Registers
            </button>
            <button
              onClick={() => setActiveTab("memory")}
              className={`hover:underline text-lg font-semibold transition ${activeTab === "memory" ? "text-yellow-400" : ""}`}
            >
              💾 Memory
            </button>
          </div>

          <div className="mt-3 flex justify-end">
            <select
              className="bg-gray-700 text-white px-3 py-2 rounded"
              value={displayMode}
              onChange={handleDisplayChange}
            >
              <option value="hex">Hex</option>
              <option value="decimal">Decimal</option>
              <option value="unsigned">Unsigned</option>
            </select>
          </div>

          {/* Registers View */}
          {activeTab === "registers" && (
            <div className="mt-2 flex-1 overflow-y-auto border border-gray-600 p-2 max-h-[60vh] scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-gray-900">
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
          )}

          {/* Memory View */}
          {activeTab === "memory" && <MemoryTab displayMode={displayMode} memoryData={memoryData} />}

        </div>
      </div>

      {/* Console Output */}
      <div className="border-t border-gray-600 p-3 mt-4 bg-gray-900 rounded-lg shadow-lg">
        <div className="flex justify-between">
          <h3 className="text-yellow-400 font-semibold">💬 Console Output</h3>
          <div className="flex gap-4">
            <button className="text-red-400 text-sm scroll:auto hover:underline" onClick={() => setConsoleOutput("")}>
              Clear Console
            </button>
          </div>
        </div>
        <div 
          ref={consoleOutputRef}
          className="bg-gray-800 p-3 text-green-400 rounded max-h-32 overflow-y-auto whitespace-pre-wrap"
        >
          {consoleOutput || "No output yet"}
        </div>
      </div>

      {/* Pipeline Stages Visualization */}
      <div className="grid grid-cols-5 gap-4 mb-4">
        {["Fetch", "Decode", "Execute", "Memory", "Writeback"].map((stage, idx) => (
          <div
            key={stage}
            className={`p-4 rounded-lg shadow-md text-white text-sm font-mono text-center ${
              ["bg-blue-500", "bg-green-500", "bg-yellow-500", "bg-purple-500", "bg-red-500"][idx]
            }`}
          >
            <h4 className="font-bold text-md mb-1">{stage}</h4>
            <p className="break-words whitespace-pre-wrap min-h-[3rem]">{pipelineStageText[stage] || "..."}</p>
          </div>
        ))}
      </div>
    </div>
  );
}