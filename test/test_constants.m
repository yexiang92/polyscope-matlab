function test_constants()
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ig = polyscope.ImGui;
    assert(ig.get_constant('ImGuiWindowFlags_None') == 0, 'ImGuiWindowFlags_None should be 0');
    assert(isnumeric(ig.get_constant('ImGuiWindowFlags_NoTitleBar')), 'flag should be numeric');

    ip = polyscope.ImPlot;
    assert(ip.get_constant('ImPlotFlags_None') == 0, 'ImPlotFlags_None should be 0');
    assert(isnumeric(ip.get_constant('ImPlotAxisFlags_None')), 'flag should be numeric');

    fprintf('test_constants passed\n');
end
