function test_surface_mesh()
%TEST_SURFACE_MESH Smoke test for surface mesh bindings.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');

    vertices = [0, 0, 0; 1, 0, 0; 0, 1, 0; 0, 0, 1];
    faces = [1, 2, 3; 1, 2, 4; 1, 3, 4; 2, 3, 4];
    sm = ps.register_surface_mesh('tet', vertices, faces);

    assert(sm.n_vertices() == 4, 'Wrong vertex count');
    assert(sm.n_faces() == 4, 'Wrong face count');

    sm.add_vertex_scalar_quantity('vvals', [1; 2; 3; 4]);
    sm.add_face_scalar_quantity('fvals', [1; 2; 3; 4]);

    sm.set_color([0, 1, 0]);
    c = sm.get_color();
    assert(abs(c(2) - 1) < 1e-6, 'Color get/set failed');

    sm.set_smooth_shade(true);

    ps.frame_tick();
    ps.shutdown();
    fprintf('test_surface_mesh passed\n');
end
