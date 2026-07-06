#include "bind_volume_grid.h"
#include "matlab_data_utils.h"
#include "quantity_options.h"

#include "polyscope/polyscope.h"
#include "polyscope/volume_grid.h"

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

polyscope::VolumeGrid* getVolumeGridChecked(MATLABEngine* matlabPtr, const std::string& name) {
  polyscope::VolumeGrid* vg = polyscope::getVolumeGrid(name);
  if (!vg) {
    throwError(matlabPtr, "No volume grid with name: " + name);
  }
  return vg;
}

// Flatten a 3D MATLAB array in Fortran order (x changes fastest).
// The input is assumed to be indexed as array(xInd, yInd, zInd) in MATLAB,
// which is already column-major with x changing fastest.
std::vector<float> flattenVolumeData(const Array& arr) {
  auto dims = arr.getDimensions();
  if (dims.size() != 3) {
    throw std::runtime_error("Expected 3D array for volume grid data");
  }
  // MATLAB dims are [xDim, yDim, zDim]; column-major means x changes fastest.
  // Polyscope expects the same layout, so we can just read linearly.
  auto v = getVectorFloat(arr);
  return v;
}

} // namespace

void bind_volume_grid_commands(CommandRegistry& reg) {
  reg.registerCommand("register_volume_grid", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 5) {
      throwError(matlabPtr, "Expected register_volume_grid(name, node_dims, bound_low, bound_high, ...)");
    }
    std::string name = getString(getInput(inputs, 1));
    auto nodeDims = getVectorUInt32(getInput(inputs, 2));
    if (nodeDims.size() != 3) throwError(matlabPtr, "node_dims must be a 3-element vector");
    auto boundLow = getMatrixFloat(getInput(inputs, 3), 3);
    auto boundHigh = getMatrixFloat(getInput(inputs, 4), 3);

    glm::uvec3 dims(nodeDims[0], nodeDims[1], nodeDims[2]);
    glm::vec3 low(boundLow(0, 0), boundLow(0, 1), boundLow(0, 2));
    glm::vec3 high(boundHigh(0, 0), boundHigh(0, 1), boundHigh(0, 2));

    auto* vg = polyscope::registerVolumeGrid(name, dims, low, high);

    OptionsParser parser(inputs, 5, matlabPtr);
    if (parser.has("enabled")) vg->setEnabled(parser.getBool("enabled", true));
    if (parser.has("color")) vg->setColor(detail::parseVec3(parser.getVec3("color", Eigen::Vector3f::Zero())));
    if (parser.has("edge_color"))
      vg->setEdgeColor(detail::parseVec3(parser.getVec3("edge_color", Eigen::Vector3f::Zero())));
    if (parser.has("edge_width")) vg->setEdgeWidth(parser.getDouble("edge_width", 0.0));
    if (parser.has("material")) vg->setMaterial(parser.getString("material", ""));
    if (parser.has("cube_size_factor")) vg->setCubeSizeFactor(parser.getDouble("cube_size_factor", 1.0));

    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("has_volume_grid", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected has_volume_grid(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::hasVolumeGrid(getString(getInput(inputs, 1))));
  });

  reg.registerCommand("remove_volume_grid", [](ArgumentList& outputs, ArgumentList& inputs,
                                               MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected remove_volume_grid(name)");
    polyscope::removeVolumeGrid(getString(getInput(inputs, 1)));
  });

  reg.registerCommand("volume_grid_n_nodes", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_n_nodes(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    static_cast<double>(getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->nNodes()));
  });

  reg.registerCommand("volume_grid_n_cells", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_n_cells(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    static_cast<double>(getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->nCells()));
  });

  reg.registerCommand("volume_grid_grid_spacing", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_grid_spacing(name)");
    auto s = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->gridSpacing();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {s.x, s.y, s.z});
  });

  reg.registerCommand("volume_grid_get_grid_node_dim", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_get_grid_node_dim(name)");
    auto d = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getGridNodeDim();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {static_cast<double>(d.x), static_cast<double>(d.y), static_cast<double>(d.z)});
  });

  reg.registerCommand("volume_grid_get_grid_cell_dim", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_get_grid_cell_dim(name)");
    auto d = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getGridCellDim();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {static_cast<double>(d.x), static_cast<double>(d.y), static_cast<double>(d.z)});
  });

  reg.registerCommand("volume_grid_get_bound_min", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_get_bound_min(name)");
    auto b = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getBoundMin();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {b.x, b.y, b.z});
  });

  reg.registerCommand("volume_grid_get_bound_max", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_get_bound_max(name)");
    auto b = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getBoundMax();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {b.x, b.y, b.z});
  });

  reg.registerCommand("volume_grid_set_enabled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_grid_set_enabled(name, val)");
    getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->setEnabled(getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("volume_grid_set_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_grid_set_color(name, color)");
    auto* vg = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    vg->setColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("volume_grid_get_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_get_color(name)");
    glm::vec3 c = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("volume_grid_set_edge_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_grid_set_edge_color(name, color)");
    auto* vg = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    vg->setEdgeColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("volume_grid_get_edge_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_get_edge_color(name)");
    glm::vec3 c = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getEdgeColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("volume_grid_set_edge_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_grid_set_edge_width(name, width)");
    getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->setEdgeWidth(getScalarDouble(getInput(inputs, 2)));
  });

  reg.registerCommand("volume_grid_get_edge_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_get_edge_width(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getEdgeWidth());
  });

  reg.registerCommand("volume_grid_set_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_grid_set_material(name, material)");
    getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->setMaterial(getString(getInput(inputs, 2)));
  });

  reg.registerCommand("volume_grid_get_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_get_material(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getMaterial());
  });

  reg.registerCommand("volume_grid_set_cube_size_factor", [](ArgumentList& outputs, ArgumentList& inputs,
                                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected volume_grid_set_cube_size_factor(name, val)");
    getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->setCubeSizeFactor(getScalarDouble(getInput(inputs, 2)));
  });

  reg.registerCommand("volume_grid_get_cube_size_factor", [](ArgumentList& outputs, ArgumentList& inputs,
                                                             MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_get_cube_size_factor(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory,
                                    getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->getCubeSizeFactor());
  });

  reg.registerCommand("volume_grid_mark_nodes_as_used", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_mark_nodes_as_used(name)");
    getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->markNodesAsUsed();
  });

  reg.registerCommand("volume_grid_mark_cells_as_used", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected volume_grid_mark_cells_as_used(name)");
    getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)))->markCellsAsUsed();
  });

  // === Quantities ==========================================================
  reg.registerCommand("volume_grid_add_node_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected volume_grid_add_node_scalar_quantity(name, qname, values, ...)");
                        auto* vg = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = flattenVolumeData(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        Eigen::VectorXf ev(vals.size());
                        for (size_t i = 0; i < vals.size(); ++i) ev(i) = vals[i];
                        auto* q = vg->addNodeScalarQuantity<Eigen::VectorXf>(qname, ev, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("volume_grid_add_cell_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected volume_grid_add_cell_scalar_quantity(name, qname, values, ...)");
                        auto* vg = getVolumeGridChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = flattenVolumeData(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        Eigen::VectorXf ev(vals.size());
                        for (size_t i = 0; i < vals.size(); ++i) ev(i) = vals[i];
                        auto* q = vg->addCellScalarQuantity<Eigen::VectorXf>(qname, ev, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });
}

} // namespace ps_mex
