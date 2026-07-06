function test_cpp_callback()
  ps = polyscope.Polyscope();

  % Start Polyscope
  ps.init('auto');
  ps.set_window_size(1200, 800);
  ps.set_build_gui(true);

  % Register a point cloud so the default UI has something to show
  cloud = rand(4, 3);
  q = ps.register_point_cloud('pts', cloud);
  q.add_color_quantity('vals', rand(4, 3));

  % Fire the C++ callback for one frame, leaving it active
  ps.test_cpp_callback_frame();

  % Render a few more frames with the callback active
  for i = 1:30
    ps.frame_tick();
  end

  % Screenshot while the callback is still active
  outDir = fullfile(fileparts(mfilename('fullpath')), 'cpp_callback_screenshot.png');
  ps.screenshot(outDir, 'include_ui', true);
  fprintf('Screenshot saved to: %s\n', outDir);

  % Now safe to clear the callback
  ps.clear_user_callback();

  ps.unshow();
  ps.shutdown();
end
