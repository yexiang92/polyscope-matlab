# Polyscope MATLAB Interface

MATLAB MEX bindings for [Polyscope](https://polyscope.run/), a lightweight C++ & Python viewer for 3D data such as point clouds, surface meshes, curve networks, and volume meshes.

This repository contains **only** the MATLAB interface. The Polyscope C++ library and Eigen are included as Git submodules.

## Features

- Register and visualize point clouds, surface meshes, curve networks, volume meshes, volume grids, sparse volume grids, and camera views.
- Control the viewer window, camera, screenshots, and background color.
- Build custom MATLAB GUIs using the included ImGui and ImPlot bindings.
- Safe split-frame render API (`frame_begin` / `frame_end`) that avoids the unsafe C++ → MATLAB callback re-entry that crashes inside a MEX-file.

## Requirements

- MATLAB R2018a or newer with the C++ MEX API (`MatlabDataArray` / `cppmex`).
- CMake ≥ 3.15.
- A C++20 compiler (tested with Visual Studio 2022 on Windows and Apple Clang on macOS).
- Git with submodule support.

The supported release platforms are Windows x86-64 (`.mexw64`) and native Apple-silicon macOS (`.mexmaca64`). `build_mex.m` uses `mexext` to detect the correct MEX extension for the running MATLAB. Apple-silicon builds require MATLAB R2023b or newer.

## Clone

```bash
git clone --recursive https://github.com/yexiang92/polyscope-matlab.git
```

If you already cloned without `--recursive`, run:

```bash
git submodule update --init --recursive
```

## Build

### From MATLAB (recommended)

```matlab
cd('path/to/polyscope-matlab');
build_mex;
```

This configures CMake, builds `polyscope_mex`, and copies the MEX binary into `src/matlab/+polyscope/private/`.

## Package and deploy

Treat `src/matlab/+polyscope` as the single source of truth. Generate a
standalone runtime package (MATLAB wrappers plus the MEX binary) with:

```matlab
release_polyscope
```

The package is written to `dist/matlab/+polyscope`. To generate and
synchronize an embedded copy in another project in one
operation, pass the destination package directory:

```matlab
release_polyscope(SyncTarget= ...
    "D:\OpenSeesMatlab\OpenSeesMatlab\OpenSeesMatlab\+plotter\+polyscope\vendor\+polyscope")
```

Synchronization replaces the complete destination package. It temporarily
backs up an existing destination and restores it automatically if copying
fails. A `POLYSCOPE_MATLAB_VERSION.txt` manifest records the source revision,
dirty-worktree state, and MATLAB release.

### From the command line

```bash
cmake -S . -B build_matlab_mex -DBUILD_MATLAB_BINDINGS=ON
cmake --build build_matlab_mex --target polyscope_mex --config Release
```

On macOS you may want to disable the GLFW backend if no display is available:

```bash
cmake -S . -B build_matlab_mex -DBUILD_MATLAB_BINDINGS=ON -DPOLYSCOPE_BACKEND_OPENGL3_GLFW=OFF
```

The MEX binary will be copied automatically to `src/matlab/+polyscope/private/` as a post-build step.

## GitHub Actions artifacts

Every workflow run builds the interactive GLFW backend and the headless mock
backend, executes the MATLAB binding tests, and uploads one runtime package per
platform:

- `polyscope-matlab-windows-x86_64`
- `polyscope-matlab-macos-arm64`

The macOS job uses an ARM64 GitHub-hosted runner and verifies that
`polyscope_mex.mexmaca64` is an Apple-silicon Mach-O binary without Homebrew
runtime paths. It targets macOS 12 or newer so the binary is not tied to the
runner's newer macOS release.

After both jobs pass, Actions verifies that all MATLAB files are identical and
combines the two binaries into one artifact named
`polyscope-matlab-supported-platforms`. Its archive contains a single shared
`+polyscope` package with both `private/polyscope_mex.mexw64` and
`private/polyscope_mex.mexmaca64`. Extract it and place that `+polyscope`
directory directly at:

```text
D:\OpenSeesMatlab\OpenSeesMatlab\OpenSeesMatlab\+plotter\+polyscope\vendor\+polyscope
```

MATLAB automatically loads the MEX extension for the current platform; no
platform-selection MATLAB code is needed.

### Publish a GitHub Release

Open **Actions → MATLAB MEX → Run workflow**. Enter a tag such as `v1.0.0`
in **release_tag** and run it. After both platform builds and MATLAB binding
tests pass, the workflow creates that GitHub Release and uploads the single
combined package. Leave **release_tag** empty when you only want to compile
and test. Re-running an existing tag replaces the package asset.

## Usage

Add `src/matlab` to your MATLAB path, then:

```matlab
ps = polyscope.Polyscope;
ps.init('openGL3_glfw');   % or 'openGL_mock' for headless testing

pc = ps.register_point_cloud('pts', randn(100, 3));
pc.add_scalar_quantity('height', randn(100, 1));

ps.show();
ps.shutdown();
```

See `test/matlab/` for more examples, including custom ImGui/ImPlot UIs.

## Repository layout

```
deps/polyscope   # Polyscope C++ library (submodule)
deps/eigen       # Eigen (submodule)
src/matlab       # MATLAB package + MEX C++ sources
test/matlab      # MATLAB tests
```

## License

This project is released under the MIT License. See `LICENSE`.

Polyscope and Eigen retain their own licenses.
