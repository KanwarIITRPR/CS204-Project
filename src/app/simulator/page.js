"use client";
import { useState, useEffect } from "react";
import MemoryTab from "../memory/page";

export default function SimulatorTab({ editorCode }) {
  const [consoleOutput, setConsoleOutput] = useState("console output");
  const [activeTab, setActiveTab] = useState("registers");
  const [displayMode, setDisplayMode] = useState("hex");
  const [outputData, setOutputData] = useState([]);
  const [isAssembled, setIsAssembled] = useState(false);
  const [memoryData, setMemoryData] = useState(new Map());

  const initialRegisters = [
    "0x00000000", "0x00000000", "0x7FFFFFDC", "0x10000000", // x0 - x3
    "0x00000000", "0x00000000", "0x00000000", "0x00000000", // x4 - x7
    "0x00000000", "0x00000000", "0x00000001", "0x7FFFFFDC", // x8 - x11
    ...Array(20).fill("0x00000000"), // x12 - x31
  ];
  const [registers, setRegisters] = useState([...initialRegisters]);

  const registerNames = [
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3",
    "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4",
    "t5", "t6"
  ];

  const Convertor = (hex, mode) => {
    let intValue = parseInt(hex, 16);
    if (isNaN(intValue)) return hex; // Handle invalid hex values

    switch (mode) {
      case "decimal":
        return intValue.toString();
      case "unsigned":
        return (intValue >>> 0).toString(); // Ensures unsigned conversion
      case "ascii":
        return String.fromCharCode(intValue & 0xFF) +
               String.fromCharCode((intValue >> 8) & 0xFF) +
               String.fromCharCode((intValue >> 16) & 0xFF) +
               String.fromCharCode((intValue >> 24) & 0xFF);
      default:
        return hex;
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


  return (
    <div className="p-4 text-white h-screen flex flex-col">
    {/* Assemble & Control Buttons */}
    <div className="flex gap-2 self-center mb-6">
      {isAssembled ? (
        <>
        <button className="bg-green-600 px-4 py-2 rounded">Run</button>
        <button className="bg-gray-500 px-4 py-2 rounded">Step</button>
        <button className="bg-gray-700 px-4 py-2 rounded text-gray-400" disabled>Prev</button>
        <button className="bg-gray-700 px-4 py-2 rounded text-gray-400" disabled>Reset</button>
        <button 
          className="bg-yellow-600 px-4 py-2 rounded" 
          onClick={() => {
            setIsAssembled(false);
            handleAssemble();         
          }}
        >
            Re-assemble from Editor
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
                  outputData.map((row, idx) => (
                    <tr key={idx} className="text-center hover:bg-gray-800 transition">
                      <td className="border px-3 py-2">{row.pc}</td>
                      <td className="border px-3 py-2">{row.machineCode}</td>
                      <td className="border px-3 py-2">{row.basicCode}</td>
                    </tr>
                  ))
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
      <button className="text-red-400 text-sm hover:underline" onClick={() => setConsoleOutput("")}>
      Clear Console
      </button>
      </div>
      <div className="bg-gray-800 p-3 text-green-400 rounded max-h-32 overflow-y-auto">
        {consoleOutput || "No output yet"}
      </div>
      </div>

    </div>
  );
}