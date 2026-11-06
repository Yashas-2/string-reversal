"""
Flask backend for the Parallel String Reversal mini-project.

The web page sends the input string (and, for parallel runs, a thread
count) to this server. The server runs the compiled C/OpenMP program
and returns its JSON result to the browser.
"""

from flask import Flask, render_template, request, jsonify
import json
import os
import subprocess
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent
app = Flask(__name__)

MAX_CHARS = 2_000_000          # keep in sync with MAX_INPUT in reversal.c


def executable_path():
    """Full path to the compiled C program."""
    name = "reversal.exe" if os.name == "nt" else "reversal"
    return BASE_DIR / name


def minGW_path_env():
    """
    On some Windows machines the MinGW folder is not on PATH.
    libgomp-1.dll and pthreadGC-3.dll live there, so we add it
    to PATH for the child process if needed.
    """
    env = os.environ.copy()
    candidates = [
        r"C:\MinGW\bin",
        r"C:\msys64\mingw64\bin",
        r"C:\msys64\mingw32\bin",
        r"C:\mingw64\bin",
    ]
    for folder in candidates:
        if Path(folder).is_dir() and folder not in env.get("PATH", ""):
            env["PATH"] = folder + os.pathsep + env.get("PATH", "")
            break
    return env


@app.get("/")
def index():
    """Serve the single-page web interface."""
    return render_template("index.html")


@app.get("/api/health")
def health():
    """Simple check that the C program has been compiled."""
    exe = executable_path()
    return jsonify({"ok": exe.exists(), "executable": exe.name})


@app.post("/api/reverse")
def reverse():
    """
    Run the C/OpenMP program once.

    Request JSON:
        text    : the string to reverse (required)
        mode    : "sequential" or "parallel" (required)
        threads : integer 2, 4, or 8 — only used when mode == "parallel"
    """
    payload = request.get_json(silent=True) or {}

    # ---- validate text ----
    text = payload.get("text", "")
    if not isinstance(text, str):
        return jsonify({"error": "Text must be a string."}), 400
    if "\n" in text or "\r" in text:
        return jsonify({"error": "Please enter a single line of text."}), 400
    if len(text) > MAX_CHARS:
        return jsonify({"error": f"Please keep the input under {MAX_CHARS:,} characters."}), 400

    # ---- validate mode ----
    mode = payload.get("mode", "sequential")
    if mode not in ("sequential", "parallel"):
        return jsonify({"error": "Mode must be 'sequential' or 'parallel'."}), 400

    # ---- validate threads (only for parallel) ----
    threads = 1
    if mode == "parallel":
        try:
            threads = int(payload.get("threads", 4))
        except (TypeError, ValueError):
            return jsonify({"error": "Thread count must be an integer."}), 400
        if threads not in (2, 4, 8):
            return jsonify({"error": "Choose 2, 4, or 8 threads."}), 400

    # ---- check that the C program exists ----
    exe = executable_path()
    if not exe.exists():
        return jsonify({
            "error": "The C/OpenMP program is not compiled yet. "
                     "Run run.bat or see the README for setup steps."
        }), 500

    # ---- build the command line for the C program ----
    if mode == "parallel":
        cmd = [str(exe), "parallel", str(threads)]
    else:
        cmd = [str(exe), "sequential"]

    # ---- run the C program ----
    try:
        completed = subprocess.run(
            cmd,
            input=text + "\n",       # C program reads one line from stdin
            text=True,
            capture_output=True,
            timeout=20,
            cwd=BASE_DIR,
            check=False,
            env=minGW_path_env(),
        )
    except subprocess.TimeoutExpired:
        return jsonify({"error": "The operation timed out. Try a shorter string."}), 504
    except OSError as exc:
        return jsonify({"error": f"Could not launch the C program: {exc}"}), 500

    if completed.returncode != 0:
        message = completed.stderr.strip() or "The C program failed."
        return jsonify({"error": message}), 500

    # ---- parse the JSON produced by the C program ----
    try:
        result = json.loads(completed.stdout)
    except json.JSONDecodeError:
        return jsonify({"error": "Unexpected response from the C program."}), 500

    return jsonify(result)


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, debug=False)
