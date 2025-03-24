import os
import subprocess
from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

memory = {}  # ✅ Dictionary to store PC → Machine Code

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
INPUT_PATH = os.path.join(BASE_DIR, "phase1", "example", "input.asm")
OUTPUT_PATH = os.path.join(BASE_DIR, "phase1", "example", "output.mc")
ASSEMBLER_PATH = os.path.join(BASE_DIR, "phase1", "risc-v-assembler", "a.exe")


@app.route("/assemble", methods=["POST"])
def assemble():
    try:
        data = request.get_json()
        code = data.get("code", "").strip()
        # print("data: ", data)
        # print("code: ", code)
        if not code:
            return jsonify({"error": "No assembly code received"}), 400

        # Save the received code to input.asm
        with open(INPUT_PATH, "w") as f:
            f.write(code)

        # Run the assembler
        result = subprocess.run(
            [ASSEMBLER_PATH, INPUT_PATH, OUTPUT_PATH], 
            capture_output=True, 
            text=True
        )

        if result.returncode != 0:
            return jsonify({"error": result.stderr}), 500

        # Read and process the machine code output
        with open(OUTPUT_PATH, "r") as f:
            machine_code_lines = f.readlines()  # ✅ Indented correctly

        assembled_code = {}
        memory_data = {}
        
        is_text_section = False 
        is_data_section = False  
        
        for index, line in enumerate(machine_code_lines):
            # print("line: ", line)
            line = line.split("#")[0].strip()  
            parts = line.split()
            # print("parts: ", parts)
            # Detect section markers
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
                continue  # Skip .data section
            # print("is_text_section: ", is_text_section)
            # print("is_data_section: ", is_data_section)
            # Process only text section
            if is_text_section and len(parts) >= 2:
                pc, instr = parts[0], parts[1]
                basic_instr = " ".join(parts[2:]) if len(parts) > 2 else ""
                pc = pc.replace("0x", "")
                instr = instr.replace("0x", "")
                assembled_code[pc] = {"machine_code": instr, "basic_code": basic_instr}
        
                memory[pc] = [instr[i: i + 2].upper() for i in range(0, len(instr), 2)]

            # Process only data section
            if is_data_section and len(parts) >= 2:
                address, data = parts[0], parts[1]
                address = address.replace("0x", "")
                data = data.replace("0x", "")
                # print("address: ", address)
                # print("data: ", data)
                memory_data[address] = data
            # print("memory_data: ", memory_data)

        return jsonify({"machine_code": assembled_code},{"memory_data":memory_data}) 

    except Exception as e:
        print("Error: ", e)
        return jsonify({"error": str(e)}), 500

@app.route("/memory", methods=["GET"])
def get_memory():
    return jsonify({"memory": memory})

if __name__ == "__main__":
    app.run(debug=True)
