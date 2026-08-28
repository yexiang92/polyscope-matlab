#include "bind_core.h"
#include "matlab_data_utils.h"

#include "polyscope/polyscope.h"
#include "polyscope/structure.h"
#include "polyscope/view.h"
#include "polyscope/options.h"
#include "polyscope/internal.h"
#include "polyscope/pick.h"
#include "imgui_internal.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/materials.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/messages.h"
#include "polyscope/imgui_config.h"
#include "polyscope/widget.h"
#include "polyscope/weak_handle.h"
#include <glm/glm.hpp>

#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

#include "stb_image_write.h"
#include "stb_image.h"

#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <array>
#include <memory>
#include <unordered_map>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// Forward declarations for Polyscope internals used in the split-frame API.
namespace polyscope {
  void renderScene();
  void renderSceneToScreen();
}

namespace {
std::unordered_map<uint64_t, std::shared_ptr<polyscope::render::TextureBuffer>> matlabImageTextures;
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

#ifdef _WIN32
std::wstring utf8ToWide(const std::string& text) {
  if (text.empty()) return std::wstring();
  int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
  if (count <= 0) {
    count = MultiByteToWideChar(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) return std::wstring(text.begin(), text.end());
    std::wstring out(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), out.data(), count);
    return out;
  }
  std::wstring out(static_cast<size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), count);
  return out;
}

struct WindowTitleUpdate {
  DWORD processId = 0;
  std::wstring oldTitle;
  std::wstring newTitle;
  bool matched = false;
};

BOOL CALLBACK updateProcessWindowTitle(HWND hwnd, LPARAM param) {
  WindowTitleUpdate* update = reinterpret_cast<WindowTitleUpdate*>(param);
  DWORD windowProcessId = 0;
  GetWindowThreadProcessId(hwnd, &windowProcessId);
  if (windowProcessId != update->processId || !IsWindowVisible(hwnd)) return TRUE;

  int len = GetWindowTextLengthW(hwnd);
  if (len <= 0) return TRUE;
  std::wstring title(static_cast<size_t>(len + 1), L'\0');
  GetWindowTextW(hwnd, title.data(), len + 1);
  title.resize(static_cast<size_t>(len));

  if (title == update->oldTitle || title == L"Polyscope" || title == L"auto - by Yexiang Yan") {
    SetWindowTextW(hwnd, update->newTitle.c_str());
    update->matched = true;
  }
  return TRUE;
}

void setNativeProgramWindowTitle(const std::string& oldTitle, const std::string& newTitle) {
  WindowTitleUpdate update;
  update.processId = GetCurrentProcessId();
  update.oldTitle = utf8ToWide(oldTitle);
  update.newTitle = utf8ToWide(newTitle);
  EnumWindows(updateProcessWindowTitle, reinterpret_cast<LPARAM>(&update));
}
#else
void setNativeProgramWindowTitle(const std::string&, const std::string&) {}
#endif

polyscope::Structure* getStructureByName(matlab::engine::MATLABEngine* matlabPtr, const std::string& name) {
  for (auto& catMap : polyscope::state::structures) {
    for (auto& s : catMap.second) {
      if (s.second->name == name) {
        return s.second.get();
      }
    }
  }
  throwError(matlabPtr, "No structure with name: " + name);
  return nullptr;
}

// Replicate the input handling that Polyscope's normal mainLoopIteration()
// performs inside processInputEvents(). When the user drives the render loop
// manually via frame_begin()/frame_end(), this function is called so the 3D
// scene remains interactive: rotate, pan, zoom, picking and selection.
void processInputEventsSplitFrame() {
  if (!polyscope::options::doDefaultMouseInteraction) return;

  ImGuiIO& io = ImGui::GetIO();

  if (ImGui::IsAnyMouseDown()) {
    polyscope::requestRedraw();
  }

  // Persistent state for picking/selection, mirroring Polyscope's internals.
  static float dragDistSinceLastRelease = 0.0f;
  static bool pendingPickActive = false;
  static polyscope::PickResult pendingPickResult{};
  static float pendingPickTime = 0.0f;

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
      dragDistSinceLastRelease += std::abs(dragDelta.x) + std::abs(dragDelta.y);

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

    // === Picking / selection
    float dragIgnoreThreshold = 0.01f;
    bool anyModifierHeld = io.KeyShift || io.KeyCtrl || io.KeyAlt;
    bool ctrlShiftHeld = io.KeyShift && io.KeyCtrl;

    // Apply a pending pick once the double-click window has passed.
    if (pendingPickActive) {
      float elapsedSec = ImGui::GetTime() - pendingPickTime;
      float pickDelaySec = io.MouseDoubleClickTime + 0.05f;
      if (polyscope::haveSelection() || elapsedSec >= pickDelaySec) {
        polyscope::setSelection(pendingPickResult);
        pendingPickActive = false;
      }
    }

    // Left click release with a single click -> pick at release location.
    if (!anyModifierHeld && io.MouseReleased[0] && io.MouseClickedLastCount[0] == 1) {
      if (dragDistSinceLastRelease < dragIgnoreThreshold) {
        glm::vec2 screenCoords{io.MousePos.x, io.MousePos.y};
        pendingPickResult = polyscope::pickAtScreenCoords(screenCoords);
        pendingPickTime = ImGui::GetTime();
        pendingPickActive = true;
      }
    }

    // Right click release -> clear selection.
    if (!anyModifierHeld && io.MouseReleased[1]) {
      if (dragDistSinceLastRelease < dragIgnoreThreshold) {
        polyscope::resetSelection();
      }
      dragDistSinceLastRelease = 0.0f;
      pendingPickActive = false;
    }

    // Double-click left, or Ctrl+Shift left click -> recenter view at clicked point.
    if ((io.MouseReleased[0] && io.MouseClickedLastCount[0] == 2) ||
        (io.MouseReleased[0] && ctrlShiftHeld)) {
      if (dragDistSinceLastRelease < dragIgnoreThreshold) {
        glm::vec2 screenCoords{io.MousePos.x, io.MousePos.y};
        polyscope::view::processSetCenter(screenCoords);
        pendingPickActive = false;
      }
    }
  }

  // Reset drag distance after any left release.
  if (io.MouseReleased[0]) {
    dragDistSinceLastRelease = 0.0f;
  }

  if (!io.WantCaptureKeyboard) {
    polyscope::view::processKeyboardNavigation(io);
  }
}

