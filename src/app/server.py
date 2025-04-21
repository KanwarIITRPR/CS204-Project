import os
import subprocess
from flask import Flask, request, jsonify
from flask_cors import CORS
import logging
app = Flask(__name__)
CORS(app)

memory = {}  # Dictionary to store PC → Machine Code

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
INPUT_PATH = os.path.join(BASE_DIR, "phase1", "example", "input.asm")
OUTPUT_PATH = os.path.join(BASE_DIR, "phase1", "example", "output.mc")
ASSEMBLER_PATH = os.path.join(BASE_DIR, "phase1", "risc-v-assembler", "a.exe")
LOG_PATH = os.path.join(BASE_DIR, "phase1", "example", "log.txt")
REGISTER_PATH = os.path.join(BASE_DIR, "phase1", "example", "preregister.txt")


@app.route("/registers", methods=["GET"])
def get_registers():
    print(f"Looking for register file at: {REGISTER_PATH}")
    if not os.path.exists(REGISTER_PATH):
        logging.warning("Register file not found at: %s", REGISTER_PATH)
        return jsonify({"error": "preregister.txt not found"}), 404

    registers = {}
    try:
        with open(REGISTER_PATH, "r") as file:
            lines = file.readlines()
            print(f"Read {len(lines)} lines from register file")
            lines = lines[9:]  # Skip first 9 lines
            print(f"{len(lines)} lines after skipping header")

        for line in lines:
            parts = line.strip().split()
            if len(parts) == 2:
                reg, value = parts
                if not value.startswith("0x"):
                    value = "0x" + value
                # Optional: validate value is valid hex
                try:
                    int(value, 16)
                    registers[reg] = value
                except ValueError:
                    logging.error("Invalid hex value for %s: %s", reg, value)
            else:
                logging.warning("Malformed register line: %s", line.strip())

        return jsonify({"registers": registers})
    except Exception as e:
        logging.exception("Error reading register file:")
        return jsonify({"error": f"Failed to read register file: {str(e)}"}), 500


@app.route('/read-log', methods=["POST"])
def read_log():
    try:
        data = request.get_json()
        knobs = data.get("knobs", [])
        pipeline_enabled = knobs[0] == 1 if len(knobs) > 0 else False

        phase_folder = "phase3" if pipeline_enabled else "phase1"
        example_folder = "example3" if pipeline_enabled else "example"
        log_path = os.path.join(BASE_DIR, phase_folder, example_folder, "log.txt")

        if not os.path.exists(log_path):
            return jsonify({"error": f"log.txt not found in {phase_folder}/{example_folder}"}), 404

        with open(log_path, "r") as f:
            lines = f.readlines()

        return jsonify({"lines": lines})
    except Exception as e:
        print("Error reading log:", e)
        return jsonify({"error": str(e)}), 500



    
@app.route("/assemble", methods=["POST"])
def assemble():
    try:
        data = request.get_json()
        code = data.get("code", "").strip()
        knobs = data.get("knobs", [])
        pipeline_enabled = knobs[0] == 1 if len(knobs) > 0 else False

        if not code:
            return jsonify({"error": "No assembly code received"}), 400

        # Updated folder names
        phase_folder = "phase3" if pipeline_enabled else "phase1"
        example_folder = "example3" if pipeline_enabled else "example"
        assembler_folder = "risc-v-assembler3" if pipeline_enabled else "risc-v-assembler"

        assembler_path = os.path.join(BASE_DIR, phase_folder, assembler_folder, "a.exe")
        input_path = os.path.join(BASE_DIR, phase_folder, example_folder, "input.asm")
        output_path = os.path.join(BASE_DIR, phase_folder, example_folder, "output.mc")

        with open(input_path, "w") as f:
            f.write(code)

        result = subprocess.run(
            [assembler_path, input_path, output_path],
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            return jsonify({"error": result.stderr}), 500

        with open(output_path, "r") as f:
            machine_code_lines = f.readlines()

        assembled_code = {}
        memory_data = {}
        is_text_section = False
        is_data_section = False

        for index, line in enumerate(machine_code_lines):
            line = line.split("#")[0].strip()
            parts = line.split()

            if "END-OF-TEXT-SEGMENT" in line:
                is_text_section = False
            elif ".text" in line:
                is_text_section = True
                is_data_section = False
                continue
            elif ".data" in line:
                is_data_section = True
                is_text_section = False
                continue
            elif "END-OF-DATA-SEGMENT" in line:
                continue

            if is_text_section and len(parts) >= 2:
                pc, instr = parts[0], parts[1]
                basic_instr = " ".join(parts[2:]) if len(parts) > 2 else ""
                pc = pc.replace("0x", "")
                instr = instr.replace("0x", "")
                assembled_code[pc] = {"machine_code": instr, "basic_code": basic_instr}
                memory[pc] = [instr[i: i + 2].upper() for i in range(0, len(instr), 2)]

            if is_data_section and len(parts) >= 2:
                address, data = parts[0], parts[1]
                address = address.replace("0x", "")
                data = data.replace("0x", "")
                memory_data[address] = data

        return jsonify([{"machine_code": assembled_code}, {"memory_data": memory_data}])

    except Exception as e:
        print("Error: ", e)
        return jsonify({"error": str(e)}), 500



@app.route("/memory", methods=["GET"])
def get_memory():
    return jsonify({"memory": memory})

if __name__ == "__main__":
    app.run(debug=True)