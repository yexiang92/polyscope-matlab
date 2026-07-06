function test_implot_objects()
%TEST_IMPLOT_OBJECTS Minimal smoke test for ImPlot object wrappers.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    ip = polyscope.ImPlot;

    ps.frame_begin();

    % ImPlotStyle (outside plot context)
    s = ip.GetStyle();
    assert(isa(s, 'polyscope.ImPlotStyle'), 'GetStyle should return polyscope.ImPlotStyle');
    v = s.LineWeight;
    assert(isnumeric(v), 'LineWeight should be numeric');
    s.LineWeight = v;
    assert(numel(s.MajorTickLen) == 2, 'MajorTickLen should be 1x2');
    assert(islogical(s.UseLocalTime), 'UseLocalTime should be logical');

    % ImPlotInputMap (outside plot context)
    m = ip.GetInputMap();
    assert(isa(m, 'polyscope.ImPlotInputMap'), 'GetInputMap should return polyscope.ImPlotInputMap');
    p = m.Pan;
    assert(isnumeric(p), 'Pan should be numeric');
    m.Pan = p;

    % Also exercise properties inside an active plot frame
    if ip.BeginPlot('Object Wrappers Plot')
        ip.SetupAxes('x', 'y');

        s2 = ip.GetStyle();
        v2 = s2.MarkerSize;
        s2.MarkerSize = v2;
        assert(numel(s2.PlotPadding) == 2, 'PlotPadding should be 1x2');
        assert(islogical(s2.UseISO8601), 'UseISO8601 should be logical');

        m2 = ip.GetInputMap();
        p2 = m2.Select;
        m2.Select = p2;

        ip.PlotLine('line', sin(linspace(0, 2*pi, 50)'));
        ip.EndPlot();
    end

    ps.frame_end();
    ps.shutdown();
    fprintf('test_implot_objects passed\n');
end
