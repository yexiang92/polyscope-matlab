#include "bind_point_cloud.h"
#include "matlab_data_utils.h"
#include "quantity_options.h"

#include "polyscope/point_cloud.h"
#include "polyscope/polyscope.h"

#include <algorithm>

namespace ps_mex {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

namespace {

polyscope::PointCloud* getPointCloudChecked(matlab::engine::MATLABEngine* matlabPtr,
                                            const std::string& name) {
  polyscope::PointCloud* pc = polyscope::getPointCloud(name);
  if (!pc) {
    throwError(matlabPtr, "No point cloud with name: " + name);
  }
  return pc;
}

polyscope::PointRenderMode parsePointRenderMode(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  if (lower == "sphere") return polyscope::PointRenderMode::Sphere;
  if (lower == "quad") return polyscope::PointRenderMode::Quad;
  throw std::runtime_error("Unknown point render mode: " + s);
}

} // namespace

void bind_point_cloud_commands(CommandRegistry& reg) {
  reg.registerCommand("register_point_cloud", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) {
      throwError(matlabPtr, "Expected register_point_cloud(name, points, ...)");
    }
    std::string name = getString(getInput(inputs, 1));
    auto pts = getMatrixFloat(getInput(inputs, 2));
    if (pts.cols() != 2 && pts.cols() != 3) {
      throwError(matlabPtr, "points must be Nx2 or Nx3");
    }

    polyscope::PointCloud* pc = nullptr;
    if (pts.cols() == 3) {
      pc = polyscope::registerPointCloud<Eigen::MatrixXf>(name, pts);
    } else {
      pc = polyscope::registerPointCloud2D<Eigen::MatrixXf>(name, pts);
    }

    OptionsParser parser(inputs, 3, matlabPtr);
    if (parser.has("enabled")) pc->setEnabled(parser.getBool("enabled", true));
    if (parser.has("radius")) pc->setPointRadius(parser.getDouble("radius", 1.0), true);
    if (parser.has("color")) {
      auto c = parser.getVec3("color", Eigen::Vector3f::Zero());
      pc->setPointColor(glm::vec3(c[0], c[1], c[2]));
    }
    if (parser.has("material")) pc->setMaterial(parser.getString("material", ""));
    if (parser.has("point_render_mode")) {
      pc->setPointRenderMode(parsePointRenderMode(parser.getString("point_render_mode", "sphere")));
    }
    if (parser.has("transparency")) pc->setTransparency(parser.getDouble("transparency", 1.0));

    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("has_point_cloud", [](ArgumentList& outputs, ArgumentList& inputs,
                                            matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected has_point_cloud(name)");
    std::string name = getString(getInput(inputs, 1));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::hasPointCloud(name));
  });

  reg.registerCommand("remove_point_cloud", [](ArgumentList& outputs, ArgumentList& inputs,
                                               matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected remove_point_cloud(name)");
    std::string name = getString(getInput(inputs, 1));
    polyscope::removePointCloud(name, false);
  });

