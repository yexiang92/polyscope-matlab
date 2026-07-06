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
- A C++20 compiler (tested with Visual Studio 2022 on Windows).
- Git with submodule support.

## Clone

```bash
git clone --recursive https://github.com/YOUR_USERNAME/polyscope-matlab.git
```

If you already cloned without `--recursive`, run:

```bash
git submodule update --init --recursive
```

## Build

### From MATLAB (recommended on Windows)

```matlab
cd('path/to/polyscope-matlab');
build_mex;
```

This configures CMake, builds `polyscope_mex`, and copies the MEX binary into `src/matlab/+polyscope/private/`.

### From the command line

```bash
cmake -S . -B build_matlab_mex -DBUILD_MATLAB_BINDINGS=ON
cmake --build build_matlab_mex --target polyscope_mex --config Release
```

The MEX binary will be copied automatically to `src/matlab/+polyscope/private/` as a post-build step.

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
