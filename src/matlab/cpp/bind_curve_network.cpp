#include "bind_curve_network.h"
#include "matlab_data_utils.h"
#include "quantity_options.h"

#include "polyscope/curve_network.h"
#include "polyscope/polyscope.h"

namespace ps_mex {

namespace {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

polyscope::CurveNetwork* getCurveNetworkChecked(MATLABEngine* matlabPtr, const std::string& name) {
  polyscope::CurveNetwork* cn = polyscope::getCurveNetwork(name);
  if (!cn) {
    throwError(matlabPtr, "No curve network with name: " + name);
  }
  return cn;
}

} // namespace

void bind_curve_network_commands(CommandRegistry& reg) {
  reg.registerCommand("register_curve_network", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 4) {
      throwError(matlabPtr, "Expected register_curve_network(name, nodes, edges, ...)");
    }
    std::string name = getString(getInput(inputs, 1));
    auto nodes = getMatrixFloat(getInput(inputs, 2));
    auto edges = getMatrixInt(getInput(inputs, 3));
    edges.array() -= 1;  // MATLAB 1-based to Polyscope 0-based
    if (nodes.cols() != 2 && nodes.cols() != 3) {
      throwError(matlabPtr, "nodes must be Nx2 or Nx3");
    }
    if (edges.cols() != 2) {
      throwError(matlabPtr, "edges must be Ex2");
    }

    polyscope::CurveNetwork* cn = nullptr;
    if (nodes.cols() == 3) {
      cn = polyscope::registerCurveNetwork<Eigen::MatrixXf, Eigen::MatrixXi>(name, nodes, edges);
    } else {
      cn = polyscope::registerCurveNetwork2D<Eigen::MatrixXf, Eigen::MatrixXi>(name, nodes, edges);
    }
    cn->setMaterial("flat");

    OptionsParser parser(inputs, 4, matlabPtr);
    if (parser.has("enabled")) cn->setEnabled(parser.getBool("enabled", true));
    if (parser.has("radius")) cn->setRadius(parser.getDouble("radius", 1.0), true);
    if (parser.has("color")) {
      auto c = parser.getVec3("color", Eigen::Vector3f::Zero());
      cn->setColor(glm::vec3(c[0], c[1], c[2]));
    }
    if (parser.has("material")) cn->setMaterial(parser.getString("material", ""));
    if (parser.has("transparency")) cn->setTransparency(parser.getDouble("transparency", 1.0));

    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("register_curve_network_line", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected register_curve_network_line(name, nodes, ...)");
    std::string name = getString(getInput(inputs, 1));
    auto nodes = getMatrixFloat(getInput(inputs, 2));
    if (nodes.cols() != 2 && nodes.cols() != 3) {
      throwError(matlabPtr, "nodes must be Nx2 or Nx3");
    }
    polyscope::CurveNetwork* cn = (nodes.cols() == 3)
                                      ? polyscope::registerCurveNetworkLine<Eigen::MatrixXf>(name, nodes)
                                      : polyscope::registerCurveNetworkLine2D<Eigen::MatrixXf>(name, nodes);
    cn->setMaterial("flat");
    OptionsParser parser(inputs, 3, matlabPtr);
    if (parser.has("enabled")) cn->setEnabled(parser.getBool("enabled", true));
    if (parser.has("radius")) cn->setRadius(parser.getDouble("radius", 1.0), true);
    if (parser.has("color")) cn->setColor(detail::parseVec3(parser.getVec3("color", Eigen::Vector3f::Zero())));
    if (parser.has("material")) cn->setMaterial(parser.getString("material", ""));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("register_curve_network_loop", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected register_curve_network_loop(name, nodes, ...)");
    std::string name = getString(getInput(inputs, 1));
    auto nodes = getMatrixFloat(getInput(inputs, 2));
    if (nodes.cols() != 2 && nodes.cols() != 3) {
      throwError(matlabPtr, "nodes must be Nx2 or Nx3");
    }
    polyscope::CurveNetwork* cn = (nodes.cols() == 3)
                                      ? polyscope::registerCurveNetworkLoop<Eigen::MatrixXf>(name, nodes)
                                      : polyscope::registerCurveNetworkLoop2D<Eigen::MatrixXf>(name, nodes);
    cn->setMaterial("flat");
    OptionsParser parser(inputs, 3, matlabPtr);
    if (parser.has("enabled")) cn->setEnabled(parser.getBool("enabled", true));
    if (parser.has("radius")) cn->setRadius(parser.getDouble("radius", 1.0), true);
    if (parser.has("color")) cn->setColor(detail::parseVec3(parser.getVec3("color", Eigen::Vector3f::Zero())));
    if (parser.has("material")) cn->setMaterial(parser.getString("material", ""));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("register_curve_network_segments", [](ArgumentList& outputs, ArgumentList& inputs,
                                                            MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected register_curve_network_segments(name, nodes, ...)");
    std::string name = getString(getInput(inputs, 1));
    auto nodes = getMatrixFloat(getInput(inputs, 2));
    if (nodes.cols() != 2 && nodes.cols() != 3) {
      throwError(matlabPtr, "nodes must be Nx2 or Nx3");
    }
    polyscope::CurveNetwork* cn = (nodes.cols() == 3)
                                      ? polyscope::registerCurveNetworkSegments<Eigen::MatrixXf>(name, nodes)
                                      : polyscope::registerCurveNetworkSegments2D<Eigen::MatrixXf>(name, nodes);
    cn->setMaterial("flat");
    OptionsParser parser(inputs, 3, matlabPtr);
    if (parser.has("enabled")) cn->setEnabled(parser.getBool("enabled", true));
    if (parser.has("radius")) cn->setRadius(parser.getDouble("radius", 1.0), true);
    if (parser.has("color")) cn->setColor(detail::parseVec3(parser.getVec3("color", Eigen::Vector3f::Zero())));
    if (parser.has("material")) cn->setMaterial(parser.getString("material", ""));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(name);
  });

  reg.registerCommand("has_curve_network", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected has_curve_network(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::hasCurveNetwork(getString(getInput(inputs, 1))));
  });

  reg.registerCommand("remove_curve_network", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected remove_curve_network(name)");
    polyscope::removeCurveNetwork(getString(getInput(inputs, 1)), false);
  });

  reg.registerCommand("curve_network_n_nodes", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected curve_network_n_nodes(name)");
    auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(cn->nNodes()));
  });

  reg.registerCommand("curve_network_n_edges", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected curve_network_n_edges(name)");
    auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(cn->nEdges()));
  });

  reg.registerCommand("curve_network_update_node_positions",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 3)
                          throwError(matlabPtr, "Expected curve_network_update_node_positions(name, nodes)");
                        auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
                        auto nodes = getMatrixFloat(getInput(inputs, 2));
                        if (nodes.cols() == 3) {
                          cn->updateNodePositions<Eigen::MatrixXf>(nodes);
                        } else if (nodes.cols() == 2) {
                          cn->updateNodePositions2D<Eigen::MatrixXf>(nodes);
                        } else {
                          throwError(matlabPtr, "nodes must be Nx2 or Nx3");
                        }
                      });

  reg.registerCommand("curve_network_set_enabled", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected curve_network_set_enabled(name, val)");
    getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))->setEnabled(getScalarBool(getInput(inputs, 2)));
  });

  reg.registerCommand("curve_network_set_radius", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected curve_network_set_radius(name, val)");
    bool relative = true;
    if (inputCount(inputs) > 3) relative = getScalarBool(getInput(inputs, 3));
    getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))->setRadius(getScalarDouble(getInput(inputs, 2)), relative);
  });

  reg.registerCommand("curve_network_get_radius", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected curve_network_get_radius(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))->getRadius());
  });

  reg.registerCommand("curve_network_set_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected curve_network_set_color(name, color)");
    auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
    auto c = getMatrixFloat(getInput(inputs, 2), 3);
    cn->setColor(glm::vec3(c(0, 0), c(0, 1), c(0, 2)));
  });

  reg.registerCommand("curve_network_get_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected curve_network_get_color(name)");
    glm::vec3 c = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))->getColor();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.r, c.g, c.b});
  });

  reg.registerCommand("curve_network_set_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected curve_network_set_material(name, material)");
    getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))->setMaterial(getString(getInput(inputs, 2)));
  });

  reg.registerCommand("curve_network_get_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected curve_network_get_material(name)");
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))->getMaterial());
  });

  reg.registerCommand("curve_network_set_transparency", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 3) throwError(matlabPtr, "Expected curve_network_set_transparency(name, val)");
    getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))->setTransparency(getScalarDouble(getInput(inputs, 2)));
  });

  reg.registerCommand("curve_network_set_node_radius_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 3)
                          throwError(matlabPtr, "Expected curve_network_set_node_radius_quantity(name, qname)");
                        bool autoscale = true;
                        if (inputCount(inputs) > 3) autoscale = getScalarBool(getInput(inputs, 3));
                        getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))
                            ->setNodeRadiusQuantity(getString(getInput(inputs, 2)), autoscale);
                      });

  reg.registerCommand("curve_network_clear_node_radius_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected curve_network_clear_node_radius_quantity(name)");
                        getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))->clearNodeRadiusQuantity();
                      });

  reg.registerCommand("curve_network_set_edge_radius_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 3)
                          throwError(matlabPtr, "Expected curve_network_set_edge_radius_quantity(name, qname)");
                        bool autoscale = true;
                        if (inputCount(inputs) > 3) autoscale = getScalarBool(getInput(inputs, 3));
                        getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))
                            ->setEdgeRadiusQuantity(getString(getInput(inputs, 2)), autoscale);
                      });

  reg.registerCommand("curve_network_clear_edge_radius_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected curve_network_clear_edge_radius_quantity(name)");
                        getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)))->clearEdgeRadiusQuantity();
                      });

  // === Quantities ==========================================================
  reg.registerCommand("curve_network_add_node_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected curve_network_add_node_scalar_quantity(name, qname, values, ...)");
                        auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getEigenVectorFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        auto* q = cn->addNodeScalarQuantity<Eigen::VectorXf>(qname, vals, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("curve_network_add_edge_scalar_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected curve_network_add_edge_scalar_quantity(name, qname, values, ...)");
                        auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getEigenVectorFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::DataType dtype = polyscope::DataType::STANDARD;
                        if (parser.has("datatype")) dtype = parseDataType(parser.getString("datatype", "standard"));
                        auto* q = cn->addEdgeScalarQuantity<Eigen::VectorXf>(qname, vals, dtype);
                        applyScalarQuantityOptions(*q, parser);
                      });

  reg.registerCommand("curve_network_add_node_color_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected curve_network_add_node_color_quantity(name, qname, values, ...)");
                        auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3), 3);
                        OptionsParser parser(inputs, 4, matlabPtr);
                        auto* q = cn->addNodeColorQuantity<Eigen::MatrixXf>(qname, vals);
                        applyColorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("curve_network_add_edge_color_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected curve_network_add_edge_color_quantity(name, qname, values, ...)");
                        auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3), 3);
                        OptionsParser parser(inputs, 4, matlabPtr);
                        auto* q = cn->addEdgeColorQuantity<Eigen::MatrixXf>(qname, vals);
                        applyColorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("curve_network_add_node_vector_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected curve_network_add_node_vector_quantity(name, qname, values, ...)");
                        auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::VectorType vtype = polyscope::VectorType::STANDARD;
                        if (parser.has("vectortype")) vtype = parseVectorType(parser.getString("vectortype", "standard"));
                        polyscope::CurveNetworkNodeVectorQuantity* q = nullptr;
                        if (vals.cols() == 3) {
                          q = cn->addNodeVectorQuantity<Eigen::MatrixXf>(qname, vals, vtype);
                        } else if (vals.cols() == 2) {
                          q = cn->addNodeVectorQuantity2D<Eigen::MatrixXf>(qname, vals, vtype);
                        } else {
                          throwError(matlabPtr, "vector values must be Nx2 or Nx3");
                        }
                        applyVectorQuantityOptions(*q, parser);
                      });

  reg.registerCommand("curve_network_add_edge_vector_quantity",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 4)
                          throwError(matlabPtr,
                                     "Expected curve_network_add_edge_vector_quantity(name, qname, values, ...)");
                        auto* cn = getCurveNetworkChecked(matlabPtr, getString(getInput(inputs, 1)));
                        std::string qname = getString(getInput(inputs, 2));
                        auto vals = getMatrixFloat(getInput(inputs, 3));
                        OptionsParser parser(inputs, 4, matlabPtr);
                        polyscope::VectorType vtype = polyscope::VectorType::STANDARD;
                        if (parser.has("vectortype")) vtype = parseVectorType(parser.getString("vectortype", "standard"));
                        polyscope::CurveNetworkEdgeVectorQuantity* q = nullptr;
                        if (vals.cols() == 3) {
                          q = cn->addEdgeVectorQuantity<Eigen::MatrixXf>(qname, vals, vtype);
                        } else if (vals.cols() == 2) {
                          q = cn->addEdgeVectorQuantity2D<Eigen::MatrixXf>(qname, vals, vtype);
                        } else {
                          throwError(matlabPtr, "vector values must be Nx2 or Nx3");
                        }
                        applyVectorQuantityOptions(*q, parser);
                      });
}

} // namespace ps_mex
