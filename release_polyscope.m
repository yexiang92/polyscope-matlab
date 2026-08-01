function packageDir = release_polyscope(options)
%RELEASE_POLYSCOPE Build and optionally deploy the MATLAB runtime package.
%   packageDir = RELEASE_POLYSCOPE() copies the public MATLAB wrappers and
%   the platform MEX binary to dist/matlab/+polyscope.
%
%   RELEASE_POLYSCOPE(SyncTarget=targetDir) also mirrors the generated
%   +polyscope directory to targetDir. targetDir must itself be named
%   +polyscope. The old target is restored if deployment fails.

    arguments
        options.OutputRoot (1, 1) string = ""
        options.SyncTarget (1, 1) string = ""
    end

    rootDir = string(fileparts(mfilename('fullpath')));
    if strlength(options.OutputRoot) == 0
        outputRoot = fullfile(rootDir, 'dist', 'matlab');
    else
        outputRoot = absolutePath_(options.OutputRoot);
    end

    sourceDir = fullfile(rootDir, 'src', 'matlab', '+polyscope');
    packageDir = fullfile(outputRoot, '+polyscope');
    mexFile = fullfile(sourceDir, 'private', "polyscope_mex." + mexext);
    if ~isfile(mexFile)
        error('polyscope:release:MissingMex', ...
            'Build the MEX binary first; expected "%s".', mexFile);
    end

    if isfolder(packageDir)
        rmdir(packageDir, 's');
    end
    if ~isfolder(outputRoot)
        mkdir(outputRoot);
    end
    [ok, message] = copyfile(sourceDir, packageDir);
    if ~ok
        error('polyscope:release:CopyFailed', '%s', message);
    end

    % Source-only documentation is not needed by the runtime package.
    matlabReadme = fullfile(packageDir, 'README.md');
    if isfile(matlabReadme)
        delete(matlabReadme);
    end

    writeManifest_(packageDir, rootDir);

    if strlength(options.SyncTarget) > 0
        syncPackage_(packageDir, absolutePath_(options.SyncTarget));
    end

    fprintf('Polyscope MATLAB runtime package: %s\n', packageDir);
end

function syncPackage_(packageDir, targetDir)
    [~, leaf] = fileparts(targetDir);
    if ~strcmp(leaf, '+polyscope')
        error('polyscope:release:InvalidTarget', ...
            'SyncTarget must be a +polyscope directory, got "%s".', targetDir);
    end

    targetParent = string(fileparts(targetDir));
    if ~isfolder(targetParent)
        error('polyscope:release:MissingTargetParent', ...
            'Sync target parent does not exist: "%s".', targetParent);
    end

    backupDir = string(tempname(targetParent));
    hadTarget = isfolder(targetDir);
    try
        if hadTarget
            makeWritableTree_(targetDir);
            [ok, message] = movefile(targetDir, backupDir);
            if ~ok
                error('polyscope:release:BackupFailed', '%s', message);
            end
        end
        [ok, message] = copyfile(packageDir, targetDir);
        if ~ok
            error('polyscope:release:DeployFailed', '%s', message);
        end
    catch err
        if isfolder(targetDir)
            makeWritableTree_(targetDir);
            rmdir(targetDir, 's');
        end
        if hadTarget && isfolder(backupDir)
            movefile(backupDir, targetDir);
        end
        rethrow(err);
    end

    if hadTarget && isfolder(backupDir)
        makeWritableTree_(backupDir);
        rmdir(backupDir, 's');
    end
    fprintf('Synced Polyscope MATLAB runtime to: %s\n', targetDir);
end

function makeWritableTree_(dirPath)
    if isfolder(dirPath)
        fileattrib(dirPath, '+w', '', 's');
    end
end

function writeManifest_(packageDir, rootDir)
    revision = "unknown";
    sourceDirty = false;
    command = sprintf('git -C "%s" rev-parse --short HEAD', rootDir);
    [status, output] = system(command);
    if status == 0
        revision = strtrim(string(output));
        statusCommand = sprintf('git -C "%s" status --porcelain --untracked-files=normal', rootDir);
        [~, statusOutput] = system(statusCommand);
        sourceDirty = strlength(strtrim(string(statusOutput))) > 0;
    end

    manifest = fullfile(packageDir, 'POLYSCOPE_MATLAB_VERSION.txt');
    fileId = fopen(manifest, 'w');
    if fileId < 0
        error('polyscope:release:ManifestFailed', ...
            'Cannot write release manifest "%s".', manifest);
    end
    cleanup = onCleanup(@() fclose(fileId)); %#ok<NASGU>
    fprintf(fileId, 'source_revision=%s\n', revision);
    fprintf(fileId, 'source_dirty=%d\n', sourceDirty);
    fprintf(fileId, 'matlab_release=%s\n', version('-release'));
    fprintf(fileId, 'mex_extension=%s\n', mexext);
end

function pathOut = absolutePath_(pathIn)
    pathIn = char(pathIn);
    if ispc
        isAbsolute = ~isempty(regexp(pathIn, '^[A-Za-z]:[\\/]', 'once')) || ...
            startsWith(pathIn, '\\');
    else
        isAbsolute = startsWith(pathIn, '/');
    end
    if isAbsolute
        pathOut = string(pathIn);
    else
        pathOut = string(fullfile(pwd, pathIn));
    end
end
