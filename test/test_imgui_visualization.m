function test_imgui_visualization(interactive)
%TEST_IMGUI_VISUALIZATION MATLAB equivalent of the Polyscope ImGui/ImPlot demo.
%   This mirrors the Python example that uses set_user_callback() + show().
%   Because C++ -> MATLAB callback re-entry crashes the MEX adapter, the
%   callback is stored in the Polyscope wrapper and executed from MATLAB
%   inside a frame_begin/frame_end loop.
%
%   test_imgui_visualization() runs for 300 frames and shows the captured
%   image in a MATLAB figure.
%
%   test_imgui_visualization(true) runs interactively until the Polyscope
%   window is closed.

    if nargin < 1
        interactive = true;
    end

    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL3_glfw');
    ps.set_window_size(1400, 900);
    ps.set_build_gui(true);

    % Make sure the GLFW window is actually shown on screen. Polyscope's
    % init() does not show the window; show() / frame_tick() do, but we are
    % driving the loop manually via frame_begin/frame_end.
    ps.show_window();
    ps.focus_window();

    % Register something so the scene is non-empty
    pc = ps.register_point_cloud('pts', rand(10, 3));
    pc.add_scalar_quantity('vals', rand(10, 1));

    ig = polyscope.ImGui;
    ip = polyscope.ImPlot;

    % UI state shared with the nested callback
    is_true1 = false;
    is_true2 = true;
    ui_int = 7;
    ui_float1 = -3.2;
    ui_float2 = 0.8;
    ui_color3 = [1.0, 0.5, 0.5];
    ui_color4 = [0.3, 0.5, 0.5, 0.8];
    ui_angle_rad = 0.2;
    ui_text = 'some input text';
    ui_options = {'option A', 'option B', 'option C'};
    ui_options_selected = ui_options{2};

    frame = 0;
    captureFile = fullfile(scriptDir, 'imgui_visualization_capture.png');

    function callback()
        % This nested function shares variables with the outer function.
        frame = frame + 1;

        % Only set position/size on first use so the user can drag/resize
        % the window afterwards.
        ig.SetNextWindowPos([300, 50], int32(ig.get_constant('ImGuiCond_FirstUseEver')));
        ig.SetNextWindowSize([700, 800], int32(ig.get_constant('ImGuiCond_FirstUseEver')));
        ig.Begin('MATLAB ImGui Visualization Demo');

        ig.PushItemWidth(150);

        ig.TextUnformatted('Some sample text');
        ig.TextUnformatted(sprintf('An important value: %d', 42));
        ig.Separator();

        if ig.Button('A button')
            disp('Hello');
        end
        ig.SameLine();
        if ig.Button('Another button')
            disp('executing function');
        end

        [~, is_true1] = ig.Checkbox('flag1', is_true1);
        ig.SameLine();
        [~, is_true2] = ig.Checkbox('flag2', is_true2);

        [~, ui_int] = ig.InputInt('ui_int', ui_int, 1, 10);

        [~, ui_float1] = ig.InputFloat('ui_float1', ui_float1);
        ig.SameLine();
        [~, ui_float2] = ig.SliderFloat('ui_float2', ui_float2, -5, 5);

        [~, ui_color3] = ig.ColorEdit3('ui_color3', ui_color3);
        ig.SameLine();
        [~, ui_color4] = ig.ColorEdit4('ui_color4', ui_color4);

        [~, ui_text] = ig.InputText('enter text', ui_text);

        ig.PushItemWidth(200);
        comboOpen = ig.BeginCombo('Pick one', ui_options_selected);
        if comboOpen
            for k = 1:numel(ui_options)
                val = ui_options{k};
                [~, selected] = ig.Selectable(val, strcmp(ui_options_selected, val));
                if selected
                    ui_options_selected = val;
                end
            end
            ig.EndCombo();
        end
        ig.PopItemWidth();

        ig.SetNextItemOpen(true, int32(ig.get_constant('ImGuiCond_FirstUseEver')));
        if ig.TreeNode('Collapsible sub-menu')
            ig.TextUnformatted('Detailed information');
            if ig.Button('sub-button')
                disp('hello');
            end
            [~, ui_angle_rad] = ig.SliderAngle('ui_angle', ui_angle_rad, -90, 90);
            ig.TreePop();
        end

        ig.PopItemWidth();

        if ip.BeginPlot('test line plot')
            ip.SetupAxes('x', 'y');
            ip.PlotLine('line plot', rand(10, 1));
            ip.PlotInfLines('horizontal lines', rand(3, 1), ...
                int32(ip.get_constant('ImPlotInfLinesFlags_Horizontal')));
            ip.EndPlot();
        end

        ig.End();

        % Capture the front buffer once so we can verify the UI off-line
        if frame == 30
            ps.capture_display(captureFile);
        end
    end

    ps.set_user_callback(@callback);

    if interactive
        ps.focus_window();
        ps.show();
    else
        ps.show(300);
    end

    % Pop up a MATLAB figure with the captured front buffer so the GUI is
    % visible even if the live window closed too quickly or was off-screen.
    if exist(captureFile, 'file')
        try
            fig = figure('Name', 'ImGui Visualization Capture', ...
                         'NumberTitle', 'off', ...
                         'Visible', 'on');
            img = imread(captureFile);
            image(img);
            axis image off;
            title('Captured front buffer (GUI visible)');
            drawnow;
        catch ME
            warning('Could not display capture:', '%s', ME.message);
        end
    end

    ps.shutdown();
    fprintf('Visualization test finished. Capture saved to:\n  %s\n', captureFile);
end
