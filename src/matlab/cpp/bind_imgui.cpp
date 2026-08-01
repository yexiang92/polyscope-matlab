#include "bind_imgui.h"
#include <cstdint>
#include <cfloat>
#include <functional>
#include "bind_imgui_constant_lookup.h"
#include "matlab_data_utils.h"

#include "polyscope/polyscope.h"
#include "imgui.h"

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

const char* optionalString(const Array& arr) {
  if (arr.getType() == matlab::data::ArrayType::CHAR) {
    std::string s = getString(arr);
    static thread_local std::string storage;
    storage = s;
    return storage.c_str();
  }
  return nullptr;
}

ImVec2 toImVec2(const Eigen::Vector2f& v) { return ImVec2(v[0], v[1]); }
ImVec4 toImVec4(const Eigen::Vector4f& v) { return ImVec4(v[0], v[1], v[2], v[3]); }

ImGuiIO* imguiIOFromHandle(const Array& arr) {
  return reinterpret_cast<ImGuiIO*>(static_cast<uintptr_t>(getScalarDouble(arr)));
}
ImGuiStyle* imguiStyleFromHandle(const Array& arr) {
  return reinterpret_cast<ImGuiStyle*>(static_cast<uintptr_t>(getScalarDouble(arr)));
}
ImDrawList* imguiDrawListFromHandle(const Array& arr) {
  return reinterpret_cast<ImDrawList*>(static_cast<uintptr_t>(getScalarDouble(arr)));
}

// Persistent storage for string fields set from MATLAB (io.IniFilename etc.)
const char* storeIOString(const std::string& s, int slot) {
  static thread_local std::string stores[4];
  stores[slot] = s;
  return stores[slot].c_str();
}

} // namespace

