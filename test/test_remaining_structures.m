function test_remaining_structures()
%TEST_REMAINING_STRUCTURES Smoke tests for curve network, volume mesh,
%volume grid, sparse volume grid, and camera view bindings.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    ps.remove_everything();

    % === Curve network ====================================================
    nodes = [0, 0, 0; 1, 0, 0; 1, 1, 0; 0, 1, 0];
    edges = [1, 2; 2, 3; 3, 4; 4, 1];  % MATLAB 1-based
    cn = ps.register_curve_network('square', nodes, edges);
    assert(cn.n_nodes() == 4, 'Curve network node count mismatch');
    assert(cn.n_edges() == 4, 'Curve network edge count mismatch');
    cn.add_node_scalar_quantity('node_vals', [0; 1; 2; 3]);
    cn.add_edge_scalar_quantity('edge_vals', [0; 1; 2; 3]);
    cn.add_node_color_quantity('node_colors', [1, 0, 0; 0, 1, 0; 0, 0, 1; 1, 1, 0]);
    cn.add_edge_vector_quantity('edge_vecs', [1, 0, 0; 0, 1, 0; -1, 0, 0; 0, -1, 0]);
    cn.set_color([1, 0, 0]);
    c = cn.get_color();
    assert(abs(c(1) - 1) < 1e-6, 'Curve network color get/set failed');
    cn.set_radius(0.02, false);
    assert(abs(cn.get_radius() - 0.02) < 1e-5, 'Curve network radius get/set failed');

    cn2 = ps.register_curve_network_line('line', [0, 0; 1, 1]);
    assert(cn2.n_nodes() == 2, 'Curve network line node count mismatch');
    assert(cn2.n_edges() == 1, 'Curve network line edge count mismatch');

    % === Volume mesh ======================================================
    vertices = [0, 0, 0; 1, 0, 0; 0, 1, 0; 0, 0, 1];
    tets = [1, 2, 3, 4];  % single tet, 1-based
    vm = ps.register_tet_mesh('single_tet', vertices, tets);
    assert(vm.n_vertices() == 4, 'Volume mesh vertex count mismatch');
    assert(vm.n_cells() == 1, 'Volume mesh cell count mismatch');
    vm.add_vertex_scalar_quantity('vvals', [0; 1; 2; 3]);
    vm.add_cell_scalar_quantity('cvals', [1]);
    vm.add_vertex_color_quantity('vcolors', [1, 0, 0; 0, 1, 0; 0, 0, 1; 1, 1, 1]);
    vm.add_cell_vector_quantity('cvec', [0, 0, 1]);
    vm.set_color([0, 1, 0]);
    c = vm.get_color();
    assert(abs(c(2) - 1) < 1e-6, 'Volume mesh color get/set failed');

    % === Volume grid ======================================================
    node_dims = uint32([5, 5, 5]);
    bound_low = [0, 0, 0];
    bound_high = [1, 1, 1];
    vg = ps.register_volume_grid('vg', node_dims, bound_low, bound_high);
    assert(vg.n_nodes() == 125, 'Volume grid node count mismatch');
    assert(vg.n_cells() == 64, 'Volume grid cell count mismatch');
    vals3d = rand(5, 5, 5, 'single');
    vg.add_node_scalar_quantity('rand_nodes', vals3d);
    cvals3d = rand(4, 4, 4, 'single');
    vg.add_cell_scalar_quantity('rand_cells', cvals3d);
    vg.set_cube_size_factor(0.9);
    assert(abs(vg.get_cube_size_factor() - 0.9) < 1e-6, 'Volume grid cube size factor failed');
    d = vg.get_grid_node_dim();
    assert(isequal(d, [5; 5; 5]), 'Volume grid node dim mismatch');

    % === Sparse volume grid ===============================================
    origin = [0, 0, 0];
    cell_width = [0.1, 0.1, 0.1];
    occupied_cells = [1, 1, 1; 2, 1, 1];  % 1-based, two adjacent cells in x
    svg = ps.register_sparse_volume_grid('svg', origin, cell_width, occupied_cells);
    assert(svg.n_cells() == 2, 'Sparse volume grid cell count mismatch');
    svg.add_cell_scalar_quantity('cell_vals', [1; 2]);
    svg.add_cell_color_quantity('cell_colors', [1, 0, 0; 0, 1, 0]);
    svg.set_render_mode('wireframe');
    svg.set_wireframe_radius(0.5);
    occ = svg.get_occupied_cells();
    assert(size(occ, 1) == 2 && size(occ, 2) == 3, 'Occupied cells size mismatch');
    assert(isequal(occ(1, :), [1, 1, 1]), 'Occupied cells not 1-based');

    % Node scalar/color on sparse grid (node indices are 1-based in MATLAB)
    svg.mark_nodes_as_used();
    node_indices = [1,1,1; 2,1,1; 3,1,1; 1,2,1; 2,2,1; 3,2,1; ...
                    1,1,2; 2,1,2; 3,1,2; 1,2,2; 2,2,2; 3,2,2];
    node_vals = (1:12)';
    svg.add_node_scalar_quantity('node_vals', node_indices, node_vals);
    node_colors = rand(12, 3);
    svg.add_node_color_quantity('node_colors', node_indices, node_colors);

    % === Camera view ======================================================
    cv = ps.register_camera_view('cam', [2, 2, 2], [-1, -1, -1], [0, 0, 1], 45, 1.333);
    p = cv.get_camera_parameters();
    assert(isstruct(p), 'Camera parameters should be a struct');
    assert(abs(p.fov_vertical_deg - 45) < 1e-4, 'Camera FOV mismatch');
    assert(abs(p.aspect_ratio - 1.333) < 1e-4, 'Camera aspect mismatch');
    cv.set_widget_thickness(2.0);
    assert(abs(cv.get_widget_thickness() - 2.0) < 1e-4, 'Camera widget thickness mismatch');
    cv.set_widget_color([1, 0, 0]);
    c = cv.get_widget_color();
    assert(abs(c(1) - 1) < 1e-6, 'Camera widget color mismatch');

    % Render a frame
    ps.frame_tick();

    ps.shutdown();
    fprintf('test_remaining_structures passed\n');
end
