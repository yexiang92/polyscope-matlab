#include "bind_volume_mesh.h"
#include "matlab_data_utils.h"
#include "quantity_options.h"

#include "polyscope/polyscope.h"
#include "polyscope/volume_mesh.h"

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

polyscope::VolumeMesh* getVolumeMeshChecked(MATLABEngine* matlabPtr, const std::string& name) {
  polyscope::VolumeMesh* vm = polyscope::getVolumeMesh(name);
  if (!vm) {
    throwError(matlabPtr, "No volume mesh with name: " + name);
  }
  return vm;
}

void applyVolumeMeshOptions(polyscope::VolumeMesh* vm, const OptionsParser& parser) {
  if (parser.has("enabled")) vm->setEnabled(parser.getBool("enabled", true));
  if (parser.has("color")) vm->setColor(detail::parseVec3(parser.getVec3("color", Eigen::Vector3f::Zero())));
  if (parser.has("interior_color"))
    vm->setInteriorColor(detail::parseVec3(parser.getVec3("interior_color", Eigen::Vector3f::Zero())));
  if (parser.has("edge_color"))
    vm->setEdgeColor(detail::parseVec3(parser.getVec3("edge_color", Eigen::Vector3f::Zero())));
  if (parser.has("edge_width")) vm->setEdgeWidth(parser.getDouble("edge_width", 0.0));
  if (parser.has("material")) vm->setMaterial(parser.getString("material", ""));
  if (parser.has("transparency")) vm->setTransparency(parser.getDouble("transparency", 1.0));
}

} // namespace

