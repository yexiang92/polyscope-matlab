function test_implot_new()
%TEST_IMPLOT_NEW Smoke test for newly added ImPlot bindings.
    scriptDir = fileparts(mfilename('fullpath'));
    rootDir = fileparts(scriptDir);
    addpath(fullfile(rootDir, 'src', 'matlab'));

    ps = polyscope.Polyscope;
    ps.init('openGL_mock');
    ip = polyscope.ImPlot;

    ps.frame_begin();

    % Style / input map / icons / draw list / show (outside plot)
    s = ip.GetStyle();
    assert(isa(s, 'polyscope.ImPlotStyle'), 'GetStyle should return polyscope.ImPlotStyle');
    assert(isnumeric(s.LineWeight), 'LineWeight should be numeric');
    m = ip.GetInputMap();
    assert(isa(m, 'polyscope.ImPlotInputMap'), 'GetInputMap should return polyscope.ImPlotInputMap');
    assert(isnumeric(m.Pan), 'Pan should be numeric');
    assert(ip.GetColormapCount() > 0, 'GetColormapCount should be positive');
    assert(isstring(ip.GetColormapName(int32(0))), 'GetColormapName should return string');
    assert(ip.GetColormapIndex('Deep') >= 0, 'GetColormapIndex Deep should exist');
    ip.ItemIcon([1, 0, 0, 1]);
    ip.ColormapIcon(int32(0));
    h = ip.GetPlotDrawList();
    assert(isnumeric(h), 'GetPlotDrawList should return numeric handle');
    ip.StyleColorsDark();
    ip.StyleColorsAuto();
    ip.MapInputDefault();
    ip.MapInputReverse();

    % Begin a plot and exercise setup/plot functions
    if ip.BeginPlot('New Bindings Plot')
        ip.SetupAxes('x', 'y');
        ip.SetupAxisTicksValues(int32(0), [0, 0.5, 1], {'a', 'b', 'c'});
        ip.SetupAxisTicksRange(int32(1), 0, 1, 3);
        ip.SetupAxisLimitsConstraints(int32(0), -10, 10);
        ip.SetupAxisZoomConstraints(int32(0), 0.001, 100);

        ip.PlotLine('line', sin(linspace(0, 2*pi, 50)'));
        ip.PlotPieChart({'a', 'b', 'c'}, [1, 2, 3], 0, 0, 1, '%.1f', 90);
        ip.PlotText('hello', 0, 0);
        ip.PlotDummy('dummy');

        % Colormap inside plot
        ip.PushColormap(int32(0));
        ip.PopColormap();
        ip.PushColormap('Jet');
        ip.PopColormap();
        c = ip.NextColormapColor();
        assert(numel(c) == 4, 'NextColormapColor should return 1x4');
        assert(ip.GetColormapSize() > 0, 'GetColormapSize should be positive');
        cc = ip.GetColormapColor(int32(0));
        assert(numel(cc) == 4, 'GetColormapColor should return 1x4');
        sc = ip.SampleColormap(0.5);
        assert(numel(sc) == 4, 'SampleColormap should return 1x4');

        % Query utils
        hovered = ip.IsPlotHovered();
        assert(islogical(hovered), 'IsPlotHovered should return logical');
        limits = ip.GetPlotLimits();
        assert(numel(limits) == 4, 'GetPlotLimits should return 1x4');
        sel = ip.GetPlotSelection();
        assert(numel(sel) == 4, 'GetPlotSelection should return 1x4');
        ip.HideNextItem(false);
        ip.CancelPlotSelection();
        ip.IsPlotSelected();

        % Drag-drop (begin calls return bool; end calls are safe no-ops if not active)
        ip.BeginDragDropTargetPlot();
        ip.BeginDragDropTargetAxis(int32(0));
        ip.BeginDragDropTargetLegend();
        ip.EndDragDropTarget();
        ip.BeginDragDropSourcePlot(int32(0));
        ip.BeginDragDropSourceAxis(int32(0), int32(0));
        ip.BeginDragDropSourceItem('line', int32(0));
        ip.EndDragDropSource();

        % Legend
        ip.IsLegendEntryHovered('line');

        ip.EndPlot();
    end

    ps.frame_end();
    ps.shutdown();
    fprintf('test_implot_new passed\n');
end
