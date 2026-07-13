#include "mex.hpp"
#include "cppmex/detail/mexIOAdapterImpl.hpp"
#include "mex_io_helpers.h"

#include "command_router.h"
#include "matlab_data_utils.h"
#include "bind_core.h"
#include "bind_point_cloud.h"
#include "bind_surface_mesh.h"
#include "bind_curve_network.h"
#include "bind_volume_mesh.h"
#include "bind_volume_grid.h"
#include "bind_sparse_volume_grid.h"
#include "bind_camera_view.h"
#include "bind_transformation_gizmo.h"
#include "bind_slice_plane.h"
#include "bind_imgui.h"
#include "bind_implot.h"

#include "polyscope/polyscope.h"

#include <sstream>

namespace ps_mex {

const matlab::data::Array& getInput(matlab::mex::ArgumentList& inputs, size_t idx) {
    return inputs[idx];
}

matlab::data::Array& getOutput(matlab::mex::ArgumentList& outputs, size_t idx) {
    return outputs[idx];
}

size_t inputCount(matlab::mex::ArgumentList& inputs) {
    return inputs.size();
}

} // namespace ps_mex

class MexFunction : public matlab::mex::Function {
public:
  MexFunction() {
    // Register all commands once when the MEX file is loaded.
    auto& reg = ps_mex::CommandRegistry::instance();
    ps_mex::bind_core_commands(reg);
    ps_mex::bind_point_cloud_commands(reg);
    ps_mex::bind_surface_mesh_commands(reg);
    ps_mex::bind_curve_network_commands(reg);
    ps_mex::bind_volume_mesh_commands(reg);
    ps_mex::bind_volume_grid_commands(reg);
    ps_mex::bind_sparse_volume_grid_commands(reg);
    ps_mex::bind_camera_view_commands(reg);
    ps_mex::bind_transformation_gizmo_commands(reg);
    ps_mex::bind_slice_plane_commands(reg);
    ps_mex::bind_imgui_commands(reg);
    ps_mex::bind_implot_commands(reg);
  }

  void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
    std::shared_ptr<matlab::engine::MATLABEngine> matlabPtr = getEngine();

    try {
      if (ps_mex::inputCount(inputs) == 0) {
        ps_mex::throwError(matlabPtr.get(), "Usage: polyscope_mex('command', ...);");
      }

      std::string cmd = ps_mex::getString(ps_mex::getInput(inputs, 0));
      auto func = ps_mex::CommandRegistry::instance().find(cmd);
      if (!func) {
        std::ostringstream oss;
        oss << "Unknown polyscope command: " << cmd;
        ps_mex::throwError(matlabPtr.get(), oss.str());
      }

      func(outputs, inputs, matlabPtr.get());
    } catch (const std::exception& e) {
      ps_mex::throwError(matlabPtr.get(), e.what());
    }
  }
};
#include "mexAdapter.hpp"
