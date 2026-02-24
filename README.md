# clothsim

A real-time cloth simulation written in **C++17** using **OpenGL 3.3**, **GLM**, and **Dear ImGui**. Physics are based on a mass-spring model with Verlet integration and maximum-stretch constraint satisfaction, following [Matt Fisher's Cloth Tutorial (Stanford, 2014)](https://graphics.stanford.edu/~mdfisher/cloth.html).

---

## Features

- Mass-spring cloth (structural, shear, and bending springs)
- Verlet integration with maximum-stretch constraint loop
- Cloth–sphere and cloth–self collision (marble algorithm)
- Phong shading with optional normal-color debug mode
- Full Dear ImGui panel — all physics parameters adjustable at runtime

---

## Dependencies

| Library | Version | How it's included |
|---------|---------|-------------------|
| [GLFW](https://github.com/glfw/glfw) | 3.4 | Git submodule |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Git submodule |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.x | Git submodule |
| [GLAD](https://glad.dav1d.de) | OpenGL 3.3 Core | Pre-generated, committed to repo |
| CMake | 3.20+ | Must be installed separately |
| Xcode | 15+ | Must be installed separately (macOS) |

> **Why is GLAD not a submodule?**
> GLAD is a *code generator*, not a static library — its output depends on which OpenGL version and profile you select. Rather than pulling in the full generator toolchain as a dependency, we generate the files once (OpenGL 3.3 Core, no extensions) and commit them directly under `external/glad/`. They are small (two headers + one `.c` file) and never need to change for this project.

---

## Prerequisites

Install the following before cloning:

```bash
# Xcode (from the Mac App Store, or via command line tools)
xcode-select --install

# Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# CMake
brew install cmake
```

---

## Getting Started

### 1. Clone the repo with submodules

```bash
git clone --recursive https://github.com/YOUR_USERNAME/clothsim.git
cd clothsim
```

If you already cloned without `--recursive`, fetch the submodules now:

```bash
git submodule update --init --recursive
```

### 2. Generate the Xcode project with CMake

```bash
mkdir build
cd build
cmake .. -G Xcode
```

This produces a `clothsim.xcodeproj` inside the `build/` directory.

### 3. Open in Xcode

```bash
open clothsim.xcodeproj
```

Or double-click `clothsim.xcodeproj` in Finder.

### 4. Build and run

In Xcode, select the **clothsim** scheme from the scheme selector at the top, then press **⌘R** to build and run.

> You can also build from the command line without opening Xcode:
> ```bash
> cmake --build . --config Debug
> ```

---

## Project Structure

```
clothsim/
├── CMakeLists.txt          # Build definition
├── README.md
├── CLAUDE.md               # AI assistant project plan
│
├── external/               # Third-party dependencies
│   ├── glfw/               # Git submodule
│   ├── glm/                # Git submodule
│   ├── imgui/              # Git submodule
│   └── glad/               # Pre-generated GL loader (committed directly)
│       ├── include/
│       │   ├── glad/gl.h
│       │   └── KHR/khrplatform.h
│       └── src/
│           └── gl.c
│
├── src/
│   ├── main.cpp
│   ├── Cloth.h / Cloth.cpp
│   ├── Particle.h
│   ├── Spring.h
│   ├── Renderer.h / Renderer.cpp
│   ├── Shader.h / Shader.cpp
│   └── UI.h / UI.cpp
│
├── shaders/
│   ├── cloth.vert
│   └── cloth.frag
│
└── assets/
```

---

## CMakeLists.txt

Below is the full `CMakeLists.txt` for the project. Copy this to the root of the repo.

```cmake
cmake_minimum_required(VERSION 3.20)
project(clothsim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── GLFW ──────────────────────────────────────────────────────────────────────
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# ── GLM (header-only) ─────────────────────────────────────────────────────────
add_subdirectory(external/glm)

# ── Dear ImGui ────────────────────────────────────────────────────────────────
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# ── GLAD ──────────────────────────────────────────────────────────────────────
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ── Main executable ───────────────────────────────────────────────────────────
file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h)

add_executable(clothsim ${SRC_FILES})

target_include_directories(clothsim PRIVATE
    src/
    external/glm
    external/glad/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(clothsim PRIVATE
    glfw
    glad
    imgui
    glm
)

# macOS: link required system frameworks
if(APPLE)
    target_link_libraries(clothsim PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

# Copy shaders next to the executable so relative paths work
add_custom_command(TARGET clothsim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:clothsim>/shaders
    COMMENT "Copying shaders to build directory"
)
```

---

## Setting Up Git Submodules (Initial Repo Setup)

> **Skip this section** if you cloned with `--recursive` — the submodules are already populated.

These are the commands used to add the submodules when the repo was first created. They are recorded here for reference.

```bash
# From the root of the repo:
git submodule add https://github.com/glfw/glfw.git      external/glfw
git submodule add https://github.com/g-truc/glm.git     external/glm
git submodule add https://github.com/ocornut/imgui.git  external/imgui

# Pin ImGui to the docking branch (recommended for stability)
cd external/imgui && git checkout docking && cd ../..

# Commit the .gitmodules file and submodule pointers
git add .gitmodules external/glfw external/glm external/imgui
git commit -m "chore: add glfw, glm, imgui as submodules"
```

---

## Generating GLAD (Initial Setup / Regeneration Only)

GLAD files are already committed to this repo under `external/glad/`. You **do not need to run this** to build the project.

If you ever need to regenerate them (e.g. to switch to a different OpenGL version):

1. Go to [https://glad.dav1d.de](https://glad.dav1d.de)
2. Set the following options:
   - **Language**: C/C++
   - **Specification**: OpenGL
   - **API → gl**: Version **3.3**
   - **Profile**: **Core**
   - **Options**: check **Generate a loader**
3. Click **Generate** and download the zip.
4. Extract and replace the contents of `external/glad/` with the new `include/` and `src/` folders.
5. Commit the updated files.

---

## Updating Submodules

To pull the latest commits on all submodules:

```bash
git submodule update --remote --merge
git add external/
git commit -m "chore: update submodules"
```

---

## Troubleshooting

**CMake can't find Xcode / "No CMAKE_C_COMPILER could be found"**
Run `xcode-select --install` and accept the license with `sudo xcodebuild -license accept`.

**`fatal: No url found for submodule path 'external/glfw'` (or similar)**
Run `git submodule update --init --recursive` from the repo root.

**Blank window / OpenGL errors on Apple Silicon**
Ensure `GLFW_OPENGL_FORWARD_COMPAT` is set to `GL_TRUE` in the window hints (already handled in `main.cpp`). macOS requires the forward-compatible flag for core profile contexts.

**Shaders not found at runtime**
Make sure you are running the executable from Xcode (which sets the working directory to the build output folder where shaders are copied). If running from Terminal, `cd` to the directory containing the `clothsim` binary first.

---

## Reference

- [Matt Fisher — Cloth (Stanford 2014)](https://graphics.stanford.edu/~mdfisher/cloth.html) — primary reference for physics and collision approaches
- [Baraff & Witkin — Large Steps in Cloth Simulation (SIGGRAPH 98)](http://www-2.cs.cmu.edu/~baraff/papers/sig98.pdf) — advanced elasticity model and implicit integration
- [LearnOpenGL](https://learnopengl.com) — OpenGL fundamentals and shader reference
- [Dear ImGui](https://github.com/ocornut/imgui)
- [GLFW Documentation](https://www.glfw.org/docs/latest/)
- [GLM Manual](https://github.com/g-truc/glm/blob/master/manual.md)

---

## License

MIT