void bind_volume_mesh_commands(CommandRegistry& reg) {
  reg.registerCommand("register_tet_mesh", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected register_tet_mesh(name, vertices, tets, ...)");
    std::string name = getString(getInput(inputs, 1));
    auto verts = getMatrixFloat(getInput(inputs, 2), 3);
    auto tets = getMatrixInt(getInput(inputs, 3));
    tets.array() -= 1;
    if (tets.cols() != 4) throwError(matlabPtr, "tets must be Tx4");
    auto* vm = polyscope::registerTetMesh<Eigen::MatrixXf, Eigen::MatrixXi>(name, verts, tets);
    applyVolumeMeshOptions(vm, OptionsParser(inputs, 4, matlabPtr));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("register_hex_mesh", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected register_hex_mesh(name, vertices, hexes, ...)");
    std::string name = getString(getInput(inputs, 1));
    auto verts = getMatrixFloat(getInput(inputs, 2), 3);
    auto hexes = getMatrixInt(getInput(inputs, 3));
    hexes.array() -= 1;
    if (hexes.cols() != 8) throwError(matlabPtr, "hexes must be Hx8");
    auto* vm = polyscope::registerHexMesh<Eigen::MatrixXf, Eigen::MatrixXi>(name, verts, hexes);
    applyVolumeMeshOptions(vm, OptionsParser(inputs, 4, matlabPtr));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("register_volume_mesh", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) throwError(matlabPtr, "Expected register_volume_mesh(name, vertices, cells, ...)");
    std::string name = getString(getInput(inputs, 1));
    auto verts = getMatrixFloat(getInput(inputs, 2), 3);
    auto cells = getMatrixInt(getInput(inputs, 3));
    cells.array() -= 1;
    auto* vm = polyscope::registerVolumeMesh<Eigen::MatrixXf, Eigen::MatrixXi>(name, verts, cells);
    applyVolumeMeshOptions(vm, OptionsParser(inputs, 4, matlabPtr));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("register_tet_hex_mesh", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) throwError(matlabPtr, "Expected register_tet_hex_mesh(name, vertices, tets, hexes, ...)");
    std::string name = getString(getInput(inputs, 1));
    auto verts = getMatrixFloat(getInput(inputs, 2), 3);
    auto tets = getMatrixInt(getInput(inputs, 3));
    auto hexes = getMatrixInt(getInput(inputs, 4));
    tets.array() -= 1;
    hexes.array() -= 1;
    if (tets.cols() != 4) throwError(matlabPtr, "tets must be Tx4");
    if (hexes.cols() != 8) throwError(matlabPtr, "hexes must be Hx8");
    auto* vm = polyscope::registerTetHexMesh<Eigen::MatrixXf, Eigen::MatrixXi, Eigen::MatrixXi>(
        name, verts, tets, hexes);
    applyVolumeMeshOptions(vm, OptionsParser(inputs, 5, matlabPtr));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("has_volume_mesh", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected has_volume_mesh(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::hasVolumeMesh(getString(getInput(inputs, 1))));
  });

  reg.registerCommand("remove_volume_mesh", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected remove_volume_mesh(name)");
    polyscope::removeVolumeMesh(getString(getInput(inputs, 1)), false);
  });

  reg.registerCommand("volume_mesh_n_vertices", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_mesh_n_vertices(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    static_cast<double>(getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->nVertices()));
  });

  reg.registerCommand("volume_mesh_n_faces", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_mesh_n_faces(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    static_cast<double>(getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->nFaces()));
  });

  reg.registerCommand("volume_mesh_n_cells", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_mesh_n_cells(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    static_cast<double>(getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->nCells()));
  });

  reg.registerCommand("volume_mesh_update_vertex_positions",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 3)
                          throwError(matlabPtr, "Expected volume_mesh_update_vertex_positions(name, vertices)");
                        auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        vm->updateVertexPositions<Eigen::MatrixXf>(getMatrixFloat(getInput(inputs, 2), 3));
                      });

  reg.registerCommand("volume_mesh_set_enabled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_mesh_set_enabled(name, val)");
    getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->setEnabled(getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("volume_mesh_set_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_mesh_set_color(name, color)");
    auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    vm->setColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("volume_mesh_get_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_mesh_get_color(name)");
    glm::vec3 c = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->getColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("volume_mesh_set_interior_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_mesh_set_interior_color(name, color)");
    auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    vm->setInteriorColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("volume_mesh_get_interior_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_mesh_get_interior_color(name)");
    glm::vec3 c = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->getInteriorColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("volume_mesh_set_edge_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_mesh_set_edge_color(name, color)");
    auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    vm->setEdgeColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("volume_mesh_get_edge_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_mesh_get_edge_color(name)");
    glm::vec3 c = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->getEdgeColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("volume_mesh_set_edge_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_mesh_set_edge_width(name, width)");
    getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->setEdgeWidth(getScalarDouble(getInput(inputs, 2)));
  });

  reg.registerCommand("volume_mesh_get_edge_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_mesh_get_edge_width(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->getEdgeWidth());
  });

  reg.registerCommand("volume_mesh_set_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_mesh_set_material(name, material)");
    getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->setMaterial(getString(getInput(inputs, 2)));
  });

  reg.registerCommand("volume_mesh_get_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_mesh_get_material(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->getMaterial());
  });

  reg.registerCommand("volume_mesh_set_transparency", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_mesh_set_transparency(name, val)");
    getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)))->setTransparency(getScalarDouble(getInput(inputs, 2)));
  });

  // === Quantities ==========================================================
  reg.registerCommand("volume_mesh_add_vertex_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected volume_mesh_add_vertex_scalar_quantity(name, qname, values, ...)");
                        auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getEigenVectorFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        auto* q = vm->addVertexScalarQuantity<Eigen::VectorXf>(qname, vals, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("volume_mesh_add_cell_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected volume_mesh_add_cell_scalar_quantity(name, qname, values, ...)");
                        auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getEigenVectorFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        auto* q = vm->addCellScalarQuantity<Eigen::VectorXf>(qname, vals, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("volume_mesh_add_vertex_color_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected volume_mesh_add_vertex_color_quantity(name, qname, values, ...)");
                        auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3), 3);
                        OptionsParser parser(inputs, 4, matlabPtr);
                        auto* q = vm->addVertexColorQuantity<Eigen::MatrixXf>(qname, vals);
                        applyColorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("volume_mesh_add_cell_color_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected volume_mesh_add_cell_color_quantity(name, qname, values, ...)");
                        auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3), 3);
                        OptionsParser parser(inputs, 4, matlabPtr);
                        auto* q = vm->addCellColorQuantity<Eigen::MatrixXf>(qname, vals);
                        applyColorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("volume_mesh_add_vertex_vector_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected volume_mesh_add_vertex_vector_quantity(name, qname, values, ...)");
                        auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::VectorType vtype = polyscope::VectorType::STANDARD;
                        if (parser.has("vectortype")) vtype = parseVectorType(parser.getString("vectortype", "standard"));
                        auto* q = vm->addVertexVectorQuantity<Eigen::MatrixXf>(qname, vals, vtype);
                        applyVectorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("volume_mesh_add_cell_vector_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected volume_mesh_add_cell_vector_quantity(name, qname, values, ...)");
                        auto* vm = getVolumeMeshChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::VectorType vtype = polyscope::VectorType::STANDARD;
                        if (parser.has("vectortype")) vtype = parseVectorType(parser.getString("vectortype", "standard"));
                        auto* q = vm->addCellVectorQuantity<Eigen::MatrixXf>(qname, vals, vtype);
                        applyVectorQuantityOptions(*q, parser);
                      });
}

} // namespace ps_mex
