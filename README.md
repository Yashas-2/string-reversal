# Parallel String Reversal Using OpenMP

A small academic mini-project for a Parallel Programming course.
It demonstrates the difference between **sequential** and **parallel**
string reversal using C and OpenMP, with a simple dark-themed web page
to run the comparison.

---

## What it does

1. You type a string into the web page.
2. Click **Sequential Reverse** — a single-threaded C loop reverses the string.
3. Click **Parallel Reverse** — OpenMP splits the same loop across 2, 4, or 8 threads.
4. The page shows the reversed output, the execution time for each run,
   a correctness check, and the speedup after both runs are done.

```
Speedup = Sequential time ÷ Parallel time
```

> Parallel execution is **not** always faster. Thread start-up overhead
> can make short strings slower in parallel. That is part of the lesson.

---

## Project files

| File | Purpose |
|------|---------|
| `reversal.c` | C source with sequential and OpenMP parallel reversal |
| `app.py` | Flask web server that runs the C program |
| `templates/index.html` | Single-page web interface |
| `static/styles.css` | Dark theme styling |
| `static/app.js` | Front-end logic (buttons, results, comparison) |
| `run.bat` | One-click setup and launch for Windows |
| `requirements.txt` | Python dependencies (Flask only) |

---

## Requirements (Windows)

1. **Python 3.10 or newer** — [python.org](https://www.python.org/downloads/)
   (tick *Add Python to PATH* during install)
2. **GCC with OpenMP support** — a MinGW-w64 toolchain that provides `gcc`
   and `libgomp`. Common locations:
   - `C:\MinGW\bin\gcc.exe`
   - `C:\msys64\mingw64\bin\gcc.exe`

---

## Quick start (Windows)

### Option A — one-click

1. Double-click **`run.bat`**.
2. Wait for compilation and package install to finish.
3. Your browser opens `http://127.0.0.1:5000`.

### Option B — manual (PowerShell)

Open PowerShell **in this folder** and run:

```powershell
# 1. Put GCC on PATH for this session (adjust the path if yours differs)
$env:PATH = "C:\MinGW\bin;$env:PATH"

# 2. Compile the C / OpenMP program
gcc -O2 -Wall -Wextra -fopenmp reversal.c -o reversal.exe

# 3. Create a virtual environment and install Flask
py -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt

# 4. Start the web server
.\.venv\Scripts\python.exe app.py
```

Then open **http://127.0.0.1:5000** in your browser.

---

## How to use the page

1. **Enter a string** in the text box (single line).
2. **Sequential Reverse** — runs the single-threaded algorithm.
3. **Parallel Reverse** — pick 2, 4, or 8 threads first, then click.
4. After both runs, the **Comparison** card appears with both times
   and the calculated speedup.

> **Tip for the demo:** use a longer string (e.g. copy a paragraph)
> so the timing differences are clearly visible. Very short strings
> may reverse in less than one timer tick.

---

## How the algorithm works

To reverse a string of length `n`, swap character pairs:

```
s[0] ↔ s[n-1]
s[1] ↔ s[n-2]
...
s[i] ↔ s[n-1-i]     for i = 0 .. (n/2)-1
```

Every iteration touches a **different** pair of positions, so the
iterations are independent. OpenMP can safely distribute them:

```c
#pragma omp parallel for num_threads(threads) schedule(static)
for (long long i = 0; i < n / 2; ++i) {
    char temp    = s[i];
    s[i]         = s[n - 1 - i];
    s[n - 1 - i] = temp;
}
```

Time is measured with `omp_get_wtime()`.

Both algorithms do **O(n)** total work. The parallel version trades
extra coordination overhead for the chance to finish sooner on many cores.

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `gcc was not found` | Install MinGW-w64 and add its `bin` folder to PATH, or edit `run.bat` to point at your install. |
| `cannot find -lpthread` | Your MinGW may be missing the pthread import library. See the project notes or reinstall a full MinGW-w64 build. |
| `The C/OpenMP program is not compiled yet` | Run step 2 above (`gcc ... -o reversal.exe`). |
| Timings show `0.000000 s` | The string is too short for the timer resolution. Use a longer input. |
| `libgomp-1.dll not found` | Make sure the MinGW `bin` folder is on PATH (the Flask app tries to add it automatically). |

---

## Notes for your report / viva

- Both algorithms perform O(n) work (each character is swapped at most once).
- OpenMP uses **shared memory**: all threads see the same `s[]` array.
- Each loop iteration writes only to its own pair `(i, n-1-i)`, so no locks
  or critical sections are required.
- `schedule(static)` gives each thread a fixed chunk of iterations up front.
- Parallel overhead (thread creation, scheduling) can exceed the benefit
  for small inputs — always measure, never assume.
- Run each configuration several times and average for more reliable numbers.
