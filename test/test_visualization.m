function test_visualization()
%TEST_VISUALIZATION Interactive visualization test for the MATLAB interface.
%   This test opens a real Polyscope window. It shows for a short number of
%   frames and then closes automatically; close the window earlier if desired.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL3_glfw');
    assert(ps.is_initialized(), 'Polyscope should be initialized');

    % Point cloud
    n = 2000;
    t = linspace(0, 4*pi, n)';
    points = [cos(t), sin(t), t / (4*pi)];
    pc = ps.register_point_cloud('helix', points);
    pc.add_scalar_quantity('height', points(:, 3), ...
                           'onscreen_colorbar_enabled', true, ...
                           'color_map', 'viridis');
    pc.add_color_quantity('rgb', abs(points));
    pc.set_radius(0.01, false);

    % Surface mesh (tetrahedron)
    vertices = [0, 0, 0; 1, 0, 0; 0, 1, 0; 0, 0, 1];
    faces = [1, 2, 3; 1, 2, 4; 1, 3, 4; 2, 3, 4];
    sm = ps.register_surface_mesh('tet', vertices, faces);
    sm.add_vertex_scalar_quantity('vvals', [0; 1; 2; 3], 'onscreen_colorbar_enabled', true);
    sm.set_smooth_shade(true);

    % Curve network (wireframe cube)
    cube_nodes = [0,0,0; 1,0,0; 1,1,0; 0,1,0; 0,0,1; 1,0,1; 1,1,1; 0,1,1];
    cube_edges = [1,2; 2,3; 3,4; 4,1; 5,6; 6,7; 7,8; 8,5; 1,5; 2,6; 3,7; 4,8];
    cn = ps.register_curve_network('cube', cube_nodes, cube_edges);
    cn.add_node_scalar_quantity('cn_vals', (0:7)');

    % Volume mesh (single tetrahedron)
    vm = ps.register_tet_mesh('single_tet', vertices, [1, 2, 3, 4]);
    vm.add_vertex_scalar_quantity('vm_vvals', [0; 1; 2; 3]);

    % Volume grid
    node_dims = uint32([16, 16, 16]);
    bound_low = [-1, -1, -1];
    bound_high = [1, 1, 1];
    vg = ps.register_volume_grid('vg', node_dims, bound_low, bound_high);
    [X, Y, Z] = meshgrid(linspace(-1, 1, 16), linspace(-1, 1, 16), linspace(-1, 1, 16));
    vg.add_node_scalar_quantity('vg_vals', single(sqrt(X.^2 + Y.^2 + Z.^2)));

    % Sparse volume grid
    origin = [-1, -1, -1];
    cell_width = [0.2, 0.2, 0.2];
    occupied_cells = [1,1,1; 2,1,1; 1,2,1; 2,2,1];
    svg = ps.register_sparse_volume_grid('svg', origin, cell_width, occupied_cells);
    svg.add_cell_scalar_quantity('svg_vals', [1; 2; 3; 4]);

    % Camera view
    cv = ps.register_camera_view('cam', [2, 2, 2], [-1, -1, -1], [0, 0, 1], 45, 1.333);
    cv.set_widget_color([1, 0, 0]);

    % Camera / view
    ps.look_at([2, 2, 2], [0, 0, 0.5], false);
    ps.set_background_color([0.1, 0.1, 0.1]);

    fprintf('Showing Polyscope window for a few frames...\n');
    ps.show();   % show 120 frames, then close automatically
    ps.shutdown();
    fprintf('test_visualization passed\n');
end