void bind_imgui_commands(CommandRegistry& reg) {
  // === Windows =============================================================
  reg.registerCommand("imgui_begin", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_begin(name, ...)");
    if (ImGui::GetCurrentContext() == nullptr) {
      throwError(matlabPtr, "No active ImGui context. Call frame_begin() before using ImGui widgets.");
    }
    std::string name = getString(getInput(inputs, 1));
    ImGuiWindowFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    bool open = true;
    bool* p_open = nullptr;
    if (inputCount(inputs) > 3) {
      open = getScalarBool(getInput(inputs, 3));
      p_open = &open;
    }
    bool visible = ImGui::Begin(name.c_str(), p_open, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, visible);
    if (p_open) getOutput(outputs, 1) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_end", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    ImGui::End();
  });

  reg.registerCommand("imgui_begin_child", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_begin_child(name, ...)");
    std::string name = getString(getInput(inputs, 1));
    ImVec2 size(0, 0);
    if (inputCount(inputs) > 2) size = toImVec2(getVec2(getInput(inputs, 2)));
    ImGuiChildFlags child_flags = 0;
    if (inputCount(inputs) > 3) child_flags = getScalarInt(getInput(inputs, 3));
    ImGuiWindowFlags window_flags = 0;
    if (inputCount(inputs) > 4) window_flags = getScalarInt(getInput(inputs, 4));
    bool visible = ImGui::BeginChild(name.c_str(), size, child_flags, window_flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, visible);
  });

  reg.registerCommand("imgui_end_child", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    ImGui::EndChild();
  });

  // === Window manipulation =================================================
  reg.registerCommand("imgui_set_next_window_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_set_next_window_pos(pos, cond)");
    ImVec2 pos = toImVec2(getVec2(getInput(inputs, 1)));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 2) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 2)));
    ImVec2 pivot(0, 0);
    if (inputCount(inputs) > 3) pivot = toImVec2(getVec2(getInput(inputs, 3)));
    ImGui::SetNextWindowPos(pos, cond, pivot);
  });

  reg.registerCommand("imgui_set_next_window_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_set_next_window_size(size, cond)");
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 1)));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 2) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 2)));
    ImGui::SetNextWindowSize(size, cond);
  });

  // === Text ================================================================
  reg.registerCommand("imgui_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_text(text)");
    ImGui::Text("%s", getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_text_unformatted", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_text_unformatted(text)");
    std::string s = getString(getInput(inputs, 1));
    ImGui::TextUnformatted(s.c_str(), s.c_str() + s.size());
  });

  reg.registerCommand("imgui_text_colored", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_text_colored(color, text)");
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 1)));
    ImGui::TextColored(col, "%s", getString(getInput(inputs, 2)).c_str());
  });

  reg.registerCommand("imgui_text_disabled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_text_disabled(text)");
    ImGui::TextDisabled("%s", getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_text_wrapped", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_text_wrapped(text)");
    ImGui::TextWrapped("%s", getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_label_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_label_text(label, text)");
    ImGui::LabelText(getString(getInput(inputs, 1)).c_str(), "%s", getString(getInput(inputs, 2)).c_str());
  });

  reg.registerCommand("imgui_bullet_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_bullet_text(text)");
    ImGui::BulletText("%s", getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_separator_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_separator_text(label)");
    ImGui::SeparatorText(getString(getInput(inputs, 1)).c_str());
  });

  // === Layout ==============================================================
  reg.registerCommand("imgui_separator", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    ImGui::Separator();
  });

  reg.registerCommand("imgui_same_line", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    float offset = 0.0f;
    float spacing = -1.0f;
    if (inputCount(inputs) > 1) offset = getScalarFloat(getInput(inputs, 1));
    if (inputCount(inputs) > 2) spacing = getScalarFloat(getInput(inputs, 2));
    ImGui::SameLine(offset, spacing);
  });

  reg.registerCommand("imgui_new_line", [](ArgumentList& outputs, ArgumentList& inputs,
                                           MATLABEngine* matlabPtr) {
    ImGui::NewLine();
  });

  reg.registerCommand("imgui_spacing", [](ArgumentList& outputs, ArgumentList& inputs,
                                          MATLABEngine* matlabPtr) {
    ImGui::Spacing();
  });

  reg.registerCommand("imgui_dummy", [](ArgumentList& outputs, ArgumentList& inputs,
                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_dummy(size)");
    ImGui::Dummy(toImVec2(getVec2(getInput(inputs, 1))));
  });

  reg.registerCommand("imgui_push_item_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_push_item_width(width)");
    ImGui::PushItemWidth(getScalarFloat(getInput(inputs, 1)));
  });

  reg.registerCommand("imgui_pop_item_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    ImGui::PopItemWidth();
  });

  reg.registerCommand("imgui_set_next_item_open", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_next_item_open(is_open, ...)");
    bool is_open = getScalarBool(getInput(inputs, 1));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 2) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 2)));
    ImGui::SetNextItemOpen(is_open, cond);
  });

  reg.registerCommand("imgui_tree_node", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_tree_node(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImGuiTreeNodeFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    bool open = ImGui::TreeNode(label.c_str(), "%s", label.c_str());
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_tree_pop", [](ArgumentList& outputs, ArgumentList& inputs,
                                           MATLABEngine* matlabPtr) {
    ImGui::TreePop();
  });

  reg.registerCommand("imgui_begin_combo", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_begin_combo(label, preview_value, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::string preview = getString(getInput(inputs, 2));
    ImGuiComboFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool open = ImGui::BeginCombo(label.c_str(), preview.c_str(), flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_end_combo", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    ImGui::EndCombo();
  });

  reg.registerCommand("imgui_selectable", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_selectable(label, selected, ...)");
    std::string label = getString(getInput(inputs, 1));
    bool selected = getScalarBool(getInput(inputs, 2));
    ImGuiSelectableFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool clicked = ImGui::Selectable(label.c_str(), &selected, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
    getOutput(outputs, 1) = createScalarBool(factory, selected);
  });

  // === Main widgets ========================================================
  reg.registerCommand("imgui_button", [](ArgumentList& outputs, ArgumentList& inputs,
                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_button(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImVec2 size(0, 0);
    if (inputCount(inputs) > 2) size = toImVec2(getVec2(getInput(inputs, 2)));
    bool clicked = ImGui::Button(label.c_str(), size);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
  });

  reg.registerCommand("imgui_small_button", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_small_button(label)");
    bool clicked = ImGui::SmallButton(getString(getInput(inputs, 1)).c_str());
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
  });

  reg.registerCommand("imgui_checkbox", [](ArgumentList& outputs, ArgumentList& inputs,
                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_checkbox(label, value)");
    std::string label = getString(getInput(inputs, 1));
    bool v = getScalarBool(getInput(inputs, 2));
    bool changed = ImGui::Checkbox(label.c_str(), &v);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarBool(factory, v);
  });

  reg.registerCommand("imgui_radio_button", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_radio_button(label, v, v_button)");
    std::string label = getString(getInput(inputs, 1));
    int v = getScalarInt(getInput(inputs, 2));
    int v_button = getScalarInt(getInput(inputs, 3));
    bool clicked = ImGui::RadioButton(label.c_str(), &v, v_button);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  // === Sliders =============================================================
  reg.registerCommand("imgui_slider_float", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_slider_float(label, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    float v = getScalarFloat(getInput(inputs, 2));
    float v_min = getScalarFloat(getInput(inputs, 3));
    float v_max = getScalarFloat(getInput(inputs, 4));
    const char* format = "%.3f";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::SliderFloat(label.c_str(), &v, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  reg.registerCommand("imgui_slider_int", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_slider_int(label, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    int v = getScalarInt(getInput(inputs, 2));
    int v_min = getScalarInt(getInput(inputs, 3));
    int v_max = getScalarInt(getInput(inputs, 4));
    const char* format = "%d";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::SliderInt(label.c_str(), &v, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  reg.registerCommand("imgui_slider_angle", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_slider_angle(label, v_rad, ...)");
    std::string label = getString(getInput(inputs, 1));
    float v = getScalarFloat(getInput(inputs, 2));
    float v_degrees_min = -360.0f;
    float v_degrees_max = 360.0f;
    if (inputCount(inputs) > 3) v_degrees_min = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) v_degrees_max = getScalarFloat(getInput(inputs, 4));
    const char* format = "%.0f deg";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::SliderAngle(label.c_str(), &v, v_degrees_min, v_degrees_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  // === Input ===============================================================
  reg.registerCommand("imgui_input_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_text(label, buf, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::string buf = getString(getInput(inputs, 2));
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    size_t max_len = 1024;
    if (inputCount(inputs) > 4) max_len = static_cast<size_t>(getScalarInt(getInput(inputs, 4)));
    if (buf.size() < max_len) buf.resize(max_len, '\0');
    bool changed = ImGui::InputText(label.c_str(), buf.data(), max_len, flags);
    // trim trailing nulls
    size_t end = strlen(buf.c_str());
    buf.resize(end);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = factory.createScalar(buf);
  });

  reg.registerCommand("imgui_input_float", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_float(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    float v = getScalarFloat(getInput(inputs, 2));
    float step = 0.0f;
    float step_fast = 0.0f;
    if (inputCount(inputs) > 3) step = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) step_fast = getScalarFloat(getInput(inputs, 4));
    const char* format = "%.3f";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::InputFloat(label.c_str(), &v, step, step_fast, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  reg.registerCommand("imgui_input_int", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_int(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    int v = getScalarInt(getInput(inputs, 2));
    int step = 1;
    int step_fast = 100;
    if (inputCount(inputs) > 3) step = getScalarInt(getInput(inputs, 3));
    if (inputCount(inputs) > 4) step_fast = getScalarInt(getInput(inputs, 4));
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    bool changed = ImGui::InputInt(label.c_str(), &v, step, step_fast, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  reg.registerCommand("imgui_input_double", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_double(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    double v = getScalarDouble(getInput(inputs, 2));
    double step = 0.0;
    double step_fast = 0.0;
    if (inputCount(inputs) > 3) step = getScalarDouble(getInput(inputs, 3));
    if (inputCount(inputs) > 4) step_fast = getScalarDouble(getInput(inputs, 4));
    const char* format = "%.6f";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::InputDouble(label.c_str(), &v, step, step_fast, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, v);
  });

  // === Color ===============================================================
  reg.registerCommand("imgui_color_edit3", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_color_edit3(label, col, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto m = getMatrixFloat(getInput(inputs, 2));
    if (m.cols() != 3 && m.cols() != 4) {
      throwError(matlabPtr, "imgui_color_edit3 expects a 1x3 or 1x4 color");
    }
    float col[3] = {m(0, 0), m(0, 1), m(0, 2)};
    ImGuiColorEditFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool changed = ImGui::ColorEdit3(label.c_str(), col, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVec4(factory, col[0], col[1], col[2], 1.0f);
  });

  reg.registerCommand("imgui_color_edit4", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_color_edit4(label, col, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto m = getMatrixFloat(getInput(inputs, 2), 4);
    float col[4] = {m(0, 0), m(0, 1), m(0, 2), m(0, 3)};
    ImGuiColorEditFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool changed = ImGui::ColorEdit4(label.c_str(), col, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVec4(factory, col[0], col[1], col[2], col[3]);
  });

  reg.registerCommand("imgui_color_button", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_color_button(label, col, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto m = getMatrixFloat(getInput(inputs, 2), 4);
    ImVec4 col(m(0, 0), m(0, 1), m(0, 2), m(0, 3));
    ImGuiColorEditFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    ImVec2 size(0, 0);
    if (inputCount(inputs) > 4) size = toImVec2(getVec2(getInput(inputs, 4)));
    bool clicked = ImGui::ColorButton(label.c_str(), col, flags, size);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
  });

  // === Drag widgets ========================================================
  reg.registerCommand("imgui_drag_float", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drag_float(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    float v = getScalarFloat(getInput(inputs, 2));
    float v_speed = 1.0f;
    float v_min = 0.0f;
    float v_max = 0.0f;
    if (inputCount(inputs) > 3) v_speed = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) v_min = getScalarFloat(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_max = getScalarFloat(getInput(inputs, 5));
    const char* format = "%.3f";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::DragFloat(label.c_str(), &v, v_speed, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  reg.registerCommand("imgui_drag_int", [](ArgumentList& outputs, ArgumentList& inputs,
                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drag_int(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    int v = getScalarInt(getInput(inputs, 2));
    float v_speed = 1.0f;
    int v_min = 0;
    int v_max = 0;
    if (inputCount(inputs) > 3) v_speed = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) v_min = getScalarInt(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_max = getScalarInt(getInput(inputs, 5));
    const char* format = "%d";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::DragInt(label.c_str(), &v, v_speed, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  // === Color pickers =======================================================
  reg.registerCommand("imgui_color_picker3", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_color_picker3(label, col, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto v3 = getVec3(getInput(inputs, 2));
    float col[3] = {v3[0], v3[1], v3[2]};
    ImGuiColorEditFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool changed = ImGui::ColorPicker3(label.c_str(), col, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVec4(factory, col[0], col[1], col[2], 1.0f);
  });

  reg.registerCommand("imgui_color_picker4", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_color_picker4(label, col, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto v4 = getVec4(getInput(inputs, 2));
    float col[4] = {v4[0], v4[1], v4[2], v4[3]};
    ImGuiColorEditFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool changed = ImGui::ColorPicker4(label.c_str(), col, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVec4(factory, col[0], col[1], col[2], col[3]);
  });

  // === Combo / List boxes ==================================================
  reg.registerCommand("imgui_combo", [](ArgumentList& outputs, ArgumentList& inputs,
                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_combo(label, current_item, items)");
    std::string label = getString(getInput(inputs, 1));
    int current_item = getScalarInt(getInput(inputs, 2));
    auto items = getStringVector(getInput(inputs, 3));
    std::vector<const char*> item_ptrs;
    item_ptrs.reserve(items.size());
    for (auto& s : items) item_ptrs.push_back(s.c_str());
    bool changed = ImGui::Combo(label.c_str(), &current_item, item_ptrs.data(), static_cast<int>(item_ptrs.size()));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(current_item));
  });

  reg.registerCommand("imgui_begin_list_box", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_begin_list_box(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImVec2 size(0, 0);
    if (inputCount(inputs) > 2) size = toImVec2(getVec2(getInput(inputs, 2)));
    bool open = ImGui::BeginListBox(label.c_str(), size);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_end_list_box", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    ImGui::EndListBox();
  });

  reg.registerCommand("imgui_list_box", [](ArgumentList& outputs, ArgumentList& inputs,
                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_list_box(label, current_item, items, ...)");
    std::string label = getString(getInput(inputs, 1));
    int current_item = getScalarInt(getInput(inputs, 2));
    auto items = getStringVector(getInput(inputs, 3));
    std::vector<const char*> item_ptrs;
    item_ptrs.reserve(items.size());
    for (auto& s : items) item_ptrs.push_back(s.c_str());
    int height_in_items = -1;
    if (inputCount(inputs) > 4) height_in_items = getScalarInt(getInput(inputs, 4));
    bool changed = ImGui::ListBox(label.c_str(), &current_item, item_ptrs.data(),
                                  static_cast<int>(item_ptrs.size()), height_in_items);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(current_item));
  });

  // === Headers / Buttons ===================================================
  reg.registerCommand("imgui_collapsing_header", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_collapsing_header(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImGuiTreeNodeFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    bool open = ImGui::CollapsingHeader(label.c_str(), flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_invisible_button", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_invisible_button(str_id, size, ...)");
    std::string str_id = getString(getInput(inputs, 1));
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 2)));
    ImGuiButtonFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool clicked = ImGui::InvisibleButton(str_id.c_str(), size, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
  });

  reg.registerCommand("imgui_arrow_button", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_arrow_button(str_id, dir)");
    std::string str_id = getString(getInput(inputs, 1));
    ImGuiDir dir = static_cast<ImGuiDir>(getScalarInt(getInput(inputs, 2)));
    bool clicked = ImGui::ArrowButton(str_id.c_str(), dir);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
  });

  // === Menus ===============================================================
  reg.registerCommand("imgui_begin_menu_bar", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    bool open = ImGui::BeginMenuBar();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_end_menu_bar", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    ImGui::EndMenuBar();
  });

  reg.registerCommand("imgui_begin_main_menu_bar", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    bool open = ImGui::BeginMainMenuBar();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_end_main_menu_bar", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    ImGui::EndMainMenuBar();
  });

  reg.registerCommand("imgui_begin_menu", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_begin_menu(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    bool enabled = true;
    if (inputCount(inputs) > 2) enabled = getScalarBool(getInput(inputs, 2));
    bool open = ImGui::BeginMenu(label.c_str(), enabled);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_end_menu", [](ArgumentList& outputs, ArgumentList& inputs,
                                           MATLABEngine* matlabPtr) {
    ImGui::EndMenu();
  });

  reg.registerCommand("imgui_menu_item", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_menu_item(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    const char* shortcut = nullptr;
    if (inputCount(inputs) > 2) shortcut = optionalString(getInput(inputs, 2));
    bool selected = false;
    bool* p_selected = nullptr;
    if (inputCount(inputs) > 3) {
      selected = getScalarBool(getInput(inputs, 3));
      p_selected = &selected;
    }
    bool enabled = true;
    if (inputCount(inputs) > 4) enabled = getScalarBool(getInput(inputs, 4));
    bool clicked = ImGui::MenuItem(label.c_str(), shortcut, p_selected, enabled);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
    if (p_selected) getOutput(outputs, 1) = createScalarBool(factory, selected);
  });

  // === Popups ==============================================================
  reg.registerCommand("imgui_begin_popup", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_begin_popup(str_id, ...)");
    std::string str_id = getString(getInput(inputs, 1));
    ImGuiWindowFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    bool open = ImGui::BeginPopup(str_id.c_str(), flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_end_popup", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    ImGui::EndPopup();
  });

  reg.registerCommand("imgui_open_popup", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_open_popup(str_id, ...)");
    std::string str_id = getString(getInput(inputs, 1));
    ImGuiPopupFlags popup_flags = 0;
    if (inputCount(inputs) > 2) popup_flags = getScalarInt(getInput(inputs, 2));
    ImGui::OpenPopup(str_id.c_str(), popup_flags);
  });

  reg.registerCommand("imgui_close_current_popup", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    ImGui::CloseCurrentPopup();
  });

  reg.registerCommand("imgui_is_popup_open", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_is_popup_open(str_id, ...)");
    std::string str_id = getString(getInput(inputs, 1));
    ImGuiPopupFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    bool open = ImGui::IsPopupOpen(str_id.c_str(), flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  // === Item / window queries ===============================================
  reg.registerCommand("imgui_is_item_hovered", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    ImGuiHoveredFlags flags = 0;
    if (inputCount(inputs) > 1) flags = getScalarInt(getInput(inputs, 1));
    bool v = ImGui::IsItemHovered(flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  reg.registerCommand("imgui_is_item_active", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    bool v = ImGui::IsItemActive();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  reg.registerCommand("imgui_is_item_clicked", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    ImGuiMouseButton mouse_button = 0;
    if (inputCount(inputs) > 1) mouse_button = static_cast<ImGuiMouseButton>(getScalarInt(getInput(inputs, 1)));
    bool v = ImGui::IsItemClicked(mouse_button);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  reg.registerCommand("imgui_is_window_hovered", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    ImGuiHoveredFlags flags = 0;
    if (inputCount(inputs) > 1) flags = getScalarInt(getInput(inputs, 1));
    bool v = ImGui::IsWindowHovered(flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  reg.registerCommand("imgui_get_window_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetWindowPos();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, v.x, v.y);
  });

  reg.registerCommand("imgui_get_window_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetWindowSize();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, v.x, v.y);
  });

  reg.registerCommand("imgui_get_mouse_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetMousePos();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, v.x, v.y);
  });

  reg.registerCommand("imgui_get_content_region_avail", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetContentRegionAvail();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, v.x, v.y);
  });

  reg.registerCommand("imgui_get_cursor_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetCursorPos();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, v.x, v.y);
  });

  // === Style / layout ======================================================
  reg.registerCommand("imgui_push_style_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_push_style_color(idx, col)");
    ImGuiCol idx = static_cast<ImGuiCol>(getScalarInt(getInput(inputs, 1)));
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 2)));
    ImGui::PushStyleColor(idx, col);
  });

  reg.registerCommand("imgui_pop_style_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    int count = 1;
    if (inputCount(inputs) > 1) count = getScalarInt(getInput(inputs, 1));
    ImGui::PopStyleColor(count);
  });

  reg.registerCommand("imgui_push_style_var", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_push_style_var(idx, val)");
    ImGuiStyleVar idx = static_cast<ImGuiStyleVar>(getScalarInt(getInput(inputs, 1)));
    const Array& valArr = getInput(inputs, 2);
    auto dims = valArr.getDimensions();
    if (dims.size() == 2 && dims[0] == 1 && dims[1] == 2) {
      ImVec2 v = toImVec2(getVec2(valArr));
      ImGui::PushStyleVar(idx, v);
    } else {
      ImGui::PushStyleVar(idx, getScalarFloat(valArr));
    }
  });

  reg.registerCommand("imgui_pop_style_var", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    int count = 1;
    if (inputCount(inputs) > 1) count = getScalarInt(getInput(inputs, 1));
    ImGui::PopStyleVar(count);
  });

  reg.registerCommand("imgui_indent", [](ArgumentList& outputs, ArgumentList& inputs,
                                         MATLABEngine* matlabPtr) {
    float indent_w = 0.0f;
    if (inputCount(inputs) > 1) indent_w = getScalarFloat(getInput(inputs, 1));
    ImGui::Indent(indent_w);
  });

  reg.registerCommand("imgui_unindent", [](ArgumentList& outputs, ArgumentList& inputs,
                                           MATLABEngine* matlabPtr) {
    float indent_w = 0.0f;
    if (inputCount(inputs) > 1) indent_w = getScalarFloat(getInput(inputs, 1));
    ImGui::Unindent(indent_w);
  });

  reg.registerCommand("imgui_begin_group", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    ImGui::BeginGroup();
  });

  reg.registerCommand("imgui_end_group", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    ImGui::EndGroup();
  });

  // === Demo / debug ========================================================
  reg.registerCommand("imgui_show_demo_window", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    bool open = true;
    if (inputCount(inputs) > 1) open = getScalarBool(getInput(inputs, 1));
    ImGui::ShowDemoWindow(&open);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_show_metrics_window", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    bool open = true;
    if (inputCount(inputs) > 1) open = getScalarBool(getInput(inputs, 1));
    ImGui::ShowMetricsWindow(&open);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  // === Multi-component / advanced widgets ==================================
  reg.registerCommand("imgui_drag_float2", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drag_float2(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> v = getVectorFloat(getInput(inputs, 2));
    if (v.size() != 2) throwError(matlabPtr, "Expected 2-element vector");
    float v_speed = 1.0f, v_min = 0.0f, v_max = 0.0f;
    if (inputCount(inputs) > 3) v_speed = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) v_min = getScalarFloat(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_max = getScalarFloat(getInput(inputs, 5));
    const char* format = "%.3f";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::DragFloat2(label.c_str(), v.data(), v_speed, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_drag_float3", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drag_float3(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> v = getVectorFloat(getInput(inputs, 2));
    if (v.size() != 3) throwError(matlabPtr, "Expected 3-element vector");
    float v_speed = 1.0f, v_min = 0.0f, v_max = 0.0f;
    if (inputCount(inputs) > 3) v_speed = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) v_min = getScalarFloat(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_max = getScalarFloat(getInput(inputs, 5));
    const char* format = "%.3f";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::DragFloat3(label.c_str(), v.data(), v_speed, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_drag_float4", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drag_float4(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> v = getVectorFloat(getInput(inputs, 2));
    if (v.size() != 4) throwError(matlabPtr, "Expected 4-element vector");
    float v_speed = 1.0f, v_min = 0.0f, v_max = 0.0f;
    if (inputCount(inputs) > 3) v_speed = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) v_min = getScalarFloat(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_max = getScalarFloat(getInput(inputs, 5));
    const char* format = "%.3f";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::DragFloat4(label.c_str(), v.data(), v_speed, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_drag_int2", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drag_int2(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<int> v = getVectorInt(getInput(inputs, 2));
    if (v.size() != 2) throwError(matlabPtr, "Expected 2-element vector");
    float v_speed = 1.0f;
    int v_min = 0, v_max = 0;
    if (inputCount(inputs) > 3) v_speed = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) v_min = getScalarInt(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_max = getScalarInt(getInput(inputs, 5));
    const char* format = "%d";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::DragInt2(label.c_str(), v.data(), v_speed, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_drag_int3", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drag_int3(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<int> v = getVectorInt(getInput(inputs, 2));
    if (v.size() != 3) throwError(matlabPtr, "Expected 3-element vector");
    float v_speed = 1.0f;
    int v_min = 0, v_max = 0;
    if (inputCount(inputs) > 3) v_speed = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) v_min = getScalarInt(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_max = getScalarInt(getInput(inputs, 5));
    const char* format = "%d";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::DragInt3(label.c_str(), v.data(), v_speed, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_drag_int4", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drag_int4(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<int> v = getVectorInt(getInput(inputs, 2));
    if (v.size() != 4) throwError(matlabPtr, "Expected 4-element vector");
    float v_speed = 1.0f;
    int v_min = 0, v_max = 0;
    if (inputCount(inputs) > 3) v_speed = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) v_min = getScalarInt(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_max = getScalarInt(getInput(inputs, 5));
    const char* format = "%d";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::DragInt4(label.c_str(), v.data(), v_speed, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_drag_float_range2", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_drag_float_range2(label, min, max, ...)");
    std::string label = getString(getInput(inputs, 1));
    float v_min_val = getScalarFloat(getInput(inputs, 2));
    float v_max_val = getScalarFloat(getInput(inputs, 3));
    float v_speed = 1.0f, v_min = 0.0f, v_max = 0.0f;
    if (inputCount(inputs) > 4) v_speed = getScalarFloat(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_min = getScalarFloat(getInput(inputs, 5));
    if (inputCount(inputs) > 6) v_max = getScalarFloat(getInput(inputs, 6));
    const char* format = "%.3f";
    if (inputCount(inputs) > 7) {
      std::string f = getString(getInput(inputs, 7));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 8) flags = getScalarInt(getInput(inputs, 8));
    bool changed = ImGui::DragFloatRange2(label.c_str(), &v_min_val, &v_max_val, v_speed, v_min, v_max, format, nullptr, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v_min_val));
    getOutput(outputs, 2) = createScalarDouble(factory, static_cast<double>(v_max_val));
  });

  reg.registerCommand("imgui_drag_int_range2", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_drag_int_range2(label, min, max, ...)");
    std::string label = getString(getInput(inputs, 1));
    int v_min_val = getScalarInt(getInput(inputs, 2));
    int v_max_val = getScalarInt(getInput(inputs, 3));
    float v_speed = 1.0f;
    int v_min = 0, v_max = 0;
    if (inputCount(inputs) > 4) v_speed = getScalarFloat(getInput(inputs, 4));
    if (inputCount(inputs) > 5) v_min = getScalarInt(getInput(inputs, 5));
    if (inputCount(inputs) > 6) v_max = getScalarInt(getInput(inputs, 6));
    const char* format = "%d";
    if (inputCount(inputs) > 7) {
      std::string f = getString(getInput(inputs, 7));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 8) flags = getScalarInt(getInput(inputs, 8));
    bool changed = ImGui::DragIntRange2(label.c_str(), &v_min_val, &v_max_val, v_speed, v_min, v_max, format, nullptr, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v_min_val));
    getOutput(outputs, 2) = createScalarDouble(factory, static_cast<double>(v_max_val));
  });

  reg.registerCommand("imgui_slider_float2", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_slider_float2(label, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> v = getVectorFloat(getInput(inputs, 2));
    if (v.size() != 2) throwError(matlabPtr, "Expected 2-element vector");
    float v_min = getScalarFloat(getInput(inputs, 3));
    float v_max = getScalarFloat(getInput(inputs, 4));
    const char* format = "%.3f";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::SliderFloat2(label.c_str(), v.data(), v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_slider_float3", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_slider_float3(label, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> v = getVectorFloat(getInput(inputs, 2));
    if (v.size() != 3) throwError(matlabPtr, "Expected 3-element vector");
    float v_min = getScalarFloat(getInput(inputs, 3));
    float v_max = getScalarFloat(getInput(inputs, 4));
    const char* format = "%.3f";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::SliderFloat3(label.c_str(), v.data(), v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_slider_float4", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_slider_float4(label, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> v = getVectorFloat(getInput(inputs, 2));
    if (v.size() != 4) throwError(matlabPtr, "Expected 4-element vector");
    float v_min = getScalarFloat(getInput(inputs, 3));
    float v_max = getScalarFloat(getInput(inputs, 4));
    const char* format = "%.3f";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::SliderFloat4(label.c_str(), v.data(), v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_slider_int2", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_slider_int2(label, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<int> v = getVectorInt(getInput(inputs, 2));
    if (v.size() != 2) throwError(matlabPtr, "Expected 2-element vector");
    int v_min = getScalarInt(getInput(inputs, 3));
    int v_max = getScalarInt(getInput(inputs, 4));
    const char* format = "%d";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::SliderInt2(label.c_str(), v.data(), v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_slider_int3", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_slider_int3(label, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<int> v = getVectorInt(getInput(inputs, 2));
    if (v.size() != 3) throwError(matlabPtr, "Expected 3-element vector");
    int v_min = getScalarInt(getInput(inputs, 3));
    int v_max = getScalarInt(getInput(inputs, 4));
    const char* format = "%d";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::SliderInt3(label.c_str(), v.data(), v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_slider_int4", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_slider_int4(label, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<int> v = getVectorInt(getInput(inputs, 2));
    if (v.size() != 4) throwError(matlabPtr, "Expected 4-element vector");
    int v_min = getScalarInt(getInput(inputs, 3));
    int v_max = getScalarInt(getInput(inputs, 4));
    const char* format = "%d";
    if (inputCount(inputs) > 5) {
      std::string f = getString(getInput(inputs, 5));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImGui::SliderInt4(label.c_str(), v.data(), v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_input_float2", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_float2(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> v = getVectorFloat(getInput(inputs, 2));
    if (v.size() != 2) throwError(matlabPtr, "Expected 2-element vector");
    const char* format = "%.3f";
    if (inputCount(inputs) > 3) {
      std::string f = getString(getInput(inputs, 3));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 4) flags = getScalarInt(getInput(inputs, 4));
    bool changed = ImGui::InputFloat2(label.c_str(), v.data(), format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_input_float3", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_float3(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> v = getVectorFloat(getInput(inputs, 2));
    if (v.size() != 3) throwError(matlabPtr, "Expected 3-element vector");
    const char* format = "%.3f";
    if (inputCount(inputs) > 3) {
      std::string f = getString(getInput(inputs, 3));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 4) flags = getScalarInt(getInput(inputs, 4));
    bool changed = ImGui::InputFloat3(label.c_str(), v.data(), format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_input_float4", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_float4(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> v = getVectorFloat(getInput(inputs, 2));
    if (v.size() != 4) throwError(matlabPtr, "Expected 4-element vector");
    const char* format = "%.3f";
    if (inputCount(inputs) > 3) {
      std::string f = getString(getInput(inputs, 3));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 4) flags = getScalarInt(getInput(inputs, 4));
    bool changed = ImGui::InputFloat4(label.c_str(), v.data(), format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_input_int2", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_int2(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<int> v = getVectorInt(getInput(inputs, 2));
    if (v.size() != 2) throwError(matlabPtr, "Expected 2-element vector");
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool changed = ImGui::InputInt2(label.c_str(), v.data(), flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_input_int3", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_int3(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<int> v = getVectorInt(getInput(inputs, 2));
    if (v.size() != 3) throwError(matlabPtr, "Expected 3-element vector");
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool changed = ImGui::InputInt3(label.c_str(), v.data(), flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_input_int4", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_int4(label, v, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<int> v = getVectorInt(getInput(inputs, 2));
    if (v.size() != 4) throwError(matlabPtr, "Expected 4-element vector");
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool changed = ImGui::InputInt4(label.c_str(), v.data(), flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createVectorDouble(factory, std::vector<double>(v.begin(), v.end()));
  });

  reg.registerCommand("imgui_v_slider_float", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 6) throwError(matlabPtr, "Expected imgui_v_slider_float(label, size, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 2)));
    float v = getScalarFloat(getInput(inputs, 3));
    float v_min = getScalarFloat(getInput(inputs, 4));
    float v_max = getScalarFloat(getInput(inputs, 5));
    const char* format = "%.3f";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::VSliderFloat(label.c_str(), size, &v, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  reg.registerCommand("imgui_v_slider_int", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 6) throwError(matlabPtr, "Expected imgui_v_slider_int(label, size, v, v_min, v_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 2)));
    int v = getScalarInt(getInput(inputs, 3));
    int v_min = getScalarInt(getInput(inputs, 4));
    int v_max = getScalarInt(getInput(inputs, 5));
    const char* format = "%d";
    if (inputCount(inputs) > 6) {
      std::string f = getString(getInput(inputs, 6));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGuiSliderFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImGui::VSliderInt(label.c_str(), size, &v, v_min, v_max, format, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(v));
  });

  reg.registerCommand("imgui_input_text_multiline", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_input_text_multiline(label, buf, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::string buf = getString(getInput(inputs, 2));
    ImVec2 size(0, 0);
    if (inputCount(inputs) > 3) size = toImVec2(getVec2(getInput(inputs, 3)));
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 4) flags = getScalarInt(getInput(inputs, 4));
    size_t max_len = 1024;
    if (inputCount(inputs) > 5) max_len = static_cast<size_t>(getScalarInt(getInput(inputs, 5)));
    if (buf.size() < max_len) buf.resize(max_len, '\0');
    bool changed = ImGui::InputTextMultiline(label.c_str(), buf.data(), max_len, size, flags);
    size_t end = strlen(buf.c_str());
    buf.resize(end);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = factory.createScalar(buf);
  });

  reg.registerCommand("imgui_input_text_with_hint", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_input_text_with_hint(label, hint, buf, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::string hint = getString(getInput(inputs, 2));
    std::string buf = getString(getInput(inputs, 3));
    ImGuiInputTextFlags flags = 0;
    if (inputCount(inputs) > 4) flags = getScalarInt(getInput(inputs, 4));
    size_t max_len = 1024;
    if (inputCount(inputs) > 5) max_len = static_cast<size_t>(getScalarInt(getInput(inputs, 5)));
    if (buf.size() < max_len) buf.resize(max_len, '\0');
    bool changed = ImGui::InputTextWithHint(label.c_str(), hint.c_str(), buf.data(), max_len, flags);
    size_t end = strlen(buf.c_str());
    buf.resize(end);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = factory.createScalar(buf);
  });

  reg.registerCommand("imgui_radio_button_active", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_radio_button_active(label, active)");
    std::string label = getString(getInput(inputs, 1));
    bool active = getScalarBool(getInput(inputs, 2));
    bool clicked = ImGui::RadioButton(label.c_str(), active);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
  });

  reg.registerCommand("imgui_checkbox_flags", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_checkbox_flags(label, flags, flags_value)");
    std::string label = getString(getInput(inputs, 1));
    int flags = getScalarInt(getInput(inputs, 2));
    int flags_value = getScalarInt(getInput(inputs, 3));
    bool changed = ImGui::CheckboxFlags(label.c_str(), &flags, flags_value);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(flags));
  });

  reg.registerCommand("imgui_progress_bar", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_progress_bar(fraction, ...)");
    float fraction = getScalarFloat(getInput(inputs, 1));
    ImVec2 size(-FLT_MIN, 0);
    if (inputCount(inputs) > 2) size = toImVec2(getVec2(getInput(inputs, 2)));
    const char* overlay = nullptr;
    if (inputCount(inputs) > 3) {
      std::string o = getString(getInput(inputs, 3));
      static thread_local std::string overlay_storage;
      overlay_storage = o;
      overlay = overlay_storage.c_str();
    }
    ImGui::ProgressBar(fraction, size, overlay);
  });

  reg.registerCommand("imgui_bullet", [](ArgumentList& outputs, ArgumentList& inputs,
                                         MATLABEngine* matlabPtr) {
    ImGui::Bullet();
  });

  reg.registerCommand("imgui_text_link", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_text_link(label)");
    bool clicked = ImGui::TextLink(getString(getInput(inputs, 1)).c_str());
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
  });

  reg.registerCommand("imgui_text_link_open_url", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_text_link_open_url(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    const char* url = nullptr;
    if (inputCount(inputs) > 2) {
      std::string u = getString(getInput(inputs, 2));
      static thread_local std::string url_storage;
      url_storage = u;
      url = url_storage.c_str();
    }
    bool clicked = ImGui::TextLinkOpenURL(label.c_str(), url);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
  });

  reg.registerCommand("imgui_image", [](ArgumentList& outputs, ArgumentList& inputs,
                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_image(tex_id, size, ...)");
    ImTextureID tex_id = static_cast<ImTextureID>(getScalarDouble(getInput(inputs, 1)));
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 2)));
    ImVec2 uv0(0, 0), uv1(1, 1);
    if (inputCount(inputs) > 3) uv0 = toImVec2(getVec2(getInput(inputs, 3)));
    if (inputCount(inputs) > 4) uv1 = toImVec2(getVec2(getInput(inputs, 4)));
    ImGui::Image(ImTextureRef(tex_id), size, uv0, uv1);
  });

  reg.registerCommand("imgui_image_button", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_image_button(str_id, tex_id, size, ...)");
    std::string str_id = getString(getInput(inputs, 1));
    ImTextureID tex_id = static_cast<ImTextureID>(getScalarDouble(getInput(inputs, 2)));
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 3)));
    ImVec2 uv0(0, 0), uv1(1, 1);
    ImVec4 bg_col(0, 0, 0, 0), tint_col(1, 1, 1, 1);
    if (inputCount(inputs) > 4) uv0 = toImVec2(getVec2(getInput(inputs, 4)));
    if (inputCount(inputs) > 5) uv1 = toImVec2(getVec2(getInput(inputs, 5)));
    if (inputCount(inputs) > 6) bg_col = toImVec4(getVec4(getInput(inputs, 6)));
    if (inputCount(inputs) > 7) tint_col = toImVec4(getVec4(getInput(inputs, 7)));
    bool clicked = ImGui::ImageButton(str_id.c_str(), ImTextureRef(tex_id), size, uv0, uv1, bg_col, tint_col);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, clicked);
  });

  reg.registerCommand("imgui_image_with_bg", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_image_with_bg(tex_id, size, ...)");
    ImTextureID tex_id = static_cast<ImTextureID>(getScalarDouble(getInput(inputs, 1)));
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 2)));
    ImVec2 uv0(0, 0), uv1(1, 1);
    ImVec4 bg_col(0, 0, 0, 0), tint_col(1, 1, 1, 1);
    if (inputCount(inputs) > 3) uv0 = toImVec2(getVec2(getInput(inputs, 3)));
    if (inputCount(inputs) > 4) uv1 = toImVec2(getVec2(getInput(inputs, 4)));
    if (inputCount(inputs) > 5) bg_col = toImVec4(getVec4(getInput(inputs, 5)));
    if (inputCount(inputs) > 6) tint_col = toImVec4(getVec4(getInput(inputs, 6)));
    ImGui::ImageWithBg(ImTextureRef(tex_id), size, uv0, uv1, bg_col, tint_col);
  });

  // === Tree / ID / value =====================================================
  reg.registerCommand("imgui_tree_node_ex", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_tree_node_ex(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImGuiTreeNodeFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    bool open = ImGui::TreeNodeEx(label.c_str(), flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, open);
  });

  reg.registerCommand("imgui_tree_push", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_tree_push(str_id)");
    ImGui::TreePush(getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_get_tree_node_to_label_spacing", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                 MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetTreeNodeToLabelSpacing()));
  });

  reg.registerCommand("imgui_set_next_item_storage_id", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_next_item_storage_id(storage_id)");
    ImGui::SetNextItemStorageID(static_cast<ImGuiID>(getScalarInt(getInput(inputs, 1))));
  });

  reg.registerCommand("imgui_push_id_str", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_push_id_str(str_id)");
    ImGui::PushID(getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_push_id_int", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_push_id_int(int_id)");
    ImGui::PushID(getScalarInt(getInput(inputs, 1)));
  });

  reg.registerCommand("imgui_pop_id", [](ArgumentList& outputs, ArgumentList& inputs,
                                         MATLABEngine* matlabPtr) {
    int count = 1;
    if (inputCount(inputs) > 1) count = getScalarInt(getInput(inputs, 1));
    for (int i = 0; i < count; ++i) ImGui::PopID();
  });

  reg.registerCommand("imgui_get_id_str", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_get_id_str(str_id)");
    ImGuiID id = ImGui::GetID(getString(getInput(inputs, 1)).c_str());
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(id));
  });

  reg.registerCommand("imgui_get_id_int", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_get_id_int(int_id)");
    ImGuiID id = ImGui::GetID(static_cast<int>(getScalarInt(getInput(inputs, 1))));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(id));
  });

  reg.registerCommand("imgui_value_bool", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_value_bool(prefix, b)");
    ImGui::Value(getString(getInput(inputs, 1)).c_str(), getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("imgui_value_int", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_value_int(prefix, v)");
    ImGui::Value(getString(getInput(inputs, 1)).c_str(), getScalarInt(getInput(inputs, 2)));
  });

  reg.registerCommand("imgui_value_float", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_value_float(prefix, v, ...)");
    std::string prefix = getString(getInput(inputs, 1));
    float v = getScalarFloat(getInput(inputs, 2));
    const char* format = nullptr;
    if (inputCount(inputs) > 3) {
      std::string f = getString(getInput(inputs, 3));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      format = fmt_storage.c_str();
    }
    ImGui::Value(prefix.c_str(), v, format);
  });

  reg.registerCommand("imgui_plot_lines", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_plot_lines(label, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> values = getVectorFloat(getInput(inputs, 2));
    int values_offset = 0;
    if (inputCount(inputs) > 3) values_offset = getScalarInt(getInput(inputs, 3));
    const char* overlay = nullptr;
    if (inputCount(inputs) > 4) {
      std::string o = getString(getInput(inputs, 4));
      static thread_local std::string overlay_storage;
      overlay_storage = o;
      overlay = overlay_storage.c_str();
    }
    float scale_min = FLT_MAX, scale_max = FLT_MAX;
    if (inputCount(inputs) > 5) scale_min = getScalarFloat(getInput(inputs, 5));
    if (inputCount(inputs) > 6) scale_max = getScalarFloat(getInput(inputs, 6));
    ImVec2 size(0, 0);
    if (inputCount(inputs) > 7) size = toImVec2(getVec2(getInput(inputs, 7)));
    ImGui::PlotLines(label.c_str(), values.data(), static_cast<int>(values.size()), values_offset, overlay, scale_min, scale_max, size);
  });

  reg.registerCommand("imgui_plot_histogram", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_plot_histogram(label, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    std::vector<float> values = getVectorFloat(getInput(inputs, 2));
    int values_offset = 0;
    if (inputCount(inputs) > 3) values_offset = getScalarInt(getInput(inputs, 3));
    const char* overlay = nullptr;
    if (inputCount(inputs) > 4) {
      std::string o = getString(getInput(inputs, 4));
      static thread_local std::string overlay_storage;
      overlay_storage = o;
      overlay = overlay_storage.c_str();
    }
    float scale_min = FLT_MAX, scale_max = FLT_MAX;
    if (inputCount(inputs) > 5) scale_min = getScalarFloat(getInput(inputs, 5));
    if (inputCount(inputs) > 6) scale_max = getScalarFloat(getInput(inputs, 6));
    ImVec2 size(0, 0);
    if (inputCount(inputs) > 7) size = toImVec2(getVec2(getInput(inputs, 7)));
    ImGui::PlotHistogram(label.c_str(), values.data(), static_cast<int>(values.size()), values_offset, overlay, scale_min, scale_max, size);
  });

  // === Item / window / mouse / keyboard queries ==============================
  auto registerBoolQuery = [&](const char* name, std::function<bool()> fn) {
    reg.registerCommand(name, [fn](ArgumentList& outputs, ArgumentList& inputs,
                                   MATLABEngine* matlabPtr) {
      matlab::data::ArrayFactory factory;
      getOutput(outputs, 0) = createScalarBool(factory, fn());
    });
  };

  registerBoolQuery("imgui_is_item_focused", []() { return ImGui::IsItemFocused(); });
  registerBoolQuery("imgui_is_item_visible", []() { return ImGui::IsItemVisible(); });
  registerBoolQuery("imgui_is_item_edited", []() { return ImGui::IsItemEdited(); });
  registerBoolQuery("imgui_is_item_activated", []() { return ImGui::IsItemActivated(); });
  registerBoolQuery("imgui_is_item_deactivated", []() { return ImGui::IsItemDeactivated(); });
  registerBoolQuery("imgui_is_item_deactivated_after_edit", []() { return ImGui::IsItemDeactivatedAfterEdit(); });
  registerBoolQuery("imgui_is_item_toggled_open", []() { return ImGui::IsItemToggledOpen(); });
  registerBoolQuery("imgui_is_any_item_hovered", []() { return ImGui::IsAnyItemHovered(); });
  registerBoolQuery("imgui_is_any_item_active", []() { return ImGui::IsAnyItemActive(); });
  registerBoolQuery("imgui_is_any_item_focused", []() { return ImGui::IsAnyItemFocused(); });
  registerBoolQuery("imgui_is_window_appearing", []() { return ImGui::IsWindowAppearing(); });
  registerBoolQuery("imgui_is_window_collapsed", []() { return ImGui::IsWindowCollapsed(); });
  registerBoolQuery("imgui_is_any_mouse_down", []() { return ImGui::IsAnyMouseDown(); });

  reg.registerCommand("imgui_is_window_focused", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    ImGuiFocusedFlags flags = 0;
    if (inputCount(inputs) > 1) flags = getScalarInt(getInput(inputs, 1));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsWindowFocused(flags));
  });

  reg.registerCommand("imgui_get_item_id", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetItemID()));
  });

  reg.registerCommand("imgui_get_item_rect_min", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetItemRectMin();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_get_item_rect_max", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetItemRectMax();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_get_item_rect_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetItemRectSize();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_get_window_draw_list", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, dl ? static_cast<double>(reinterpret_cast<uintptr_t>(dl)) : 0.0);
  });

  reg.registerCommand("imgui_get_window_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetWindowWidth()));
  });

  reg.registerCommand("imgui_get_window_height", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetWindowHeight()));
  });

  reg.registerCommand("imgui_get_scroll_x", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetScrollX()));
  });

  reg.registerCommand("imgui_get_scroll_y", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetScrollY()));
  });

  reg.registerCommand("imgui_set_scroll_x", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_scroll_x(x)");
    ImGui::SetScrollX(getScalarFloat(getInput(inputs, 1)));
  });

  reg.registerCommand("imgui_set_scroll_y", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_scroll_y(y)");
    ImGui::SetScrollY(getScalarFloat(getInput(inputs, 1)));
  });

  reg.registerCommand("imgui_get_scroll_max_x", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetScrollMaxX()));
  });

  reg.registerCommand("imgui_get_scroll_max_y", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetScrollMaxY()));
  });

  reg.registerCommand("imgui_set_scroll_here_x", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    float center_x_ratio = 0.5f;
    if (inputCount(inputs) > 1) center_x_ratio = getScalarFloat(getInput(inputs, 1));
    ImGui::SetScrollHereX(center_x_ratio);
  });

  reg.registerCommand("imgui_set_scroll_here_y", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    float center_y_ratio = 0.5f;
    if (inputCount(inputs) > 1) center_y_ratio = getScalarFloat(getInput(inputs, 1));
    ImGui::SetScrollHereY(center_y_ratio);
  });

  reg.registerCommand("imgui_set_scroll_from_pos_x", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_scroll_from_pos_x(local_x, ...)");
    float local_x = getScalarFloat(getInput(inputs, 1));
    float center_x_ratio = 0.5f;
    if (inputCount(inputs) > 2) center_x_ratio = getScalarFloat(getInput(inputs, 2));
    ImGui::SetScrollFromPosX(local_x, center_x_ratio);
  });

  reg.registerCommand("imgui_set_scroll_from_pos_y", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_scroll_from_pos_y(local_y, ...)");
    float local_y = getScalarFloat(getInput(inputs, 1));
    float center_y_ratio = 0.5f;
    if (inputCount(inputs) > 2) center_y_ratio = getScalarFloat(getInput(inputs, 2));
    ImGui::SetScrollFromPosY(local_y, center_y_ratio);
  });

  reg.registerCommand("imgui_get_mouse_pos_on_opening_current_popup", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                           MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetMousePosOnOpeningCurrentPopup();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_is_mouse_hovering_rect", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_is_mouse_hovering_rect(r_min, r_max, ...)");
    ImVec2 r_min = toImVec2(getVec2(getInput(inputs, 1)));
    ImVec2 r_max = toImVec2(getVec2(getInput(inputs, 2)));
    bool clip = true;
    if (inputCount(inputs) > 3) clip = getScalarBool(getInput(inputs, 3));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsMouseHoveringRect(r_min, r_max, clip));
  });

  reg.registerCommand("imgui_is_mouse_pos_valid", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) > 1) {
      ImVec2 pos = toImVec2(getVec2(getInput(inputs, 1)));
      matlab::data::ArrayFactory factory;
      getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsMousePosValid(&pos));
    } else {
      matlab::data::ArrayFactory factory;
      getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsMousePosValid());
    }
  });

  reg.registerCommand("imgui_get_mouse_clicked_count", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_get_mouse_clicked_count(button)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetMouseClickedCount(getScalarInt(getInput(inputs, 1)))));
  });

  reg.registerCommand("imgui_is_mouse_dragging", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_is_mouse_dragging(button, ...)");
    ImGuiMouseButton button = static_cast<ImGuiMouseButton>(getScalarInt(getInput(inputs, 1)));
    float lock_threshold = -1.0f;
    if (inputCount(inputs) > 2) lock_threshold = getScalarFloat(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsMouseDragging(button, lock_threshold));
  });

  reg.registerCommand("imgui_get_mouse_drag_delta", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    ImGuiMouseButton button = 0;
    if (inputCount(inputs) > 1) button = static_cast<ImGuiMouseButton>(getScalarInt(getInput(inputs, 1)));
    float lock_threshold = -1.0f;
    if (inputCount(inputs) > 2) lock_threshold = getScalarFloat(getInput(inputs, 2));
    ImVec2 v = ImGui::GetMouseDragDelta(button, lock_threshold);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_reset_mouse_drag_delta", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    ImGuiMouseButton button = 0;
    if (inputCount(inputs) > 1) button = static_cast<ImGuiMouseButton>(getScalarInt(getInput(inputs, 1)));
    ImGui::ResetMouseDragDelta(button);
  });

  reg.registerCommand("imgui_get_mouse_cursor", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetMouseCursor()));
  });

  reg.registerCommand("imgui_set_mouse_cursor", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_mouse_cursor(cursor_type)");
    ImGui::SetMouseCursor(static_cast<ImGuiMouseCursor>(getScalarInt(getInput(inputs, 1))));
  });

  reg.registerCommand("imgui_is_key_down", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_is_key_down(key)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsKeyDown(static_cast<ImGuiKey>(getScalarInt(getInput(inputs, 1)))));
  });

  reg.registerCommand("imgui_is_key_pressed", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_is_key_pressed(key, ...)");
    ImGuiKey key = static_cast<ImGuiKey>(getScalarInt(getInput(inputs, 1)));
    bool repeat = true;
    if (inputCount(inputs) > 2) repeat = getScalarBool(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsKeyPressed(key, repeat));
  });

  reg.registerCommand("imgui_is_key_released", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_is_key_released(key)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsKeyReleased(static_cast<ImGuiKey>(getScalarInt(getInput(inputs, 1)))));
  });

  reg.registerCommand("imgui_is_key_chord_pressed", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_is_key_chord_pressed(key_chord)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsKeyChordPressed(static_cast<ImGuiKeyChord>(getScalarInt(getInput(inputs, 1)))));
  });

  reg.registerCommand("imgui_get_key_pressed_amount", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_get_key_pressed_amount(key, repeat_delay, rate)");
    ImGuiKey key = static_cast<ImGuiKey>(getScalarInt(getInput(inputs, 1)));
    float repeat_delay = getScalarFloat(getInput(inputs, 2));
    float rate = getScalarFloat(getInput(inputs, 3));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetKeyPressedAmount(key, repeat_delay, rate)));
  });

  reg.registerCommand("imgui_get_key_name", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_get_key_name(key)");
    const char* name = ImGui::GetKeyName(static_cast<ImGuiKey>(getScalarInt(getInput(inputs, 1))));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(std::string(name ? name : ""));
  });

  reg.registerCommand("imgui_set_next_frame_want_capture_keyboard", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_next_frame_want_capture_keyboard(want)");
    ImGui::SetNextFrameWantCaptureKeyboard(getScalarBool(getInput(inputs, 1)));
  });

  reg.registerCommand("imgui_set_next_frame_want_capture_mouse", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_next_frame_want_capture_mouse(want)");
    ImGui::SetNextFrameWantCaptureMouse(getScalarBool(getInput(inputs, 1)));
  });

  // === Layout / style / utility ==============================================
  reg.registerCommand("imgui_set_next_item_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_next_item_width(width)");
    ImGui::SetNextItemWidth(getScalarFloat(getInput(inputs, 1)));
  });

  reg.registerCommand("imgui_calc_item_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::CalcItemWidth()));
  });

  reg.registerCommand("imgui_push_text_wrap_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    float wrap_pos_x = 0.0f;
    if (inputCount(inputs) > 1) wrap_pos_x = getScalarFloat(getInput(inputs, 1));
    ImGui::PushTextWrapPos(wrap_pos_x);
  });

  reg.registerCommand("imgui_pop_text_wrap_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    ImGui::PopTextWrapPos();
  });

  reg.registerCommand("imgui_get_font_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetFontSize()));
  });

  reg.registerCommand("imgui_get_font_tex_uv_white_pixel", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetFontTexUvWhitePixel();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_get_color_u32_idx", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_get_color_u32_idx(idx, ...)");
    ImGuiCol idx = static_cast<ImGuiCol>(getScalarInt(getInput(inputs, 1)));
    float alpha_mul = 1.0f;
    if (inputCount(inputs) > 2) alpha_mul = getScalarFloat(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar<uint32_t>(ImGui::GetColorU32(idx, alpha_mul));
  });

  reg.registerCommand("imgui_get_color_u32_vec4", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_get_color_u32_vec4(col)");
    ImU32 col = ImGui::GetColorU32(toImVec4(getVec4(getInput(inputs, 1))));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar<uint32_t>(col);
  });

  reg.registerCommand("imgui_get_color_u32_u32", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_get_color_u32_u32(col, ...)");
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 1)));
    float alpha_mul = 1.0f;
    if (inputCount(inputs) > 2) alpha_mul = getScalarFloat(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar<uint32_t>(ImGui::GetColorU32(col, alpha_mul));
  });

  reg.registerCommand("imgui_get_style_color_vec4", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_get_style_color_vec4(idx)");
    ImVec4 v = ImGui::GetStyleColorVec4(static_cast<ImGuiCol>(getScalarInt(getInput(inputs, 1))));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec4(factory, static_cast<double>(v.x), static_cast<double>(v.y), static_cast<double>(v.z), static_cast<double>(v.w));
  });

  reg.registerCommand("imgui_get_cursor_screen_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetCursorScreenPos();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_set_cursor_screen_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_cursor_screen_pos(pos)");
    ImGui::SetCursorScreenPos(toImVec2(getVec2(getInput(inputs, 1))));
  });

  reg.registerCommand("imgui_get_cursor_pos_x", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetCursorPosX()));
  });

  reg.registerCommand("imgui_get_cursor_pos_y", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetCursorPosY()));
  });

  reg.registerCommand("imgui_set_cursor_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_cursor_pos(pos)");
    ImGui::SetCursorPos(toImVec2(getVec2(getInput(inputs, 1))));
  });

  reg.registerCommand("imgui_set_cursor_pos_x", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_cursor_pos_x(x)");
    ImGui::SetCursorPosX(getScalarFloat(getInput(inputs, 1)));
  });

  reg.registerCommand("imgui_set_cursor_pos_y", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_cursor_pos_y(y)");
    ImGui::SetCursorPosY(getScalarFloat(getInput(inputs, 1)));
  });

  reg.registerCommand("imgui_get_cursor_start_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    ImVec2 v = ImGui::GetCursorStartPos();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_get_text_line_height", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetTextLineHeight()));
  });

  reg.registerCommand("imgui_get_text_line_height_with_spacing", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                    MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetTextLineHeightWithSpacing()));
  });

  reg.registerCommand("imgui_get_frame_height", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetFrameHeight()));
  });

  reg.registerCommand("imgui_get_frame_height_with_spacing", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetFrameHeightWithSpacing()));
  });

  reg.registerCommand("imgui_align_text_to_frame_padding", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    ImGui::AlignTextToFramePadding();
  });

  reg.registerCommand("imgui_get_time", [](ArgumentList& outputs, ArgumentList& inputs,
                                           MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetTime()));
  });

  reg.registerCommand("imgui_get_frame_count", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetFrameCount()));
  });

  reg.registerCommand("imgui_get_style_color_name", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_get_style_color_name(idx)");
    const char* name = ImGui::GetStyleColorName(static_cast<ImGuiCol>(getScalarInt(getInput(inputs, 1))));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(std::string(name ? name : ""));
  });

  reg.registerCommand("imgui_calc_text_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_calc_text_size(text, ...)");
    std::string text = getString(getInput(inputs, 1));
    bool hide_after_double_hash = false;
    float wrap_width = -1.0f;
    if (inputCount(inputs) > 2) hide_after_double_hash = getScalarBool(getInput(inputs, 2));
    if (inputCount(inputs) > 3) wrap_width = getScalarFloat(getInput(inputs, 3));
    ImVec2 v = ImGui::CalcTextSize(text.c_str(), nullptr, hide_after_double_hash, wrap_width);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_color_convert_u32_to_float4", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_color_convert_u32_to_float4(in_val)");
    ImU32 in_val = static_cast<ImU32>(getScalarDouble(getInput(inputs, 1)));
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(in_val);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec4(factory, static_cast<double>(v.x), static_cast<double>(v.y), static_cast<double>(v.z), static_cast<double>(v.w));
  });

  reg.registerCommand("imgui_color_convert_float4_to_u32", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_color_convert_float4_to_u32(col)");
    ImU32 col = ImGui::ColorConvertFloat4ToU32(toImVec4(getVec4(getInput(inputs, 1))));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar<uint32_t>(col);
  });

  reg.registerCommand("imgui_color_convert_rgb_to_hsv", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_color_convert_rgb_to_hsv(r, g, b)");
    float r = getScalarFloat(getInput(inputs, 1));
    float g = getScalarFloat(getInput(inputs, 2));
    float b = getScalarFloat(getInput(inputs, 3));
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(r, g, b, h, s, v);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, std::vector<double>{static_cast<double>(h), static_cast<double>(s), static_cast<double>(v)});
  });

  reg.registerCommand("imgui_color_convert_hsv_to_rgb", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_color_convert_hsv_to_rgb(h, s, v)");
    float h = getScalarFloat(getInput(inputs, 1));
    float s = getScalarFloat(getInput(inputs, 2));
    float v = getScalarFloat(getInput(inputs, 3));
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, std::vector<double>{static_cast<double>(r), static_cast<double>(g), static_cast<double>(b)});
  });

  reg.registerCommand("imgui_get_clipboard_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    const char* text = ImGui::GetClipboardText();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(std::string(text ? text : ""));
  });

  reg.registerCommand("imgui_set_clipboard_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_clipboard_text(text)");
    ImGui::SetClipboardText(getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_load_ini_settings_from_disk", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_load_ini_settings_from_disk(filename)");
    ImGui::LoadIniSettingsFromDisk(getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_load_ini_settings_from_memory", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_load_ini_settings_from_memory(data)");
    std::string data = getString(getInput(inputs, 1));
    ImGui::LoadIniSettingsFromMemory(data.c_str(), data.size());
  });

  reg.registerCommand("imgui_save_ini_settings_to_disk", [](ArgumentList& outputs, ArgumentList& inputs,
                                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_save_ini_settings_to_disk(filename)");
    ImGui::SaveIniSettingsToDisk(getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_save_ini_settings_to_memory", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    size_t size = 0;
    const char* data = ImGui::SaveIniSettingsToMemory(&size);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(std::string(data ? data : ""));
  });

  reg.registerCommand("imgui_push_item_flag", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_push_item_flag(option, enabled)");
    ImGuiItemFlags option = static_cast<ImGuiItemFlags>(getScalarInt(getInput(inputs, 1)));
    bool enabled = getScalarBool(getInput(inputs, 2));
    ImGui::PushItemFlag(option, enabled);
  });

  reg.registerCommand("imgui_pop_item_flag", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    int count = 1;
    if (inputCount(inputs) > 1) count = getScalarInt(getInput(inputs, 1));
    for (int i = 0; i < count; ++i) ImGui::PopItemFlag();
  });

  // === Window manipulation ===================================================
  reg.registerCommand("imgui_set_next_window_size_constraints", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_set_next_window_size_constraints(size_min, size_max)");
    ImVec2 size_min = toImVec2(getVec2(getInput(inputs, 1)));
    ImVec2 size_max = toImVec2(getVec2(getInput(inputs, 2)));
    ImGui::SetNextWindowSizeConstraints(size_min, size_max);
  });

  reg.registerCommand("imgui_set_next_window_content_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_next_window_content_size(size)");
    ImGui::SetNextWindowContentSize(toImVec2(getVec2(getInput(inputs, 1))));
  });

  reg.registerCommand("imgui_set_next_window_collapsed", [](ArgumentList& outputs, ArgumentList& inputs,
                                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_next_window_collapsed(collapsed, ...)");
    bool collapsed = getScalarBool(getInput(inputs, 1));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 2) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 2)));
    ImGui::SetNextWindowCollapsed(collapsed, cond);
  });

  reg.registerCommand("imgui_set_next_window_focus", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    ImGui::SetNextWindowFocus();
  });

  reg.registerCommand("imgui_set_next_window_scroll", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_next_window_scroll(scroll)");
    ImGui::SetNextWindowScroll(toImVec2(getVec2(getInput(inputs, 1))));
  });

  reg.registerCommand("imgui_set_next_window_bg_alpha", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_next_window_bg_alpha(alpha)");
    ImGui::SetNextWindowBgAlpha(getScalarFloat(getInput(inputs, 1)));
  });

  reg.registerCommand("imgui_set_window_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_window_pos(pos, ...)");
    ImVec2 pos = toImVec2(getVec2(getInput(inputs, 1)));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 2) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 2)));
    ImGui::SetWindowPos(pos, cond);
  });

  reg.registerCommand("imgui_set_window_pos_named", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_set_window_pos_named(name, pos, ...)");
    std::string name = getString(getInput(inputs, 1));
    ImVec2 pos = toImVec2(getVec2(getInput(inputs, 2)));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 3) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 3)));
    ImGui::SetWindowPos(name.c_str(), pos, cond);
  });

  reg.registerCommand("imgui_set_window_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_window_size(size, ...)");
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 1)));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 2) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 2)));
    ImGui::SetWindowSize(size, cond);
  });

  reg.registerCommand("imgui_set_window_size_named", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_set_window_size_named(name, size, ...)");
    std::string name = getString(getInput(inputs, 1));
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 2)));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 3) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 3)));
    ImGui::SetWindowSize(name.c_str(), size, cond);
  });

  reg.registerCommand("imgui_set_window_collapsed", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_window_collapsed(collapsed, ...)");
    bool collapsed = getScalarBool(getInput(inputs, 1));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 2) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 2)));
    ImGui::SetWindowCollapsed(collapsed, cond);
  });

  reg.registerCommand("imgui_set_window_collapsed_named", [](ArgumentList& outputs, ArgumentList& inputs,
                                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_set_window_collapsed_named(name, collapsed, ...)");
    std::string name = getString(getInput(inputs, 1));
    bool collapsed = getScalarBool(getInput(inputs, 2));
    ImGuiCond cond = ImGuiCond_Always;
    if (inputCount(inputs) > 3) cond = static_cast<ImGuiCond>(getScalarInt(getInput(inputs, 3)));
    ImGui::SetWindowCollapsed(name.c_str(), collapsed, cond);
  });

  reg.registerCommand("imgui_set_window_focus", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    ImGui::SetWindowFocus();
  });

  reg.registerCommand("imgui_set_window_focus_named", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_window_focus_named(name)");
    ImGui::SetWindowFocus(getString(getInput(inputs, 1)).c_str());
  });

  // === Tab bar ===============================================================
  reg.registerCommand("imgui_begin_tab_bar", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_begin_tab_bar(str_id, ...)");
    std::string str_id = getString(getInput(inputs, 1));
    ImGuiTabBarFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::BeginTabBar(str_id.c_str(), flags));
  });

  reg.registerCommand("imgui_end_tab_bar", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    ImGui::EndTabBar();
  });

  reg.registerCommand("imgui_begin_tab_item", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_begin_tab_item(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    bool p_open = true;
    bool* p_open_ptr = nullptr;
    ImGuiTabItemFlags flags = 0;
    if (inputCount(inputs) > 2) {
      p_open = getScalarBool(getInput(inputs, 2));
      p_open_ptr = &p_open;
    }
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool selected = ImGui::BeginTabItem(label.c_str(), p_open_ptr, flags);
    matlab::data::ArrayFactory factory;
    if (p_open_ptr) {
      getOutput(outputs, 0) = createScalarBool(factory, p_open);
      getOutput(outputs, 1) = createScalarBool(factory, selected);
    } else {
      getOutput(outputs, 0) = createScalarBool(factory, selected);
    }
  });

  reg.registerCommand("imgui_end_tab_item", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    ImGui::EndTabItem();
  });

  reg.registerCommand("imgui_tab_item_button", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_tab_item_button(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImGuiTabItemFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::TabItemButton(label.c_str(), flags));
  });

  reg.registerCommand("imgui_set_tab_item_closed", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_set_tab_item_closed(tab_or_docked_window_label)");
    ImGui::SetTabItemClosed(getString(getInput(inputs, 1)).c_str());
  });

  // === Table =================================================================
  reg.registerCommand("imgui_begin_table", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_begin_table(str_id, columns, ...)");
    std::string str_id = getString(getInput(inputs, 1));
    int columns = getScalarInt(getInput(inputs, 2));
    ImGuiTableFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    ImVec2 outer_size(0.0f, 0.0f);
    if (inputCount(inputs) > 4) outer_size = toImVec2(getVec2(getInput(inputs, 4)));
    float inner_width = 0.0f;
    if (inputCount(inputs) > 5) inner_width = getScalarFloat(getInput(inputs, 5));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::BeginTable(str_id.c_str(), columns, flags, outer_size, inner_width));
  });

  reg.registerCommand("imgui_end_table", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    ImGui::EndTable();
  });

  reg.registerCommand("imgui_table_next_row", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    ImGuiTableRowFlags row_flags = 0;
    float min_row_height = 0.0f;
    if (inputCount(inputs) > 1) row_flags = getScalarInt(getInput(inputs, 1));
    if (inputCount(inputs) > 2) min_row_height = getScalarFloat(getInput(inputs, 2));
    ImGui::TableNextRow(row_flags, min_row_height);
  });

  reg.registerCommand("imgui_table_next_column", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::TableNextColumn());
  });

  reg.registerCommand("imgui_table_set_column_index", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_table_set_column_index(column_n)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::TableSetColumnIndex(getScalarInt(getInput(inputs, 1))));
  });

  reg.registerCommand("imgui_table_setup_column", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_table_setup_column(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImGuiTableColumnFlags flags = 0;
    float init_width_or_weight = 0.0f;
    ImGuiID user_id = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    if (inputCount(inputs) > 3) init_width_or_weight = getScalarFloat(getInput(inputs, 3));
    if (inputCount(inputs) > 4) user_id = static_cast<ImGuiID>(getScalarInt(getInput(inputs, 4)));
    ImGui::TableSetupColumn(label.c_str(), flags, init_width_or_weight, user_id);
  });

  reg.registerCommand("imgui_table_setup_scroll_freeze", [](ArgumentList& outputs, ArgumentList& inputs,
                                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_table_setup_scroll_freeze(cols, rows)");
    ImGui::TableSetupScrollFreeze(getScalarInt(getInput(inputs, 1)), getScalarInt(getInput(inputs, 2)));
  });

  reg.registerCommand("imgui_table_header", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_table_header(label)");
    ImGui::TableHeader(getString(getInput(inputs, 1)).c_str());
  });

  reg.registerCommand("imgui_table_headers_row", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    ImGui::TableHeadersRow();
  });

  reg.registerCommand("imgui_table_angled_headers_row", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    ImGui::TableAngledHeadersRow();
  });

  reg.registerCommand("imgui_table_get_column_count", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::TableGetColumnCount()));
  });

  reg.registerCommand("imgui_table_get_column_index", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::TableGetColumnIndex()));
  });

  reg.registerCommand("imgui_table_get_row_index", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::TableGetRowIndex()));
  });

  reg.registerCommand("imgui_table_get_column_name", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    int column_n = -1;
    if (inputCount(inputs) > 1) column_n = getScalarInt(getInput(inputs, 1));
    const char* name = ImGui::TableGetColumnName(column_n);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(std::string(name ? name : ""));
  });

  reg.registerCommand("imgui_table_get_column_flags", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    int column_n = -1;
    if (inputCount(inputs) > 1) column_n = getScalarInt(getInput(inputs, 1));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::TableGetColumnFlags(column_n)));
  });

  reg.registerCommand("imgui_table_set_column_enabled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_table_set_column_enabled(column_n, v)");
    ImGui::TableSetColumnEnabled(getScalarInt(getInput(inputs, 1)), getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("imgui_table_get_hovered_column", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::TableGetHoveredColumn()));
  });

  reg.registerCommand("imgui_table_set_bg_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_table_set_bg_color(target, color, ...)");
    ImGuiTableBgTarget target = static_cast<ImGuiTableBgTarget>(getScalarInt(getInput(inputs, 1)));
    ImU32 color = static_cast<ImU32>(getScalarDouble(getInput(inputs, 2)));
    int column_n = -1;
    if (inputCount(inputs) > 3) column_n = getScalarInt(getInput(inputs, 3));
    ImGui::TableSetBgColor(target, color, column_n);
  });

  // === Legacy columns ========================================================
  reg.registerCommand("imgui_columns", [](ArgumentList& outputs, ArgumentList& inputs,
                                          MATLABEngine* matlabPtr) {
    int count = 1;
    const char* id = nullptr;
    bool borders = true;
    if (inputCount(inputs) > 1) count = getScalarInt(getInput(inputs, 1));
    if (inputCount(inputs) > 2) {
      std::string s = getString(getInput(inputs, 2));
      static thread_local std::string id_storage;
      id_storage = s;
      id = id_storage.c_str();
    }
    if (inputCount(inputs) > 3) borders = getScalarBool(getInput(inputs, 3));
    ImGui::Columns(count, id, borders);
  });

  reg.registerCommand("imgui_next_column", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    ImGui::NextColumn();
  });

  reg.registerCommand("imgui_get_column_index", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetColumnIndex()));
  });

  reg.registerCommand("imgui_get_column_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    int column_index = -1;
    if (inputCount(inputs) > 1) column_index = getScalarInt(getInput(inputs, 1));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetColumnWidth(column_index)));
  });

  reg.registerCommand("imgui_set_column_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_set_column_width(column_index, width)");
    ImGui::SetColumnWidth(getScalarInt(getInput(inputs, 1)), getScalarFloat(getInput(inputs, 2)));
  });

  reg.registerCommand("imgui_get_column_offset", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    int column_index = -1;
    if (inputCount(inputs) > 1) column_index = getScalarInt(getInput(inputs, 1));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetColumnOffset(column_index)));
  });

  reg.registerCommand("imgui_set_column_offset", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_set_column_offset(column_index, offset_x)");
    ImGui::SetColumnOffset(getScalarInt(getInput(inputs, 1)), getScalarFloat(getInput(inputs, 2)));
  });

  reg.registerCommand("imgui_get_columns_count", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(ImGui::GetColumnsCount()));
  });

  // === Viewport / draw lists =================================================
  reg.registerCommand("imgui_get_main_viewport", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, vp ? static_cast<double>(reinterpret_cast<uintptr_t>(vp)) : 0.0);
  });

  reg.registerCommand("imgui_get_background_draw_list", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, dl ? static_cast<double>(reinterpret_cast<uintptr_t>(dl)) : 0.0);
  });

  reg.registerCommand("imgui_get_foreground_draw_list", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, dl ? static_cast<double>(reinterpret_cast<uintptr_t>(dl)) : 0.0);
  });

  reg.registerCommand("imgui_is_rect_visible", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_is_rect_visible(size)");
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsRectVisible(size));
  });

  reg.registerCommand("imgui_is_rect_visible_rect", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_is_rect_visible_rect(rect_min, rect_max)");
    ImVec2 r_min = toImVec2(getVec2(getInput(inputs, 1)));
    ImVec2 r_max = toImVec2(getVec2(getInput(inputs, 2)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ImGui::IsRectVisible(r_min, r_max));
  });

  // === Object handles (ImGuiIO, ImGuiStyle, ImDrawList) ======================
  reg.registerCommand("imgui_get_io", [](ArgumentList& outputs, ArgumentList& inputs,
                                         MATLABEngine* matlabPtr) {
    ImGuiIO* io = &ImGui::GetIO();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, io ? static_cast<double>(reinterpret_cast<uintptr_t>(io)) : 0.0);
  });

  reg.registerCommand("imgui_get_style", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    ImGuiStyle* style = &ImGui::GetStyle();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, style ? static_cast<double>(reinterpret_cast<uintptr_t>(style)) : 0.0);
  });

  reg.registerCommand("imgui_style_colors_dark", [](ArgumentList&, ArgumentList&,
                                                      MATLABEngine*) {
    ImGui::StyleColorsDark();
  });

  reg.registerCommand("imgui_style_colors_light", [](ArgumentList&, ArgumentList&,
                                                       MATLABEngine*) {
    ImGui::StyleColorsLight();
  });

  reg.registerCommand("imgui_set_style_color", [](ArgumentList&, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_set_style_color(idx, color)");
    int idx = getScalarInt(getInput(inputs, 1));
    if (idx < 0 || idx >= ImGuiCol_COUNT) throwError(matlabPtr, "ImGui style color index is out of range");
    ImGui::GetStyle().Colors[idx] = toImVec4(getVec4(getInput(inputs, 2)));
  });

  reg.registerCommand("imgui_io_get", [](ArgumentList& outputs, ArgumentList& inputs,
                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_io_get(handle, prop_name)");
    ImGuiIO* io = imguiIOFromHandle(getInput(inputs, 1));
    if (!io) throwError(matlabPtr, "Invalid ImGuiIO handle");
    std::string prop = getString(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;

    if (prop == "ConfigFlags") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->ConfigFlags));
    } else if (prop == "BackendFlags") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->BackendFlags));
    } else if (prop == "DisplaySize") {
      getOutput(outputs, 0) = createVec2(factory, static_cast<double>(io->DisplaySize.x),
                                                 static_cast<double>(io->DisplaySize.y));
    } else if (prop == "DeltaTime") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->DeltaTime));
    } else if (prop == "IniSavingRate") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->IniSavingRate));
    } else if (prop == "IniFilename") {
      getOutput(outputs, 0) = factory.createCharArray(io->IniFilename ? io->IniFilename : "");
    } else if (prop == "LogFilename") {
      getOutput(outputs, 0) = factory.createCharArray(io->LogFilename ? io->LogFilename : "");
    } else if (prop == "FontAllowUserScaling") {
      getOutput(outputs, 0) = createScalarBool(factory, io->FontAllowUserScaling);
    } else if (prop == "DisplayFramebufferScale") {
      getOutput(outputs, 0) = createVec2(factory, static_cast<double>(io->DisplayFramebufferScale.x),
                                                 static_cast<double>(io->DisplayFramebufferScale.y));
    } else if (prop == "ConfigNavSwapGamepadButtons") {
      getOutput(outputs, 0) = createScalarBool(factory, io->ConfigNavSwapGamepadButtons);
    } else if (prop == "ConfigNavMoveSetMousePos") {
      getOutput(outputs, 0) = createScalarBool(factory, io->ConfigNavMoveSetMousePos);
    } else if (prop == "ConfigNavCaptureKeyboard") {
      getOutput(outputs, 0) = createScalarBool(factory, io->ConfigNavCaptureKeyboard);
    } else if (prop == "ConfigNavEscapeClearFocusItem") {
      getOutput(outputs, 0) = createScalarBool(factory, io->ConfigNavEscapeClearFocusItem);
    } else if (prop == "ConfigNavEscapeClearFocusWindow") {
      getOutput(outputs, 0) = createScalarBool(factory, io->ConfigNavEscapeClearFocusWindow);
    } else if (prop == "ConfigNavCursorVisibleAuto") {
      getOutput(outputs, 0) = createScalarBool(factory, io->ConfigNavCursorVisibleAuto);
    } else if (prop == "ConfigNavCursorVisibleAlways") {
      getOutput(outputs, 0) = createScalarBool(factory, io->ConfigNavCursorVisibleAlways);
    } else if (prop == "MouseDrawCursor") {
      getOutput(outputs, 0) = createScalarBool(factory, io->MouseDrawCursor);
    } else if (prop == "ConfigMacOSXBehaviors") {
      getOutput(outputs, 0) = createScalarBool(factory, io->ConfigMacOSXBehaviors);
    } else if (prop == "MouseDoubleClickTime") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->MouseDoubleClickTime));
    } else if (prop == "MouseDoubleClickMaxDist") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->MouseDoubleClickMaxDist));
    } else if (prop == "MouseDragThreshold") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->MouseDragThreshold));
    } else if (prop == "KeyRepeatDelay") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->KeyRepeatDelay));
    } else if (prop == "KeyRepeatRate") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->KeyRepeatRate));
    } else if (prop == "BackendPlatformName") {
      getOutput(outputs, 0) = factory.createCharArray(io->BackendPlatformName ? io->BackendPlatformName : "");
    } else if (prop == "BackendRendererName") {
      getOutput(outputs, 0) = factory.createCharArray(io->BackendRendererName ? io->BackendRendererName : "");
    } else if (prop == "WantCaptureMouse") {
      getOutput(outputs, 0) = createScalarBool(factory, io->WantCaptureMouse);
    } else if (prop == "WantCaptureKeyboard") {
      getOutput(outputs, 0) = createScalarBool(factory, io->WantCaptureKeyboard);
    } else if (prop == "WantTextInput") {
      getOutput(outputs, 0) = createScalarBool(factory, io->WantTextInput);
    } else if (prop == "WantSetMousePos") {
      getOutput(outputs, 0) = createScalarBool(factory, io->WantSetMousePos);
    } else if (prop == "WantSaveIniSettings") {
      getOutput(outputs, 0) = createScalarBool(factory, io->WantSaveIniSettings);
    } else if (prop == "NavActive") {
      getOutput(outputs, 0) = createScalarBool(factory, io->NavActive);
    } else if (prop == "NavVisible") {
      getOutput(outputs, 0) = createScalarBool(factory, io->NavVisible);
    } else if (prop == "Framerate") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->Framerate));
    } else if (prop == "MousePos") {
      getOutput(outputs, 0) = createVec2(factory, static_cast<double>(io->MousePos.x),
                                                 static_cast<double>(io->MousePos.y));
    } else if (prop == "MouseWheel") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->MouseWheel));
    } else if (prop == "MouseWheelH") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->MouseWheelH));
    } else if (prop == "KeyCtrl") {
      getOutput(outputs, 0) = createScalarBool(factory, io->KeyCtrl);
    } else if (prop == "KeyShift") {
      getOutput(outputs, 0) = createScalarBool(factory, io->KeyShift);
    } else if (prop == "KeyAlt") {
      getOutput(outputs, 0) = createScalarBool(factory, io->KeyAlt);
    } else if (prop == "KeySuper") {
      getOutput(outputs, 0) = createScalarBool(factory, io->KeySuper);
    } else if (prop == "KeyMods") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(io->KeyMods));
    } else if (prop == "Fonts" || prop == "FontDefault") {
      getOutput(outputs, 0) = createScalarDouble(factory, 0.0);
    } else {
      throwError(matlabPtr, "Unknown ImGuiIO property: " + prop);
    }
  });

  reg.registerCommand("imgui_io_set", [](ArgumentList& outputs, ArgumentList& inputs,
                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_io_set(handle, prop_name, value)");
    ImGuiIO* io = imguiIOFromHandle(getInput(inputs, 1));
    if (!io) throwError(matlabPtr, "Invalid ImGuiIO handle");
    std::string prop = getString(getInput(inputs, 2));
    const Array& val = getInput(inputs, 3);

    if (prop == "ConfigFlags") {
      io->ConfigFlags = static_cast<ImGuiConfigFlags>(getScalarInt(val));
    } else if (prop == "BackendFlags") {
      io->BackendFlags = static_cast<ImGuiBackendFlags>(getScalarInt(val));
    } else if (prop == "DisplaySize") {
      Eigen::Vector2f v = getVec2(val);
      io->DisplaySize = ImVec2(v[0], v[1]);
    } else if (prop == "DeltaTime") {
      io->DeltaTime = getScalarFloat(val);
    } else if (prop == "IniSavingRate") {
      io->IniSavingRate = getScalarFloat(val);
    } else if (prop == "IniFilename") {
      io->IniFilename = storeIOString(getString(val), 0);
    } else if (prop == "LogFilename") {
      io->LogFilename = storeIOString(getString(val), 1);
    } else if (prop == "FontAllowUserScaling") {
      io->FontAllowUserScaling = getScalarBool(val);
    } else if (prop == "DisplayFramebufferScale") {
      Eigen::Vector2f v = getVec2(val);
      io->DisplayFramebufferScale = ImVec2(v[0], v[1]);
    } else if (prop == "ConfigNavSwapGamepadButtons") {
      io->ConfigNavSwapGamepadButtons = getScalarBool(val);
    } else if (prop == "ConfigNavMoveSetMousePos") {
      io->ConfigNavMoveSetMousePos = getScalarBool(val);
    } else if (prop == "ConfigNavCaptureKeyboard") {
      io->ConfigNavCaptureKeyboard = getScalarBool(val);
    } else if (prop == "ConfigNavEscapeClearFocusItem") {
      io->ConfigNavEscapeClearFocusItem = getScalarBool(val);
    } else if (prop == "ConfigNavEscapeClearFocusWindow") {
      io->ConfigNavEscapeClearFocusWindow = getScalarBool(val);
    } else if (prop == "ConfigNavCursorVisibleAuto") {
      io->ConfigNavCursorVisibleAuto = getScalarBool(val);
    } else if (prop == "ConfigNavCursorVisibleAlways") {
      io->ConfigNavCursorVisibleAlways = getScalarBool(val);
    } else if (prop == "MouseDrawCursor") {
      io->MouseDrawCursor = getScalarBool(val);
    } else if (prop == "ConfigMacOSXBehaviors") {
      io->ConfigMacOSXBehaviors = getScalarBool(val);
    } else if (prop == "MouseDoubleClickTime") {
      io->MouseDoubleClickTime = getScalarFloat(val);
    } else if (prop == "MouseDoubleClickMaxDist") {
      io->MouseDoubleClickMaxDist = getScalarFloat(val);
    } else if (prop == "MouseDragThreshold") {
      io->MouseDragThreshold = getScalarFloat(val);
    } else if (prop == "KeyRepeatDelay") {
      io->KeyRepeatDelay = getScalarFloat(val);
    } else if (prop == "KeyRepeatRate") {
      io->KeyRepeatRate = getScalarFloat(val);
    } else if (prop == "BackendPlatformName") {
      io->BackendPlatformName = storeIOString(getString(val), 2);
    } else if (prop == "BackendRendererName") {
      io->BackendRendererName = storeIOString(getString(val), 3);
    } else if (prop == "MousePos") {
      Eigen::Vector2f v = getVec2(val);
      io->MousePos = ImVec2(v[0], v[1]);
    } else {
      throwError(matlabPtr, "ImGuiIO property is read-only or unknown: " + prop);
    }
  });

  reg.registerCommand("imgui_style_get", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_style_get(handle, prop_name)");
    ImGuiStyle* style = imguiStyleFromHandle(getInput(inputs, 1));
    if (!style) throwError(matlabPtr, "Invalid ImGuiStyle handle");
    std::string prop = getString(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;

    auto vec2out = [&](const ImVec2& v) {
      getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
    };

    if (prop == "Alpha") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->Alpha));
    } else if (prop == "DisabledAlpha") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->DisabledAlpha));
    } else if (prop == "WindowPadding") {
      vec2out(style->WindowPadding);
    } else if (prop == "WindowRounding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->WindowRounding));
    } else if (prop == "WindowBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->WindowBorderSize));
    } else if (prop == "WindowMinSize") {
      vec2out(style->WindowMinSize);
    } else if (prop == "WindowTitleAlign") {
      vec2out(style->WindowTitleAlign);
    } else if (prop == "ChildRounding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ChildRounding));
    } else if (prop == "ChildBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ChildBorderSize));
    } else if (prop == "PopupRounding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->PopupRounding));
    } else if (prop == "PopupBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->PopupBorderSize));
    } else if (prop == "FramePadding") {
      vec2out(style->FramePadding);
    } else if (prop == "FrameRounding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->FrameRounding));
    } else if (prop == "FrameBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->FrameBorderSize));
    } else if (prop == "ItemSpacing") {
      vec2out(style->ItemSpacing);
    } else if (prop == "ItemInnerSpacing") {
      vec2out(style->ItemInnerSpacing);
    } else if (prop == "CellPadding") {
      vec2out(style->CellPadding);
    } else if (prop == "TouchExtraPadding") {
      vec2out(style->TouchExtraPadding);
    } else if (prop == "IndentSpacing") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->IndentSpacing));
    } else if (prop == "ColumnsMinSpacing") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ColumnsMinSpacing));
    } else if (prop == "ScrollbarSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ScrollbarSize));
    } else if (prop == "ScrollbarRounding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ScrollbarRounding));
    } else if (prop == "ScrollbarPadding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ScrollbarPadding));
    } else if (prop == "GrabMinSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->GrabMinSize));
    } else if (prop == "GrabRounding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->GrabRounding));
    } else if (prop == "LogSliderDeadzone") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->LogSliderDeadzone));
    } else if (prop == "ImageBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ImageBorderSize));
    } else if (prop == "TabRounding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TabRounding));
    } else if (prop == "TabBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TabBorderSize));
    } else if (prop == "TabCloseButtonMinWidthSelected") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TabCloseButtonMinWidthSelected));
    } else if (prop == "TabCloseButtonMinWidthUnselected") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TabCloseButtonMinWidthUnselected));
    } else if (prop == "TabBarBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TabBarBorderSize));
    } else if (prop == "TabBarOverlineSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TabBarOverlineSize));
    } else if (prop == "TableAngledHeadersAngle") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TableAngledHeadersAngle));
    } else if (prop == "TableAngledHeadersTextAlign") {
      vec2out(style->TableAngledHeadersTextAlign);
    } else if (prop == "TreeLinesFlags") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TreeLinesFlags));
    } else if (prop == "TreeLinesSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TreeLinesSize));
    } else if (prop == "TreeLinesRounding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->TreeLinesRounding));
    } else if (prop == "DragDropTargetRounding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->DragDropTargetRounding));
    } else if (prop == "DragDropTargetBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->DragDropTargetBorderSize));
    } else if (prop == "DragDropTargetPadding") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->DragDropTargetPadding));
    } else if (prop == "ColorButtonPosition") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ColorButtonPosition));
    } else if (prop == "ButtonTextAlign") {
      vec2out(style->ButtonTextAlign);
    } else if (prop == "SelectableTextAlign") {
      vec2out(style->SelectableTextAlign);
    } else if (prop == "SeparatorTextBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->SeparatorTextBorderSize));
    } else if (prop == "SeparatorTextAlign") {
      vec2out(style->SeparatorTextAlign);
    } else if (prop == "SeparatorTextPadding") {
      vec2out(style->SeparatorTextPadding);
    } else if (prop == "DisplayWindowPadding") {
      vec2out(style->DisplayWindowPadding);
    } else if (prop == "DisplaySafeAreaPadding") {
      vec2out(style->DisplaySafeAreaPadding);
    } else if (prop == "MouseCursorScale") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->MouseCursorScale));
    } else if (prop == "AntiAliasedLines") {
      getOutput(outputs, 0) = createScalarBool(factory, style->AntiAliasedLines);
    } else if (prop == "AntiAliasedLinesUseTex") {
      getOutput(outputs, 0) = createScalarBool(factory, style->AntiAliasedLinesUseTex);
    } else if (prop == "AntiAliasedFill") {
      getOutput(outputs, 0) = createScalarBool(factory, style->AntiAliasedFill);
    } else if (prop == "CurveTessellationTol") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->CurveTessellationTol));
    } else if (prop == "CircleTessellationMaxError") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->CircleTessellationMaxError));
    } else if (prop == "HoverStationaryDelay") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->HoverStationaryDelay));
    } else if (prop == "HoverDelayShort") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->HoverDelayShort));
    } else if (prop == "HoverDelayNormal") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->HoverDelayNormal));
    } else if (prop == "HoverFlagsForTooltipMouse") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->HoverFlagsForTooltipMouse));
    } else if (prop == "HoverFlagsForTooltipNav") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->HoverFlagsForTooltipNav));
    } else {
      throwError(matlabPtr, "Unknown ImGuiStyle property: " + prop);
    }
  });

  reg.registerCommand("imgui_style_set", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_style_set(handle, prop_name, value)");
    ImGuiStyle* style = imguiStyleFromHandle(getInput(inputs, 1));
    if (!style) throwError(matlabPtr, "Invalid ImGuiStyle handle");
    std::string prop = getString(getInput(inputs, 2));
    const Array& val = getInput(inputs, 3);

    auto setVec2 = [&](ImVec2& out) {
      Eigen::Vector2f v = getVec2(val);
      out = ImVec2(v[0], v[1]);
    };

    if (prop == "Alpha") {
      style->Alpha = getScalarFloat(val);
    } else if (prop == "DisabledAlpha") {
      style->DisabledAlpha = getScalarFloat(val);
    } else if (prop == "WindowPadding") {
      setVec2(style->WindowPadding);
    } else if (prop == "WindowRounding") {
      style->WindowRounding = getScalarFloat(val);
    } else if (prop == "WindowBorderSize") {
      style->WindowBorderSize = getScalarFloat(val);
    } else if (prop == "WindowMinSize") {
      setVec2(style->WindowMinSize);
    } else if (prop == "WindowTitleAlign") {
      setVec2(style->WindowTitleAlign);
    } else if (prop == "ChildRounding") {
      style->ChildRounding = getScalarFloat(val);
    } else if (prop == "ChildBorderSize") {
      style->ChildBorderSize = getScalarFloat(val);
    } else if (prop == "PopupRounding") {
      style->PopupRounding = getScalarFloat(val);
    } else if (prop == "PopupBorderSize") {
      style->PopupBorderSize = getScalarFloat(val);
    } else if (prop == "FramePadding") {
      setVec2(style->FramePadding);
    } else if (prop == "FrameRounding") {
      style->FrameRounding = getScalarFloat(val);
    } else if (prop == "FrameBorderSize") {
      style->FrameBorderSize = getScalarFloat(val);
    } else if (prop == "ItemSpacing") {
      setVec2(style->ItemSpacing);
    } else if (prop == "ItemInnerSpacing") {
      setVec2(style->ItemInnerSpacing);
    } else if (prop == "CellPadding") {
      setVec2(style->CellPadding);
    } else if (prop == "TouchExtraPadding") {
      setVec2(style->TouchExtraPadding);
    } else if (prop == "IndentSpacing") {
      style->IndentSpacing = getScalarFloat(val);
    } else if (prop == "ColumnsMinSpacing") {
      style->ColumnsMinSpacing = getScalarFloat(val);
    } else if (prop == "ScrollbarSize") {
      style->ScrollbarSize = getScalarFloat(val);
    } else if (prop == "ScrollbarRounding") {
      style->ScrollbarRounding = getScalarFloat(val);
    } else if (prop == "ScrollbarPadding") {
      style->ScrollbarPadding = getScalarFloat(val);
    } else if (prop == "GrabMinSize") {
      style->GrabMinSize = getScalarFloat(val);
    } else if (prop == "GrabRounding") {
      style->GrabRounding = getScalarFloat(val);
    } else if (prop == "LogSliderDeadzone") {
      style->LogSliderDeadzone = getScalarFloat(val);
    } else if (prop == "ImageBorderSize") {
      style->ImageBorderSize = getScalarFloat(val);
    } else if (prop == "TabRounding") {
      style->TabRounding = getScalarFloat(val);
    } else if (prop == "TabBorderSize") {
      style->TabBorderSize = getScalarFloat(val);
    } else if (prop == "TabCloseButtonMinWidthSelected") {
      style->TabCloseButtonMinWidthSelected = getScalarFloat(val);
    } else if (prop == "TabCloseButtonMinWidthUnselected") {
      style->TabCloseButtonMinWidthUnselected = getScalarFloat(val);
    } else if (prop == "TabBarBorderSize") {
      style->TabBarBorderSize = getScalarFloat(val);
    } else if (prop == "TabBarOverlineSize") {
      style->TabBarOverlineSize = getScalarFloat(val);
    } else if (prop == "TableAngledHeadersAngle") {
      style->TableAngledHeadersAngle = getScalarFloat(val);
    } else if (prop == "TableAngledHeadersTextAlign") {
      setVec2(style->TableAngledHeadersTextAlign);
    } else if (prop == "TreeLinesFlags") {
      style->TreeLinesFlags = static_cast<ImGuiTreeNodeFlags>(getScalarInt(val));
    } else if (prop == "TreeLinesSize") {
      style->TreeLinesSize = getScalarFloat(val);
    } else if (prop == "TreeLinesRounding") {
      style->TreeLinesRounding = getScalarFloat(val);
    } else if (prop == "DragDropTargetRounding") {
      style->DragDropTargetRounding = getScalarFloat(val);
    } else if (prop == "DragDropTargetBorderSize") {
      style->DragDropTargetBorderSize = getScalarFloat(val);
    } else if (prop == "DragDropTargetPadding") {
      style->DragDropTargetPadding = getScalarFloat(val);
    } else if (prop == "ColorButtonPosition") {
      style->ColorButtonPosition = static_cast<ImGuiDir>(getScalarInt(val));
    } else if (prop == "ButtonTextAlign") {
      setVec2(style->ButtonTextAlign);
    } else if (prop == "SelectableTextAlign") {
      setVec2(style->SelectableTextAlign);
    } else if (prop == "SeparatorTextBorderSize") {
      style->SeparatorTextBorderSize = getScalarFloat(val);
    } else if (prop == "SeparatorTextAlign") {
      setVec2(style->SeparatorTextAlign);
    } else if (prop == "SeparatorTextPadding") {
      setVec2(style->SeparatorTextPadding);
    } else if (prop == "DisplayWindowPadding") {
      setVec2(style->DisplayWindowPadding);
    } else if (prop == "DisplaySafeAreaPadding") {
      setVec2(style->DisplaySafeAreaPadding);
    } else if (prop == "MouseCursorScale") {
      style->MouseCursorScale = getScalarFloat(val);
    } else if (prop == "AntiAliasedLines") {
      style->AntiAliasedLines = getScalarBool(val);
    } else if (prop == "AntiAliasedLinesUseTex") {
      style->AntiAliasedLinesUseTex = getScalarBool(val);
    } else if (prop == "AntiAliasedFill") {
      style->AntiAliasedFill = getScalarBool(val);
    } else if (prop == "CurveTessellationTol") {
      style->CurveTessellationTol = getScalarFloat(val);
    } else if (prop == "CircleTessellationMaxError") {
      style->CircleTessellationMaxError = getScalarFloat(val);
    } else if (prop == "HoverStationaryDelay") {
      style->HoverStationaryDelay = getScalarFloat(val);
    } else if (prop == "HoverDelayShort") {
      style->HoverDelayShort = getScalarFloat(val);
    } else if (prop == "HoverDelayNormal") {
      style->HoverDelayNormal = getScalarFloat(val);
    } else if (prop == "HoverFlagsForTooltipMouse") {
      style->HoverFlagsForTooltipMouse = static_cast<ImGuiHoveredFlags>(getScalarInt(val));
    } else if (prop == "HoverFlagsForTooltipNav") {
      style->HoverFlagsForTooltipNav = static_cast<ImGuiHoveredFlags>(getScalarInt(val));
    } else {
      throwError(matlabPtr, "Unknown ImGuiStyle property: " + prop);
    }
  });

  reg.registerCommand("imgui_style_scale_all_sizes", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_style_scale_all_sizes(handle, scale_factor)");
    ImGuiStyle* style = imguiStyleFromHandle(getInput(inputs, 1));
    if (!style) throwError(matlabPtr, "Invalid ImGuiStyle handle");
    style->ScaleAllSizes(getScalarFloat(getInput(inputs, 2)));
  });

  reg.registerCommand("imgui_drawlist_add_line", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_drawlist_add_line(handle, p1, p2, col, thickness)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 p1 = toImVec2(getVec2(getInput(inputs, 2)));
    ImVec2 p2 = toImVec2(getVec2(getInput(inputs, 3)));
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 4)));
    float thickness = inputCount(inputs) > 5 ? getScalarFloat(getInput(inputs, 5)) : 1.0f;
    dl->AddLine(p1, p2, col, thickness);
  });

  reg.registerCommand("imgui_drawlist_add_rect", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_drawlist_add_rect(handle, p_min, p_max, col, rounding, flags, thickness)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 p_min = toImVec2(getVec2(getInput(inputs, 2)));
    ImVec2 p_max = toImVec2(getVec2(getInput(inputs, 3)));
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 4)));
    float rounding = inputCount(inputs) > 5 ? getScalarFloat(getInput(inputs, 5)) : 0.0f;
    ImDrawFlags flags = inputCount(inputs) > 6 ? static_cast<ImDrawFlags>(getScalarInt(getInput(inputs, 6))) : 0;
    float thickness = inputCount(inputs) > 7 ? getScalarFloat(getInput(inputs, 7)) : 1.0f;
    dl->AddRect(p_min, p_max, col, rounding, flags, thickness);
  });

  reg.registerCommand("imgui_drawlist_add_rect_filled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_drawlist_add_rect_filled(handle, p_min, p_max, col, rounding, flags)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 p_min = toImVec2(getVec2(getInput(inputs, 2)));
    ImVec2 p_max = toImVec2(getVec2(getInput(inputs, 3)));
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 4)));
    float rounding = inputCount(inputs) > 5 ? getScalarFloat(getInput(inputs, 5)) : 0.0f;
    ImDrawFlags flags = inputCount(inputs) > 6 ? static_cast<ImDrawFlags>(getScalarInt(getInput(inputs, 6))) : 0;
    dl->AddRectFilled(p_min, p_max, col, rounding, flags);
  });

  reg.registerCommand("imgui_drawlist_add_circle", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_drawlist_add_circle(handle, center, radius, col, num_segments, thickness)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 center = toImVec2(getVec2(getInput(inputs, 2)));
    float radius = getScalarFloat(getInput(inputs, 3));
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 4)));
    int num_segments = inputCount(inputs) > 5 ? getScalarInt(getInput(inputs, 5)) : 0;
    float thickness = inputCount(inputs) > 6 ? getScalarFloat(getInput(inputs, 6)) : 1.0f;
    dl->AddCircle(center, radius, col, num_segments, thickness);
  });

  reg.registerCommand("imgui_drawlist_add_circle_filled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_drawlist_add_circle_filled(handle, center, radius, col, num_segments)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 center = toImVec2(getVec2(getInput(inputs, 2)));
    float radius = getScalarFloat(getInput(inputs, 3));
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 4)));
    int num_segments = inputCount(inputs) > 5 ? getScalarInt(getInput(inputs, 5)) : 0;
    dl->AddCircleFilled(center, radius, col, num_segments);
  });

  reg.registerCommand("imgui_drawlist_add_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_drawlist_add_text(handle, pos, col, text)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 pos = toImVec2(getVec2(getInput(inputs, 2)));
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 3)));
    std::string text = getString(getInput(inputs, 4));
    dl->AddText(pos, col, text.c_str(), text.c_str() + text.size());
  });

  reg.registerCommand("imgui_drawlist_add_image", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected imgui_drawlist_add_image(handle, texture, p_min, p_max, ...)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImTextureID texture = static_cast<ImTextureID>(static_cast<uintptr_t>(getScalarDouble(getInput(inputs, 2))));
    ImVec2 pMin = toImVec2(getVec2(getInput(inputs, 3)));
    ImVec2 pMax = toImVec2(getVec2(getInput(inputs, 4)));
    ImVec2 uv0 = inputCount(inputs) > 5 ? toImVec2(getVec2(getInput(inputs, 5))) : ImVec2(0, 0);
    ImVec2 uv1 = inputCount(inputs) > 6 ? toImVec2(getVec2(getInput(inputs, 6))) : ImVec2(1, 1);
    ImU32 tint = inputCount(inputs) > 7 ? static_cast<ImU32>(getScalarDouble(getInput(inputs, 7))) : IM_COL32_WHITE;
    dl->AddImage(texture, pMin, pMax, uv0, uv1, tint);
  });

  reg.registerCommand("imgui_drawlist_add_triangle", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 6) throwError(matlabPtr, "Expected imgui_drawlist_add_triangle(handle, p1, p2, p3, col, thickness)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 p1 = toImVec2(getVec2(getInput(inputs, 2)));
    ImVec2 p2 = toImVec2(getVec2(getInput(inputs, 3)));
    ImVec2 p3 = toImVec2(getVec2(getInput(inputs, 4)));
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 5)));
    float thickness = inputCount(inputs) > 6 ? getScalarFloat(getInput(inputs, 6)) : 1.0f;
    dl->AddTriangle(p1, p2, p3, col, thickness);
  });

  reg.registerCommand("imgui_drawlist_add_triangle_filled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 6) throwError(matlabPtr, "Expected imgui_drawlist_add_triangle_filled(handle, p1, p2, p3, col)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 p1 = toImVec2(getVec2(getInput(inputs, 2)));
    ImVec2 p2 = toImVec2(getVec2(getInput(inputs, 3)));
    ImVec2 p3 = toImVec2(getVec2(getInput(inputs, 4)));
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 5)));
    dl->AddTriangleFilled(p1, p2, p3, col);
  });

  reg.registerCommand("imgui_drawlist_path_clear", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_drawlist_path_clear(handle)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    dl->PathClear();
  });

  reg.registerCommand("imgui_drawlist_path_line_to", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drawlist_path_line_to(handle, pos)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    dl->PathLineTo(toImVec2(getVec2(getInput(inputs, 2))));
  });

  reg.registerCommand("imgui_drawlist_path_stroke", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_drawlist_path_stroke(handle, col, flags, thickness)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 2)));
    ImDrawFlags flags = inputCount(inputs) > 3 ? static_cast<ImDrawFlags>(getScalarInt(getInput(inputs, 3))) : 0;
    float thickness = inputCount(inputs) > 4 ? getScalarFloat(getInput(inputs, 4)) : 1.0f;
    dl->PathStroke(col, flags, thickness);
  });

  reg.registerCommand("imgui_drawlist_path_fill_convex", [](ArgumentList& outputs, ArgumentList& inputs,
                                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected imgui_drawlist_path_fill_convex(handle, col)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImU32 col = static_cast<ImU32>(getScalarDouble(getInput(inputs, 2)));
    dl->PathFillConvex(col);
  });

  reg.registerCommand("imgui_drawlist_push_clip_rect", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected imgui_drawlist_push_clip_rect(handle, clip_rect_min, clip_rect_max, intersect_current)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 clip_min = toImVec2(getVec2(getInput(inputs, 2)));
    ImVec2 clip_max = toImVec2(getVec2(getInput(inputs, 3)));
    bool intersect = inputCount(inputs) > 4 ? getScalarBool(getInput(inputs, 4)) : false;
    dl->PushClipRect(clip_min, clip_max, intersect);
  });

  reg.registerCommand("imgui_drawlist_pop_clip_rect", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_drawlist_pop_clip_rect(handle)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    dl->PopClipRect();
  });

  reg.registerCommand("imgui_drawlist_get_clip_rect_min", [](ArgumentList& outputs, ArgumentList& inputs,
                                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_drawlist_get_clip_rect_min(handle)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 v = dl->GetClipRectMin();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  reg.registerCommand("imgui_drawlist_get_clip_rect_max", [](ArgumentList& outputs, ArgumentList& inputs,
                                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected imgui_drawlist_get_clip_rect_max(handle)");
    ImDrawList* dl = imguiDrawListFromHandle(getInput(inputs, 1));
    if (!dl) throwError(matlabPtr, "Invalid ImDrawList handle");
    ImVec2 v = dl->GetClipRectMax();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
  });

  bind_imgui_constant_lookup(reg);
}

} // namespace ps_mex
