#include "bind_core.h"
#include "matlab_data_utils.h"

#include "polyscope/polyscope.h"
#include "polyscope/view.h"
#include "polyscope/options.h"
#include "imgui_internal.h"
#include "polyscope/render/engine.h"
#include "polyscope/messages.h"
#include "polyscope/widget.h"
#include "polyscope/weak_handle.h"
#include <glm/glm.hpp>

#include <glad/glad.h>

#include "stb_image_write.h"

#include <vector>
#include <cstdint>
#include <cstring>

// Forward declarations for Polyscope internals used in the split-frame API.
namespace polyscope {
  void renderScene();
  void renderSceneToScreen();
}

namespace ps_mex {

using matlab::mex::ArgumentList;
using matlab::engine::MATLABEngine;

namespace {

void checkNArgs(matlab::engine::MATLABEngine* matlabPtr, size_t actual, size_t expected,
                const std::string& msg = "") {
  if (actual != expected) {
    std::ostringstream oss;
    oss << "Expected " << expected << " arguments, got " << actual;
    if (!msg.empty()) oss << ": " << msg;
    throwError(matlabPtr, oss.str());
  }
}

void checkMinArgs(matlab::engine::MATLABEngine* matlabPtr, size_t actual, size_t min,
                  const std::string& msg = "") {
  if (actual < min) {
    std::ostringstream oss;
    oss << "Expected at least " << min << " arguments, got " << actual;
    if (!msg.empty()) oss << ": " << msg;
    throwError(matlabPtr, oss.str());
  }
}

// Replicate the camera/scene input handling that Polyscope's normal
// mainLoopIteration() performs inside processInputEvents(). When the user
// drives the render loop manually via frame_begin()/frame_end(), this
// function is called so the 3D scene remains interactive (rotate, pan, zoom).
void processInputEventsSplitFrame() {
  if (!polyscope::options::doDefaultMouseInteraction) return;

  ImGuiIO& io = ImGui::GetIO();

  if (ImGui::IsAnyMouseDown()) {
    polyscope::requestRedraw();
  }

  if (!io.WantCaptureMouse) {
    // Scroll to zoom / shift clip plane
    float xoffset = io.MouseWheelH;
    float yoffset = io.MouseWheel;
    float scrollOffset = yoffset;
    float clipPlaneOffset = std::abs(xoffset) > std::abs(yoffset) ? xoffset : yoffset;

    bool scrollClipPlane = io.KeyShift && !io.KeyCtrl;
    if (scrollClipPlane && clipPlaneOffset != 0.0f) {
      polyscope::view::processClipPlaneShift(clipPlaneOffset);
      polyscope::requestRedraw();
    }
    if (!scrollClipPlane && scrollOffset != 0.0f) {
      polyscope::view::processZoom(0.5f * scrollOffset);
      polyscope::requestRedraw();
    }

    // Mouse drag to rotate / translate / zoom
    bool dragLeft = ImGui::IsMouseDragging(0);
    bool dragRight = !dragLeft && ImGui::IsMouseDragging(1);
    if (dragLeft || dragRight) {
      glm::vec2 dragDelta{io.MouseDelta.x / polyscope::view::windowWidth,
                          -io.MouseDelta.y / polyscope::view::windowHeight};

      bool isRotate = dragLeft && !io.KeyShift && !io.KeyCtrl;
      bool isTranslate = (dragLeft && io.KeyShift && !io.KeyCtrl) || dragRight;
      bool isDragZoom = dragLeft && io.KeyShift && io.KeyCtrl;

      if (isDragZoom) {
        polyscope::view::processZoom(dragDelta.y * 5.0f);
      }
      if (isRotate) {
        glm::vec2 currPos{io.MousePos.x / polyscope::view::windowWidth,
                          (polyscope::view::windowHeight - io.MousePos.y) / polyscope::view::windowHeight};
        currPos = (currPos * 2.0f) - glm::vec2{1.0f, 1.0f};
        if (std::abs(currPos.x) <= 1.0f && std::abs(currPos.y) <= 1.0f) {
          polyscope::view::processRotate(currPos - 2.0f * dragDelta, currPos);
        }
      }
      if (isTranslate) {
        polyscope::view::processTranslate(dragDelta);
      }
    }
  }

  if (!io.WantCaptureKeyboard) {
    polyscope::view::processKeyboardNavigation(io);
  }
}

} // namespace

void bind_core_commands(CommandRegistry& reg) {
  // === Basic lifecycle =====================================================
  reg.registerCommand("init", [](ArgumentList& outputs, ArgumentList& inputs,
                                 matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 1);
    std::string backend = "auto";
    if (inputCount(inputs) > 1) backend = getString(getInput(inputs, 1));
    polyscope::init(backend);
  });