std::string normalizeToken(std::string s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if (c == '_' || c == '-' || std::isspace(c)) continue;
    out.push_back(static_cast<char>(std::tolower(c)));
  }
  return out;
}

polyscope::NavigateStyle parseNavigateStyle(matlab::engine::MATLABEngine* matlabPtr, const std::string& style) {
  std::string key = normalizeToken(style);
  if (key == "turntable") return polyscope::NavigateStyle::Turntable;
  if (key == "free") return polyscope::NavigateStyle::Free;
  if (key == "planar") return polyscope::NavigateStyle::Planar;
  if (key == "arcball") return polyscope::NavigateStyle::Arcball;
  if (key == "none") return polyscope::NavigateStyle::None;
  if (key == "firstperson") return polyscope::NavigateStyle::FirstPerson;
  throwError(matlabPtr, "Unknown navigation style: " + style);
  return polyscope::NavigateStyle::Turntable;
}

polyscope::UpDir parseUpDir(matlab::engine::MATLABEngine* matlabPtr, const std::string& dir) {
  std::string key = normalizeToken(dir);
  if (key == "xup") return polyscope::UpDir::XUp;
  if (key == "yup") return polyscope::UpDir::YUp;
  if (key == "zup") return polyscope::UpDir::ZUp;
  if (key == "negxup") return polyscope::UpDir::NegXUp;
  if (key == "negyup") return polyscope::UpDir::NegYUp;
  if (key == "negzup") return polyscope::UpDir::NegZUp;
  throwError(matlabPtr, "Unknown up direction: " + dir);
  return polyscope::UpDir::YUp;
}

polyscope::FrontDir parseFrontDir(matlab::engine::MATLABEngine* matlabPtr, const std::string& dir) {
  std::string key = normalizeToken(dir);
  if (key == "xfront" || key == "xforward") return polyscope::FrontDir::XFront;
  if (key == "yfront" || key == "yforward") return polyscope::FrontDir::YFront;
  if (key == "zfront" || key == "zforward") return polyscope::FrontDir::ZFront;
  if (key == "negxfront" || key == "negxforward") return polyscope::FrontDir::NegXFront;
  if (key == "negyfront" || key == "negyforward") return polyscope::FrontDir::NegYFront;
  if (key == "negzfront" || key == "negzforward") return polyscope::FrontDir::NegZFront;
  throwError(matlabPtr, "Unknown front direction: " + dir);
  return polyscope::FrontDir::ZFront;
}

polyscope::ProjectionMode parseProjectionMode(matlab::engine::MATLABEngine* matlabPtr, const std::string& mode) {
  std::string key = normalizeToken(mode);
  if (key == "perspective") return polyscope::ProjectionMode::Perspective;
  if (key == "orthographic") return polyscope::ProjectionMode::Orthographic;
  throwError(matlabPtr, "Unknown projection mode: " + mode);
  return polyscope::ProjectionMode::Perspective;
}

polyscope::LimitFPSMode parseLimitFPSMode(matlab::engine::MATLABEngine* matlabPtr, const std::string& mode) {
  std::string key = normalizeToken(mode);
  if (key == "ignorelimits") return polyscope::LimitFPSMode::IgnoreLimits;
  if (key == "blocktohittarget") return polyscope::LimitFPSMode::BlockToHitTarget;
  if (key == "skipframestohittarget") return polyscope::LimitFPSMode::SkipFramesToHitTarget;
  throwError(matlabPtr, "Unknown FPS limit mode: " + mode);
  return polyscope::LimitFPSMode::BlockToHitTarget;
}

polyscope::GroundPlaneMode parseGroundPlaneMode(matlab::engine::MATLABEngine* matlabPtr, const std::string& mode) {
  std::string key = normalizeToken(mode);
  if (key == "none") return polyscope::GroundPlaneMode::None;
  if (key == "tile") return polyscope::GroundPlaneMode::Tile;
  if (key == "tilereflection") return polyscope::GroundPlaneMode::TileReflection;
  if (key == "shadowonly") return polyscope::GroundPlaneMode::ShadowOnly;
  throwError(matlabPtr, "Unknown ground plane mode: " + mode);
  return polyscope::GroundPlaneMode::None;
}

