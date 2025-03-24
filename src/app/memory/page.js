"use client";
import { useState, useEffect } from "react";

export default function MemoryTab({ displayMode, memoryData }) {
  const SEGMENTS = {
    text: 0x00000000,
    data: 0x10000000,
    heap: 0x20000000,
    stack: 0x7FFFFFFC,
  };

  const MEMORY_STEP = 0x40; 
  const SCROLL_STEP = 0x10;

  const [currentSegment, setCurrentSegment] = useState("text");
  const [startAddress, setStartAddress] = useState(SEGMENTS.text);
  const [memory, setMemory] = useState(() => generateMemory(SEGMENTS.text));
  const [addressInput, setAddressInput] = useState("");

  function generateMemory(start) {
    let mem = {};
    for (let i = start; i < start + MEMORY_STEP; i += 4) {
        const addressHex = `0x${i.toString(16).toUpperCase().padStart(8, "0")}`;

        if (memoryData.has(addressHex)) {
          mem[addressHex] = memoryData.get(addressHex);
      } else {
          if (currentSegment === "data") {
              // Assign default values in the `.data` segment dynamically
              memoryData.set(addressHex, ["00", "00", "00", "00"]);
              mem[addressHex] = ["00", "00", "00", "00"];
          } else {
              mem[addressHex] = ["00", "00", "00", "00"];
          }
      }      
    }
    return mem;
}

  const handleMemoryChange = (address, index, value) => {
    let formattedValue = parseInt(value, 16).toString(16).padStart(2, "0").toUpperCase();
    setMemory((prev) => ({
      ...prev,
      [address]: [...prev[address].slice(0, index), formattedValue, ...prev[address].slice(index + 1)],
    }));
};  

  const handleScroll = (direction) => {
    let newStart = startAddress + (direction === "up" ? -SCROLL_STEP : SCROLL_STEP);
    if (newStart < SEGMENTS[currentSegment]) return;
    setStartAddress(newStart);
    setMemory(generateMemory(newStart));
  };

  const jumpToAddress = () => {
    let address = parseInt(addressInput, 16);
    if (!isNaN(address)) {  
        let segment = Object.entries(SEGMENTS).find(([_, start]) => address >= start && address < start + MEMORY_STEP);
        if (segment) {
          setStartAddress(address); // Instead of segment[1]
        }        
    }
  };

  function Convertor(hex, mode) {
    let intValue = parseInt(hex, 16);
    if (isNaN(intValue)) return hex;

    switch (mode) {
      case "decimal":
        return intValue.toString();
      case "unsigned":
        return (intValue >>> 0).toString();
      case "ascii":
        return String.fromCharCode(intValue & 0xFF) +
               String.fromCharCode((intValue >> 8) & 0xFF) +
               String.fromCharCode((intValue >> 16) & 0xFF) +
               String.fromCharCode((intValue >> 24) & 0xFF);
      default:
        return hex;
    }
  } 


  const handleSegmentChange = (segment) => {
    setCurrentSegment(segment);
    setStartAddress(SEGMENTS[segment]);
    setMemory(generateMemory(SEGMENTS[segment]));
  };

  useEffect(() => {
    setMemory(generateMemory(startAddress));
  }, [memoryData, startAddress]);
  
  
  return (
    <div className="p-4 bg-gray-900 text-white rounded-lg">
      <div className="flex gap-4 items-center">
        <label className="font-bold">Address:</label>
        <input
          type="text"
          className="p-1 bg-gray-800 border border-gray-600 rounded w-32"
          value={addressInput}
          onChange={(e) => setAddressInput(e.target.value)}
          placeholder="0x00000000"
        />
        <button
          className="px-3 py-1 bg-gray-600 rounded hover:bg-gray-700"
          onClick={jumpToAddress}
        >
          Go
        </button>
      </div>

      <div className="p-4 bg-gray-900 text-white rounded-lg">
      <div className="flex gap-4 items-center">
        <label className="font-bold">Jump to:</label>
        <select
          className="p-1 bg-gray-800 border border-gray-600 rounded"
          onChange={(e) => handleSegmentChange(e.target.value)}
          value={currentSegment}
        >
          {Object.keys(SEGMENTS).map((segment) => (
            <option key={segment} value={segment}>
              {segment.toUpperCase()}
            </option>
          ))}
        </select>
        <button
          className="px-3 py-1 bg-blue-600 rounded hover:bg-blue-700"
          onClick={() => handleScroll("up")}
        >
          Up
        </button>
        <button
          className="px-3 py-1 bg-blue-600 rounded hover:bg-blue-700"
          onClick={() => handleScroll("down")}
        >   
          Down
        </button>
      </div>

      <div className="mt-4 max-h-60 overflow-y-auto border border-gray-700 p-2">
        <table className="w-full text-left border-collapse">
          <thead>
            <tr className="border-b border-gray-700">
              <th className="p-2">Address</th>
              <th className="p-2">+3</th>
              <th className="p-2">+2</th>
              <th className="p-2">+1</th>
              <th className="p-2">+0</th>
            </tr>
          </thead>
          <tbody>
            {Object.entries(memory).map(([address, values]) => (
              <tr key={address} className="border-b border-gray-800">
                <td className="p-2 text-yellow-400">{address}</td>
                {values.slice().reverse().map((value, index) => (
  <td key={index} className="p-2">
    <input
      type="text"
      className="bg-gray-800 border border-gray-600 p-1 rounded w-20"
      value={Convertor(value, displayMode)}
      onChange={(e) => handleMemoryChange(address, 3 - index, e.target.value)}
    />
  </td>
                ))}
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
    </div>
  );
}