  reg.registerCommand("check_initialized", [](ArgumentList& outputs, ArgumentList& inputs,
                                              matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::checkInitialized();
  });

  reg.registerCommand("is_initialized", [](ArgumentList& outputs, ArgumentList& inputs,
                                           matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::isInitialized());
  });

  reg.registerCommand("show", [](ArgumentList& outputs, ArgumentList& inputs,
                                 matlab::engine::MATLABEngine* matlabPtr) {
    size_t forFrames = std::numeric_limits<size_t>::max();
    if (inputCount(inputs) > 1) {
      forFrames = static_cast<size_t>(getScalarInt(getInput(inputs, 1)));
    }
    polyscope::show(forFrames);
  });

  reg.registerCommand("unshow", [](ArgumentList& outputs, ArgumentList& inputs,
                                   matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::unshow();
  });

  reg.registerCommand("window_requests_close", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::windowRequestsClose());
  });

  reg.registerCommand("frame_tick", [](ArgumentList& outputs, ArgumentList& inputs,
                                       matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::frameTick();
  });

  reg.registerCommand("show_window", [](ArgumentList& outputs, ArgumentList& inputs,
                                        matlab::engine::MATLABEngine* matlabPtr) {
    if (polyscope::render::engine) {
      polyscope::render::engine->showWindow();
    }
  });

  reg.registerCommand("focus_window", [](ArgumentList& outputs, ArgumentList& inputs,
                                         matlab::engine::MATLABEngine* matlabPtr) {
    if (polyscope::render::engine) {
      polyscope::render::engine->focusWindow();
    }
  });

  // === Split-frame API ======================================================
  // These commands let MATLAB inject ImGui code during a frame without
  // requiring a C++ -> MATLAB callback (which crashes inside the MEX adapter).
  reg.registerCommand("frame_begin", [](ArgumentList& outputs, ArgumentList& inputs,
                                        matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::processLazyProperties();
    polyscope::processLazyPropertiesOutsideOfImGui();
    polyscope::render::engine->makeContextCurrent();
    polyscope::render::engine->updateWindowSize();
    polyscope::render::engine->pollEvents();

    // Start the ImGui frame.
    polyscope::render::engine->ImGuiNewFrame();

    // Process mouse/keyboard so the 3D scene remains interactive even though
    // we are not running Polyscope's own mainLoopIteration().
    processInputEventsSplitFrame();

    polyscope::view::updateFlight();
    polyscope::showDelayedWarnings();

    // Ensure the user callback does not fire; the user builds UI from MATLAB
    // between frame_begin and frame_end.
    polyscope::state::userCallback = nullptr;
  });

  reg.registerCommand("frame_end", [](ArgumentList& outputs, ArgumentList& inputs,
                                      matlab::engine::MATLABEngine* matlabPtr) {
    // Build default Polyscope GUI if requested.
    if (polyscope::options::buildGui && polyscope::options::buildDefaultGuiPanels) {
      polyscope::buildPolyscopeGui();
      polyscope::buildStructureGui();
      polyscope::buildPickGui();
    }

    // Build widget UIs.
    for (auto& wHandle : polyscope::state::widgets) {
      if (wHandle.isValid()) {
        wHandle.get().buildUI();
      }
    }

    polyscope::processLazyProperties();

    // Render the 3D scene (without UI); this also clears redraw flags.
    polyscope::draw(false, false);

    // Render the ImGui draw lists built by MATLAB between frame_begin and frame_end.
    polyscope::render::engine->bindDisplay();
    polyscope::render::engine->ImGuiRender();
    polyscope::render::engine->swapDisplayBuffers();
  });

  reg.registerCommand("shutdown", [](ArgumentList& outputs, ArgumentList& inputs,
                                     matlab::engine::MATLABEngine* matlabPtr) {
    bool allowMidFrame = false;
    if (inputCount(inputs) > 1) allowMidFrame = getScalarBool(getInput(inputs, 1));
    polyscope::shutdown(allowMidFrame);
  });

  reg.registerCommand("remove_everything", [](ArgumentList& outputs, ArgumentList& inputs,
                                              matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::removeEverything();
  });

  reg.registerCommand("remove_all_structures", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::removeAllStructures();
  });

  // === Small options =======================================================
  reg.registerCommand("set_program_name", [](ArgumentList& outputs, ArgumentList& inputs,
                                             matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::programName = getString(getInput(inputs, 1));
  });

  reg.registerCommand("set_verbosity", [](ArgumentList& outputs, ArgumentList& inputs,
                                          matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::verbosity = getScalarInt(getInput(inputs, 1));
  });

  reg.registerCommand("set_errors_throw_exceptions", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::errorsThrowExceptions = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_max_fps", [](ArgumentList& outputs, ArgumentList& inputs,
                                        matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::maxFPS = getScalarInt(getInput(inputs, 1));
  });

  reg.registerCommand("set_enable_vsync", [](ArgumentList& outputs, ArgumentList& inputs,
                                             matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::enableVSync = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_use_prefs_file", [](ArgumentList& outputs, ArgumentList& inputs,
                                               matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::usePrefsFile = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_allow_headless_backends", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::allowHeadlessBackends = getScalarBool(getInput(inputs, 1));
  });

  // === View / camera =======================================================
  reg.registerCommand("set_window_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                            matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 3);
    int w = getScalarInt(getInput(inputs, 1));
    int h = getScalarInt(getInput(inputs, 2));
    polyscope::view::setWindowSize(w, h);
  });

  reg.registerCommand("get_window_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                            matlab::engine::MATLABEngine* matlabPtr) {
    auto sz = polyscope::view::getWindowSize();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {static_cast<double>(std::get<0>(sz)),
                                              static_cast<double>(std::get<1>(sz))});
  });

  reg.registerCommand("reset_camera_to_home_view", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::view::resetCameraToHomeView();
  });

  reg.registerCommand("look_at", [](ArgumentList& outputs, ArgumentList& inputs,
                                    matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 3);
    auto eye = getMatrixFloat(getInput(inputs, 1), 3);
    auto target = getMatrixFloat(getInput(inputs, 2), 3);
    bool flyTo = false;
    if (inputCount(inputs) > 3) flyTo = getScalarBool(getInput(inputs, 3));
    polyscope::view::lookAt(glm::vec3(eye(0, 0), eye(0, 1), eye(0, 2)),
                            glm::vec3(target(0, 0), target(0, 1), target(0, 2)), flyTo);
  });

  reg.registerCommand("set_background_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    auto c = getMatrixFloat(getInput(inputs, 1));
    if (c.cols() != 3 && c.cols() != 4) {
      throwError(matlabPtr, "background color must be 1x3 or 1x4");
    }
    polyscope::view::bgColor = {c(0, 0), c(0, 1), c(0, 2), c.cols() == 4 ? c(0, 3) : 1.0f};
  });

  reg.registerCommand("get_background_color", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    auto& c = polyscope::view::bgColor;
    getOutput(outputs, 0) = createVectorDouble(factory, {c[0], c[1], c[2], c[3]});
  });

  // === Screenshots =========================================================
  reg.registerCommand("screenshot", [](ArgumentList& outputs, ArgumentList& inputs,
                                       matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::ScreenshotOptions opts;
    opts.includeUI = false;
    opts.transparentBackground = true;

    if (inputCount(inputs) > 1) {
      std::string filename = getString(getInput(inputs, 1));
      if (inputCount(inputs) > 2) {
        OptionsParser parser(inputs, 2, matlabPtr);
        opts.includeUI = parser.getBool("include_ui", opts.includeUI);
        opts.transparentBackground = parser.getBool("transparent_bg", opts.transparentBackground);
      }
      polyscope::screenshot(filename, opts);
    } else {
      if (inputCount(inputs) > 1) {
        OptionsParser parser(inputs, 1, matlabPtr);
        opts.includeUI = parser.getBool("include_ui", opts.includeUI);
        opts.transparentBackground = parser.getBool("transparent_bg", opts.transparentBackground);
      }
      polyscope::screenshot(opts);
    }
  });

  reg.registerCommand("set_screenshot_extension", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::screenshotExtension = getString(getInput(inputs, 1));
  });

  // === Render engine =======================================================
  reg.registerCommand("get_render_engine_backend_name", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(polyscope::render::getRenderEngineBackendName());
  });

  reg.registerCommand("is_headless", [](ArgumentList& outputs, ArgumentList& inputs,
                                        matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::isHeadless());
  });

  reg.registerCommand("set_build_gui", [](ArgumentList& outputs, ArgumentList& inputs,
                                          matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::buildGui = getScalarBool(getInput(inputs, 1));
  });

  // === Temporary debug: C++ callback frame tick =============================
  reg.registerCommand("test_cpp_callback_frame", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    MATLABEngine* matlabPtr) {
    polyscope::options::openImGuiWindowForUserCallback = false;
    static int s_cbCounter = 0;
    s_cbCounter++;
    polyscope::state::userCallback = [&]() {
      polyscope::view::bgColor = {1.0, 0.0, 0.0, 1.0};
      ImGuiContext* ctx = ImGui::GetCurrentContext();
      size_t nBefore = ctx ? ctx->Windows.size() : 9999;
      char name[64];
      snprintf(name, sizeof(name), "C++ callback window %d", s_cbCounter);
      ImGui::SetNextWindowPos(ImVec2(500, 100), ImGuiCond_Always);
      ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Always);
      ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
      bool winVisible = ImGui::Begin(name);
      ImGui::Text("visible=%d", winVisible);
      ImGui::End();
      ImGui::PopStyleColor();
      size_t nAfter = ctx ? ctx->Windows.size() : 9999;
      if (nAfter > nBefore) polyscope::view::bgColor = {0.0, 1.0, 0.0, 1.0};
    };
    polyscope::frameTick();
    // callback intentionally left active so a later screenshot captures it
  });

  // === User callbacks =======================================================
  reg.registerCommand("set_user_callback", [](ArgumentList& outputs, ArgumentList& inputs,
                                              MATLABEngine* matlabPtr) {
    // Callbacks invoked from the C++ render loop are unsafe inside a MEX-file (they crash
    // the MEX adapter when it tries to re-enter MATLAB). Use the frame_begin/frame_end
    // split-frame API instead.
    throwError(matlabPtr, "set_user_callback is not supported from MATLAB. Use frame_begin()/frame_end() to drive the render loop from MATLAB.");
  });

  reg.registerCommand("clear_user_callback", [](ArgumentList& outputs, ArgumentList& inputs,
                                                MATLABEngine* matlabPtr) {
    polyscope::state::userCallback = nullptr;
  });

  // === Framebuffer capture ===================================================
  // Reads the currently displayed front buffer and writes it to a PNG file.
  // Useful for verifying what the split-frame path actually draws.
  reg.registerCommand("capture_display", [](ArgumentList& outputs, ArgumentList& inputs,
                                            MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 2);
    std::string filename = getString(getInput(inputs, 1));

    polyscope::render::engine->makeContextCurrent();
    auto sz = polyscope::view::getWindowSize();
    int w = static_cast<int>(std::get<0>(sz));
    int h = static_cast<int>(std::get<1>(sz));
    if (w <= 0 || h <= 0) {
      throwError(matlabPtr, "window size is invalid for capture");
    }

    std::vector<uint8_t> pixels(static_cast<size_t>(w * h * 3));
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically for PNG (origin at top-left).
    std::vector<uint8_t> flipped(static_cast<size_t>(w * h * 3));
    for (int y = 0; y < h; ++y) {
      std::memcpy(&flipped[y * w * 3],
                  &pixels[(h - 1 - y) * w * 3],
                  static_cast<size_t>(w * 3));
    }

    if (!stbi_write_png(filename.c_str(), w, h, 3, flipped.data(), w * 3)) {
      throwError(matlabPtr, "failed to write capture PNG: " + filename);
    }
  });
}

} // namespace ps_mex
