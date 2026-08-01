#pragma once

#include "matlab_data_utils.h"

#include "polyscope/quantity.h"
#include "polyscope/scalar_quantity.h"
#include "polyscope/vector_quantity.h"
#include "polyscope/color_quantity.h"
#include "polyscope/parameterization_quantity.h"

#include <algorithm>
#include <cctype>

namespace ps_mex {

namespace detail {

inline std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}

inline glm::vec2 parseVec2(const Eigen::Vector2f& v) { return glm::vec2(v[0], v[1]); }

inline glm::vec3 parseVec3(const Eigen::Vector3f& v) { return glm::vec3(v[0], v[1], v[2]); }

inline glm::vec4 parseColor4(const matlab::data::Array& arr) {
  auto v = getMatrixFloat(arr);
  if (v.size() != 3 && v.size() != 4) {
    throw std::runtime_error("Color must have 3 (RGB) or 4 (RGBA) components");
  }
  return glm::vec4(v(0), v(1), v(2), v.size() == 4 ? v(3) : 1.f);
}

inline std::pair<glm::vec3, glm::vec3> parseColorPair(const Eigen::MatrixXf& m) {
  if (m.rows() != 2 || m.cols() != 3) {
    throw std::runtime_error("Expected 2x3 matrix for color pair");
  }
  return {parseVec3(m.row(0)), parseVec3(m.row(1))};
}

} // namespace detail

// Enum parsing helpers shared across bindings.
inline polyscope::DataType parseDataType(const std::string& s) {
  std::string lower = detail::toLower(s);
  if (lower == "standard") return polyscope::DataType::STANDARD;
  if (lower == "symmetric") return polyscope::DataType::SYMMETRIC;
  if (lower == "magnitude") return polyscope::DataType::MAGNITUDE;
  if (lower == "categorical") return polyscope::DataType::CATEGORICAL;
  throw std::runtime_error("Unknown data type: " + s);
}

inline polyscope::VectorType parseVectorType(const std::string& s) {
  std::string lower = detail::toLower(s);
  if (lower == "standard") return polyscope::VectorType::STANDARD;
  if (lower == "ambient") return polyscope::VectorType::AMBIENT;
  throw std::runtime_error("Unknown vector type: " + s);
}

inline polyscope::IsolineStyle parseIsolineStyle(const std::string& s) {
  std::string lower = detail::toLower(s);
  if (lower == "stripe") return polyscope::IsolineStyle::Stripe;
  if (lower == "contour") return polyscope::IsolineStyle::Contour;
  throw std::runtime_error("Unknown isoline style: " + s);
}

inline polyscope::ParamVizStyle parseParamVizStyle(const std::string& s) {
  std::string lower = detail::toLower(s);
  if (lower == "checker") return polyscope::ParamVizStyle::CHECKER;
  if (lower == "grid") return polyscope::ParamVizStyle::GRID;
  if (lower == "local_check") return polyscope::ParamVizStyle::LOCAL_CHECK;
  if (lower == "local_rad") return polyscope::ParamVizStyle::LOCAL_RAD;
  if (lower == "checker_islands") return polyscope::ParamVizStyle::CHECKER_ISLANDS;
  throw std::runtime_error("Unknown parameterization style: " + s);
}

inline polyscope::ParamCoordsType parseParamCoordsType(const std::string& s) {
  std::string lower = detail::toLower(s);
  if (lower == "unit") return polyscope::ParamCoordsType::UNIT;
  if (lower == "world") return polyscope::ParamCoordsType::WORLD;
  throw std::runtime_error("Unknown parameterization coords type: " + s);
}

// Apply common options understood by most quantities.
inline void applyCommonQuantityOptions(polyscope::Quantity& q, const OptionsParser& parser) {
  if (parser.has("enabled")) q.setEnabled(parser.getBool("enabled", true));
}

