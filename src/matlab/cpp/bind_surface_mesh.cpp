#include "bind_surface_mesh.h"
#include "matlab_data_utils.h"
#include "quantity_options.h"

#include "polyscope/surface_mesh.h"
#include "polyscope/polyscope.h"

#include <algorithm>

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

polyscope::SurfaceMesh* getSurfaceMeshChecked(MATLABEngine* matlabPtr, const std::string& name) {
  polyscope::SurfaceMesh* sm = polyscope::getSurfaceMesh(name);
  if (!sm) {
    throwError(matlabPtr, "No surface mesh with name: " + name);
  }
  return sm;
}

} // namespace

void bind_surface_mesh_commands(CommandRegistry& reg) {
  reg.registerCommand("register_surface_mesh", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) {
      throwError(matlabPtr, "Expected register_surface_mesh(name, vertices, faces, ...)");
    }
    std::string name = getString(getInput(inputs, 1));
    auto verts = getMatrixFloat(getInput(inputs, 2));
    auto faces = getMatrixInt(getInput(inputs, 3));
    // MATLAB users typically use 1-based face indices; Polyscope expects 0-based.
    faces.array() -= 1;
    if (verts.cols() != 2 && verts.cols() != 3) {
      throwError(matlabPtr, "vertices must be Vx2 or Vx3");
    }
    if (faces.cols() != 3) {
      throwError(matlabPtr, "faces must be Fx3 (triangular mesh)");
    }

    polyscope::SurfaceMesh* sm = nullptr;
    if (verts.cols() == 3) {
      sm = polyscope::registerSurfaceMesh<Eigen::MatrixXf, Eigen::MatrixXi>(name, verts, faces);
    } else {
      sm = polyscope::registerSurfaceMesh2D<Eigen::MatrixXf, Eigen::MatrixXi>(name, verts, faces);
    }

    OptionsParser parser(inputs, 4, matlabPtr);
    if (parser.has("enabled")) sm->setEnabled(parser.getBool("enabled", true));
    if (parser.has("color")) {
      auto c = parser.getVec3("color", Eigen::Vector3f::Zero());
      sm->setSurfaceColor(glm::vec3(c[0], c[1], c[2]));
    }
    if (parser.has("edge_color")) {
      auto c = parser.getVec3("edge_color", Eigen::Vector3f::Zero());
      sm->setEdgeColor(glm::vec3(c[0], c[1], c[2]));
    }
    if (parser.has("edge_width")) sm->setEdgeWidth(parser.getDouble("edge_width", 0.0));
    if (parser.has("material")) sm->setMaterial(parser.getString("material", ""));
    if (parser.has("smooth_shade")) sm->setSmoothShade(parser.getBool("smooth_shade", false));
    if (parser.has("transparency")) sm->setTransparency(parser.getDouble("transparency", 1.0));

    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("has_surface_mesh", [](ArgumentList& outputs, ArgumentList& inputs,
                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected has_surface_mesh(name)");
    std::string name = getString(getInput(inputs, 1));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::hasSurfaceMesh(name));
  });

  reg.registerCommand("remove_surface_mesh", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected remove_surface_mesh(name)");
    std::string name = getString(getInput(inputs, 1));
    polyscope::removeSurfaceMesh(name, false);
  });

  reg.registerCommand("surface_mesh_n_vertices", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected surface_mesh_n_vertices(name)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(sm->nVertices()));
  });

  reg.registerCommand("surface_mesh_n_faces", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected surface_mesh_n_faces(name)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(sm->nFaces()));
  });

  reg.registerCommand("surface_mesh_update_vertex_positions",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 3)
                          throwError(matlabPtr, "Expected surface_mesh_update_vertex_positions(name, vertices)");
                        auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        auto verts = getMatrixFloat(getInput(inputs, 2));
                        if (verts.cols() == 3) {
                          sm->updateVertexPositions<Eigen::MatrixXf>(verts);
                        } else if (verts.cols() == 2) {
                          sm->updateVertexPositions2D<Eigen::MatrixXf>(verts);
                        } else {
                          throwError(matlabPtr, "vertices must be Vx2 or Vx3");
                        }
                      });

  reg.registerCommand("surface_mesh_set_enabled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected surface_mesh_set_enabled(name, val)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    sm->setEnabled(getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("surface_mesh_set_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected surface_mesh_set_color(name, color)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    sm->setSurfaceColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("surface_mesh_get_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected surface_mesh_get_color(name)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    glm::vec3 c = sm->getSurfaceColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("surface_mesh_set_edge_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected surface_mesh_set_edge_color(name, color)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    sm->setEdgeColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("surface_mesh_set_edge_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected surface_mesh_set_edge_width(name, width)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    sm->setEdgeWidth(getScalarDouble(getInput(inputs, 2)));
  });

  reg.registerCommand("surface_mesh_set_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected surface_mesh_set_material(name, material)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    sm->setMaterial(getString(getInput(inputs, 2)));
  });

  reg.registerCommand("surface_mesh_get_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected surface_mesh_get_material(name)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(sm->getMaterial());
  });

  reg.registerCommand("surface_mesh_set_smooth_shade", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected surface_mesh_set_smooth_shade(name, val)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    sm->setSmoothShade(getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("surface_mesh_set_transparency", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected surface_mesh_set_transparency(name, val)");
    auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    sm->setTransparency(getScalarDouble(getInput(inputs, 2)));
  });

  // === Quantities ==========================================================
  reg.registerCommand("surface_mesh_add_vertex_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected surface_mesh_add_vertex_scalar_quantity(name, qname, values, ...)");
                        auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getEigenVectorFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        auto* q = sm->addVertexScalarQuantity<Eigen::VectorXf>(qname, vals, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("surface_mesh_add_face_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected surface_mesh_add_face_scalar_quantity(name, qname, values, ...)");
                        auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getEigenVectorFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        auto* q = sm->addFaceScalarQuantity<Eigen::VectorXf>(qname, vals, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("surface_mesh_add_vertex_color_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected surface_mesh_add_vertex_color_quantity(name, qname, values, ...)");
                        auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3), 3);
                        OptionsParser parser(inputs, 4, matlabPtr);
                        auto* q = sm->addVertexColorQuantity<Eigen::MatrixXf>(qname, vals);
                        applyColorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("surface_mesh_add_face_color_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected surface_mesh_add_face_color_quantity(name, qname, values, ...)");
                        auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3), 3);
                        OptionsParser parser(inputs, 4, matlabPtr);
                        auto* q = sm->addFaceColorQuantity<Eigen::MatrixXf>(qname, vals);
                        applyColorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("surface_mesh_add_vertex_vector_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected surface_mesh_add_vertex_vector_quantity(name, qname, values, ...)");
                        auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::VectorType vtype = polyscope::VectorType::STANDARD;
                        if (parser.has("vectortype")) vtype = parseVectorType(parser.getString("vectortype", "standard"));
                        polyscope::SurfaceVertexVectorQuantity* q = nullptr;
                        if (vals.cols() == 3) {
                          q = sm->addVertexVectorQuantity<Eigen::MatrixXf>(qname, vals, vtype);
                        } else if (vals.cols() == 2) {
                          q = sm->addVertexVectorQuantity2D<Eigen::MatrixXf>(qname, vals, vtype);
                        } else {
                          throwError(matlabPtr, "vector values must be Nx2 or Nx3");
                        }
                        applyVectorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("surface_mesh_add_face_vector_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected surface_mesh_add_face_vector_quantity(name, qname, values, ...)");
                        auto* sm = getSurfaceMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::VectorType vtype = polyscope::VectorType::STANDARD;
                        if (parser.has("vectortype")) vtype = parseVectorType(parser.getString("vectortype", "standard"));
                        polyscope::SurfaceFaceVectorQuantity* q = nullptr;
                        if (vals.cols() == 3) {
                          q = sm->addFaceVectorQuantity<Eigen::MatrixXf>(qname, vals, vtype);
                        } else if (vals.cols() == 2) {
                          q = sm->addFaceVectorQuantity2D<Eigen::MatrixXf>(qname, vals, vtype);
                        } else {
                          throwError(matlabPtr, "vector values must be Nx2 or Nx3");
                        }
                        applyVectorQuantityOptions(*q, parser);
                      });
}

} // namespace ps_mex
