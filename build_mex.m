function build_mex()
%BUILD_MEX Build the polyscope_mex MEX interface using CMake.
%   This function configures and builds the MATLAB MEX target.
%   The resulting MEX binary is copied to src/matlab/+polyscope/private/.

    rootDir = fileparts(mfilename('fullpath'));
    buildDir = fullfile(rootDir, 'build_matlab_mex');
    if ~exist(buildDir, 'dir')
        mkdir(buildDir);
    end

    cmakeCmd = 'cmake';
    if ispc
        % On Windows prefer the newest available generator with 64-bit toolset
        generator = ' -G "Visual Studio 17 2022" -A x64';
    else
        generator = '';
    end

    fprintf('Configuring CMake in %s ...\n', buildDir);
    configCmd = sprintf('"%s" -S "%s" -B "%s"%s -DBUILD_MATLAB_BINDINGS=ON', ...
        cmakeCmd, rootDir, buildDir, generator);
    status = system(configCmd);
    if status ~= 0
        error('CMake configuration failed. Command: %s', configCmd);
    end

    fprintf('Building polyscope_mex ...\n');
    buildCmd = sprintf('"%s" --build "%s" --target polyscope_mex --config Release', ...
        cmakeCmd, buildDir);
    status = system(buildCmd);
    if status ~= 0
        error('Build failed. Command: %s', buildCmd);
    end

    % Locate the produced MEX file
    if ispc
        mexName = 'polyscope_mex.mexw64';
    elseif ismac
        mexName = 'polyscope_mex.mexmaci64';
    else
        mexName = 'polyscope_mex.mexa64';
    end
    sourceMex = fullfile(buildDir, 'Release', mexName);
    if ~isfile(sourceMex)
        sourceMex = fullfile(buildDir, mexName);
    end
    destDir = fullfile(rootDir, 'src', 'matlab', '+polyscope', 'private');
    destMex = fullfile(destDir, mexName);

    if isfile(sourceMex)
        if ~exist(destDir, 'dir')
            mkdir(destDir);
        end
        copyfile(sourceMex, destMex, 'f');
        fprintf('Copied %s -> %s\n', sourceMex, destMex);
    else
        error('MEX file not found at %s', sourceMex);
    end

    fprintf('Build complete. Add %s to your MATLAB path.\n', fullfile(rootDir, 'src', 'matlab'));
end
