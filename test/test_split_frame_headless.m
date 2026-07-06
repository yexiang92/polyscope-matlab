function test_split_frame_headless()
%TEST_SPLIT_FRAME_HEADLESS Exercise frame_begin/frame_end with the mock backend.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    ps.remove_everything();

    pc = ps.register_point_cloud('pts', randn(20, 3));
    pc.add_scalar_quantity('s', randn(20, 1));

    ig = polyscope.ImGui;

    for frame = 1:5
        ps.frame_begin();
        if ig.Begin('Headless UI')
            ig.TextUnformatted('hello');
            ig.End();
        end
        ps.frame_end();
    end

    ps.shutdown();
    fprintf('test_split_frame_headless passed\n');
end
