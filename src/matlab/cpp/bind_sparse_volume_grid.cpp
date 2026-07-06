#include "bind_sparse_volume_grid.h"
#include "matlab_data_utils.h"
#include "quantity_options.h"

#include "polyscope/polyscope.h"
#include "polyscope/sparse_volume_grid.h"

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

polyscope::SparseVolumeGrid* getSparseVolumeGridChecked(MATLABEngine* matlabPtr, const std::string& name) {
  polyscope::SparseVolumeGrid* svg = polyscope::getSparseVolumeGrid(name);
  if (!svg) {
    throwError(matlabPtr, "No sparse volume grid with name: " + name);
  }
  return svg;
}

polyscope::SparseVolumeGridRenderMode parseSparseVolumeGridRenderMode(const std::string& s) {
  std::string t = s;
  std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return std::tolower(c); });
  if (t == "gridcube") return polyscope::SparseVolumeGridRenderMode::Gridcube;
  if (t == "wireframe") return polyscope::SparseVolumeGridRenderMode::Wireframe;
  throw std::runtime_error("Unknown sparse volume grid render mode: " + s);
}

std::vector<glm::ivec3> toIvec3Vector(const Eigen::MatrixXi& m) {
  std::vector<glm::ivec3> out;
  out.reserve(m.rows());
  for (Eigen::Index i = 0; i < m.rows(); ++i) {
    out.emplace_back(m(i, 0), m(i, 1), m(i, 2));
  }
  return out;
}

std::vector<glm::vec3> toVec3Vector(const Eigen::MatrixXf& m) {
  std::vector<glm::vec3> out;
  out.reserve(m.rows());
  for (Eigen::Index i = 0; i < m.rows(); ++i) {
    out.emplace_back(m(i, 0), m(i, 1), m(i, 2));
  }
  return out;
}

} // namespace

