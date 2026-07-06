function test_imgui_position()
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL3_glfw');
    pc = ps.register_point_cloud('pts', [0, 0, 0; 1, 0, 0; 0, 1, 0; 0, 0, 1]);
    pc.add_scalar_quantity('vals', [0; 1; 2; 3]);

    ig = polyscope.ImGui;

    imgFile = fullfile(scriptDir, 'imgui_position_screenshot.png');

    for frame = 1:60
        ps.frame_begin();

        % Place the window well away from the default Polyscope left panel
        ig.SetNextWindowPos([300, 100]);
        ig.SetNextWindowSize([400, 200]);
        ig.Begin('Custom MATLAB Window');
        ig.Text('If you see this, ImGui works!');
        if ig.Button('Click me')
            fprintf('Clicked\n');
        end
        ig.End();

        ps.frame_end();

        if ps.window_requests_close()
            break;
        end
    end

    ps.screenshot(imgFile, 'include_ui', true);
    ps.shutdown();

    fprintf('Screenshot saved to: %s\n', imgFile);
    fprintf('test_imgui_position passed\n');
end