// Apply options specific to scalar quantities.
template <typename Q>
inline void applyScalarQuantityOptions(polyscope::ScalarQuantity<Q>& q, const OptionsParser& parser) {
  applyCommonQuantityOptions(q.quantity, parser);

  if (parser.has("color_map")) q.setColorMap(parser.getString("color_map", ""));
  if (parser.has("cmap")) q.setColorMap(parser.getString("cmap", ""));

  if (parser.has("map_range")) {
    auto r = getMatrixFloat(parser.get("map_range"));
    if (r.size() != 2) {
      throw std::runtime_error("map_range must be a 2-element vector");
    }
    q.setMapRange({r(0), r(1)});
  }
  if (parser.has("vmin") && parser.has("vmax")) {
    q.setMapRange({parser.getDouble("vmin", 0.0), parser.getDouble("vmax", 1.0)});
  }

  if (parser.has("onscreen_colorbar_enabled")) {
    q.setOnscreenColorbarEnabled(parser.getBool("onscreen_colorbar_enabled", false));
  }

  if (parser.has("onscreen_colorbar_location")) {
    auto& arr = parser.get("onscreen_colorbar_location");
    if (arr.getType() == matlab::data::ArrayType::MATLAB_STRING ||
        arr.getType() == matlab::data::ArrayType::CHAR) {
      std::string loc = detail::toLower(getString(arr));
      if (loc == "top_left") {
        q.setOnscreenColorbarLocation(glm::vec2(-1, 1));
      } else if (loc == "top_right") {
        q.setOnscreenColorbarLocation(glm::vec2(1, 1));
      } else if (loc == "bottom_left") {
        q.setOnscreenColorbarLocation(glm::vec2(-1, -1));
      } else if (loc == "bottom_right") {
        q.setOnscreenColorbarLocation(glm::vec2(1, -1));
      } else {
        throw std::runtime_error("Unknown colorbar location: " + loc);
      }
    } else {
      auto v = getMatrixFloat(arr, 2);
      q.setOnscreenColorbarLocation(detail::parseVec2(v.row(0)));
    }
  }

  if (parser.has("onscreen_colorbar_background_color")) {
    q.setOnscreenColorbarBackgroundColor(detail::parseColor4(parser.get("onscreen_colorbar_background_color")));
  }
  if (parser.has("onscreen_colorbar_tick_color")) {
    q.setOnscreenColorbarTickColor(detail::parseColor4(parser.get("onscreen_colorbar_tick_color")));
  }
  if (parser.has("onscreen_colorbar_label_color")) {
    q.setOnscreenColorbarLabelColor(detail::parseColor4(parser.get("onscreen_colorbar_label_color")));
  }
  if (parser.has("onscreen_colorbar_title_color")) {
    q.setOnscreenColorbarTitleColor(detail::parseColor4(parser.get("onscreen_colorbar_title_color")));
  }
  if (parser.has("onscreen_colorbar_title")) {
    q.setOnscreenColorbarTitle(parser.getString("onscreen_colorbar_title", ""));
  }

  if (parser.has("isolines_enabled")) {
    q.setIsolinesEnabled(parser.getBool("isolines_enabled", false));
  }
  if (parser.has("isoline_style")) {
    q.setIsolineStyle(parseIsolineStyle(parser.getString("isoline_style", "lines")));
  }
  if (parser.has("isoline_period")) {
    bool relative = parser.getBool("isoline_period_relative", true);
    q.setIsolinePeriod(parser.getDouble("isoline_period", 1.0), relative);
  }
  if (parser.has("isoline_darkness")) {
    q.setIsolineDarkness(parser.getDouble("isoline_darkness", 0.0));
  }
  if (parser.has("isoline_contour_thickness")) {
    q.setIsolineContourThickness(parser.getDouble("isoline_contour_thickness", 0.0));
  }
}

// Apply options specific to vector quantities.
template <typename Q>
inline void applyVectorQuantityOptions(polyscope::VectorQuantityBase<Q>& q, const OptionsParser& parser) {
  applyCommonQuantityOptions(q.quantity, parser);

  if (parser.has("length")) {
    bool relative = parser.getBool("length_relative", true);
    q.setVectorLengthScale(parser.getDouble("length", 1.0), relative);
  }
  if (parser.has("length_range")) {
    q.setVectorLengthRange(parser.getDouble("length_range", 1.0));
  }
  if (parser.has("radius")) {
    bool relative = parser.getBool("radius_relative", true);
    q.setVectorRadius(parser.getDouble("radius", 1.0), relative);
  }
  if (parser.has("color")) {
    q.setVectorColor(detail::parseVec3(parser.getVec3("color", Eigen::Vector3f::Zero())));
  }
  if (parser.has("material")) {
    q.setMaterial(parser.getString("material", ""));
  }
}

// Apply options specific to color quantities.
template <typename Q>
inline void applyColorQuantityOptions(polyscope::ColorQuantity<Q>& q, const OptionsParser& parser) {
  applyCommonQuantityOptions(q.quantity, parser);
}

// Apply options specific to parameterization quantities.
template <typename Q>
inline void applyParameterizationQuantityOptions(polyscope::ParameterizationQuantity<Q>& q,
                                                 const OptionsParser& parser) {
  applyCommonQuantityOptions(q.quantity, parser);

  if (parser.has("style")) {
    q.setStyle(parseParamVizStyle(parser.getString("style", "checker")));
  }
  if (parser.has("grid_colors")) {
    q.setGridColors(detail::parseColorPair(getMatrixFloat(parser.get("grid_colors"), 3)));
  }
  if (parser.has("checker_colors")) {
    q.setCheckerColors(detail::parseColorPair(getMatrixFloat(parser.get("checker_colors"), 3)));
  }
  if (parser.has("checker_size")) {
    q.setCheckerSize(parser.getDouble("checker_size", 1.0));
  }
  if (parser.has("color_map")) {
    q.setColorMap(parser.getString("color_map", ""));
  }
  if (parser.has("alt_darkness")) {
    q.setAltDarkness(parser.getDouble("alt_darkness", 0.0));
  }
}

} // namespace ps_mex
