function test_imgui()
%TEST_IMGUI Headless smoke test for the ImGui / ImPlot MATLAB bindings.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');

    % Register something so the scene is non-empty
    pc = ps.register_point_cloud('pts', [0, 0, 0; 1, 0, 0; 0, 1, 0]);
    pc.add_scalar_quantity('vals', [1; 2; 3]);

    ig = polyscope.ImGui;
    ip = polyscope.ImPlot;

    % Verify constants lookup
    assert(isnumeric(ig.get_constant('ImGuiWindowFlags_NoTitleBar')), 'ImGuiWindowFlags_NoTitleBar should be numeric');
    assert(isnumeric(ip.get_constant('ImPlotFlags_None')), 'ImPlotFlags_None should be numeric');

    % Tick a few frames, building UI from MATLAB between frame_begin/frame_end
    frameCount = 0;
    for i = 1:3
        ps.frame_begin();
        frameCount = frameCount + 1;

        % ImGui window
        flags = int32(ig.get_constant('ImGuiWindowFlags_NoTitleBar') + ig.get_constant('ImGuiWindowFlags_NoResize'));
        ig.Begin('Test Window', flags);
        ig.Text('Hello from MATLAB ImGui');
        clicked = ig.Button('Click me');
        if clicked
            fprintf('Button clicked on frame %d\n', frameCount);
        end
        [changed, val] = ig.SliderFloat('slider', 0.5, 0, 1);
        assert(islogical(changed) && isnumeric(val), 'SliderFloat return type mismatch');
        [changed, txt] = ig.InputText('input', 'default');
        assert(ischar(txt) || isstring(txt), 'InputText should return string');
        [changed, col] = ig.ColorEdit3('color', [1, 0, 0]);
        assert(numel(col) == 4, 'ColorEdit3 should return 1x4');
        ig.End();

        % ImPlot
        if ip.BeginPlot('Test Plot')
            ip.SetupAxes('x', 'y');
            ip.PlotLine('line', sin(linspace(0, 2*pi, 50)'));
            ip.EndPlot();
        end

        ps.frame_end();
    end

    ps.shutdown();
    fprintf('test_imgui passed\n');
end