polyscope::GroundPlaneHeightMode parseGroundPlaneHeightMode(matlab::engine::MATLABEngine* matlabPtr,
                                                            const std::string& mode) {
  std::string key = normalizeToken(mode);
  if (key == "automatic") return polyscope::GroundPlaneHeightMode::Automatic;
  if (key == "manual") return polyscope::GroundPlaneHeightMode::Manual;
  throwError(matlabPtr, "Unknown ground plane height mode: " + mode);
  return polyscope::GroundPlaneHeightMode::Automatic;
}

polyscope::TransparencyMode parseTransparencyMode(matlab::engine::MATLABEngine* matlabPtr, const std::string& mode) {
  std::string key = normalizeToken(mode);
  if (key == "none") return polyscope::TransparencyMode::None;
  if (key == "simple") return polyscope::TransparencyMode::Simple;
  if (key == "pretty") return polyscope::TransparencyMode::Pretty;
  throwError(matlabPtr, "Unknown transparency mode: " + mode);
  return polyscope::TransparencyMode::None;
}

std::string toSnake(polyscope::NavigateStyle style) {
  switch (style) {
  case polyscope::NavigateStyle::Turntable: return "turntable";
  case polyscope::NavigateStyle::Free: return "free";
  case polyscope::NavigateStyle::Planar: return "planar";
  case polyscope::NavigateStyle::Arcball: return "arcball";
  case polyscope::NavigateStyle::None: return "none";
  case polyscope::NavigateStyle::FirstPerson: return "first_person";
  }
  return "";
}

std::string toSnake(polyscope::UpDir dir) {
  switch (dir) {
  case polyscope::UpDir::XUp: return "x_up";
  case polyscope::UpDir::YUp: return "y_up";
  case polyscope::UpDir::ZUp: return "z_up";
  case polyscope::UpDir::NegXUp: return "neg_x_up";
  case polyscope::UpDir::NegYUp: return "neg_y_up";
  case polyscope::UpDir::NegZUp: return "neg_z_up";
  }
  return "";
}

std::string toSnake(polyscope::FrontDir dir) {
  switch (dir) {
  case polyscope::FrontDir::XFront: return "x_front";
  case polyscope::FrontDir::YFront: return "y_front";
  case polyscope::FrontDir::ZFront: return "z_front";
  case polyscope::FrontDir::NegXFront: return "neg_x_front";
  case polyscope::FrontDir::NegYFront: return "neg_y_front";
  case polyscope::FrontDir::NegZFront: return "neg_z_front";
  }
  return "";
}

std::string toSnake(polyscope::ProjectionMode mode) {
  switch (mode) {
  case polyscope::ProjectionMode::Perspective: return "perspective";
  case polyscope::ProjectionMode::Orthographic: return "orthographic";
  }
  return "";
}

