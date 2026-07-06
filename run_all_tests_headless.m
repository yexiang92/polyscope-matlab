function run_all_tests_headless()
%RUN_ALL_TESTS_HEADLESS Run all headless-safe tests and fail if any throw.
%   These tests use the openGL_mock backend and can run on CI without a
%   display server.

    addpath('src/matlab');
    addpath('test');

    tests = {
        'test_constants'
        'test_basic'
        'test_point_cloud_quantities'
        'test_surface_mesh'
        'test_surface_mesh_quantities'
        'test_remove_and_has'
        'test_camera_and_view'
        'test_quantity_options'
        'test_split_frame_headless'
        'test_screenshot_headless'
        'test_remaining_structures'
        'test_imgui'
        'test_imgui_objects'
        'test_implot_new'
        'test_implot_objects'
    };

    failures = {};
    for i = 1:numel(tests)
        name = tests{i};
        try
            fprintf('==> Running %s ...\n', name);
            feval(name);
        catch ME
            failures{end+1} = sprintf('%s: %s', name, ME.message);
            fprintf('FAILED: %s\n', failures{end});
        end
    end

    if ~isempty(failures)
        error('Some tests failed:\n%s', strjoin(failures, '\n'));
    end

    fprintf('\nAll headless tests passed.\n');
end
