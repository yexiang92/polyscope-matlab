function test_point_cloud_quantities()
%TEST_POINT_CLOUD_QUANTITIES Exercise scalar/color/vector/parameterization quantities.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    ps.remove_everything();

    pts = randn(20, 3);
    pc = ps.register_point_cloud('cloud', pts);
    assert(pc.n_points() == 20, 'Point cloud size mismatch');

    pc.add_scalar_quantity('scalar', randn(20, 1), ...
        'datatype', 'symmetric', 'color_map', 'coolwarm');
    pc.add_color_quantity('color', rand(20, 3));
    pc.add_vector_quantity('vec', randn(20, 3), ...
        'vectortype', 'ambient', 'length', 0.1);
    pc.add_parameterization_quantity('param', rand(20, 2), 'style', 'grid');

    pc.update_point_positions(pts + 0.1);

    pc.set_point_render_mode('quad');
    assert(strcmp(pc.get_point_render_mode(), 'quad'), 'Render mode mismatch');

    ps.frame_tick();
    ps.shutdown();
    fprintf('test_point_cloud_quantities passed\n');
end
