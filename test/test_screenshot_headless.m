function test_screenshot_headless()
%TEST_SCREENSHOT_HEADLESS Verify that screenshot() works with the mock backend.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    ps.remove_everything();

    pc = ps.register_point_cloud('pts', randn(50, 3));
    pc.add_scalar_quantity('z', randn(50, 1));

    imgFile = fullfile(rootDir, 'test', 'headless_screenshot.png');
    if isfile(imgFile)
        delete(imgFile);
    end

    ps.frame_tick();
    ps.screenshot(imgFile, 'include_ui', false);

    assert(isfile(imgFile), 'Screenshot file was not created');
    info = dir(imgFile);
    assert(info.bytes > 0, 'Screenshot file is empty');

    delete(imgFile);

    ps.shutdown();
    fprintf('test_screenshot_headless passed\n');
end
