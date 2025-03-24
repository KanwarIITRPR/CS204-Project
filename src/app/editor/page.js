"use client";
import { useState } from "react";
import dynamic from "next/dynamic";
import SimulatorTab from "../simulator/page";

const MonacoEditor = dynamic(() => import("@monaco-editor/react"), { ssr: false });
const BackgroundVideo = () => {
  return (
    <video autoPlay loop muted className="absolute top-0 left-0 w-full h-full object-cover -z-10">
      <source src="/bg1.webm" type="video/mp4" />
      Your browser does not support the video tag.
    </video>
  );
};

export default function EditorPage() {
  const [editorCode, setEditorCode] = useState("# Start coding...");
  const [activeTab, setActiveTab] = useState("editor");
  const [isAssembled, setIsAssembled] = useState(false);
  const [memoryData, setMemoryData] = useState(new Map());
  const [outputData, setOutputData] = useState([]);

  return (
    <div className="relative w-full h-screen">
      <BackgroundVideo />
      <div className="relative z-10 p-4 bg-black/50 text-white rounded-xl">
        <div style={{ maxWidth: "auto", height: "96vh", display: "flex", flexDirection: "column" }}>
          {/* Tabs */}
          <div style={{ display: "flex", justifyContent: "center", marginTop: "20px" }}>
            <div style={{ display: "flex", background: "black", padding: "4px", borderRadius: "8px" }}>
              <button onClick={() => setActiveTab("editor")} 
              style={{
                  padding: "8px 16px",
                  borderRadius: "6px",
                  background: activeTab === "editor" ? "red" : "transparent",
                  fontWeight: activeTab === "editor" ? "bold" : "normal",
                  border: "none",
                  cursor: "pointer",
                  color: "white",
                }}>
                Editor
              </button>
              <button onClick={() => setActiveTab("simulator")} style={{
                  padding: "8px 16px",
                  borderRadius: "6px",
                  background: activeTab === "simulator" ? "red" : "transparent",
                  fontWeight: activeTab === "simulator" ? "bold" : "normal",
                  border: "none",
                  cursor: "pointer",
                  color: "white",
                }}>
                Simulator
              </button>
            </div>
          </div>

          {/* Editor */}
          {activeTab === "editor" && (
            <div style={{ border: "1px solid #ddd", borderRadius: "5px", marginTop: "10px", overflow: "hidden", flex: 1 }}>
              <MonacoEditor
                height="calc(100vh - 150px)"
                defaultLanguage="assembly"
                theme="vs-dark"
                value={editorCode}
                onChange={(value) => setEditorCode(value)}
                options={{ fontSize: 14, minimap: { enabled: false } }}
              />
            </div>
          )}

          {/* Simulator */}
          {activeTab === "simulator" && (
            <SimulatorTab
              editorCode={editorCode}
              isAssembled={isAssembled}
              setIsAssembled={setIsAssembled}
              memoryData={memoryData}
              setMemoryData={setMemoryData}
              outputData={outputData}
              setOutputData={setOutputData}
            />
          )}
        </div>
      </div>
    </div>
  );
}
