#include "bind_implot.h"
#include "bind_implot_constant_lookup.h"
#include "matlab_data_utils.h"

#include "polyscope/polyscope.h"
#include "implot.h"

#include <cstdint>

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

ImVec2 toImVec2(const Eigen::Vector2f& v) { return ImVec2(v[0], v[1]); }
ImVec4 toImVec4(const Eigen::Vector4f& v) { return ImVec4(v[0], v[1], v[2], v[3]); }

ImPlotStyle* implotStyleFromHandle(const Array& arr) {
  return reinterpret_cast<ImPlotStyle*>(static_cast<uintptr_t>(getScalarDouble(arr)));
}
ImPlotInputMap* implotInputMapFromHandle(const Array& arr) {
  return reinterpret_cast<ImPlotInputMap*>(static_cast<uintptr_t>(getScalarDouble(arr)));
}

std::vector<double> getVectorDoubleChecked(const Array& arr) {
  auto v = getVectorDouble(arr);
  if (v.empty()) throw std::runtime_error("Expected non-empty numeric vector");
  return v;
}

} // namespace

void bind_implot_commands(CommandRegistry& reg) {
  reg.registerCommand("implot_begin_plot", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_begin_plot(title_id, ...)");
    std::string title = getString(getInput(inputs, 1));
    ImVec2 size(-1, 0);
    if (inputCount(inputs) > 2) size = toImVec2(getVec2(getInput(inputs, 2)));
    ImPlotFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    bool ok = ImPlot::BeginPlot(title.c_str(), size, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ok);
  });

  reg.registerCommand("implot_end_plot", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    ImPlot::EndPlot();
  });

  reg.registerCommand("implot_setup_axes", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_setup_axes(x_label, y_label, ...)");
    std::string x_label = getString(getInput(inputs, 1));
    std::string y_label = getString(getInput(inputs, 2));
    ImPlotAxisFlags x_flags = 0;
    ImPlotAxisFlags y_flags = 0;
    if (inputCount(inputs) > 3) x_flags = getScalarInt(getInput(inputs, 3));
    if (inputCount(inputs) > 4) y_flags = getScalarInt(getInput(inputs, 4));
    ImPlot::SetupAxes(x_label.c_str(), y_label.c_str(), x_flags, y_flags);
  });

  reg.registerCommand("implot_setup_axes_limits", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5)
      throwError(matlabPtr, "Expected implot_setup_axes_limits(x_min, x_max, y_min, y_max, ...)");
    double x_min = getScalarDouble(getInput(inputs, 1));
    double x_max = getScalarDouble(getInput(inputs, 2));
    double y_min = getScalarDouble(getInput(inputs, 3));
    double y_max = getScalarDouble(getInput(inputs, 4));
    ImPlotCond cond = ImPlotCond_Once;
    if (inputCount(inputs) > 5) cond = static_cast<ImPlotCond>(getScalarInt(getInput(inputs, 5)));
    ImPlot::SetupAxesLimits(x_min, x_max, y_min, y_max, cond);
  });

  reg.registerCommand("implot_setup_axis", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_setup_axis(axis, label, ...)");
    ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    std::string label = getString(getInput(inputs, 2));
    ImPlotAxisFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    ImPlot::SetupAxis(axis, label.c_str(), flags);
  });

  reg.registerCommand("implot_setup_axis_limits", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_setup_axis_limits(axis, vmin, vmax, ...)");
    ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    double vmin = getScalarDouble(getInput(inputs, 2));
    double vmax = getScalarDouble(getInput(inputs, 3));
    ImPlotCond cond = ImPlotCond_Once;
    if (inputCount(inputs) > 4) cond = static_cast<ImPlotCond>(getScalarInt(getInput(inputs, 4)));
    ImPlot::SetupAxisLimits(axis, vmin, vmax, cond);
  });

  reg.registerCommand("implot_plot_line", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_line(label_id, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    double xscale = 1.0;
    double xstart = 0.0;
    ImPlotLineFlags flags = 0;
    if (inputCount(inputs) > 3) xscale = getScalarDouble(getInput(inputs, 3));
    if (inputCount(inputs) > 4) xstart = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotLine(label.c_str(), values.data(), static_cast<int>(values.size()), xscale, xstart, flags);
  });

  reg.registerCommand("implot_plot_line_xy", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_plot_line_xy(label_id, xs, ys, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    if (xs.size() != ys.size()) throwError(matlabPtr, "xs and ys must have the same size");
    ImPlotLineFlags flags = 0;
    if (inputCount(inputs) > 4) flags = getScalarInt(getInput(inputs, 4));
    ImPlot::PlotLine(label.c_str(), xs.data(), ys.data(), static_cast<int>(xs.size()), flags);
  });

  reg.registerCommand("implot_plot_inf_lines", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_inf_lines(label_id, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    ImPlotInfLinesFlags flags = 0;
    if (inputCount(inputs) > 3) flags = getScalarInt(getInput(inputs, 3));
    ImPlot::PlotInfLines(label.c_str(), values.data(), static_cast<int>(values.size()), flags);
  });

  reg.registerCommand("implot_plot_scatter", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_scatter(label_id, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    double xscale = 1.0;
    double xstart = 0.0;
    ImPlotScatterFlags flags = 0;
    if (inputCount(inputs) > 3) xscale = getScalarDouble(getInput(inputs, 3));
    if (inputCount(inputs) > 4) xstart = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotScatter(label.c_str(), values.data(), static_cast<int>(values.size()), xscale, xstart, flags);
  });

  reg.registerCommand("implot_plot_scatter_xy", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_plot_scatter_xy(label_id, xs, ys, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    if (xs.size() != ys.size()) throwError(matlabPtr, "xs and ys must have the same size");
    ImPlotScatterFlags flags = 0;
    if (inputCount(inputs) > 4) flags = getScalarInt(getInput(inputs, 4));
    ImPlot::PlotScatter(label.c_str(), xs.data(), ys.data(), static_cast<int>(xs.size()), flags);
  });

  reg.registerCommand("implot_plot_bars", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_bars(label_id, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    double bar_size = 0.67;
    double shift = 0.0;
    ImPlotBarsFlags flags = 0;
    if (inputCount(inputs) > 3) bar_size = getScalarDouble(getInput(inputs, 3));
    if (inputCount(inputs) > 4) shift = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotBars(label.c_str(), values.data(), static_cast<int>(values.size()), bar_size, shift, flags);
  });

  reg.registerCommand("implot_plot_bars_xy", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected implot_plot_bars_xy(label_id, xs, ys, bar_size, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    if (xs.size() != ys.size()) throwError(matlabPtr, "xs and ys must have the same size");
    double bar_size = getScalarDouble(getInput(inputs, 4));
    ImPlotBarsFlags flags = 0;
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotBars(label.c_str(), xs.data(), ys.data(), static_cast<int>(xs.size()), bar_size, flags);
  });

  reg.registerCommand("implot_plot_heatmap", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected implot_plot_heatmap(label_id, values, rows, cols, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    int rows = getScalarInt(getInput(inputs, 3));
    int cols = getScalarInt(getInput(inputs, 4));
    if (rows * cols != static_cast<int>(values.size()))
      throwError(matlabPtr, "values size must equal rows*cols");
    double scale_min = 0.0;
    double scale_max = 0.0;
    if (inputCount(inputs) > 5) scale_min = getScalarDouble(getInput(inputs, 5));
    if (inputCount(inputs) > 6) scale_max = getScalarDouble(getInput(inputs, 6));
    const char* label_fmt = "%.1f";
    if (inputCount(inputs) > 7) {
      std::string f = getString(getInput(inputs, 7));
      static thread_local std::string fmt_storage;
      fmt_storage = f;
      label_fmt = fmt_storage.c_str();
    }
    ImPlotPoint bounds_min(0, 0);
    ImPlotPoint bounds_max(1, 1);
    if (inputCount(inputs) > 8) {
      auto bmin = getVec2d(getInput(inputs, 8));
      bounds_min = ImPlotPoint(bmin[0], bmin[1]);
    }
    if (inputCount(inputs) > 9) {
      auto bmax = getVec2d(getInput(inputs, 9));
      bounds_max = ImPlotPoint(bmax[0], bmax[1]);
    }
    ImPlotHeatmapFlags flags = 0;
    if (inputCount(inputs) > 10) flags = getScalarInt(getInput(inputs, 10));
    ImPlot::PlotHeatmap(label.c_str(), values.data(), rows, cols, scale_min, scale_max, label_fmt, bounds_min,
                        bounds_max, flags);
  });

  reg.registerCommand("implot_set_next_line_style", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    ImVec4 col = IMPLOT_AUTO_COL;
    float weight = IMPLOT_AUTO;
    if (inputCount(inputs) > 1) col = toImVec4(getVec4(getInput(inputs, 1)));
    if (inputCount(inputs) > 2) weight = getScalarFloat(getInput(inputs, 2));
    ImPlot::SetNextLineStyle(col, weight);
  });

  reg.registerCommand("implot_set_next_marker_style", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    ImPlotMarker marker = IMPLOT_AUTO;
    float size = IMPLOT_AUTO;
    ImVec4 fill = IMPLOT_AUTO_COL;
    float weight = IMPLOT_AUTO;
    ImVec4 outline = IMPLOT_AUTO_COL;
    if (inputCount(inputs) > 1) marker = static_cast<ImPlotMarker>(getScalarInt(getInput(inputs, 1)));
    if (inputCount(inputs) > 2) size = getScalarFloat(getInput(inputs, 2));
    if (inputCount(inputs) > 3) fill = toImVec4(getVec4(getInput(inputs, 3)));
    if (inputCount(inputs) > 4) weight = getScalarFloat(getInput(inputs, 4));
    if (inputCount(inputs) > 5) outline = toImVec4(getVec4(getInput(inputs, 5)));
    ImPlot::SetNextMarkerStyle(marker, size, fill, weight, outline);
  });

  // === Subplots ============================================================
  reg.registerCommand("implot_begin_subplots", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected implot_begin_subplots(title_id, rows, cols, size, ...)");
    std::string title = getString(getInput(inputs, 1));
    int rows = getScalarInt(getInput(inputs, 2));
    int cols = getScalarInt(getInput(inputs, 3));
    ImVec2 size = toImVec2(getVec2(getInput(inputs, 4)));
    ImPlotSubplotFlags flags = 0;
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    bool ok = ImPlot::BeginSubplots(title.c_str(), rows, cols, size, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ok);
  });

  reg.registerCommand("implot_end_subplots", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    ImPlot::EndSubplots();
  });

  // === Setup ===============================================================
  reg.registerCommand("implot_setup_axis_format", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_setup_axis_format(axis, fmt)");
    ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    std::string fmt = getString(getInput(inputs, 2));
    ImPlot::SetupAxisFormat(axis, fmt.c_str());
  });

  reg.registerCommand("implot_setup_axis_scale", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_setup_axis_scale(axis, scale)");
    ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    ImPlotScale scale = static_cast<ImPlotScale>(getScalarInt(getInput(inputs, 2)));
    ImPlot::SetupAxisScale(axis, scale);
  });

  reg.registerCommand("implot_setup_legend", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_setup_legend(location, ...)");
    ImPlotLocation location = static_cast<ImPlotLocation>(getScalarInt(getInput(inputs, 1)));
    ImPlotLegendFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    ImPlot::SetupLegend(location, flags);
  });

  reg.registerCommand("implot_setup_mouse_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_setup_mouse_text(location, ...)");
    ImPlotLocation location = static_cast<ImPlotLocation>(getScalarInt(getInput(inputs, 1)));
    ImPlotMouseTextFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    ImPlot::SetupMouseText(location, flags);
  });

  reg.registerCommand("implot_setup_finish", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    ImPlot::SetupFinish();
  });

  // === Plot items ==========================================================
  reg.registerCommand("implot_plot_stairs", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_stairs(label_id, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    double xscale = 1.0;
    double xstart = 0.0;
    ImPlotStairsFlags flags = 0;
    if (inputCount(inputs) > 3) xscale = getScalarDouble(getInput(inputs, 3));
    if (inputCount(inputs) > 4) xstart = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotStairs(label.c_str(), values.data(), static_cast<int>(values.size()), xscale, xstart, flags);
  });

  reg.registerCommand("implot_plot_stairs_xy", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_plot_stairs_xy(label_id, xs, ys, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    if (xs.size() != ys.size()) throwError(matlabPtr, "xs and ys must have the same size");
    ImPlotStairsFlags flags = 0;
    if (inputCount(inputs) > 4) flags = getScalarInt(getInput(inputs, 4));
    ImPlot::PlotStairs(label.c_str(), xs.data(), ys.data(), static_cast<int>(xs.size()), flags);
  });

  reg.registerCommand("implot_plot_shaded", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_shaded(label_id, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    double yref = 0.0;
    double xscale = 1.0;
    double xstart = 0.0;
    ImPlotShadedFlags flags = 0;
    if (inputCount(inputs) > 3) yref = getScalarDouble(getInput(inputs, 3));
    if (inputCount(inputs) > 4) xscale = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) xstart = getScalarDouble(getInput(inputs, 5));
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    ImPlot::PlotShaded(label.c_str(), values.data(), static_cast<int>(values.size()), yref, xscale, xstart, flags);
  });

  reg.registerCommand("implot_plot_shaded_xy", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_plot_shaded_xy(label_id, xs, ys, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    if (xs.size() != ys.size()) throwError(matlabPtr, "xs and ys must have the same size");
    double yref = 0.0;
    ImPlotShadedFlags flags = 0;
    if (inputCount(inputs) > 4) yref = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotShaded(label.c_str(), xs.data(), ys.data(), static_cast<int>(xs.size()), yref, flags);
  });

  reg.registerCommand("implot_plot_shaded_between", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5)
      throwError(matlabPtr, "Expected implot_plot_shaded_between(label_id, xs, ys1, ys2, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys1 = getVectorDoubleChecked(getInput(inputs, 3));
    auto ys2 = getVectorDoubleChecked(getInput(inputs, 4));
    if (xs.size() != ys1.size() || xs.size() != ys2.size())
      throwError(matlabPtr, "xs, ys1, and ys2 must have the same size");
    ImPlotShadedFlags flags = 0;
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotShaded(label.c_str(), xs.data(), ys1.data(), ys2.data(), static_cast<int>(xs.size()), flags);
  });

  reg.registerCommand("implot_plot_bar_groups", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_bar_groups(label_ids, values, ...)");
    auto labels = getStringVector(getInput(inputs, 1));
    std::vector<const char*> label_ptrs;
    label_ptrs.reserve(labels.size());
    for (auto& s : labels) label_ptrs.push_back(s.c_str());
    auto values = getMatrixDouble(getInput(inputs, 2));
    int item_count = static_cast<int>(values.rows());
    int group_count = static_cast<int>(values.cols());
    if (static_cast<int>(labels.size()) != item_count)
      throwError(matlabPtr, "label_ids count must match values row count");
    double group_size = 0.67;
    double shift = 0.0;
    ImPlotBarGroupsFlags flags = 0;
    if (inputCount(inputs) > 3) group_size = getScalarDouble(getInput(inputs, 3));
    if (inputCount(inputs) > 4) shift = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    Eigen::MatrixXd valuesT = values.transpose();
    ImPlot::PlotBarGroups(label_ptrs.data(), valuesT.data(), item_count, group_count, group_size, shift, flags);
  });

  reg.registerCommand("implot_plot_error_bars", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected implot_plot_error_bars(label_id, xs, ys, err, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    auto err = getVectorDoubleChecked(getInput(inputs, 4));
    if (xs.size() != ys.size() || xs.size() != err.size()) throwError(matlabPtr, "xs, ys, and err must have the same size");
    ImPlotErrorBarsFlags flags = 0;
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotErrorBars(label.c_str(), xs.data(), ys.data(), err.data(), static_cast<int>(xs.size()), flags);
  });

  reg.registerCommand("implot_plot_error_bars_asymmetric", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 6)
      throwError(matlabPtr, "Expected implot_plot_error_bars_asymmetric(label_id, xs, ys, neg, pos, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    auto neg = getVectorDoubleChecked(getInput(inputs, 4));
    auto pos = getVectorDoubleChecked(getInput(inputs, 5));
    if (xs.size() != ys.size() || xs.size() != neg.size() || xs.size() != pos.size())
      throwError(matlabPtr, "xs, ys, neg, and pos must have the same size");
    ImPlotErrorBarsFlags flags = 0;
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    ImPlot::PlotErrorBars(label.c_str(), xs.data(), ys.data(), neg.data(), pos.data(), static_cast<int>(xs.size()),
                          flags);
  });

  reg.registerCommand("implot_plot_stems", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_stems(label_id, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    double ref = 0.0;
    double scale = 1.0;
    double start = 0.0;
    ImPlotStemsFlags flags = 0;
    if (inputCount(inputs) > 3) ref = getScalarDouble(getInput(inputs, 3));
    if (inputCount(inputs) > 4) scale = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) start = getScalarDouble(getInput(inputs, 5));
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    ImPlot::PlotStems(label.c_str(), values.data(), static_cast<int>(values.size()), ref, scale, start, flags);
  });

  reg.registerCommand("implot_plot_stems_xy", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_plot_stems_xy(label_id, xs, ys, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    if (xs.size() != ys.size()) throwError(matlabPtr, "xs and ys must have the same size");
    double ref = 0.0;
    ImPlotStemsFlags flags = 0;
    if (inputCount(inputs) > 4) ref = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotStems(label.c_str(), xs.data(), ys.data(), static_cast<int>(xs.size()), ref, flags);
  });

  reg.registerCommand("implot_plot_histogram", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_histogram(label_id, values, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    int bins = IMPLOT_AUTO;
    double bar_scale = 1.0;
    ImPlotRange range;
    ImPlotHistogramFlags flags = 0;
    if (inputCount(inputs) > 3) bins = getScalarInt(getInput(inputs, 3));
    if (inputCount(inputs) > 4) bar_scale = getScalarDouble(getInput(inputs, 4));
    if (inputCount(inputs) > 5) range.Min = getScalarDouble(getInput(inputs, 5));
    if (inputCount(inputs) > 6) range.Max = getScalarDouble(getInput(inputs, 6));
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    double result = ImPlot::PlotHistogram(label.c_str(), values.data(), static_cast<int>(values.size()), bins,
                                          bar_scale, range, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, result);
  });

  reg.registerCommand("implot_plot_histogram2d", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_plot_histogram2d(label_id, xs, ys, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    if (xs.size() != ys.size()) throwError(matlabPtr, "xs and ys must have the same size");
    int x_bins = IMPLOT_AUTO;
    int y_bins = IMPLOT_AUTO;
    ImPlotRect range;
    ImPlotHistogramFlags flags = 0;
    if (inputCount(inputs) > 4) x_bins = getScalarInt(getInput(inputs, 4));
    if (inputCount(inputs) > 5) y_bins = getScalarInt(getInput(inputs, 5));
    if (inputCount(inputs) > 6) {
      auto r = getVec4d(getInput(inputs, 6));
      range = ImPlotRect(r[0], r[1], r[2], r[3]);
    }
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    double result =
        ImPlot::PlotHistogram2D(label.c_str(), xs.data(), ys.data(), static_cast<int>(xs.size()), x_bins, y_bins,
                                range, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, result);
  });

  reg.registerCommand("implot_plot_digital", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_plot_digital(label_id, xs, ys, ...)");
    std::string label = getString(getInput(inputs, 1));
    auto xs = getVectorDoubleChecked(getInput(inputs, 2));
    auto ys = getVectorDoubleChecked(getInput(inputs, 3));
    if (xs.size() != ys.size()) throwError(matlabPtr, "xs and ys must have the same size");
    ImPlotDigitalFlags flags = 0;
    if (inputCount(inputs) > 4) flags = getScalarInt(getInput(inputs, 4));
    ImPlot::PlotDigital(label.c_str(), xs.data(), ys.data(), static_cast<int>(xs.size()), flags);
  });

  // === Plot tools ==========================================================
  reg.registerCommand("implot_drag_point", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected implot_drag_point(id, x, y, col, ...)");
    int id = getScalarInt(getInput(inputs, 1));
    double x = getScalarDouble(getInput(inputs, 2));
    double y = getScalarDouble(getInput(inputs, 3));
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 4)));
    float size = 4.0f;
    ImPlotDragToolFlags flags = 0;
    if (inputCount(inputs) > 5) size = getScalarFloat(getInput(inputs, 5));
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    bool changed = ImPlot::DragPoint(id, &x, &y, col, size, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, x);
    getOutput(outputs, 2) = createScalarDouble(factory, y);
  });

  reg.registerCommand("implot_drag_line_x", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_drag_line_x(id, x, col, ...)");
    int id = getScalarInt(getInput(inputs, 1));
    double x = getScalarDouble(getInput(inputs, 2));
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 3)));
    float thickness = 1.0f;
    ImPlotDragToolFlags flags = 0;
    if (inputCount(inputs) > 4) thickness = getScalarFloat(getInput(inputs, 4));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    bool changed = ImPlot::DragLineX(id, &x, col, thickness, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, x);
  });

  reg.registerCommand("implot_drag_line_y", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_drag_line_y(id, y, col, ...)");
    int id = getScalarInt(getInput(inputs, 1));
    double y = getScalarDouble(getInput(inputs, 2));
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 3)));
    float thickness = 1.0f;
    ImPlotDragToolFlags flags = 0;
    if (inputCount(inputs) > 4) thickness = getScalarFloat(getInput(inputs, 4));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    bool changed = ImPlot::DragLineY(id, &y, col, thickness, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, y);
  });

  reg.registerCommand("implot_drag_rect", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 7) throwError(matlabPtr, "Expected implot_drag_rect(id, x1, y1, x2, y2, col, ...)");
    int id = getScalarInt(getInput(inputs, 1));
    double x1 = getScalarDouble(getInput(inputs, 2));
    double y1 = getScalarDouble(getInput(inputs, 3));
    double x2 = getScalarDouble(getInput(inputs, 4));
    double y2 = getScalarDouble(getInput(inputs, 5));
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 6)));
    ImPlotDragToolFlags flags = 0;
    if (inputCount(inputs) > 7) flags = getScalarInt(getInput(inputs, 7));
    bool changed = ImPlot::DragRect(id, &x1, &y1, &x2, &y2, col, flags);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, x1);
    getOutput(outputs, 2) = createScalarDouble(factory, y1);
    getOutput(outputs, 3) = createScalarDouble(factory, x2);
    getOutput(outputs, 4) = createScalarDouble(factory, y2);
  });

  reg.registerCommand("implot_annotation", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 6)
      throwError(matlabPtr, "Expected implot_annotation(x, y, col, pix_offset, clamp, text)");
    double x = getScalarDouble(getInput(inputs, 1));
    double y = getScalarDouble(getInput(inputs, 2));
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 3)));
    ImVec2 pix_offset = toImVec2(getVec2(getInput(inputs, 4)));
    bool clamp = getScalarBool(getInput(inputs, 5));
    std::string text = getString(getInput(inputs, 6));
    ImPlot::Annotation(x, y, col, pix_offset, clamp, "%s", text.c_str());
  });

  reg.registerCommand("implot_tag_x", [](ArgumentList& outputs, ArgumentList& inputs,
                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_tag_x(x, col, text)");
    double x = getScalarDouble(getInput(inputs, 1));
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 2)));
    std::string text = getString(getInput(inputs, 3));
    ImPlot::TagX(x, col, "%s", text.c_str());
  });

  reg.registerCommand("implot_tag_y", [](ArgumentList& outputs, ArgumentList& inputs,
                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_tag_y(y, col, text)");
    double y = getScalarDouble(getInput(inputs, 1));
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 2)));
    std::string text = getString(getInput(inputs, 3));
    ImPlot::TagY(y, col, "%s", text.c_str());
  });

  // === Axis management =====================================================
  reg.registerCommand("implot_set_axis", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_set_axis(axis)");
    ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    ImPlot::SetAxis(axis);
  });

  reg.registerCommand("implot_set_axes", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_set_axes(x_axis, y_axis)");
    ImAxis x_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    ImAxis y_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 2)));
    ImPlot::SetAxes(x_axis, y_axis);
  });

  // === Coordinate transforms ===============================================
  reg.registerCommand("implot_pixels_to_plot", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_pixels_to_plot(x, y, ...)");
    double x = getScalarDouble(getInput(inputs, 1));
    double y = getScalarDouble(getInput(inputs, 2));
    ImAxis x_axis = IMPLOT_AUTO;
    ImAxis y_axis = IMPLOT_AUTO;
    if (inputCount(inputs) > 3) x_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 3)));
    if (inputCount(inputs) > 4) y_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 4)));
    ImPlotPoint p = ImPlot::PixelsToPlot(x, y, x_axis, y_axis);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, p.x, p.y);
  });

  reg.registerCommand("implot_plot_to_pixels", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_plot_to_pixels(x, y, ...)");
    double x = getScalarDouble(getInput(inputs, 1));
    double y = getScalarDouble(getInput(inputs, 2));
    ImAxis x_axis = IMPLOT_AUTO;
    ImAxis y_axis = IMPLOT_AUTO;
    if (inputCount(inputs) > 3) x_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 3)));
    if (inputCount(inputs) > 4) y_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 4)));
    ImVec2 v = ImPlot::PlotToPixels(x, y, x_axis, y_axis);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, v.x, v.y);
  });

  reg.registerCommand("implot_get_plot_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    ImVec2 v = ImPlot::GetPlotPos();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, v.x, v.y);
  });

  reg.registerCommand("implot_get_plot_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    ImVec2 v = ImPlot::GetPlotSize();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, v.x, v.y);
  });

  reg.registerCommand("implot_get_plot_mouse_pos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    ImAxis x_axis = IMPLOT_AUTO;
    ImAxis y_axis = IMPLOT_AUTO;
    if (inputCount(inputs) > 1) x_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    if (inputCount(inputs) > 2) y_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 2)));
    ImPlotPoint p = ImPlot::GetPlotMousePos(x_axis, y_axis);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec2(factory, p.x, p.y);
  });

  reg.registerCommand("implot_is_plot_hovered", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    bool v = ImPlot::IsPlotHovered();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  reg.registerCommand("implot_is_axis_hovered", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_is_axis_hovered(axis)");
    ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    bool v = ImPlot::IsAxisHovered(axis);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  // === Style ===============================================================
  reg.registerCommand("implot_push_style_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_push_style_color(idx, col)");
    ImPlotCol idx = static_cast<ImPlotCol>(getScalarInt(getInput(inputs, 1)));
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 2)));
    ImPlot::PushStyleColor(idx, col);
  });

  reg.registerCommand("implot_pop_style_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    int count = 1;
    if (inputCount(inputs) > 1) count = getScalarInt(getInput(inputs, 1));
    ImPlot::PopStyleColor(count);
  });

  reg.registerCommand("implot_push_style_var", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_push_style_var(idx, val)");
    ImPlotStyleVar idx = static_cast<ImPlotStyleVar>(getScalarInt(getInput(inputs, 1)));
    const Array& valArr = getInput(inputs, 2);
    auto dims = valArr.getDimensions();
    if (dims.size() == 2 && dims[0] == 1 && dims[1] == 2) {
      ImVec2 v = toImVec2(getVec2(valArr));
      ImPlot::PushStyleVar(idx, v);
    } else {
      ImPlot::PushStyleVar(idx, getScalarFloat(valArr));
    }
  });

  reg.registerCommand("implot_pop_style_var", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    int count = 1;
    if (inputCount(inputs) > 1) count = getScalarInt(getInput(inputs, 1));
    ImPlot::PopStyleVar(count);
  });

  reg.registerCommand("implot_set_next_fill_style", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_set_next_fill_style(col, ...)");
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 1)));
    float alpha_mod = IMPLOT_AUTO;
    if (inputCount(inputs) > 2) alpha_mod = getScalarFloat(getInput(inputs, 2));
    ImPlot::SetNextFillStyle(col, alpha_mod);
  });

  reg.registerCommand("implot_get_last_item_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    ImVec4 v = ImPlot::GetLastItemColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec4(factory, v.x, v.y, v.z, v.w);
  });

  // === Setup / axis ticks and constraints ==================================
  reg.registerCommand("implot_setup_axis_ticks_values", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3)
      throwError(matlabPtr, "Expected implot_setup_axis_ticks_values(axis, values, ...");
    ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    std::vector<const char*> label_ptrs;
    std::vector<std::string> label_storage;
    if (inputCount(inputs) > 3) {
      label_storage = getStringVector(getInput(inputs, 3));
      if (!label_storage.empty() && label_storage.size() != values.size())
        throwError(matlabPtr, "labels count must match values count");
      label_ptrs.reserve(label_storage.size());
      for (auto& s : label_storage) label_ptrs.push_back(s.c_str());
    }
    bool keep_default = false;
    if (inputCount(inputs) > 4) keep_default = getScalarBool(getInput(inputs, 4));
    ImPlot::SetupAxisTicks(axis, values.data(), static_cast<int>(values.size()),
                           label_ptrs.empty() ? nullptr : label_ptrs.data(), keep_default);
  });

  reg.registerCommand("implot_setup_axis_ticks_range", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5)
      throwError(matlabPtr, "Expected implot_setup_axis_ticks_range(axis, v_min, v_max, n_ticks, ...");
    ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    double v_min = getScalarDouble(getInput(inputs, 2));
    double v_max = getScalarDouble(getInput(inputs, 3));
    int n_ticks = getScalarInt(getInput(inputs, 4));
    std::vector<const char*> label_ptrs;
    std::vector<std::string> label_storage;
    if (inputCount(inputs) > 5) {
      label_storage = getStringVector(getInput(inputs, 5));
      if (!label_storage.empty() && static_cast<int>(label_storage.size()) != n_ticks)
        throwError(matlabPtr, "labels count must match n_ticks");
      label_ptrs.reserve(label_storage.size());
      for (auto& s : label_storage) label_ptrs.push_back(s.c_str());
    }
    bool keep_default = false;
    if (inputCount(inputs) > 6) keep_default = getScalarBool(getInput(inputs, 6));
    ImPlot::SetupAxisTicks(axis, v_min, v_max, n_ticks,
                           label_ptrs.empty() ? nullptr : label_ptrs.data(), keep_default);
  });

  reg.registerCommand("implot_setup_axis_limits_constraints",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected implot_setup_axis_limits_constraints(axis, v_min, v_max)");
                        ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
                        double v_min = getScalarDouble(getInput(inputs, 2));
                        double v_max = getScalarDouble(getInput(inputs, 3));
                        ImPlot::SetupAxisLimitsConstraints(axis, v_min, v_max);
                      });

  reg.registerCommand("implot_setup_axis_zoom_constraints",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected implot_setup_axis_zoom_constraints(axis, z_min, z_max)");
                        ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
                        double z_min = getScalarDouble(getInput(inputs, 2));
                        double z_max = getScalarDouble(getInput(inputs, 3));
                        ImPlot::SetupAxisZoomConstraints(axis, z_min, z_max);
                      });

  // === Plot items not yet bound ============================================
  reg.registerCommand("implot_plot_pie_chart", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 8)
      throwError(matlabPtr,
                 "Expected implot_plot_pie_chart(label_ids, values, x, y, radius, label_fmt, angle0, "
                 "flags)");
    auto labels = getStringVector(getInput(inputs, 1));
    auto values = getVectorDoubleChecked(getInput(inputs, 2));
    if (labels.size() != values.size())
      throwError(matlabPtr, "label_ids count must match values count");
    double x = getScalarDouble(getInput(inputs, 3));
    double y = getScalarDouble(getInput(inputs, 4));
    double radius = getScalarDouble(getInput(inputs, 5));
    std::string label_fmt = getString(getInput(inputs, 6));
    double angle0 = getScalarDouble(getInput(inputs, 7));
    ImPlotPieChartFlags flags = 0;
    if (inputCount(inputs) > 8) flags = getScalarInt(getInput(inputs, 8));
    std::vector<const char*> label_ptrs;
    label_ptrs.reserve(labels.size());
    for (auto& s : labels) label_ptrs.push_back(s.c_str());
    ImPlot::PlotPieChart(label_ptrs.data(), values.data(), static_cast<int>(values.size()), x, y,
                         radius, label_fmt.c_str(), angle0, flags);
  });

  reg.registerCommand("implot_plot_image", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5)
      throwError(matlabPtr,
                 "Expected implot_plot_image(label_id, tex_id, bounds_min, bounds_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImTextureID tex_id = static_cast<ImTextureID>(getScalarDouble(getInput(inputs, 2)));
    auto bmin = getVec2d(getInput(inputs, 3));
    auto bmax = getVec2d(getInput(inputs, 4));
    ImPlotPoint bounds_min(bmin[0], bmin[1]);
    ImPlotPoint bounds_max(bmax[0], bmax[1]);
    ImVec2 uv0(0, 0);
    ImVec2 uv1(1, 1);
    ImVec4 tint_col(1, 1, 1, 1);
    ImPlotImageFlags flags = 0;
    if (inputCount(inputs) > 5) uv0 = toImVec2(getVec2(getInput(inputs, 5)));
    if (inputCount(inputs) > 6) uv1 = toImVec2(getVec2(getInput(inputs, 6)));
    if (inputCount(inputs) > 7) tint_col = toImVec4(getVec4(getInput(inputs, 7)));
    if (inputCount(inputs) > 8) flags = getScalarInt(getInput(inputs, 8));
    ImPlot::PlotImage(label.c_str(), ImTextureRef(tex_id), bounds_min, bounds_max, uv0, uv1,
                      tint_col, flags);
  });

  reg.registerCommand("implot_plot_text", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4)
      throwError(matlabPtr, "Expected implot_plot_text(text, x, y, ...)");
    std::string text = getString(getInput(inputs, 1));
    double x = getScalarDouble(getInput(inputs, 2));
    double y = getScalarDouble(getInput(inputs, 3));
    ImVec2 pix_offset(0, 0);
    ImPlotTextFlags flags = 0;
    if (inputCount(inputs) > 4) pix_offset = toImVec2(getVec2(getInput(inputs, 4)));
    if (inputCount(inputs) > 5) flags = getScalarInt(getInput(inputs, 5));
    ImPlot::PlotText(text.c_str(), x, y, pix_offset, flags);
  });

  reg.registerCommand("implot_plot_dummy", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_plot_dummy(label_id, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImPlotDummyFlags flags = 0;
    if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
    ImPlot::PlotDummy(label.c_str(), flags);
  });

  // === Subplots / selection / popup / aligned ==============================
  reg.registerCommand("implot_begin_aligned_plots", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2)
      throwError(matlabPtr, "Expected implot_begin_aligned_plots(group_id, ...)");
    std::string group_id = getString(getInput(inputs, 1));
    bool vertical = true;
    if (inputCount(inputs) > 2) vertical = getScalarBool(getInput(inputs, 2));
    bool ok = ImPlot::BeginAlignedPlots(group_id.c_str(), vertical);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ok);
  });

  reg.registerCommand("implot_end_aligned_plots", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    ImPlot::EndAlignedPlots();
  });

  reg.registerCommand("implot_begin_legend_popup", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2)
      throwError(matlabPtr, "Expected implot_begin_legend_popup(label_id, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImGuiMouseButton mouse_button = 1;
    if (inputCount(inputs) > 2) mouse_button = static_cast<ImGuiMouseButton>(getScalarInt(getInput(inputs, 2)));
    bool ok = ImPlot::BeginLegendPopup(label.c_str(), mouse_button);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, ok);
  });

  reg.registerCommand("implot_end_legend_popup", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    ImPlot::EndLegendPopup();
  });

  reg.registerCommand("implot_is_legend_entry_hovered",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2)
                          throwError(matlabPtr, "Expected implot_is_legend_entry_hovered(label_id)");
                        std::string label = getString(getInput(inputs, 1));
                        bool v = ImPlot::IsLegendEntryHovered(label.c_str());
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, v);
                      });

  reg.registerCommand("implot_hide_next_item", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    bool hidden = true;
    ImPlotCond cond = ImPlotCond_Once;
    if (inputCount(inputs) > 1) hidden = getScalarBool(getInput(inputs, 1));
    if (inputCount(inputs) > 2) cond = static_cast<ImPlotCond>(getScalarInt(getInput(inputs, 2)));
    ImPlot::HideNextItem(hidden, cond);
  });

  reg.registerCommand("implot_cancel_plot_selection", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    ImPlot::CancelPlotSelection();
  });

  reg.registerCommand("implot_is_subplots_hovered", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    bool v = ImPlot::IsSubplotsHovered();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  reg.registerCommand("implot_is_plot_selected", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    bool v = ImPlot::IsPlotSelected();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  reg.registerCommand("implot_get_plot_selection", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    ImAxis x_axis = IMPLOT_AUTO;
    ImAxis y_axis = IMPLOT_AUTO;
    if (inputCount(inputs) > 1) x_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    if (inputCount(inputs) > 2) y_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 2)));
    ImPlotRect r = ImPlot::GetPlotSelection(x_axis, y_axis);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) =
        createVec4(factory, r.X.Min, r.X.Max, r.Y.Min, r.Y.Max);
  });

  reg.registerCommand("implot_get_plot_limits", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    ImAxis x_axis = IMPLOT_AUTO;
    ImAxis y_axis = IMPLOT_AUTO;
    if (inputCount(inputs) > 1) x_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
    if (inputCount(inputs) > 2) y_axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 2)));
    ImPlotRect r = ImPlot::GetPlotLimits(x_axis, y_axis);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec4(factory, r.X.Min, r.X.Max, r.Y.Min, r.Y.Max);
  });

  // === Drag-drop ===========================================================
  reg.registerCommand("implot_begin_drag_drop_target_plot",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        bool v = ImPlot::BeginDragDropTargetPlot();
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, v);
                      });

  reg.registerCommand("implot_begin_drag_drop_target_axis",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2)
                          throwError(matlabPtr, "Expected implot_begin_drag_drop_target_axis(axis)");
                        ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
                        bool v = ImPlot::BeginDragDropTargetAxis(axis);
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, v);
                      });

  reg.registerCommand("implot_begin_drag_drop_target_legend",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        bool v = ImPlot::BeginDragDropTargetLegend();
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, v);
                      });

  reg.registerCommand("implot_end_drag_drop_target", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    ImPlot::EndDragDropTarget();
  });

  reg.registerCommand("implot_begin_drag_drop_source_plot",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        ImGuiDragDropFlags flags = 0;
                        if (inputCount(inputs) > 1) flags = getScalarInt(getInput(inputs, 1));
                        bool v = ImPlot::BeginDragDropSourcePlot(flags);
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, v);
                      });

  reg.registerCommand("implot_begin_drag_drop_source_axis",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2)
                          throwError(matlabPtr, "Expected implot_begin_drag_drop_source_axis(axis, ...)");
                        ImAxis axis = static_cast<ImAxis>(getScalarInt(getInput(inputs, 1)));
                        ImGuiDragDropFlags flags = 0;
                        if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
                        bool v = ImPlot::BeginDragDropSourceAxis(axis, flags);
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, v);
                      });

  reg.registerCommand("implot_begin_drag_drop_source_item",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2)
                          throwError(matlabPtr, "Expected implot_begin_drag_drop_source_item(label_id, ...)");
                        std::string label = getString(getInput(inputs, 1));
                        ImGuiDragDropFlags flags = 0;
                        if (inputCount(inputs) > 2) flags = getScalarInt(getInput(inputs, 2));
                        bool v = ImPlot::BeginDragDropSourceItem(label.c_str(), flags);
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, v);
                      });

  reg.registerCommand("implot_end_drag_drop_source", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    ImPlot::EndDragDropSource();
  });

  // === Colormap ============================================================
  reg.registerCommand("implot_add_colormap", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3)
      throwError(matlabPtr, "Expected implot_add_colormap(name, cols, qual)");
    std::string name = getString(getInput(inputs, 1));
    auto cols = getMatrixFloat(getInput(inputs, 2), 4);
    bool qual = true;
    if (inputCount(inputs) > 3) qual = getScalarBool(getInput(inputs, 3));
    std::vector<ImVec4> imcols;
    imcols.reserve(cols.rows());
    for (int i = 0; i < cols.rows(); ++i) {
      imcols.emplace_back(cols(i, 0), cols(i, 1), cols(i, 2), cols(i, 3));
    }
    int idx = ImPlot::AddColormap(name.c_str(), imcols.data(), static_cast<int>(imcols.size()), qual);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, idx);
  });

  reg.registerCommand("implot_get_colormap_count", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    int v = ImPlot::GetColormapCount();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, v);
  });

  reg.registerCommand("implot_get_colormap_name", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_get_colormap_name(cmap)");
    ImPlotColormap cmap = static_cast<ImPlotColormap>(getScalarInt(getInput(inputs, 1)));
    const char* name = ImPlot::GetColormapName(cmap);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name ? name : "");
  });

  reg.registerCommand("implot_get_colormap_index", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_get_colormap_index(name)");
    std::string name = getString(getInput(inputs, 1));
    int idx = ImPlot::GetColormapIndex(name.c_str());
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, idx);
  });

  reg.registerCommand("implot_push_colormap", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_push_colormap(cmap_or_name)");
    const Array& arg = getInput(inputs, 1);
    if (arg.getType() == ArrayType::MATLAB_STRING || arg.getType() == ArrayType::CHAR) {
      ImPlot::PushColormap(getString(arg).c_str());
    } else {
      ImPlot::PushColormap(static_cast<ImPlotColormap>(getScalarInt(arg)));
    }
  });

  reg.registerCommand("implot_pop_colormap", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    int count = 1;
    if (inputCount(inputs) > 1) count = getScalarInt(getInput(inputs, 1));
    ImPlot::PopColormap(count);
  });

  reg.registerCommand("implot_next_colormap_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    ImVec4 v = ImPlot::NextColormapColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec4(factory, v.x, v.y, v.z, v.w);
  });

  reg.registerCommand("implot_get_colormap_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    ImPlotColormap cmap = IMPLOT_AUTO;
    if (inputCount(inputs) > 1) cmap = static_cast<ImPlotColormap>(getScalarInt(getInput(inputs, 1)));
    int v = ImPlot::GetColormapSize(cmap);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, v);
  });

  reg.registerCommand("implot_get_colormap_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2)
      throwError(matlabPtr, "Expected implot_get_colormap_color(idx, ...)");
    int idx = getScalarInt(getInput(inputs, 1));
    ImPlotColormap cmap = IMPLOT_AUTO;
    if (inputCount(inputs) > 2) cmap = static_cast<ImPlotColormap>(getScalarInt(getInput(inputs, 2)));
    ImVec4 v = ImPlot::GetColormapColor(idx, cmap);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec4(factory, v.x, v.y, v.z, v.w);
  });

  reg.registerCommand("implot_sample_colormap", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_sample_colormap(t, ...)");
    float t = getScalarFloat(getInput(inputs, 1));
    ImPlotColormap cmap = IMPLOT_AUTO;
    if (inputCount(inputs) > 2) cmap = static_cast<ImPlotColormap>(getScalarInt(getInput(inputs, 2)));
    ImVec4 v = ImPlot::SampleColormap(t, cmap);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVec4(factory, v.x, v.y, v.z, v.w);
  });

  reg.registerCommand("implot_colormap_scale", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4)
      throwError(matlabPtr, "Expected implot_colormap_scale(label, scale_min, scale_max, ...)");
    std::string label = getString(getInput(inputs, 1));
    double scale_min = getScalarDouble(getInput(inputs, 2));
    double scale_max = getScalarDouble(getInput(inputs, 3));
    ImVec2 size(0, 0);
    std::string format = "%g";
    ImPlotColormapScaleFlags flags = 0;
    ImPlotColormap cmap = IMPLOT_AUTO;
    if (inputCount(inputs) > 4) size = toImVec2(getVec2(getInput(inputs, 4)));
    if (inputCount(inputs) > 5) format = getString(getInput(inputs, 5));
    if (inputCount(inputs) > 6) flags = getScalarInt(getInput(inputs, 6));
    if (inputCount(inputs) > 7) cmap = static_cast<ImPlotColormap>(getScalarInt(getInput(inputs, 7)));
    ImPlot::ColormapScale(label.c_str(), scale_min, scale_max, size, format.c_str(), flags, cmap);
  });

  reg.registerCommand("implot_colormap_slider", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3)
      throwError(matlabPtr, "Expected implot_colormap_slider(label, t, ...)");
    std::string label = getString(getInput(inputs, 1));
    float t = getScalarFloat(getInput(inputs, 2));
    std::string format = "";
    ImPlotColormap cmap = IMPLOT_AUTO;
    if (inputCount(inputs) > 3) format = getString(getInput(inputs, 3));
    if (inputCount(inputs) > 4) cmap = static_cast<ImPlotColormap>(getScalarInt(getInput(inputs, 4)));
    ImVec4 out_col;
    bool changed = ImPlot::ColormapSlider(label.c_str(), &t, &out_col, format.c_str(), cmap);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, changed);
    getOutput(outputs, 1) = createScalarDouble(factory, t);
    getOutput(outputs, 2) = createVec4(factory, out_col.x, out_col.y, out_col.z, out_col.w);
  });

  reg.registerCommand("implot_colormap_button", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_colormap_button(label, ...)");
    std::string label = getString(getInput(inputs, 1));
    ImVec2 size(0, 0);
    ImPlotColormap cmap = IMPLOT_AUTO;
    if (inputCount(inputs) > 2) size = toImVec2(getVec2(getInput(inputs, 2)));
    if (inputCount(inputs) > 3) cmap = static_cast<ImPlotColormap>(getScalarInt(getInput(inputs, 3)));
    bool v = ImPlot::ColormapButton(label.c_str(), size, cmap);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  reg.registerCommand("implot_bust_color_cache", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    std::string plot_title_id;
    if (inputCount(inputs) > 1) plot_title_id = getString(getInput(inputs, 1));
    ImPlot::BustColorCache(plot_title_id.empty() ? nullptr : plot_title_id.c_str());
  });

  // === Style / input map / icons / draw list / show ========================
  reg.registerCommand("implot_get_style", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    ImPlotStyle* style = &ImPlot::GetStyle();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, style ? static_cast<double>(reinterpret_cast<uintptr_t>(style)) : 0.0);
  });

  reg.registerCommand("implot_style_colors_auto", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    ImPlot::StyleColorsAuto();
  });

  reg.registerCommand("implot_style_colors_classic", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    ImPlot::StyleColorsClassic();
  });

  reg.registerCommand("implot_style_colors_dark", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    ImPlot::StyleColorsDark();
  });

  reg.registerCommand("implot_style_colors_light", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    ImPlot::StyleColorsLight();
  });

  reg.registerCommand("implot_get_input_map", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    ImPlotInputMap* map = &ImPlot::GetInputMap();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, map ? static_cast<double>(reinterpret_cast<uintptr_t>(map)) : 0.0);
  });

  reg.registerCommand("implot_style_get", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_style_get(handle, prop_name)");
    ImPlotStyle* style = implotStyleFromHandle(getInput(inputs, 1));
    if (!style) throwError(matlabPtr, "Invalid ImPlotStyle handle");
    std::string prop = getString(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;

    auto vec2out = [&](const ImVec2& v) {
      getOutput(outputs, 0) = createVec2(factory, static_cast<double>(v.x), static_cast<double>(v.y));
    };

    if (prop == "LineWeight") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->LineWeight));
    } else if (prop == "Marker") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->Marker));
    } else if (prop == "MarkerSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->MarkerSize));
    } else if (prop == "MarkerWeight") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->MarkerWeight));
    } else if (prop == "FillAlpha") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->FillAlpha));
    } else if (prop == "ErrorBarSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ErrorBarSize));
    } else if (prop == "ErrorBarWeight") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->ErrorBarWeight));
    } else if (prop == "DigitalBitHeight") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->DigitalBitHeight));
    } else if (prop == "DigitalBitGap") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->DigitalBitGap));
    } else if (prop == "PlotBorderSize") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->PlotBorderSize));
    } else if (prop == "MinorAlpha") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->MinorAlpha));
    } else if (prop == "MajorTickLen") {
      vec2out(style->MajorTickLen);
    } else if (prop == "MinorTickLen") {
      vec2out(style->MinorTickLen);
    } else if (prop == "MajorTickSize") {
      vec2out(style->MajorTickSize);
    } else if (prop == "MinorTickSize") {
      vec2out(style->MinorTickSize);
    } else if (prop == "MajorGridSize") {
      vec2out(style->MajorGridSize);
    } else if (prop == "MinorGridSize") {
      vec2out(style->MinorGridSize);
    } else if (prop == "PlotPadding") {
      vec2out(style->PlotPadding);
    } else if (prop == "LabelPadding") {
      vec2out(style->LabelPadding);
    } else if (prop == "LegendPadding") {
      vec2out(style->LegendPadding);
    } else if (prop == "LegendInnerPadding") {
      vec2out(style->LegendInnerPadding);
    } else if (prop == "LegendSpacing") {
      vec2out(style->LegendSpacing);
    } else if (prop == "MousePosPadding") {
      vec2out(style->MousePosPadding);
    } else if (prop == "AnnotationPadding") {
      vec2out(style->AnnotationPadding);
    } else if (prop == "FitPadding") {
      vec2out(style->FitPadding);
    } else if (prop == "PlotDefaultSize") {
      vec2out(style->PlotDefaultSize);
    } else if (prop == "PlotMinSize") {
      vec2out(style->PlotMinSize);
    } else if (prop == "Colormap") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(style->Colormap));
    } else if (prop == "UseLocalTime") {
      getOutput(outputs, 0) = createScalarBool(factory, style->UseLocalTime);
    } else if (prop == "UseISO8601") {
      getOutput(outputs, 0) = createScalarBool(factory, style->UseISO8601);
    } else if (prop == "Use24HourClock") {
      getOutput(outputs, 0) = createScalarBool(factory, style->Use24HourClock);
    } else {
      throwError(matlabPtr, "Unknown ImPlotStyle property: " + prop);
    }
  });

  reg.registerCommand("implot_style_set", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_style_set(handle, prop_name, value)");
    ImPlotStyle* style = implotStyleFromHandle(getInput(inputs, 1));
    if (!style) throwError(matlabPtr, "Invalid ImPlotStyle handle");
    std::string prop = getString(getInput(inputs, 2));
    const Array& val = getInput(inputs, 3);

    auto setVec2 = [&](ImVec2& out) {
      Eigen::Vector2f v = getVec2(val);
      out = ImVec2(v[0], v[1]);
    };

    if (prop == "LineWeight") {
      style->LineWeight = getScalarFloat(val);
    } else if (prop == "Marker") {
      style->Marker = getScalarInt(val);
    } else if (prop == "MarkerSize") {
      style->MarkerSize = getScalarFloat(val);
    } else if (prop == "MarkerWeight") {
      style->MarkerWeight = getScalarFloat(val);
    } else if (prop == "FillAlpha") {
      style->FillAlpha = getScalarFloat(val);
    } else if (prop == "ErrorBarSize") {
      style->ErrorBarSize = getScalarFloat(val);
    } else if (prop == "ErrorBarWeight") {
      style->ErrorBarWeight = getScalarFloat(val);
    } else if (prop == "DigitalBitHeight") {
      style->DigitalBitHeight = getScalarFloat(val);
    } else if (prop == "DigitalBitGap") {
      style->DigitalBitGap = getScalarFloat(val);
    } else if (prop == "PlotBorderSize") {
      style->PlotBorderSize = getScalarFloat(val);
    } else if (prop == "MinorAlpha") {
      style->MinorAlpha = getScalarFloat(val);
    } else if (prop == "MajorTickLen") {
      setVec2(style->MajorTickLen);
    } else if (prop == "MinorTickLen") {
      setVec2(style->MinorTickLen);
    } else if (prop == "MajorTickSize") {
      setVec2(style->MajorTickSize);
    } else if (prop == "MinorTickSize") {
      setVec2(style->MinorTickSize);
    } else if (prop == "MajorGridSize") {
      setVec2(style->MajorGridSize);
    } else if (prop == "MinorGridSize") {
      setVec2(style->MinorGridSize);
    } else if (prop == "PlotPadding") {
      setVec2(style->PlotPadding);
    } else if (prop == "LabelPadding") {
      setVec2(style->LabelPadding);
    } else if (prop == "LegendPadding") {
      setVec2(style->LegendPadding);
    } else if (prop == "LegendInnerPadding") {
      setVec2(style->LegendInnerPadding);
    } else if (prop == "LegendSpacing") {
      setVec2(style->LegendSpacing);
    } else if (prop == "MousePosPadding") {
      setVec2(style->MousePosPadding);
    } else if (prop == "AnnotationPadding") {
      setVec2(style->AnnotationPadding);
    } else if (prop == "FitPadding") {
      setVec2(style->FitPadding);
    } else if (prop == "PlotDefaultSize") {
      setVec2(style->PlotDefaultSize);
    } else if (prop == "PlotMinSize") {
      setVec2(style->PlotMinSize);
    } else if (prop == "Colormap") {
      style->Colormap = static_cast<ImPlotColormap>(getScalarInt(val));
    } else if (prop == "UseLocalTime") {
      style->UseLocalTime = getScalarBool(val);
    } else if (prop == "UseISO8601") {
      style->UseISO8601 = getScalarBool(val);
    } else if (prop == "Use24HourClock") {
      style->Use24HourClock = getScalarBool(val);
    } else {
      throwError(matlabPtr, "Unknown ImPlotStyle property: " + prop);
    }
  });

  reg.registerCommand("implot_input_map_get", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected implot_input_map_get(handle, prop_name)");
    ImPlotInputMap* map = implotInputMapFromHandle(getInput(inputs, 1));
    if (!map) throwError(matlabPtr, "Invalid ImPlotInputMap handle");
    std::string prop = getString(getInput(inputs, 2));
    matlab::data::ArrayFactory factory;

    if (prop == "Pan") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->Pan));
    } else if (prop == "PanMod") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->PanMod));
    } else if (prop == "Fit") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->Fit));
    } else if (prop == "Select") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->Select));
    } else if (prop == "SelectCancel") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->SelectCancel));
    } else if (prop == "SelectMod") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->SelectMod));
    } else if (prop == "SelectHorzMod") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->SelectHorzMod));
    } else if (prop == "SelectVertMod") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->SelectVertMod));
    } else if (prop == "Menu") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->Menu));
    } else if (prop == "OverrideMod") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->OverrideMod));
    } else if (prop == "ZoomMod") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->ZoomMod));
    } else if (prop == "ZoomRate") {
      getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(map->ZoomRate));
    } else {
      throwError(matlabPtr, "Unknown ImPlotInputMap property: " + prop);
    }
  });

  reg.registerCommand("implot_input_map_set", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected implot_input_map_set(handle, prop_name, value)");
    ImPlotInputMap* map = implotInputMapFromHandle(getInput(inputs, 1));
    if (!map) throwError(matlabPtr, "Invalid ImPlotInputMap handle");
    std::string prop = getString(getInput(inputs, 2));
    const Array& val = getInput(inputs, 3);

    if (prop == "Pan") {
      map->Pan = static_cast<ImGuiMouseButton>(getScalarInt(val));
    } else if (prop == "PanMod") {
      map->PanMod = getScalarInt(val);
    } else if (prop == "Fit") {
      map->Fit = static_cast<ImGuiMouseButton>(getScalarInt(val));
    } else if (prop == "Select") {
      map->Select = static_cast<ImGuiMouseButton>(getScalarInt(val));
    } else if (prop == "SelectCancel") {
      map->SelectCancel = static_cast<ImGuiMouseButton>(getScalarInt(val));
    } else if (prop == "SelectMod") {
      map->SelectMod = getScalarInt(val);
    } else if (prop == "SelectHorzMod") {
      map->SelectHorzMod = getScalarInt(val);
    } else if (prop == "SelectVertMod") {
      map->SelectVertMod = getScalarInt(val);
    } else if (prop == "Menu") {
      map->Menu = static_cast<ImGuiMouseButton>(getScalarInt(val));
    } else if (prop == "OverrideMod") {
      map->OverrideMod = getScalarInt(val);
    } else if (prop == "ZoomMod") {
      map->ZoomMod = getScalarInt(val);
    } else if (prop == "ZoomRate") {
      map->ZoomRate = getScalarFloat(val);
    } else {
      throwError(matlabPtr, "Unknown ImPlotInputMap property: " + prop);
    }
  });

  reg.registerCommand("implot_map_input_default", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    ImPlot::MapInputDefault();
  });

  reg.registerCommand("implot_map_input_reverse", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    ImPlot::MapInputReverse();
  });

  reg.registerCommand("implot_item_icon", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_item_icon(col)");
    ImVec4 col = toImVec4(getVec4(getInput(inputs, 1)));
    ImPlot::ItemIcon(col);
  });

  reg.registerCommand("implot_colormap_icon", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected implot_colormap_icon(cmap)");
    ImPlotColormap cmap = static_cast<ImPlotColormap>(getScalarInt(getInput(inputs, 1)));
    ImPlot::ColormapIcon(cmap);
  });

  reg.registerCommand("implot_get_plot_draw_list", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    ImDrawList* dl = ImPlot::GetPlotDrawList();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(reinterpret_cast<uintptr_t>(dl)));
  });

  reg.registerCommand("implot_push_plot_clip_rect", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    float expand = 0.0f;
    if (inputCount(inputs) > 1) expand = getScalarFloat(getInput(inputs, 1));
    ImPlot::PushPlotClipRect(expand);
  });

  reg.registerCommand("implot_pop_plot_clip_rect", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    ImPlot::PopPlotClipRect();
  });

  reg.registerCommand("implot_show_style_selector", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2)
      throwError(matlabPtr, "Expected implot_show_style_selector(label)");
    std::string label = getString(getInput(inputs, 1));
    bool v = ImPlot::ShowStyleSelector(label.c_str());
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, v);
  });

  reg.registerCommand("implot_show_colormap_selector",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2)
                          throwError(matlabPtr, "Expected implot_show_colormap_selector(label)");
                        std::string label = getString(getInput(inputs, 1));
                        bool v = ImPlot::ShowColormapSelector(label.c_str());
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, v);
                      });

  reg.registerCommand("implot_show_input_map_selector",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2)
                          throwError(matlabPtr, "Expected implot_show_input_map_selector(label)");
                        std::string label = getString(getInput(inputs, 1));
                        bool v = ImPlot::ShowInputMapSelector(label.c_str());
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, v);
                      });

  reg.registerCommand("implot_show_style_editor", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    ImPlot::ShowStyleEditor();
  });

  reg.registerCommand("implot_show_user_guide", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    ImPlot::ShowUserGuide();
  });

  bind_implot_constant_lookup(reg);
}

} // namespace ps_mex