  reg.registerCommand("point_cloud_n_points", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected point_cloud_n_points(name)");
    auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(pc->nPoints()));
  });

  reg.registerCommand("point_cloud_update_point_positions",
                      [](ArgumentList& outputs, ArgumentList& inputs,
                         matlab::engine::MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 3)
                          throwError(matlabPtr, "Expected point_cloud_update_point_positions(name, points)");
                        auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
                        auto pts = getMatrixFloat(getInput(inputs, 2));
                        if (pts.cols() == 3) {
                          pc->updatePointPositions<Eigen::MatrixXf>(pts);
                        } else if (pts.cols() == 2) {
                          pc->updatePointPositions2D<Eigen::MatrixXf>(pts);
                        } else {
                          throwError(matlabPtr, "points must be Nx2 or Nx3");
                        }
                      });

  reg.registerCommand("point_cloud_set_enabled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected point_cloud_set_enabled(name, val)");
    auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
    pc->setEnabled(getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("point_cloud_set_radius", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected point_cloud_set_radius(name, val)");
    auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
    bool relative = true;
    if (inputCount(inputs) > 3) relative = getScalarBool(getInput(inputs, 3));
    pc->setPointRadius(getScalarDouble(getInput(inputs, 2)), relative);
  });

  reg.registerCommand("point_cloud_get_radius", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected point_cloud_get_radius(name)");
    auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, pc->getPointRadius());
  });

  reg.registerCommand("point_cloud_set_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected point_cloud_set_color(name, color)");
    auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    pc->setPointColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("point_cloud_get_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected point_cloud_get_color(name)");
    auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
    glm::vec3 c = pc->getPointColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("point_cloud_set_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected point_cloud_set_material(name, material)");
    auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
    pc->setMaterial(getString(getInput(inputs, 2)));
  });

  reg.registerCommand("point_cloud_get_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected point_cloud_get_material(name)");
    auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(pc->getMaterial());
  });

  reg.registerCommand("point_cloud_set_point_render_mode",
                      [](ArgumentList& outputs, ArgumentList& inputs,
                         matlab::engine::MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 3)
                          throwError(matlabPtr, "Expected point_cloud_set_point_render_mode(name, mode)");
                        auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
                        pc->setPointRenderMode(parsePointRenderMode(getString(getInput(inputs, 2))));
                      });

  reg.registerCommand("point_cloud_get_point_render_mode",
                      [](ArgumentList& outputs, ArgumentList& inputs,
                         matlab::engine::MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2)
                          throwError(matlabPtr, "Expected point_cloud_get_point_render_mode(name)");
                        auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
                        matlab::data::ArrayFactory factory;
                        auto mode = pc->getPointRenderMode();
                        std::string s = (mode == polyscope::PointRenderMode::Sphere) ? "sphere" : "quad";
                        getOutput(outputs, 0) = factory.createScalar(s);
                      });

  reg.registerCommand("point_cloud_set_transparency", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3)
      throwError(matlabPtr, "Expected point_cloud_set_transparency(name, val)");
    auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
    pc->setTransparency(getScalarDouble(getInput(inputs, 2)));
  });

  // === Quantities ==========================================================
  reg.registerCommand("point_cloud_add_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs,
                         MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr, "Expected point_cloud_add_scalar_quantity(name, qname, values, ...)");
                        auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getEigenVectorFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        auto* q = pc->addScalarQuantity<Eigen::VectorXf>(qname, vals, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("point_cloud_add_color_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs,
                         MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr, "Expected point_cloud_add_color_quantity(name, qname, values, ...)");
                        auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3), 3);
                        OptionsParser parser(inputs, 4, matlabPtr);
                        auto* q = pc->addColorQuantity<Eigen::MatrixXf>(qname, vals);
                        applyColorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("point_cloud_add_vector_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs,
                         MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr, "Expected point_cloud_add_vector_quantity(name, qname, values, ...)");
                        auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::VectorType vtype = polyscope::VectorType::STANDARD;
                        if (parser.has("vectortype")) vtype = parseVectorType(parser.getString("vectortype", "standard"));
                        polyscope::PointCloudVectorQuantity* q = nullptr;
                        if (vals.cols() == 3) {
                          q = pc->addVectorQuantity<Eigen::MatrixXf>(qname, vals, vtype);
                        } else if (vals.cols() == 2) {
                          q = pc->addVectorQuantity2D<Eigen::MatrixXf>(qname, vals, vtype);
                        } else {
                          throwError(matlabPtr, "vector values must be Nx2 or Nx3");
                        }
                        applyVectorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("point_cloud_add_parameterization_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs,
                         MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected point_cloud_add_parameterization_quantity(name, qname, values, ...)");
                        auto* pc = getPointCloudChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3), 2);
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::ParamCoordsType ctype = polyscope::ParamCoordsType::UNIT;
                        if (parser.has("coords_type")) ctype = parseParamCoordsType(parser.getString("coords_type", "unit"));
                        auto* q = pc->addParameterizationQuantity<Eigen::MatrixXf>(qname, vals, ctype);
                        applyParameterizationQuantityOptions(*q, parser);
                      });
}

} // namespace ps_mex
