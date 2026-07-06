function test_remove_and_has()
%TEST_REMOVE_AND_HAS Verify has_* / remove_* methods for all structure types.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    ps.remove_everything();

    % Point cloud
    pc = ps.register_point_cloud('pc', randn(10, 3));
    assert(ps.has_point_cloud('pc'), 'has_point_cloud failed');
    ps.remove_point_cloud('pc');
    assert(~ps.has_point_cloud('pc'), 'remove_point_cloud failed');

    % Surface mesh
    sm = ps.register_surface_mesh('sm', [0,0,0; 1,0,0; 0,1,0], [1,2,3]);
    assert(ps.has_surface_mesh('sm'), 'has_surface_mesh failed');
    ps.remove_surface_mesh('sm');
    assert(~ps.has_surface_mesh('sm'), 'remove_surface_mesh failed');

    % Curve network
    cn = ps.register_curve_network('cn', [0,0; 1,1], [1,2]);
    assert(ps.has_curve_network('cn'), 'has_curve_network failed');
    ps.remove_curve_network('cn');
    assert(~ps.has_curve_network('cn'), 'remove_curve_network failed');

    % Volume mesh
    vm = ps.register_tet_mesh('vm', [0,0,0; 1,0,0; 0,1,0; 0,0,1], [1,2,3,4]);
    assert(ps.has_volume_mesh('vm'), 'has_volume_mesh failed');
    ps.remove_volume_mesh('vm');
    assert(~ps.has_volume_mesh('vm'), 'remove_volume_mesh failed');

    % Volume grid
    vg = ps.register_volume_grid('vg', uint32([4,4,4]), [0,0,0], [1,1,1]);
    assert(ps.has_volume_grid('vg'), 'has_volume_grid failed');
    ps.remove_volume_grid('vg');
    assert(~ps.has_volume_grid('vg'), 'remove_volume_grid failed');

    % Sparse volume grid
    svg = ps.register_sparse_volume_grid('svg', [0,0,0], [0.1,0.1,0.1], [1,1,1]);
    assert(ps.has_sparse_volume_grid('svg'), 'has_sparse_volume_grid failed');
    ps.remove_sparse_volume_grid('svg');
    assert(~ps.has_sparse_volume_grid('svg'), 'remove_sparse_volume_grid failed');

    % Camera view
    cv = ps.register_camera_view('cv', [1,1,1], [-1,-1,-1], [0,0,1], 45, 1);
    assert(ps.has_camera_view('cv'), 'has_camera_view failed');
    ps.remove_camera_view('cv');
    assert(~ps.has_camera_view('cv'), 'remove_camera_view failed');

    ps.shutdown();
    fprintf('test_remove_and_has passed\n');
end
