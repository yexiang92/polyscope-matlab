classdef Polyscope < handle
    %POLYSCOPE MATLAB wrapper for the Polyscope viewer.
    %   This class mirrors the Python polyscope API as closely as possible.
    %   Usage:
    %       ps = polyscope.Polyscope;
    %       ps.init('openGL_mock');
    %       pc = ps.register_point_cloud('my cloud', points);
    %       pc.add_scalar_quantity('height', vals);
    %       ps.show();

    properties (Access = private)
        % MATLAB callbacks cannot be invoked from the C++ render loop (it
        % crashes the MEX adapter). We store the user callback here and run
        % it from MATLAB inside a frame_begin/frame_end loop in show().
        userCallback_ = []
    end

    methods
        function init(~, backend)
            if nargin < 2, backend = 'auto'; end
            call_mex('init', backend);
        end

        function check_initialized(~)
            call_mex('check_initialized');
        end

        function tf = is_initialized(~)
            tf = call_mex('is_initialized');
        end

        function show(obj, forFrames)
            % If the user has installed a MATLAB callback, run the render loop
            % from MATLAB so the callback executes safely between
            % frame_begin()/frame_end(). Otherwise, defer to the native C++ show().
            if ~isempty(obj.userCallback_)
                if nargin < 2
                    while ~obj.window_requests_close()
                        obj.frame_begin();
                        obj.userCallback_();
                        obj.frame_end();
                        drawnow limitrate;
                        pause(0.005);
                    end
                else
                    frame = 1;
                    while frame <= forFrames && ~obj.window_requests_close()
                        obj.frame_begin();
                        obj.userCallback_();
                        obj.frame_end();
                        drawnow limitrate;
                        pause(0.005);
                        frame = frame + 1;
                    end
                end
            else
                if nargin < 2
                    call_mex('show');
                else
                    call_mex('show', forFrames);
                end
            end
        end

        function unshow(~)
            call_mex('unshow');
        end

        function show_window(~)
            call_mex('show_window');
        end

        function focus_window(~)
            call_mex('focus_window');
        end

        function tf = window_requests_close(~)
            tf = call_mex('window_requests_close');
        end

        function frame_tick(~)
            call_mex('frame_tick');
        end

        function shutdown(~, allowMidFrame)
            if nargin < 2, allowMidFrame = false; end
            call_mex('shutdown', allowMidFrame);
        end

        function remove_everything(~)
            call_mex('remove_everything');
        end

        function remove_all_structures(~)
            call_mex('remove_all_structures');
        end

        % === Options ======================================================
        function set_program_name(~, name)
            call_mex('set_program_name', name);
        end

        function set_verbosity(~, v)
            call_mex('set_verbosity', v);
        end

        function set_errors_throw_exceptions(~, val)
            call_mex('set_errors_throw_exceptions', val);
        end

        function set_max_fps(~, f)
            call_mex('set_max_fps', f);
        end

        function set_enable_vsync(~, val)
            call_mex('set_enable_vsync', val);
        end

        function set_use_prefs_file(~, val)
            call_mex('set_use_prefs_file', val);
        end

        function set_allow_headless_backends(~, val)
            call_mex('set_allow_headless_backends', val);
        end

        function set_build_gui(~, val)
            call_mex('set_build_gui', val);
        end

        function set_user_callback(obj, cb)
            % Store a MATLAB function handle that will be invoked from MATLAB
            % inside the split-frame render loop (frame_begin/frame_end).
            % This avoids the unsafe C++ -> MATLAB callback re-entry that
            % crashes the MEX adapter.
            if ~isempty(cb) && ~isa(cb, 'function_handle')
                error('Polyscope:setUserCallback', ...
                      'Callback must be a function handle or empty.');
            end
            if isempty(cb)
                obj.userCallback_ = [];
            else
                obj.userCallback_ = cb;
            end
            call_mex('clear_user_callback');
        end

        function clear_user_callback(obj)
            obj.userCallback_ = [];
            call_mex('clear_user_callback');
        end

        function frame_begin(~)
            call_mex('frame_begin');
        end

        function frame_end(~)
            call_mex('frame_end');
        end

        function test_cpp_callback_frame(~)
            call_mex('test_cpp_callback_frame');
        end

        % === View / camera ================================================
        function set_window_size(~, w, h)
            call_mex('set_window_size', w, h);
        end

        function sz = get_window_size(~)
            sz = call_mex('get_window_size');
        end

        function reset_camera_to_home_view(~)
            call_mex('reset_camera_to_home_view');
        end

        function look_at(~, eye, target, flyTo)
            if nargin < 4, flyTo = false; end
            call_mex('look_at', eye, target, flyTo);
        end

        function set_background_color(~, c)
            call_mex('set_background_color', c);
        end

        function c = get_background_color(~)
            c = call_mex('get_background_color');
        end

        % === Screenshots ==================================================
        function screenshot(~, filename, varargin)
            if nargin < 2
                call_mex('screenshot');
            else
                call_mex('screenshot', filename, varargin{:});
            end
        end

        function capture_display(~, filename)
            call_mex('capture_display', filename);
        end

        function set_screenshot_extension(~, ext)
            call_mex('set_screenshot_extension', ext);
        end

        % === Point clouds =================================================
        function pc = register_point_cloud(~, name, points, varargin)
            call_mex('register_point_cloud', name, points, varargin{:});
            pc = polyscope.PointCloud(name);
        end

        function tf = has_point_cloud(~, name)
            tf = call_mex('has_point_cloud', name);
        end

        function remove_point_cloud(~, name)
            call_mex('remove_point_cloud', name);
        end

        % === Surface meshes ===============================================
        function sm = register_surface_mesh(~, name, vertices, faces, varargin)
            call_mex('register_surface_mesh', name, vertices, faces, varargin{:});
            sm = polyscope.SurfaceMesh(name);
        end

        function tf = has_surface_mesh(~, name)
            tf = call_mex('has_surface_mesh', name);
        end

        function remove_surface_mesh(~, name)
            call_mex('remove_surface_mesh', name);
        end

        % === Curve networks ===============================================
        function cn = register_curve_network(~, name, nodes, edges, varargin)
            call_mex('register_curve_network', name, nodes, edges, varargin{:});
            cn = polyscope.CurveNetwork(name);
        end

        function cn = register_curve_network_line(~, name, nodes, varargin)
            call_mex('register_curve_network_line', name, nodes, varargin{:});
            cn = polyscope.CurveNetwork(name);
        end

        function cn = register_curve_network_loop(~, name, nodes, varargin)
            call_mex('register_curve_network_loop', name, nodes, varargin{:});
            cn = polyscope.CurveNetwork(name);
        end

        function cn = register_curve_network_segments(~, name, nodes, varargin)
            call_mex('register_curve_network_segments', name, nodes, varargin{:});
            cn = polyscope.CurveNetwork(name);
        end

        function tf = has_curve_network(~, name)
            tf = call_mex('has_curve_network', name);
        end

        function remove_curve_network(~, name)
            call_mex('remove_curve_network', name);
        end

        % === Volume meshes ================================================
        function vm = register_tet_mesh(~, name, vertices, tets, varargin)
            call_mex('register_tet_mesh', name, vertices, tets, varargin{:});
            vm = polyscope.VolumeMesh(name);
        end

        function vm = register_hex_mesh(~, name, vertices, hexes, varargin)
            call_mex('register_hex_mesh', name, vertices, hexes, varargin{:});
            vm = polyscope.VolumeMesh(name);
        end

        function vm = register_volume_mesh(~, name, vertices, cells, varargin)
            call_mex('register_volume_mesh', name, vertices, cells, varargin{:});
            vm = polyscope.VolumeMesh(name);
        end

        function vm = register_tet_hex_mesh(~, name, vertices, tets, hexes, varargin)
            call_mex('register_tet_hex_mesh', name, vertices, tets, hexes, varargin{:});
            vm = polyscope.VolumeMesh(name);
        end

        function tf = has_volume_mesh(~, name)
            tf = call_mex('has_volume_mesh', name);
        end

        function remove_volume_mesh(~, name)
            call_mex('remove_volume_mesh', name);
        end

        % === Volume grids =================================================
        function vg = register_volume_grid(~, name, node_dims, bound_low, bound_high, varargin)
            call_mex('register_volume_grid', name, node_dims, bound_low, bound_high, varargin{:});
            vg = polyscope.VolumeGrid(name);
        end

        function tf = has_volume_grid(~, name)
            tf = call_mex('has_volume_grid', name);
        end

        function remove_volume_grid(~, name)
            call_mex('remove_volume_grid', name);
        end

        % === Sparse volume grids ==========================================
        function svg = register_sparse_volume_grid(~, name, origin, cell_width, occupied_cells, varargin)
            call_mex('register_sparse_volume_grid', name, origin, cell_width, occupied_cells, varargin{:});
            svg = polyscope.SparseVolumeGrid(name);
        end

        function tf = has_sparse_volume_grid(~, name)
            tf = call_mex('has_sparse_volume_grid', name);
        end

        function remove_sparse_volume_grid(~, name)
            call_mex('remove_sparse_volume_grid', name);
        end

        % === Camera views =================================================
        function cv = register_camera_view(~, name, position, look_dir, up_dir, fov_deg, aspect_ratio, varargin)
            call_mex('register_camera_view', name, position, look_dir, up_dir, fov_deg, aspect_ratio, varargin{:});
            cv = polyscope.CameraView(name);
        end

        function tf = has_camera_view(~, name)
            tf = call_mex('has_camera_view', name);
        end

        function remove_camera_view(~, name)
            call_mex('remove_camera_view', name);
        end

        % === Render engine info ===========================================
        function name = get_render_engine_backend_name(~)
            name = call_mex('get_render_engine_backend_name');
        end

        function tf = is_headless(~)
            tf = call_mex('is_headless');
        end
    end
end
