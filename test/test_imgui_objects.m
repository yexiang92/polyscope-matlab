function test_imgui_objects()
%TEST_IMGUI_OBJECTS Minimal smoke test for ImGui object wrappers.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');

    pc = ps.register_point_cloud('pts', [0, 0, 0; 1, 0, 0; 0, 1, 0]);
    pc.add_scalar_quantity('vals', [1; 2; 3]);

    ig = polyscope.ImGui;

    ps.frame_begin();

    % ImGuiIO
    io = ig.GetIO();
    assert(isa(io, 'polyscope.ImGuiIO'), 'GetIO should return polyscope.ImGuiIO');
    val = io.ConfigFlags;
    assert(isnumeric(val), 'ConfigFlags should be numeric');
    io.ConfigFlags = val;
    assert(isnumeric(io.DeltaTime), 'DeltaTime should be numeric');
    assert(islogical(io.WantCaptureMouse), 'WantCaptureMouse should be logical');
    sz = io.DisplaySize;
    assert(numel(sz) == 2, 'DisplaySize should be 1x2');

    % ImGuiStyle
    style = ig.GetStyle();
    assert(isa(style, 'polyscope.ImGuiStyle'), 'GetStyle should return polyscope.ImGuiStyle');
    assert(isnumeric(style.Alpha), 'Alpha should be numeric');
    assert(numel(style.WindowPadding) == 2, 'WindowPadding should be 1x2');
    style.ScaleAllSizes(1.0);

    % ImDrawList
    dl = ig.GetWindowDrawList();
    assert(isa(dl, 'polyscope.ImDrawList'), 'GetWindowDrawList should return polyscope.ImDrawList');
    dl.AddText([10, 10], 4294967295, 'hello');
    dl.AddLine([0, 0], [100, 100], 4294967295, 2.0);
    dl.AddRect([0, 0], [100, 100], 4294967295, 0, 0, 1.0);
    dl.AddRectFilled([0, 0], [100, 100], 4294967295, 0, 0);
    dl.AddCircle([50, 50], 20, 4294967295, 12, 1.0);
    dl.AddCircleFilled([50, 50], 20, 4294967295, 12);
    dl.AddTriangle([0, 0], [50, 100], [100, 0], 4294967295, 1.0);
    dl.AddTriangleFilled([0, 0], [50, 100], [100, 0], 4294967295);

    dl.PathClear();
    dl.PathLineTo([10, 10]);
    dl.PathLineTo([50, 50]);
    dl.PathStroke(4294967295, 0, 1.0);

    dl.PathClear();
    dl.PathLineTo([10, 10]);
    dl.PathLineTo([50, 50]);
    dl.PathLineTo([10, 50]);
    dl.PathFillConvex(4294967295);

    dl.PushClipRect([0, 0], [100, 100], false);
    cr_min = dl.GetClipRectMin();
    cr_max = dl.GetClipRectMax();
    assert(numel(cr_min) == 2 && numel(cr_max) == 2, 'Clip rect should be 1x2');
    dl.PopClipRect();

    ps.frame_end();

    ps.shutdown();
    fprintf('test_imgui_objects passed\n');
end
