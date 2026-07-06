function test_surface_mesh_quantities()
%TEST_SURFACE_MESH_QUANTITIES Exercise vertex/face scalar/color/vector quantities.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    ps.remove_everything();

    vertices = [0,0,0; 1,0,0; 0,1,0; 0,0,1];
    faces = [1,2,3; 1,2,4; 1,3,4; 2,3,4];
    sm = ps.register_surface_mesh('mesh', vertices, faces);

    assert(sm.n_vertices() == 4, 'Vertex count mismatch');
    assert(sm.n_faces() == 4, 'Face count mismatch');

    sm.add_vertex_scalar_quantity('vs', [0; 1; 2; 3], 'map_range', [0, 3]);
    sm.add_face_scalar_quantity('fs', [0; 1; 2; 3]);
    sm.add_vertex_color_quantity('vc', rand(4, 3));
    sm.add_face_color_quantity('fc', rand(4, 3));
    sm.add_vertex_vector_quantity('vv', randn(4, 3), 'length', 0.2);
    sm.add_face_vector_quantity('fv', randn(4, 3), 'vectortype', 'standard');

    sm.update_vertex_positions(vertices * 2);
    sm.set_edge_width(2.0);
    sm.set_smooth_shade(true);

    ps.frame_tick();
    ps.shutdown();
    fprintf('test_surface_mesh_quantities passed\n');
end
