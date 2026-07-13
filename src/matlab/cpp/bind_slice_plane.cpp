#include "bind_slice_plane.h"
#include "matlab_data_utils.h"

#include "polyscope/polyscope.h"
#include "polyscope/slice_plane.h"

#include <glm/glm.hpp>

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

polyscope::SlicePlane* getSliceChecked(MATLABEngine* matlabPtr, const std::string& name) {
  polyscope::SlicePlane* sp = polyscope::getSlicePlane(name);
  if (!sp) throwError(matlabPtr, "No slice plane with name: " + name);
  return sp;
}

glm::vec3 readVec3(const Array& arr) {
  auto v = getVec3(arr);
  return glm::vec3(v(0), v(1), v(2));
}

void setBool(CommandRegistry& reg, const std::string& cmd,
             void (polyscope::SlicePlane::*method)(bool)) {
  reg.registerCommand(cmd, [cmd, method](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected " + cmd + "(name, value)");
    (getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->*method)(getScalarBool(getInput(inputs, 2)));
  });
}

void getBool(CommandRegistry& reg, const std::string& cmd,
             bool (polyscope::SlicePlane::*method)()) {
  reg.registerCommand(cmd, [cmd, method](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected " + cmd + "(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory,
        (getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->*method)());
  });
}

} // namespace

void bind_slice_plane_commands(CommandRegistry& reg) {
  reg.registerCommand("add_slice_plane", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    polyscope::SlicePlane* sp = nullptr;
    if (inputCount(inputs) > 1) {
      sp = polyscope::addSlicePlane(getString(getInput(inputs, 1)));
    } else {
      sp = polyscope::addSlicePlane();
    }
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(sp->name);
  });

  reg.registerCommand("add_scene_slice_plane", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    bool visible = false;
    if (inputCount(inputs) > 1) visible = getScalarBool(getInput(inputs, 1));
    auto* sp = polyscope::addSceneSlicePlane(visible);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(sp->name);
  });

  reg.registerCommand("get_slice_plane", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected get_slice_plane(name)");
    auto* sp = getSliceChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(sp->name);
  });

  reg.registerCommand("has_slice_plane", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected has_slice_plane(name)");
    std::string name = getString(getInput(inputs, 1));
    bool found = false;
    for (const auto& sp : polyscope::state::slicePlanes) {
      if (sp && sp->name == name) {
        found = true;
        break;
      }
    }
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, found);
  });

  reg.registerCommand("remove_slice_plane", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected remove_slice_plane(name)");
    polyscope::removeSlicePlane(getString(getInput(inputs, 1)));
  });

  reg.registerCommand("remove_last_scene_slice_plane", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    polyscope::removeLastSceneSlicePlane();
  });

  reg.registerCommand("remove_all_slice_planes", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    polyscope::removeAllSlicePlanes();
  });

  reg.registerCommand("slice_plane_remove", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected slice_plane_remove(name)");
    getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->remove();
  });

  setBool(reg, "slice_plane_set_enabled", &polyscope::SlicePlane::setEnabled);
  getBool(reg, "slice_plane_get_enabled", &polyscope::SlicePlane::getEnabled);
  setBool(reg, "slice_plane_set_active", &polyscope::SlicePlane::setActive);
  getBool(reg, "slice_plane_get_active", &polyscope::SlicePlane::getActive);
  setBool(reg, "slice_plane_set_draw_plane", &polyscope::SlicePlane::setDrawPlane);
  getBool(reg, "slice_plane_get_draw_plane", &polyscope::SlicePlane::getDrawPlane);
  setBool(reg, "slice_plane_set_draw_widget", &polyscope::SlicePlane::setDrawWidget);
  getBool(reg, "slice_plane_get_draw_widget", &polyscope::SlicePlane::getDrawWidget);

  reg.registerCommand("slice_plane_set_pose", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected slice_plane_set_pose(name, center, normal)");
    getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->setPose(readVec3(getInput(inputs, 2)), readVec3(getInput(inputs, 3)));
  });

  reg.registerCommand("slice_plane_get_center", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected slice_plane_get_center(name)");
    glm::vec3 v = getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->getCenter();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {v.x, v.y, v.z});
  });

  reg.registerCommand("slice_plane_get_normal", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected slice_plane_get_normal(name)");
    glm::vec3 v = getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->getNormal();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {v.x, v.y, v.z});
  });

  reg.registerCommand("slice_plane_set_widget_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected slice_plane_set_widget_size(name, value)");
    getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->setWidgetSize(getScalarFloat(getInput(inputs, 2)));
  });

  reg.registerCommand("slice_plane_get_widget_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected slice_plane_get_widget_size(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(
        factory, getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->getWidgetSize());
  });

  reg.registerCommand("slice_plane_set_color", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected slice_plane_set_color(name, color)");
    getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->setColor(readVec3(getInput(inputs, 2)));
  });

  reg.registerCommand("slice_plane_set_grid_line_color", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected slice_plane_set_grid_line_color(name, color)");
    getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->setGridLineColor(readVec3(getInput(inputs, 2)));
  });

  reg.registerCommand("slice_plane_set_transparency", [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected slice_plane_set_transparency(name, value)");
    getSliceChecked(matlabPtr, getString(getInput(inputs, 1)))->setTransparency(getScalarDouble(getInput(inputs, 2)));
  });
}

} // namespace ps_mex