matlab::data::StructArray createPickResultStruct(matlab::data::ArrayFactory& factory,
                                                 const polyscope::PickResult& p) {
  std::vector<std::string> fields = {"is_hit",        "structure_type_name", "structure_name", "quantity_name",
                                     "screen_coords", "buffer_inds",         "position",       "depth",
                                     "local_index"};
  auto s = factory.createStructArray({1, 1}, fields);
  s[0]["is_hit"] = createScalarBool(factory, p.isHit);
  s[0]["structure_type_name"] = factory.createScalar(p.structureType);
  s[0]["structure_name"] = factory.createScalar(p.structureName);
  s[0]["quantity_name"] = factory.createScalar(p.quantityName);
  s[0]["screen_coords"] = createVectorDouble(factory, {p.screenCoords.x, p.screenCoords.y});
  s[0]["buffer_inds"] = createVectorDouble(factory, {static_cast<double>(p.bufferInds.x), static_cast<double>(p.bufferInds.y)});
  s[0]["position"] = createVectorDouble(factory, {p.position.x, p.position.y, p.position.z});
  s[0]["depth"] = createScalarDouble(factory, p.depth);
  s[0]["local_index"] = createScalarDouble(factory, static_cast<double>(p.localIndex) + 1.0);
  return s;
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

  reg.registerCommand("hide_window", [](ArgumentList& outputs, ArgumentList& inputs,
                                        matlab::engine::MATLABEngine* matlabPtr) {
    if (polyscope::render::engine && !polyscope::isHeadless()) {
      polyscope::render::engine->hideWindow();
    }
  });

  reg.registerCommand("focus_window", [](ArgumentList& outputs, ArgumentList& inputs,
                                         matlab::engine::MATLABEngine* matlabPtr) {
    if (polyscope::render::engine) {
      polyscope::render::engine->focusWindow();
    }
  });

  reg.registerCommand("set_up_dir", [](ArgumentList& outputs, ArgumentList& inputs,
                                       matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::UpDir up = parseUpDir(matlabPtr, getString(getInput(inputs, 1)));
    bool animate = false;
    if (inputCount(inputs) > 2) {
      animate = getScalarBool(getInput(inputs, 2));
    }
    polyscope::view::setUpDir(up, animate);
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

    // Render widgets (e.g. transformation gizmos / slice plane widgets) which are
    // drawn in the 3D scene using ImGuizmo. They are skipped by draw(false, false)
    // but must be issued before ImGuiRender().
    polyscope::render::engine->bindDisplay();
    for (auto& wHandle : polyscope::state::widgets) {
      if (wHandle.isValid()) {
        wHandle.get().draw();
      }
    }

    // Render the ImGui draw lists built by MATLAB between frame_begin and frame_end.
    polyscope::render::engine->bindDisplay();
    polyscope::render::engine->ImGuiRender();
    polyscope::render::engine->swapDisplayBuffers();
  });

  reg.registerCommand("shutdown", [](ArgumentList& outputs, ArgumentList& inputs,
                                     matlab::engine::MATLABEngine* matlabPtr) {
    bool allowMidFrame = false;
    if (inputCount(inputs) > 1) allowMidFrame = getScalarBool(getInput(inputs, 1));
    matlabImageTextures.clear();
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
    std::string oldProgramName = polyscope::options::programName;
    polyscope::options::programName = getString(getInput(inputs, 1));
    setNativeProgramWindowTitle(oldProgramName, polyscope::options::programName);
  });

  reg.registerCommand("set_verbosity", [](ArgumentList& outputs, ArgumentList& inputs,
                                          matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::verbosity = getScalarInt(getInput(inputs, 1));
  });

  reg.registerCommand("set_print_prefix", [](ArgumentList& outputs, ArgumentList& inputs,
                                             matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::printPrefix = getString(getInput(inputs, 1));
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

  reg.registerCommand("set_ssaa_factor", [](ArgumentList& outputs, ArgumentList& inputs,
                                            matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    int factor = getScalarInt(getInput(inputs, 1));
    factor = std::max(1, std::min(4, factor));
    polyscope::options::ssaaFactor = factor;
    // Apply immediately if the engine is already initialized so the UI / render
    // state reflects the new value without waiting for the lazy update.
    if (polyscope::isInitialized() && polyscope::render::engine) {
      polyscope::render::engine->setSSAAFactor(factor);
      polyscope::internal::lazy::ssaaFactor = factor;
      polyscope::requestRedraw();
    }
  });

  reg.registerCommand("get_ssaa_factor", [](ArgumentList& outputs, ArgumentList& inputs,
                                            matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 1);
    matlab::data::ArrayFactory factory;
    if (polyscope::isInitialized()) {
      getOutput(outputs, 0) = factory.createScalar(polyscope::render::engine->getSSAAFactor());
    } else {
      getOutput(outputs, 0) = factory.createScalar(polyscope::options::ssaaFactor);
    }
  });

  reg.registerCommand("set_navigation_style", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::view::setNavigateStyle(parseNavigateStyle(matlabPtr, getString(getInput(inputs, 1))));
  });

  reg.registerCommand("get_navigation_style", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(toSnake(polyscope::view::getNavigateStyle()));
  });

  reg.registerCommand("get_up_dir", [](ArgumentList& outputs, ArgumentList& inputs,
                                       matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(toSnake(polyscope::view::getUpDir()));
  });

  reg.registerCommand("set_front_dir", [](ArgumentList& outputs, ArgumentList& inputs,
                                          matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 2);
    bool animate = false;
    if (inputCount(inputs) > 2) animate = getScalarBool(getInput(inputs, 2));
    polyscope::view::setFrontDir(parseFrontDir(matlabPtr, getString(getInput(inputs, 1))), animate);
  });

  reg.registerCommand("get_front_dir", [](ArgumentList& outputs, ArgumentList& inputs,
                                          matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(toSnake(polyscope::view::getFrontDir()));
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

  reg.registerCommand("set_do_default_mouse_interaction", [](ArgumentList& outputs, ArgumentList& inputs,
                                                             matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::doDefaultMouseInteraction = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("request_redraw", [](ArgumentList& outputs, ArgumentList& inputs,
                                           matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::requestRedraw();
  });

  reg.registerCommand("get_redraw_requested", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::redrawRequested());
  });

  reg.registerCommand("set_window_icon", [](ArgumentList& outputs, ArgumentList& inputs,
                                            matlab::engine::MATLABEngine* matlabPtr) {
    if (inputCount(inputs) < 2) throwError(matlabPtr, "Expected set_window_icon(filename)");
    polyscope::setWindowIcon(getString(getInput(inputs, 1)));
  });

  reg.registerCommand("set_always_redraw", [](ArgumentList& outputs, ArgumentList& inputs,
                                              matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::alwaysRedraw = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_frame_tick_limit_fps_mode", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::frameTickLimitFPSMode = parseLimitFPSMode(matlabPtr, getString(getInput(inputs, 1)));
  });

  reg.registerCommand("set_enable_render_error_checks", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::enableRenderErrorChecks = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_egl_device_index", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::eglDeviceIndex = getScalarInt(getInput(inputs, 1));
  });

  reg.registerCommand("set_autocenter_structures", [](ArgumentList& outputs, ArgumentList& inputs,
                                                      matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::autocenterStructures = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_autoscale_structures", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::autoscaleStructures = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_ui_scale", [](ArgumentList& outputs, ArgumentList& inputs,
                                         matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::uiScale = getScalarFloat(getInput(inputs, 1));
  });

  reg.registerCommand("get_ui_scale", [](ArgumentList& outputs, ArgumentList& inputs,
                                         matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, polyscope::options::uiScale);
  });

  reg.registerCommand("set_user_gui_is_on_right_side", [](ArgumentList& outputs, ArgumentList& inputs,
                                                          matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::userGuiIsOnRightSide = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_build_default_gui_panels", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::buildDefaultGuiPanels = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_right_gui_pane_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::rightGuiPaneWidth = getScalarInt(getInput(inputs, 1));
  });

  reg.registerCommand("get_right_gui_pane_width", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, polyscope::options::rightGuiPaneWidth);
  });

  reg.registerCommand("set_render_scene", [](ArgumentList& outputs, ArgumentList& inputs,
                                             matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::renderScene = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_open_imgui_window_for_user_callback", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                    matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::openImGuiWindowForUserCallback = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_invoke_user_callback_for_nested_show", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                     matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::invokeUserCallbackForNestedShow = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_give_focus_on_show", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::giveFocusOnShow = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_hide_window_after_show", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::hideWindowAfterShow = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_warn_for_invalid_values", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::warnForInvalidValues = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_display_message_popups", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::displayMessagePopups = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("clear_configure_imgui_style_callback", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                 matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::options::configureImGuiStyleCallback = polyscope::configureImGuiStyle;
  });

  reg.registerCommand("clear_prepare_imgui_fonts_callback", [](ArgumentList& outputs, ArgumentList& inputs,
                                                               matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::options::prepareImGuiFontsCallback = polyscope::loadBaseFonts;
  });

  reg.registerCommand("clear_files_dropped_callback", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::state::filesDroppedCallback = nullptr;
  });

  // === Scene extents =======================================================
  reg.registerCommand("set_automatically_compute_scene_extents", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                    matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::automaticallyComputeSceneExtents = getScalarBool(getInput(inputs, 1));
  });

  reg.registerCommand("set_length_scale", [](ArgumentList& outputs, ArgumentList& inputs,
                                             matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::state::lengthScale = getScalarFloat(getInput(inputs, 1));
  });

  reg.registerCommand("get_length_scale", [](ArgumentList& outputs, ArgumentList& inputs,
                                             matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, polyscope::state::lengthScale);
  });

  reg.registerCommand("set_bounding_box", [](ArgumentList& outputs, ArgumentList& inputs,
                                             matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 3);
    Eigen::Vector3f low = getVec3(getInput(inputs, 1));
    Eigen::Vector3f high = getVec3(getInput(inputs, 2));
    polyscope::state::boundingBox = std::tuple<glm::vec3, glm::vec3>(
        glm::vec3(low(0), low(1), low(2)), glm::vec3(high(0), high(1), high(2)));
  });

  reg.registerCommand("get_bounding_box", [](ArgumentList& outputs, ArgumentList& inputs,
                                             matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    auto [low, high] = polyscope::state::boundingBox;
    getOutput(outputs, 0) = createVectorDouble(factory, {low.x, low.y, low.z});
    getOutput(outputs, 1) = createVectorDouble(factory, {high.x, high.y, high.z});
  });

  reg.registerCommand("update_scene_extents", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::updateStructureExtents();
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

  reg.registerCommand("look_at_dir", [](ArgumentList& outputs, ArgumentList& inputs,
                                        matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 4);
    Eigen::Vector3f eye = getVec3(getInput(inputs, 1));
    Eigen::Vector3f target = getVec3(getInput(inputs, 2));
    Eigen::Vector3f up = getVec3(getInput(inputs, 3));
    bool flyTo = false;
    if (inputCount(inputs) > 4) flyTo = getScalarBool(getInput(inputs, 4));
    polyscope::view::lookAt(glm::vec3(eye(0), eye(1), eye(2)), glm::vec3(target(0), target(1), target(2)),
                            glm::vec3(up(0), up(1), up(2)), flyTo);
  });

  reg.registerCommand("set_view_projection_mode", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::view::setProjectionMode(parseProjectionMode(matlabPtr, getString(getInput(inputs, 1))));
  });

  reg.registerCommand("get_view_projection_mode", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(toSnake(polyscope::view::getProjectionMode()));
  });

  reg.registerCommand("set_vertical_fov_degrees", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::view::setVerticalFieldOfViewDegrees(getScalarFloat(getInput(inputs, 1)));
  });

  reg.registerCommand("get_vertical_fov_degrees", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, polyscope::view::getVerticalFieldOfViewDegrees());
  });

  reg.registerCommand("get_aspect_ratio_width_over_height", [](ArgumentList& outputs, ArgumentList& inputs,
                                                               matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, polyscope::view::getAspectRatioWidthOverHeight());
  });

  reg.registerCommand("get_buffer_size", [](ArgumentList& outputs, ArgumentList& inputs,
                                            matlab::engine::MATLABEngine* matlabPtr) {
    auto sz = polyscope::view::getBufferSize();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {static_cast<double>(std::get<0>(sz)),
                                                         static_cast<double>(std::get<1>(sz))});
  });

  reg.registerCommand("set_window_resizable", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::view::setWindowResizable(getScalarBool(getInput(inputs, 1)));
  });

  reg.registerCommand("get_window_resizable", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::view::getWindowResizable());
  });

  reg.registerCommand("set_view_from_json", [](ArgumentList& outputs, ArgumentList& inputs,
                                               matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 2);
    bool flyTo = false;
    if (inputCount(inputs) > 2) flyTo = getScalarBool(getInput(inputs, 2));
    polyscope::view::setViewFromJson(getString(getInput(inputs, 1)), flyTo);
  });

  reg.registerCommand("get_view_as_json", [](ArgumentList& outputs, ArgumentList& inputs,
                                             matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = factory.createScalar(polyscope::view::getViewAsJson());
  });

  reg.registerCommand("screen_coords_to_world_ray", [](ArgumentList& outputs, ArgumentList& inputs,
                                                       matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    Eigen::Vector2f coords = getVec2(getInput(inputs, 1));
    glm::vec3 ray = polyscope::view::screenCoordsToWorldRay(glm::vec2(coords(0), coords(1)));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {ray.x, ray.y, ray.z});
  });

  reg.registerCommand("world_coords_to_screen", [](ArgumentList& outputs, ArgumentList& inputs,
                                                     matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    Eigen::MatrixXf points = getMatrixFloat(getInput(inputs, 1));
    if (points.cols() != 3) throwError(matlabPtr, "world coordinates must be an N-by-3 array");

    auto [bufferWidth, bufferHeight] = polyscope::view::getBufferSize();
    auto [windowWidth, windowHeight] = polyscope::view::getWindowSize();
    glm::mat4 viewMat = polyscope::view::getCameraViewMatrix();
    glm::mat4 projectionMat = polyscope::view::getCameraPerspectiveMatrix();
    Eigen::MatrixXd out(points.rows(), 3);
    const double sx = bufferWidth > 0 ? static_cast<double>(windowWidth) / bufferWidth : 1.0;
    const double sy = bufferHeight > 0 ? static_cast<double>(windowHeight) / bufferHeight : 1.0;

    for (Eigen::Index i = 0; i < points.rows(); ++i) {
      glm::vec4 clip = projectionMat * viewMat * glm::vec4(points(i, 0), points(i, 1), points(i, 2), 1.f);
      bool visible = clip.w > 0.f;
      glm::vec3 ndc(0.f);
      if (visible) {
        ndc = glm::vec3(clip) / clip.w;
        visible = ndc.x >= -1.f && ndc.x <= 1.f && ndc.y >= -1.f && ndc.y <= 1.f &&
                  ndc.z >= -1.f && ndc.z <= 1.f;
      }
      const double bx = (static_cast<double>(ndc.x) + 1.0) * 0.5 * bufferWidth;
      const double by = (static_cast<double>(ndc.y) + 1.0) * 0.5 * bufferHeight;
      out(i, 0) = bx * sx;
      out(i, 1) = (bufferHeight - by) * sy;
      out(i, 2) = visible ? 1.0 : 0.0;
    }
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createMatrixDouble(factory, out);
  });

  reg.registerCommand("set_camera_view_matrix", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    Eigen::MatrixXf mat = getMatrixFloat(getInput(inputs, 1));
    if (mat.rows() != 4 || mat.cols() != 4) {
      throwError(matlabPtr, "camera view matrix must be 4x4");
    }
    glm::mat4 viewMat(1.0f);
    for (int j = 0; j < 4; ++j) {
      for (int i = 0; i < 4; ++i) {
        viewMat[j][i] = mat(i, j);
      }
    }
    polyscope::view::setCameraViewMatrix(viewMat);
  });

  reg.registerCommand("get_camera_view_matrix", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   matlab::engine::MATLABEngine* matlabPtr) {
    glm::mat4 mat = polyscope::view::getCameraViewMatrix();
    Eigen::MatrixXd out(4, 4);
    for (int j = 0; j < 4; ++j) {
      for (int i = 0; i < 4; ++i) {
        out(i, j) = mat[j][i];
      }
    }
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createMatrixDouble(factory, out);
  });

  reg.registerCommand("set_view_center_and_look_at", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 2);
    Eigen::Vector3f pos = getVec3(getInput(inputs, 1));
    bool flyTo = false;
    if (inputCount(inputs) > 2) flyTo = getScalarBool(getInput(inputs, 2));
    polyscope::view::setViewCenterAndLookAt(glm::vec3(pos(0), pos(1), pos(2)), flyTo);
  });

  reg.registerCommand("set_view_center_and_project", [](ArgumentList& outputs, ArgumentList& inputs,
                                                        matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    Eigen::Vector3f pos = getVec3(getInput(inputs, 1));
    polyscope::view::setViewCenterAndProject(glm::vec3(pos(0), pos(1), pos(2)));
  });

  reg.registerCommand("set_view_center_raw", [](ArgumentList& outputs, ArgumentList& inputs,
                                                matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    Eigen::Vector3f pos = getVec3(getInput(inputs, 1));
    polyscope::view::setViewCenterRaw(glm::vec3(pos(0), pos(1), pos(2)));
  });

  reg.registerCommand("get_view_center", [](ArgumentList& outputs, ArgumentList& inputs,
                                            matlab::engine::MATLABEngine* matlabPtr) {
    glm::vec3 c = polyscope::view::getViewCenter();
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createVectorDouble(factory, {c.x, c.y, c.z});
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

  reg.registerCommand("screenshot_to_buffer", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::ScreenshotOptions opts;
    opts.transparentBackground = true;
    opts.includeUI = false;
    bool verticalFlip = true;
    if (inputCount(inputs) > 1) {
      OptionsParser parser(inputs, 1, matlabPtr);
      opts.transparentBackground = parser.getBool("transparent_bg", opts.transparentBackground);
      opts.includeUI = parser.getBool("include_ui", opts.includeUI);
      verticalFlip = parser.getBool("vertical_flip", verticalFlip);
    }

    std::vector<unsigned char> raw = polyscope::screenshotToBuffer(opts);
    auto [w, h] = polyscope::view::getBufferSize();
    matlab::data::ArrayFactory factory;
    auto arr = factory.createArray<uint8_t>({static_cast<size_t>(h), static_cast<size_t>(w), 4});
    for (int y = 0; y < h; ++y) {
      int srcY = verticalFlip ? (h - 1 - y) : y;
      for (int x = 0; x < w; ++x) {
        for (int c = 0; c < 4; ++c) {
          size_t srcInd = static_cast<size_t>((srcY * w + x) * 4 + c);
          size_t dstInd = static_cast<size_t>(y + x * h + c * h * w);
          arr[dstInd] = raw[srcInd];
        }
      }
    }
    getOutput(outputs, 0) = arr;
  });

  // === Advanced UI management =============================================
  reg.registerCommand("build_polyscope_gui", [](ArgumentList& outputs, ArgumentList& inputs,
                                                matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::buildPolyscopeGui();
  });

  reg.registerCommand("build_structure_gui", [](ArgumentList& outputs, ArgumentList& inputs,
                                                matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::buildStructureGui();
  });

  reg.registerCommand("build_pick_gui", [](ArgumentList& outputs, ArgumentList& inputs,
                                           matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::buildPickGui();
  });

  reg.registerCommand("build_user_gui_and_invoke_callback", [](ArgumentList& outputs, ArgumentList& inputs,
                                                               matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::buildUserGuiAndInvokeCallback();
  });

  // === Messages ============================================================
  reg.registerCommand("info", [](ArgumentList& outputs, ArgumentList& inputs,
                                 matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 2);
    int verbosity = 0;
    std::string message;
    if (inputCount(inputs) > 2) {
      verbosity = getScalarInt(getInput(inputs, 1));
      message = getString(getInput(inputs, 2));
    } else {
      message = getString(getInput(inputs, 1));
    }
    polyscope::info(verbosity, message);
  });

  reg.registerCommand("warning", [](ArgumentList& outputs, ArgumentList& inputs,
                                    matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 2);
    std::string detail = "";
    if (inputCount(inputs) > 2) detail = getString(getInput(inputs, 2));
    polyscope::warning(getString(getInput(inputs, 1)), detail);
  });

  reg.registerCommand("error", [](ArgumentList& outputs, ArgumentList& inputs,
                                  matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::error(getString(getInput(inputs, 1)));
  });

  reg.registerCommand("terminating_error", [](ArgumentList& outputs, ArgumentList& inputs,
                                              matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::terminatingError(getString(getInput(inputs, 1)));
  });

  // === Picking =============================================================
  reg.registerCommand("pick_at_screen_coords", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    Eigen::Vector2f coords = getVec2(getInput(inputs, 1));
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createPickResultStruct(factory, polyscope::pickAtScreenCoords(glm::vec2(coords(0), coords(1))));
  });

  reg.registerCommand("pick_at_buffer_inds", [](ArgumentList& outputs, ArgumentList& inputs,
                                                matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    auto coords = getVectorInt(getInput(inputs, 1));
    if (coords.size() != 2) {
      throwError(matlabPtr, "buffer indices must be a 2-vector");
    }
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createPickResultStruct(factory, polyscope::pickAtBufferInds(glm::ivec2(coords[0], coords[1])));
  });

  reg.registerCommand("have_selection", [](ArgumentList& outputs, ArgumentList& inputs,
                                           matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarBool(factory, polyscope::haveSelection());
  });

  reg.registerCommand("get_selection", [](ArgumentList& outputs, ArgumentList& inputs,
                                          matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createPickResultStruct(factory, polyscope::getSelection());
  });

  reg.registerCommand("reset_selection", [](ArgumentList& outputs, ArgumentList& inputs,
                                            matlab::engine::MATLABEngine* matlabPtr) {
    polyscope::resetSelection();
  });

  // === Ground plane, shadows, transparency, rendering ======================
  reg.registerCommand("set_ground_plane_mode", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::groundPlaneMode = parseGroundPlaneMode(matlabPtr, getString(getInput(inputs, 1)));
  });

  reg.registerCommand("set_ground_plane_height_mode", [](ArgumentList& outputs, ArgumentList& inputs,
                                                         matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::groundPlaneHeightMode = parseGroundPlaneHeightMode(matlabPtr, getString(getInput(inputs, 1)));
  });

  reg.registerCommand("set_ground_plane_height", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::groundPlaneHeightMode = polyscope::GroundPlaneHeightMode::Manual;
    polyscope::options::groundPlaneHeight = getScalarFloat(getInput(inputs, 1));
  });

  reg.registerCommand("set_ground_plane_height_factor", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 2);
    bool isRelative = true;
    if (inputCount(inputs) > 2) isRelative = getScalarBool(getInput(inputs, 2));
    polyscope::options::groundPlaneHeightMode = polyscope::GroundPlaneHeightMode::Automatic;
    polyscope::options::groundPlaneHeightFactor.set(getScalarFloat(getInput(inputs, 1)), isRelative);
  });

  reg.registerCommand("set_shadow_blur_iters", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::shadowBlurIters = getScalarInt(getInput(inputs, 1));
  });

  reg.registerCommand("set_shadow_darkness", [](ArgumentList& outputs, ArgumentList& inputs,
                                                matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::shadowDarkness = getScalarFloat(getInput(inputs, 1));
  });

  reg.registerCommand("set_transparency_mode", [](ArgumentList& outputs, ArgumentList& inputs,
                                                  matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::transparencyMode = parseTransparencyMode(matlabPtr, getString(getInput(inputs, 1)));
  });

  reg.registerCommand("set_transparency_render_passes", [](ArgumentList& outputs, ArgumentList& inputs,
                                                           matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    polyscope::options::transparencyRenderPasses = getScalarInt(getInput(inputs, 1));
  });

  reg.registerCommand("get_final_scene_color_texture_native_handle", [](ArgumentList& outputs, ArgumentList& inputs,
                                                                        matlab::engine::MATLABEngine* matlabPtr) {
    matlab::data::ArrayFactory factory;
    uint64_t handle = 0;
    if (polyscope::render::engine) {
      handle = static_cast<uint64_t>(polyscope::render::engine->getFinalSceneColorTexture().getNativeBufferID());
    }
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(handle));
  });

  reg.registerCommand("load_image_texture", [](ArgumentList& outputs, ArgumentList& inputs,
                                                matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    if (!polyscope::render::engine) throwError(matlabPtr, "Polyscope must be initialized before loading an image texture");
    const std::string filename = getString(getInput(inputs, 1));
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    if (!pixels) throwError(matlabPtr, "Could not load image texture: " + filename);
    auto texture = polyscope::render::engine->generateTextureBuffer(
        polyscope::TextureFormat::RGBA8, static_cast<unsigned int>(width),
        static_cast<unsigned int>(height), pixels);
    stbi_image_free(pixels);
    texture->setFilterMode(polyscope::FilterMode::Linear);
    const uint64_t handle = static_cast<uint64_t>(texture->getNativeBufferID());
    // Logos are typically drawn much smaller than their source PNG. Build a
    // complete mip chain and use trilinear minification to avoid the blurred,
    // aliased result produced by sampling only the full-resolution level.
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(handle));
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    matlabImageTextures[handle] = std::move(texture);
    matlab::data::ArrayFactory factory;
    getOutput(outputs, 0) = createScalarDouble(factory, static_cast<double>(handle));
    getOutput(outputs, 1) = createScalarDouble(factory, static_cast<double>(width));
    getOutput(outputs, 2) = createScalarDouble(factory, static_cast<double>(height));
  });

  reg.registerCommand("release_image_texture", [](ArgumentList& outputs, ArgumentList& inputs,
                                                   matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 2);
    const uint64_t handle = static_cast<uint64_t>(getScalarDouble(getInput(inputs, 1)));
    matlabImageTextures.erase(handle);
  });

  // === Materials and colormaps ============================================
  reg.registerCommand("load_static_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                 matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 3);
    polyscope::loadStaticMaterial(getString(getInput(inputs, 1)), getString(getInput(inputs, 2)));
  });

  reg.registerCommand("load_blendable_material", [](ArgumentList& outputs, ArgumentList& inputs,
                                                    matlab::engine::MATLABEngine* matlabPtr) {
    checkMinArgs(matlabPtr, inputCount(inputs), 3);
    std::string matName = getString(getInput(inputs, 1));
    if (inputCount(inputs) == 3) {
      std::vector<std::string> filenames = getStringVector(getInput(inputs, 2));
      if (filenames.size() != 4) {
        throwError(matlabPtr, "blendable material filename list must contain exactly 4 filenames");
      }
      polyscope::loadBlendableMaterial(matName, std::array<std::string, 4>{filenames[0], filenames[1], filenames[2], filenames[3]});
    } else {
      checkNArgs(matlabPtr, inputCount(inputs), 4);
      polyscope::loadBlendableMaterial(matName, getString(getInput(inputs, 2)), getString(getInput(inputs, 3)));
    }
  });

  reg.registerCommand("load_color_map", [](ArgumentList& outputs, ArgumentList& inputs,
                                           matlab::engine::MATLABEngine* matlabPtr) {
    checkNArgs(matlabPtr, inputCount(inputs), 3);
    polyscope::loadColorMap(getString(getInput(inputs, 1)), getString(getInput(inputs, 2)));
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

  // === Per-structure slice plane options ====================================
  reg.registerCommand("structure_set_cull_whole_elements",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 3)
                          throwError(matlabPtr, "Expected structure_set_cull_whole_elements(name, val)");
                        getStructureByName(matlabPtr, getString(getInput(inputs, 1)))
                            ->setCullWholeElements(getScalarBool(getInput(inputs, 2)));
                      });

  reg.registerCommand("structure_get_cull_whole_elements",
                      [](ArgumentList& outputs, ArgumentList& inputs, MATLABEngine* matlabPtr) {
                        if (inputCount(inputs) < 2)
                          throwError(matlabPtr, "Expected structure_get_cull_whole_elements(name)");
                        bool val = getStructureByName(matlabPtr, getString(getInput(inputs, 1)))
                                       ->getCullWholeElements();
                        matlab::data::ArrayFactory factory;
                        getOutput(outputs, 0) = createScalarBool(factory, val);
                      });
}

} // namespace ps_mex
