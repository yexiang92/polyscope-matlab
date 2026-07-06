function test_camera_and_view()
%TEST_CAMERA_AND_VIEW Exercise camera, window size and background color APIs.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    ps.remove_everything();

    ps.set_window_size(800, 600);
    sz = ps.get_window_size();
    assert(sz(1) == 800 && sz(2) == 600, 'Window size mismatch');

    ps.set_background_color([0.1, 0.2, 0.3]);
    c = ps.get_background_color();
    assert(abs(c(1)-0.1) < 1e-5 && abs(c(2)-0.2) < 1e-5 && abs(c(3)-0.3) < 1e-5, ...
           'Background color mismatch');

    pc = ps.register_point_cloud('pts', randn(10, 3));
    ps.look_at([2, 2, 2], [0, 0, 0], false);
    ps.reset_camera_to_home_view();

    ps.frame_tick();
    ps.shutdown();
    fprintf('test_camera_and_view passed\n');
end
