import React, { useState } from 'react';
import './burgermenu.css'; 

const knobDescriptions = [
  "Pipelining",
  "Data forwarding",
  "Register file print at each cycle",
  "Pipeline register print each cycle",
  "Enable specific instruction pipeline trace",
  "Branch prediction unit print"
];

export default function BurgerMenu({ code, onLogLoaded }) {
  const [menuOpen, setMenuOpen] = useState(false);
  const [knobs, setKnobs] = useState([0, 0, 0, 0, 0, 0]);
  const [simulationOutput, setSimulationOutput] = useState(null);
  const [logOutput, setLogOutput] = useState(null);

  const toggleKnob = (index, value) => {
    const updated = [...knobs];
    updated[index] = value;
    setKnobs(updated);
  };

  const closeMenu = () => {
    setMenuOpen(false);
  };

  const runSimulation = async () => {
    try {
      // First, run the simulator
      const simulationResponse = await fetch('http://localhost:5000/assemble', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          code: code,
          knobs: knobs
        }),
      });
      
      const simulationResult = await simulationResponse.json();
      console.log("Output:", simulationResult.output);
      setSimulationOutput(simulationResult.output);
      
      if (simulationResult.error) {
        console.error("Error:", simulationResult.error);
        setSimulationOutput(`Error: ${simulationResult.error}`);
        return; // Don't continue if simulation failed
      }
      
      // Then, read the log file
      const logResponse = await fetch("http://localhost:5000/read-log", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({ knobs }),
      });
  
      const logResult = await logResponse.json();
  
      if (logResult.error) {
        console.error("Log error:", logResult.error);
        setLogOutput(`Error: ${logResult.error}`);
      } else {
        console.log("Log lines:", logResult.lines);
        setLogOutput(logResult.lines.join(''));
        
        // If a callback is provided, use it to pass the log data to the parent component
        if (onLogLoaded) {
          onLogLoaded(logResult.lines);
        }
      }
    } catch (err) {
      console.error("Request failed:", err);
      setSimulationOutput(`Request failed: ${err.message}`);
    }
  };

  return (
    <div className="burger-menu-container">
      {!menuOpen && (
        <button className="burger-button" onClick={() => setMenuOpen(true)}>
          ☰
        </button>
      )}

      <div className={`menu-panel ${menuOpen ? 'open' : ''}`}>
        <div className="cancel-button-container">
          <button className="cancel-button" onClick={closeMenu}>
            X
          </button>
        </div>
        <h3 className="menu-title">Execution Knobs</h3>
        {knobDescriptions.map((desc, index) => (
          <div key={index} className="knob-option">
            <div className="knob-label">
              <span className="knob-index">{index + 1}.</span>
              <span className="knob-text">{desc}</span>
            </div>
            <label className="switch">
              <input
                type="checkbox"
                checked={knobs[index] === 1}
                onChange={(e) => toggleKnob(index, e.target.checked ? 1 : 0)}
              />
              <span className="slider round"></span>
            </label>
            <span className={`knob-status ${knobs[index] ? 'enabled' : 'disabled'}`}>
              {knobs[index] === 1 ? 'Enabled' : 'Disabled'}
            </span>
          </div>
        ))}

        <div className="run-button-container">
          <button className="run-button" onClick={runSimulation}>
            Run Simulation
          </button>
        </div>

        {simulationOutput && (
          <div className="output-panel">
            <h4>Simulation Output</h4>
            <pre>{simulationOutput}</pre>
          </div>
        )}

        {logOutput && (
          <div className="output-panel">
            <h4>Log Output {knobs[0] ? '(Pipeline Enabled)' : '(Pipeline Disabled)'}</h4>
            <pre>{logOutput}</pre>
          </div>
        )}
      </div>
    </div>
  );
}