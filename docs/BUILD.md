# Building Unified Engineering Workbench from Source

This guide provides instructions for compiling the Unified Engineering Workbench C++ core, backend server, tests, and standalone launcher from source code using CMake.

---

## 1. Prerequisites

- **Operating System:** Windows 11 x64 (or Windows 10 x64)
- **C++ Compiler:** Any standard C++17 compliant compiler:
  - MinGW-w64 (GCC 10 or later; GCC 16.2.0 recommended)
  - Clang/LLVM (v14 or later)
  - Microsoft Visual Studio 2019/2022 (MSVC v142/v143)
- **Build System:** CMake v3.16 or later
- **Python (Optional for plotting):** Python 3.8+ with `matplotlib` (only needed if regenerating PNG plot graphics from raw CSVs).

*Note:* All third-party C++ libraries (`doctest.h`, `nlohmann/json.hpp`, `httplib.h`) and frontend libraries (`three.min.js`) are vendored in `third_party/` and `web/js/` respectively. No package manager (`vcpkg`, `conan`, `npm`) is required.

---

## 2. Standard CMake Build Procedure

Open PowerShell or Command Prompt at the repository root:

```powershell
# 1. Configure the build directory for Release with static linking
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# 2. Compile all targets in Release configuration
cmake --build build --config Release
```

For Visual Studio (MSVC):
```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

---

## 3. Generated Build Artifacts

Upon successful compilation, the following binaries are generated:

| Executable Path | Target Role |
| :--- | :--- |
| `Launch.exe` (Root) | Self-contained Windows GUI launcher that opens browser and runs backend |
| `bin/workbench_server.exe` | Localhost HTTP/JSON REST backend server executable |
| `build/workbench_tests.exe` | Unit test suite executable (20 test cases, 1,500 assertions) |
| `build/generate_fea_results.exe` | CLI utility computing 1D FEA convergence and exporting CSVs |
| `build/generate_backprop_results.exe` | CLI utility verifying analytical vs numerical gradients |
| `build/generate_multicore_benchmark.exe` | CLI utility benchmarking 1, 2, 4 worker threads |
| `build/test_file_size_limits.exe` | CLI stress testing harness measuring parser throughput and memory |

---

## 4. Running the Tests

To verify all test cases:
```powershell
.\build\workbench_tests.exe
```

Or execute the automated end-to-end test runner:
```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_all_tests.ps1
```

---

## 5. Compiler Flags & Static Runtime Notice

By default, `CMakeLists.txt` applies static runtime linking:
- **GCC / Clang:** `-static -static-libgcc -static-libstdc++`
- **MSVC:** `/MT` (MultiThreaded Static Runtime)

This ensures that generated executables have zero dependencies on external compiler runtime DLLs (e.g. `libstdc++-6.dll`, `msvcp140.dll`), allowing immediate execution on clean machines without installing redistributable packages.
