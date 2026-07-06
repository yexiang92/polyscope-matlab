#include "bind_camera_view.h"
#include "matlab_data_utils.h"
#include "quantity_options.h"

#include "polyscope/polyscope.h"
#include "polyscope/camera_view.h"

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

polyscope::CameraView* getCameraViewChecked(MATLABEngine* matlabPtr, const std::string& name) {
  polyscope::CameraView* cv = polyscope::getCameraView(name);
  if (!cv) {
    throwError(matlabPtr, "No camera view with name: " + name);
  }
  return cv;
}

polyscope::CameraParameters buildCameraParameters(MATLABEngine* matlabPtr, const Array& posArr,
                                                  const Array& lookArr, const Array& upArr,
                                                  const Array& fovArr, const Array& aspectArr) {
  auto pos = getMatrixFloat(posArr, 3);
  auto look = getMatrixFloat(lookArr, 3);
  auto up = getMatrixFloat(upArr, 3);
  if (pos.rows() != 1 || look.rows() != 1 || up.rows() != 1) {
    throwError(matlabPtr, "position, look_dir, and up_dir must be 1x3 vectors");
  }
  float fov = getScalarFloat(fovArr);
  float aspect = getScalarFloat(aspectArr);

  glm::vec3 root(pos(0, 0), pos(0, 1), pos(0, 2));
  glm::vec3 lookDir(look(0, 0), look(0, 1), look(0, 2));
  glm::vec3 upDir(up(0, 0), up(0, 1), up(0, 2));

  polyscope::CameraIntrinsics intrinsics =
      polyscope::CameraIntrinsics::fromFoVDegVerticalAndAspect(fov, aspect);
  polyscope::CameraExtrinsics extrinsics =
      polyscope::CameraExtrinsics::fromVectors(root, lookDir, upDir);

  return polyscope::CameraParameters(intrinsics, extrinsics);
}

matlab::data::StructArray createCameraParametersStruct(matlab::data::ArrayFactory& factory,
                                                       const polyscope::CameraParameters& params) {
  std::vector<std::string> fieldNames = {"position", "look_dir", "up_dir", "right_dir",
                                         "fov_vertical_deg", "aspect_ratio"};
  auto s = factory.createStructArray({1, 1}, fieldNames);
  auto p = params.getPosition();
  auto l = params.getLookDir();
  auto u = params.getUpDir();
  auto r = params.getRightDir();
  s[0]["position"] = createVectorDouble(factory, {p.x, p.y, p.z});
  s[0]["look_dir"] = createVectorDouble(factory, {l.x, l.y, l.z});
  s[0]["up_dir"] = createVectorDouble(factory, {u.x, u.y, u.z});
  s[0]["right_dir"] = createVectorDouble(factory, {r.x, r.y, r.z});
  s[0]["fov_vertical_deg"] = createScalarDouble(factory, params.getFoVVerticalDegrees());
  s[0]["aspect_ratio"] = createScalarDouble(factory, params.getAspectRatioWidthOverHeight());
  return s;
}

} // namespace

void bind_camera_view_commands(CommandRegistry& reg) {
  reg.registerCommand("register_camera_view", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 7) {
      throwError(matlabPtr,
                 "Expected register_camera_view(name, position, look_dir, up_dir, fov_deg, aspect_ratio, ...)");
    }
    std::string name = getString(getInput(inputs, 1));
    polyscope::CameraParameters params =
        buildCameraParameters(matlabPtr, getInput(inputs, 2), getInput(inputs, 3), getInput(inputs, 4), getInput(inputs, 5), getInput(inputs, 6));
    auto* cv = polyscope::registerCameraView(name, params);

    OptionsParser parser(inputs, 7, matlabPtr);
    if (parser.has("enabled")) cv->setEnabled(parser.getBool("enabled", true));
    if (parser.has("widget_focal_length"))
      cv->setWidgetFocalLength(parser.getFloat("widget_focal_length", 1.0f), true);
    if (parser.has("widget_thickness")) cv->setWidgetThickness(parser.getFloat("widget_thickness", 1.0f));
    if (parser.has("widget_color"))
      cv->setWidgetColor(detail::parseVec3(parser.getVec3("widget_color", Eigen::Vector3f::Zero())));

    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("has_camera_view", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected has_camera_view(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::hasCameraView(getString(getInput(inputs, 1))));
  });

  reg.registerCommand("remove_camera_view", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected remove_camera_view(name)");
    polyscope::removeCameraView(getString(getInput(inputs, 1)), false);
  });

  reg.registerCommand("camera_view_update_camera_parameters", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 7) {
      throwError(matlabPtr,
                 "Expected camera_view_update_camera_parameters(name, position, look_dir, up_dir, fov_deg, aspect_ratio)");
    }
    auto* cv = getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)));
    polyscope::CameraParameters params =
        buildCameraParameters(matlabPtr, getInput(inputs, 2), getInput(inputs, 3), getInput(inputs, 4), getInput(inputs, 5), getInput(inputs, 6));
    cv->updateCameraParameters(params);
  });

  reg.registerCommand("camera_view_get_camera_parameters", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected camera_view_get_camera_parameters(name)");
    auto* cv = getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createCameraParametersStruct(factory, cv->getCameraParameters());
  });

  reg.registerCommand("camera_view_set_view_to_this_camera", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected camera_view_set_view_to_this_camera(name, flyTo)");
    bool flyTo = false;
    if (inputCount(inputs) > 2) flyTo = getScalarBool(getInput(inputs, 2));
    getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)))->setViewToThisCamera(flyTo);
  });

  reg.registerCommand("camera_view_set_enabled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected camera_view_set_enabled(name, val)");
    getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)))->setEnabled(getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("camera_view_set_widget_focal_length", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected camera_view_set_widget_focal_length(name, val)");
    bool relative = true;
    if (inputCount(inputs) > 3) relative = getScalarBool(getInput(inputs, 3));
    getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)))->setWidgetFocalLength(getScalarFloat(getInput(inputs, 2)), relative);
  });

  reg.registerCommand("camera_view_get_widget_focal_length", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected camera_view_get_widget_focal_length(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)))->getWidgetFocalLength());
  });

  reg.registerCommand("camera_view_set_widget_thickness", [](ArgumentList& outputs, ArgumentList& inputs,
                                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected camera_view_set_widget_thickness(name, val)");
    getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)))->setWidgetThickness(getScalarFloat(getInput(inputs, 2)));
  });

  reg.registerCommand("camera_view_get_widget_thickness", [](ArgumentList& outputs, ArgumentList& inputs,
                                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected camera_view_get_widget_thickness(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)))->getWidgetThickness());
  });

  reg.registerCommand("camera_view_set_widget_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected camera_view_set_widget_color(name, color)");
    auto* cv = getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    cv->setWidgetColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("camera_view_get_widget_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected camera_view_get_widget_color(name)");
    glm::vec3 c = getCameraViewChecked(matlabPtr, getString(getInput(inputs, 1)))->getWidgetColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });
}

} // namespace ps_mex
