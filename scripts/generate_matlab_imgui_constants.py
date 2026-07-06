#!/usr/bin/env python3
"""Generate MEX constant-lookup code and MATLAB wrappers for ImGui/ImPlot enums."""
import re
from pathlib import Path


def parse_enums(content: str):
    pattern = r'^enum\s+(Im\w+?)(_?)\s*(?::\s*\w+)?\s*[{\n]'
    enums = []
    for match in re.finditer(pattern, content, re.MULTILINE):
        enum_name = match.group(1) + match.group(2)
        brace_pos = content.find('{', match.start())
        if brace_pos == -1:
            continue
        depth = 1
        pos = brace_pos + 1
        while pos < len(content) and depth > 0:
            if content[pos] == '{':
                depth += 1
            elif content[pos] == '}':
                depth -= 1
            pos += 1
        block = content[brace_pos + 1:pos - 1]
        values = []
        for line in block.split('\n'):
            line = line.strip()
            if not line or line.startswith('//'):
                continue
            if line.startswith('#'):
                continue
            if '//' in line:
                line = line[:line.index('//')]
            for part in line.split(','):
                part = part.strip()
                if not part:
                    continue
                m = re.match(r'(Im\w+)', part)
                if m:
                    values.append(m.group(1))
        if values:
            enums.append({'name': enum_name, 'values': values})
    return enums


def generate_cpp_lookup(enums, lib):
    header = f'// AUTO-GENERATED {lib} constant lookup\n'
    header += '#include "command_router.h"\n'
    header += '#include "matlab_data_utils.h"\n'
    if lib == 'imgui':
        header += '#include "imgui.h"\n\n'
    else:
        header += '#include "implot.h"\n\n'
    header += 'namespace ps_mex {\n\n'
    header += f'void bind_{lib}_constant_lookup(CommandRegistry& reg) {{\n'
    header += f'  reg.registerCommand("{lib}_get_constant", [](matlab::mex::ArgumentList& outputs,\n'
    header += '                                                   matlab::mex::ArgumentList& inputs,\n'
    header += '                                                   matlab::engine::MATLABEngine* matlabPtr) {\n'
    header += '    if (inputs.size() < 2) throwError(matlabPtr, "Expected {lib}_get_constant(name)");\n'
    header += '    std::string name = getString(inputs[1]);\n'
    header += '    matlab::data::ArrayFactory factory;\n'
    for enum in enums:
        for value in enum['values']:
            header += f'    if (name == "{value}") {{ outputs[0] = createScalarDouble(factory, static_cast<double>({value})); return; }}\n'
    header += '    throwError(matlabPtr, "Unknown {lib} constant: " + name);\n'
    header += '  });\n}\n\n} // namespace ps_mex\n'
    return header


def generate_h_lookup(lib):
    return f'''#pragma once

#include "command_router.h"

namespace ps_mex {{
void bind_{lib}_constant_lookup(CommandRegistry& reg);
}} // namespace ps_mex
'''


def generate_matlab_wrapper(lib, enums):
    names = [v for e in enums for v in e['values']]
    lines = [f"function C = {lib}_constants()"]
    lines.append(f"%AUTO-GENERATED {lib.upper()} CONSTANTS - DO NOT EDIT")
    lines.append("    names = {...")
    line = "        "
    for i, name in enumerate(names):
        line += f"'{name}'"
        if i < len(names) - 1:
            line += ", "
        if (i + 1) % 4 == 0:
            lines.append(line + " ...")
            line = "        "
    if line.strip():
        lines.append(line)
    lines.append("    };")
    lines.append("    C = struct();")
    lines.append("    for i = 1:numel(names)")
    lines.append(f"        C.(names{{i}}) = int32(call_mex('{lib}_get_constant', names{{i}}));")
    lines.append("    end")
    lines.append("end")
    return '\n'.join(lines) + '\n'


def main():
    root = Path(__file__).parent.parent
    imgui_h = root / 'deps/polyscope/deps/imgui/imgui/imgui.h'
    implot_h = root / 'deps/polyscope/deps/imgui/implot/implot.h'
    cpp_out = root / 'src/matlab/cpp'
    matlab_out = root / 'src/matlab/+polyscope'

    for header, lib in [(imgui_h, 'imgui'), (implot_h, 'implot')]:
        content = header.read_text()
        enums = parse_enums(content)
        (cpp_out / f'bind_{lib}_constant_lookup.h').write_text(generate_h_lookup(lib))
        (cpp_out / f'bind_{lib}_constant_lookup.cpp').write_text(generate_cpp_lookup(enums, lib))
        print(f"Wrote {cpp_out / f'bind_{lib}_constant_lookup.cpp'} with {sum(len(e['values']) for e in enums)} constants")
        (matlab_out / f'{lib}_constants.m').write_text(generate_matlab_wrapper(lib, enums))
        print(f"Wrote {matlab_out / f'{lib}_constants.m'}")


if __name__ == '__main__':
    main()
