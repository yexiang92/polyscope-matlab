# Polyscope MATLAB Interface

This directory contains the MATLAB package and the C++ MEX sources that implement the bindings.

## Building

The fastest way is to run the provided MATLAB script from the repository root:

```matlab
cd('path/to/polyscope-matlab');
build_mex;
```

This configures CMake, builds the `polyscope_mex` MEX file, and copies it into `+polyscope/private/`.

Alternatively, from a terminal:

```bash
cmake -S . -B build_matlab_mex -DBUILD_MATLAB_BINDINGS=ON
cmake --build build_matlab_mex --target polyscope_mex --config Release
```

## Usage

Add `src/matlab` to your MATLAB path, then:

```matlab
ps = polyscope.Polyscope;
ps.init('openGL_mock');   % use openGL3_glfw for an interactive window

% Point cloud
points = randn(100, 3);
pc = ps.register_point_cloud('my cloud', points);
pc.add_scalar_quantity('height', points(:, 3));

% Surface mesh (face indices are 1-based, as in MATLAB)
vertices = [0,0,0; 1,0,0; 0,1,0; 0,0,1];
faces = [1,2,3; 1,2,4; 1,3,4; 2,3,4];
sm = ps.register_surface_mesh('tet', vertices, faces);
sm.add_vertex_scalar_quantity('v', [1;2;3;4]);

ps.frame_tick();   % render one frame
ps.show();         % interactive window (openGL3_glfw backend)
ps.shutdown();
```

## Currently supported APIs

### Core (`polyscope.Polyscope`)

- `init(backend)`
- `show()`, `unshow()`, `frame_tick()`, `shutdown()`
- `is_initialized()`, `check_initialized()`
- `remove_all_structures()`, `remove_everything()`
- Window: `set_window_size(w, h)`, `get_window_size()`, `show_window()`, `focus_window()`
- Camera: `reset_camera_to_home_view()`, `look_at(eye, target, flyTo)`
- Background: `set_background_color(c)`, `get_background_color()`
- Screenshot: `screenshot(filename, 'transparent_bg', true, 'include_ui', false)`
- Options: `set_program_name`, `set_verbosity`, `set_max_fps`, etc.
- Safe callback loop: `set_user_callback(@cb)` followed by `ps.show()` or `ps.show(nFrames)`.

### Point Cloud (`polyscope.PointCloud`)

- `n_points()`, `update_point_positions(points)`
- `set_enabled`, `set_radius(rad, relative)`, `get_radius`
- `set_color`, `get_color`, `set_material`, `get_material`
- `set_point_render_mode('sphere'|'quad')`, `get_point_render_mode`
- `set_transparency`
- Quantities: `add_scalar_quantity`, `add_color_quantity`, `add_vector_quantity`, `add_parameterization_quantity`

### Surface Mesh (`polyscope.SurfaceMesh`)

- `n_vertices()`, `n_faces()`, `update_vertex_positions(vertices)`
- `set_enabled`, `set_color`, `get_color`, `set_edge_color`, `set_edge_width`
- `set_material`, `get_material`, `set_smooth_shade`, `set_transparency`
- Quantities:
  - `add_vertex_scalar_quantity`, `add_face_scalar_quantity`
  - `add_vertex_color_quantity`, `add_face_color_quantity`
  - `add_vertex_vector_quantity`, `add_face_vector_quantity`

### Quantity options

All `add_*_quantity` methods accept trailing name-value pairs that mirror the Python keyword arguments:

```matlab
pc.add_scalar_quantity('height', vals, ...
    'datatype', 'symmetric', ...
    'color_map', 'viridis', ...
    'map_range', [0, 1], ...
    'onscreen_colorbar_enabled', true);

pc.add_vector_quantity('vec', vecs, ...
    'vectortype', 'ambient', ...
    'length', 0.1, ...
    'color', [1, 0, 0]);
```

Supported scalar options: `enabled`, `datatype`, `color_map`, `cmap`, `map_range`, `vmin`/`vmax`, `onscreen_colorbar_enabled`, `onscreen_colorbar_location`, `isolines_enabled`, `isoline_style`, `isoline_period`, `isoline_period_relative`, `isoline_darkness`, `isoline_contour_thickness`.

Supported vector options: `enabled`, `vectortype`, `length`, `length_relative`, `length_range`, `radius`, `radius_relative`, `color`, `material`.

Supported parameterization options: `enabled`, `coords_type`, `style`, `grid_colors`, `checker_colors`, `checker_size`, `color_map`, `alt_darkness`.

### ImGui / ImPlot

Custom UIs can be built via `polyscope.ImGui` and `polyscope.ImPlot` inside the split-frame render loop or inside a callback registered with `ps.set_user_callback(@cb)`.

## Notes

- **1-based indices for meshes**: Face indices passed to `register_surface_mesh` are expected to be 1-based (MATLAB convention). They are converted to 0-based internally before being sent to Polyscope.
- **Column-major data**: MATLAB arrays are already column-major, matching Eigen's default layout, so N×3 vertex arrays are copied directly.
- **Backends**: Use `openGL_mock` for headless/unit testing, `openGL3_glfw` for interactive windows, and `openGL3_egl` on Linux headless servers if available.