void bind_sparse_volume_grid_commands(CommandRegistry& reg) {
  reg.registerCommand("register_sparse_volume_grid", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) {
      throwError(matlabPtr, "Expected register_sparse_volume_grid(name, origin, cell_width, occupied_cells, ...)");
    }
    std::string name = getString(getInput(inputs, 1));
    auto origin = getMatrixFloat(getInput(inputs, 2), 3);
    auto cellWidth = getMatrixFloat(getInput(inputs, 3), 3);
    auto occCells = getMatrixInt(getInput(inputs, 4));
    if (occCells.cols() != 3) throwError(matlabPtr, "occupied_cells must be Cx3");
    occCells.array() -= 1;  // MATLAB 1-based to Polyscope 0-based

    glm::vec3 o(origin(0, 0), origin(0, 1), origin(0, 2));
    glm::vec3 cw(cellWidth(0, 0), cellWidth(0, 1), cellWidth(0, 2));
    auto occ = toIvec3Vector(occCells);

    auto* svg = polyscope::registerSparseVolumeGrid<std::vector<glm::ivec3>>(name, o, cw, occ);

    OptionsParser parser(inputs, 5, matlabPtr);
    if (parser.has("enabled")) svg->setEnabled(parser.getBool("enabled", true));
    if (parser.has("color")) svg->setColor(detail::parseVec3(parser.getVec3("color", Eigen::Vector3f::Zero())));
    if (parser.has("edge_color"))
      svg->setEdgeColor(detail::parseVec3(parser.getVec3("edge_color", Eigen::Vector3f::Zero())));
    if (parser.has("edge_width")) svg->setEdgeWidth(parser.getDouble("edge_width", 0.0));
    if (parser.has("material")) svg->setMaterial(parser.getString("material", ""));
    if (parser.has("cube_size_factor")) svg->setCubeSizeFactor(parser.getDouble("cube_size_factor", 1.0));
    if (parser.has("render_mode")) svg->setRenderMode(parseSparseVolumeGridRenderMode(parser.getString("render_mode", "gridcube")));
    if (parser.has("wireframe_radius")) svg->setWireframeRadius(parser.getDouble("wireframe_radius", 1.0));
    if (parser.has("wireframe_color"))
      svg->setWireframeColor(detail::parseVec3(parser.getVec3("wireframe_color", Eigen::Vector3f::Zero())));

    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("has_sparse_volume_grid", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected has_sparse_volume_grid(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::hasSparseVolumeGrid(getString(getInput(inputs, 1))));
  });

  reg.registerCommand("remove_sparse_volume_grid", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected remove_sparse_volume_grid(name)");
    polyscope::removeSparseVolumeGrid(getString(getInput(inputs, 1)), false);
  });

  reg.registerCommand("sparse_volume_grid_n_cells", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_n_cells(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(
                                                getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->nCells()));
  });

  reg.registerCommand("sparse_volume_grid_n_nodes", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_n_nodes(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(
                                                getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->nNodes()));
  });

  reg.registerCommand("sparse_volume_grid_get_origin", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_get_origin(name)");
    auto o = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getOrigin();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {o.x, o.y, o.z});
  });

  reg.registerCommand("sparse_volume_grid_get_grid_cell_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_get_grid_cell_width(name)");
    auto w = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getGridCellWidth();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {w.x, w.y, w.z});
  });

  reg.registerCommand("sparse_volume_grid_get_occupied_cells", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_get_occupied_cells(name)");
    auto cells = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getOccupiedCells();
    Eigen::MatrixXd m(static_cast<Eigen::Index>(cells.size()), 3);
    for (size_t i = 0; i < cells.size(); ++i) {
      m(static_cast<Eigen::Index>(i), 0) = static_cast<double>(cells[i].x + 1);
      m(static_cast<Eigen::Index>(i), 1) = static_cast<double>(cells[i].y + 1);
      m(static_cast<Eigen::Index>(i), 2) = static_cast<double>(cells[i].z + 1);
    }
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createMatrixDouble(factory, m);
  });

  reg.registerCommand("sparse_volume_grid_set_enabled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected sparse_volume_grid_set_enabled(name, val)");
    getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->setEnabled(getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("sparse_volume_grid_set_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected sparse_volume_grid_set_color(name, color)");
    auto* svg = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    svg->setColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("sparse_volume_grid_get_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_get_color(name)");
    glm::vec3 c = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("sparse_volume_grid_set_edge_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected sparse_volume_grid_set_edge_color(name, color)");
    auto* svg = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    svg->setEdgeColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("sparse_volume_grid_get_edge_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_get_edge_color(name)");
    glm::vec3 c = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getEdgeColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("sparse_volume_grid_set_edge_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected sparse_volume_grid_set_edge_width(name, width)");
    getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->setEdgeWidth(getScalarDouble(getInput(inputs, 2)));
  });

  reg.registerCommand("sparse_volume_grid_get_edge_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_get_edge_width(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getEdgeWidth());
  });

  reg.registerCommand("sparse_volume_grid_set_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected sparse_volume_grid_set_material(name, material)");
    getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->setMaterial(getString(getInput(inputs, 2)));
  });

  reg.registerCommand("sparse_volume_grid_get_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_get_material(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getMaterial());
  });

  reg.registerCommand("sparse_volume_grid_set_cube_size_factor", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected sparse_volume_grid_set_cube_size_factor(name, val)");
    getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->setCubeSizeFactor(getScalarDouble(getInput(inputs, 2)));
  });

  reg.registerCommand("sparse_volume_grid_get_cube_size_factor", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_get_cube_size_factor(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getCubeSizeFactor());
  });

  reg.registerCommand("sparse_volume_grid_set_render_mode", [](ArgumentList& outputs, ArgumentList& inputs,
                                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected sparse_volume_grid_set_render_mode(name, mode)");
    getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))
        ->setRenderMode(parseSparseVolumeGridRenderMode(getString(getInput(inputs, 2))));
  });

  reg.registerCommand("sparse_volume_grid_set_wireframe_radius", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected sparse_volume_grid_set_wireframe_radius(name, val)");
    getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->setWireframeRadius(getScalarDouble(getInput(inputs, 2)));
  });

  reg.registerCommand("sparse_volume_grid_set_wireframe_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected sparse_volume_grid_set_wireframe_color(name, color)");
    auto* svg = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    svg->setWireframeColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("sparse_volume_grid_mark_nodes_as_used", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected sparse_volume_grid_mark_nodes_as_used(name)");
    getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->markNodesAsUsed();
  });

  // === Quantities ==========================================================
  reg.registerCommand("sparse_volume_grid_add_cell_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected sparse_volume_grid_add_cell_scalar_quantity(name, qname, values, ...)");
                        auto* svg = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getEigenVectorFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        auto* q = svg->addCellScalarQuantity<Eigen::VectorXf>(qname, vals, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("sparse_volume_grid_add_node_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 5)
                          throwError(matlabPtr,
                                     "Expected sparse_volume_grid_add_node_scalar_quantity(name, qname, node_indices, values, ...)");
                        auto* svg = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto nodeIndices = getMatrixInt(getInput(inputs, 3));
                        if (nodeIndices.cols() != 3) throwError(matlabPtr, "node_indices must be Nx3");
                        nodeIndices.array() -= 1;  // MATLAB 1-based to Polyscope 0-based
                        auto vals = getEigenVectorFloat(getInput(inputs, 4));
                        if (nodeIndices.rows() != vals.size())
                          throwError(matlabPtr, "node_indices and values must have the same length");
                        OptionsParser parser(inputs, 5, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        auto indices = toIvec3Vector(nodeIndices);
                        auto* q = svg->addNodeScalarQuantity<std::vector<glm::ivec3>, Eigen::VectorXf>(qname, indices, vals, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("sparse_volume_grid_add_cell_color_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected sparse_volume_grid_add_cell_color_quantity(name, qname, colors, ...)");
                        auto* svg = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto colors = getMatrixFloat(getInput(inputs, 3), 3);
                        OptionsParser parser(inputs, 4, matlabPtr);
                        auto* q = svg->addCellColorQuantity<std::vector<glm::vec3>>(qname, toVec3Vector(colors));
                        applyColorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("sparse_volume_grid_add_node_color_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 5)
                          throwError(matlabPtr,
                                     "Expected sparse_volume_grid_add_node_color_quantity(name, qname, node_indices, colors, ...)");
                        auto* svg = getSparseVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto nodeIndices = getMatrixInt(getInput(inputs, 3));
                        if (nodeIndices.cols() != 3) throwError(matlabPtr, "node_indices must be Nx3");
                        nodeIndices.array() -= 1;  // MATLAB 1-based to Polyscope 0-based
                        auto colors = getMatrixFloat(getInput(inputs, 4), 3);
                        if (nodeIndices.rows() != colors.rows())
                          throwError(matlabPtr, "node_indices and colors must have the same length");
                        OptionsParser parser(inputs, 5, matlabPtr);
                        auto indices = toIvec3Vector(nodeIndices);
                        auto* q = svg->addNodeColorQuantity<std::vector<glm::ivec3>, std::vector<glm::vec3>>(
                            qname, indices, toVec3Vector(colors));
                        applyColorQuantityOptions(*q, parser);
                      });
}

} // namespace ps_mex
