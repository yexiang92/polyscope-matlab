function test_basic()
%TEST_BASIC Smoke test for the MATLAB Polyscope interface.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);  % project root
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    assert(ps.is_initialized(), 'Polyscope should be initialized');

    % Register a small point cloud
    points = [0, 0, 0; 1, 0, 0; 0, 1, 0; 0, 0, 1];
    pc = ps.register_point_cloud('test cloud', points);
    assert(pc.n_points() == 4, 'Point cloud should have 4 points');

    % Add a scalar quantity
    vals = [1; 2; 3; 4];
    pc.add_scalar_quantity('vals', vals);

    % Radius get/set
    pc.set_radius(0.05, false);  % use absolute radius
    r = pc.get_radius();
    assert(abs(r - 0.05) < 1e-5, 'Radius get/set failed');

    % Color get/set
    pc.set_color([1, 0, 0]);
    c = pc.get_color();
    assert(abs(c(1) - 1) < 1e-6 && abs(c(2)) < 1e-6 && abs(c(3)) < 1e-6, ...
           'Color get/set failed');

    % Render a frame with the mock backend
    ps.frame_tick();

    ps.shutdown();
    fprintf('test_basic passed\n');
end
