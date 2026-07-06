function test_imgui_visible()
%TEST_IMGUI_VISIBLE Visual verification of the split-frame ImGui path.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('auto');
    ps.set_window_size(1200, 800);
    ps.set_build_gui(true);

    pc = ps.register_point_cloud('pts', rand(4, 3));
    pc.add_scalar_quantity('vals', rand(4, 1));

    ig = polyscope.ImGui;
    outFile = fullfile(scriptDir, 'imgui_visible_capture.png');

    for i = 1:5
        ps.frame_begin();

        % Draw a large, obvious window away from the default left panel
        ig.SetNextWindowPos([600, 200], int32(ig.get_constant('ImGuiCond_Always')));
        ig.SetNextWindowSize([400, 300], int32(ig.get_constant('ImGuiCond_Always')));
        ig.Begin('MATLAB ImGui Window', int32(0));
        ig.Text('Hello from MATLAB split-frame ImGui');
        ig.Text(['Frame ' num2str(i)]);
        ig.End();

        ps.frame_end();

        % Capture the front buffer after the first few frames
        if i == 3
            ps.capture_display(outFile);
        end
    end

    ps.shutdown();
    fprintf('Capture saved to: %s\n', outFile);
end
