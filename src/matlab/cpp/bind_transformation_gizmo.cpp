#include "bind_transformation_gizmo.h"
#include "matlab_data_utils.h"

#include "polyscope/transformation_gizmo.h"

#include <glm/glm.hpp>

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

polyscope::TransformationGizmo* getGizmoChecked(MATLABEngine* matlabPtr, const std::string& name) {
  polyscope::TransformationGizmo* gizmo = polyscope::getTransformationGizmo(name);
  if (!gizmo) {
    throwError(matlabPtr, "No transformation gizmo with name: " + name);
  }
  return gizmo;
}

glm::mat4 getMat4(MATLABEngine* matlabPtr, const Array& arr) {
  auto mat = getMatrixDouble(arr, 4);
  if (mat.rows() != 4) {
    throwError(matlabPtr, "Expected a 4x4 transform matrix");
  }

  glm::mat4 out(1.0f);
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      out[c][r] = static_cast<float>(mat(r, c));
    }
  }
  return out;
}

Eigen::MatrixXd createEigenMat4(const glm::mat4& mat) {
  Eigen::MatrixXd out(4, 4);
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      out(r, c) = mat[c][r];
    }
  }
  return out;
}

glm::vec3 getVec3Checked(MATLABEngine* matlabPtr, const Array& arr) {
  auto vec = getVec3(arr);
  if (vec.size() != 3) {
    throwError(matlabPtr, "Expected a 1x3 vector");
  }
  return glm::vec3(vec(0), vec(1), vec(2));
}

} // namespace

void bind_transformation_gizmo_commands(CommandRegistry& reg) {
  reg.registerCommand("add_transformation_gizmo", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    std::string name = "";
    if (inputCount(inputs) > 1) name = getString(getInput(inputs, 1));

    auto* gizmo = polyscope::addTransformationGizmo(name);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(gizmo->name);
  });

  reg.registerCommand("get_transformation_gizmo", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected get_transformation_gizmo(name)");
    auto* gizmo = getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)));

    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(gizmo->name);
  });

  reg.registerCommand("transformation_gizmo_get_name", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected transformation_gizmo_get_name(name)");
    auto* gizmo = getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)));

    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(gizmo->name);
  });

  reg.registerCommand("remove_transformation_gizmo", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected remove_transformation_gizmo(name)");
    polyscope::removeTransformationGizmo(getString(getInput(inputs, 1)));
  });

  reg.registerCommand("remove_all_transformation_gizmos", [](ArgumentList& outputs, ArgumentList& inputs,
                                                            MATLABEngine* matlabPtr) {
    polyscope::removeAllTransformationGizmos();
  });

  reg.registerCommand("transformation_gizmo_remove", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected transformation_gizmo_remove(name)");
    getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->remove();
  });

#define PS_GIZMO_BOOL_SETTER(CMD, METHOD)                                                                   \
  reg.registerCommand(CMD, [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {       \
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected " CMD "(name, value)");                    \
    getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->METHOD(getScalarBool(getInput(inputs, 2))); \
  });

#define PS_GIZMO_BOOL_GETTER(CMD, METHOD)                                                            \
  reg.registerCommand(CMD, [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) { \
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected " CMD "(name)");                    \
    matlab::data::ArrayFactory factory;                                                              \
    getOutput(outputs, 0) = createScalarBool(                                                        \
        factory, getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->METHOD());               \
  });

  PS_GIZMO_BOOL_SETTER("transformation_gizmo_set_enabled", setEnabled)
  PS_GIZMO_BOOL_GETTER("transformation_gizmo_get_enabled", getEnabled)
  PS_GIZMO_BOOL_SETTER("transformation_gizmo_set_allow_translation", setAllowTranslation)
  PS_GIZMO_BOOL_GETTER("transformation_gizmo_get_allow_translation", getAllowTranslation)
  PS_GIZMO_BOOL_SETTER("transformation_gizmo_set_allow_rotation", setAllowRotation)
  PS_GIZMO_BOOL_GETTER("transformation_gizmo_get_allow_rotation", getAllowRotation)
  PS_GIZMO_BOOL_SETTER("transformation_gizmo_set_allow_scaling", setAllowScaling)
  PS_GIZMO_BOOL_GETTER("transformation_gizmo_get_allow_scaling", getAllowScaling)
  PS_GIZMO_BOOL_SETTER("transformation_gizmo_set_allow_nonuniform_scaling", setAllowNonUniformScaling)
  PS_GIZMO_BOOL_GETTER("transformation_gizmo_get_allow_nonuniform_scaling", getAllowNonUniformScaling)
  PS_GIZMO_BOOL_SETTER("transformation_gizmo_set_interact_in_local_space", setInteractInLocalSpace)
  PS_GIZMO_BOOL_GETTER("transformation_gizmo_get_interact_in_local_space", getInteractInLocalSpace)

#undef PS_GIZMO_BOOL_SETTER
#undef PS_GIZMO_BOOL_GETTER

  reg.registerCommand("transformation_gizmo_set_transform", [](ArgumentList& outputs, ArgumentList& inputs,
                                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected transformation_gizmo_set_transform(name, transform)");
    getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->setTransform(getMat4(matlabPtr, getInput(inputs, 2)));
  });

  reg.registerCommand("transformation_gizmo_get_transform", [](ArgumentList& outputs, ArgumentList& inputs,
                                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected transformation_gizmo_get_transform(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createMatrixDouble(
        factory, createEigenMat4(getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->getTransform()));
  });

  reg.registerCommand("transformation_gizmo_set_position", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected transformation_gizmo_set_position(name, position)");
    getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->setPosition(getVec3Checked(matlabPtr, getInput(inputs, 2)));
  });

  reg.registerCommand("transformation_gizmo_get_position", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected transformation_gizmo_get_position(name)");
    glm::vec3 p = getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->getPosition();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {p.x, p.y, p.z});
  });

  reg.registerCommand("transformation_gizmo_set_gizmo_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected transformation_gizmo_set_gizmo_size(name, value)");
    getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->setGizmoSize(getScalarFloat(getInput(inputs, 2)));
  });

  reg.registerCommand("transformation_gizmo_set_gizmo_scale", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected transformation_gizmo_set_gizmo_scale(name, value)");
    getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->setGizmoSize(getScalarFloat(getInput(inputs, 2)));
  });

  reg.registerCommand("transformation_gizmo_get_gizmo_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected transformation_gizmo_get_gizmo_size(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(
        factory, getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->getGizmoSize());
  });

  reg.registerCommand("transformation_gizmo_get_gizmo_scale", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected transformation_gizmo_get_gizmo_scale(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(
        factory, getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->getGizmoSize());
  });

  reg.registerCommand("transformation_gizmo_build_inline_transform_ui",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected transformation_gizmo_build_inline_transform_ui(name)");
    getGizmoChecked(matlabPtr, getString(getInput(inputs, 1)))->buildInlineTransformUI();
  });
}

} // namespace ps_mex
