
// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "Interface.h"

#include "AxisDisplay.h"
#include "ExposureWindow.h"
#include "GammaWindow.h"
#include "HelpWindow.h"
#include "KeyBindingsWindow.h"
#include "ImageDisplay.h"
#include "PathVisualizerWindow.h"
#include "PixelInspector.h"
#include "SceneInspector.h"
#include "SnapshotWindow.h"
#include "StatusBar.h"
#include "../Viewport.h"
#include "../../FileUtil.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <stdexcept>    // for std::runtime_error

namespace moonray_gui_v2 {

Interface::Interface(Viewport* viewport)
: mViewport(viewport)
, mAxisDisplay(std::make_unique<AxisDisplay>())
, mExposureWindow(std::make_unique<ExposureWindow>())
, mGammaWindow(std::make_unique<GammaWindow>())
, mHelpWindow(std::make_unique<HelpWindow>())
, mKeyBindingsWindow(std::make_unique<KeyBindingsWindow>())
, mImageDisplay(std::make_unique<ImageDisplay>())
, mPathVisualizerWindow(std::make_unique<PathVisualizerWindow>())
, mPixelInspector(std::make_unique<PixelInspector>())
, mSceneInspector(std::make_unique<SceneInspector>())
, mSnapshotWindow(std::make_unique<SnapshotWindow>())
, mStatusBar(std::make_unique<StatusBar>())
{
    if (!viewport) {
        throw std::invalid_argument("Viewport cannot be null");
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    // Enable automatic installation of callbacks so that ImGui can handle input events
    // (e.g. slider dragging, button clicks, etc.) within ImGui windows automatically
    if (!ImGui_ImplGlfw_InitForOpenGL(mViewport->getGLFWWindow(), /*automatic callback install*/ true)) {
        throw std::runtime_error("Failed to initialize ImGui GLFW implementation");
    }
    
    if (!ImGui_ImplOpenGL3_Init()) {
        ImGui_ImplGlfw_Shutdown();
        throw std::runtime_error("Failed to initialize ImGui OpenGL3 implementation");
    }

    // Configure ImGui settings
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

    // Set ImGui ini file to user config directory instead of current working directory.
    // This prevents cluttering the project directory with imgui.ini files.
    // The path is stored as a static variable to ensure it persists for ImGui's lifetime.

    /// TODO: A imgui.ini file is used to preserve ImGui window layouts 
    /// and settings between sessions. However, the current implementation
    /// was not designed to leverage this feature fully -- for instance, window positions and sizes
    /// are often calculated dynamically based on the viewport size. In the future,
    /// we should evaluate how to better utilize this feature, or disable it altogether.
    static std::string iniFilePath;
    std::string configDir = getConfigDirectory();
    if (!configDir.empty()) {
        iniFilePath = configDir + "/imgui.ini";
        io.IniFilename = iniFilePath.c_str();
    } else {
        // If we can't create the config directory, disable ini file persistence
        io.IniFilename = nullptr;
        std::cerr << "Warning: ImGui settings will not be saved between sessions." << std::endl;
    }

    // Add UI components here
    mComponents.push_back(mAxisDisplay.get());
    mComponents.push_back(mExposureWindow.get());
    mComponents.push_back(mGammaWindow.get());
    mComponents.push_back(mHelpWindow.get());
    mComponents.push_back(mKeyBindingsWindow.get());
    mComponents.push_back(mPathVisualizerWindow.get());
    mComponents.push_back(mPixelInspector.get());
    mComponents.push_back(mSceneInspector.get());
    mComponents.push_back(mSnapshotWindow.get());
    mComponents.push_back(mStatusBar.get());

    // Add components to docks (order matters)
    mBottomDock.addComponent(mStatusBar.get());
    mBottomDock.addComponent(mSnapshotWindow.get());
    mRightDock.addComponent(mPathVisualizerWindow.get());

    // Set up callback for help button in StatusBar
    mStatusBar->setHelpButtonCallback([this]() { this->toggleHelpWindow(); });
}

Interface::~Interface()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool
Interface::handleKeyPressEvent(const Action action)
{
    // If we are currently capturing a new keybinding, only 
    // the key bindings window should handle the input
    if (mKeyBindingsWindow->isCapturing()) { return true; }

    switch(action) {
    case ACTION_WINDOW_TOGGLE_EXPOSURE:         toggleExposureWindow();                     return true;
    case ACTION_WINDOW_TOGGLE_GAMMA:            toggleGammaWindow();                        return true;
    case ACTION_WINDOW_TOGGLE_KEY_BINDINGS:     toggleKeyBindings();                        return true;
    case ACTION_WINDOW_TOGGLE_PATH_VISUALIZER:  togglePathVisualizerWindow();               return true;
    case ACTION_WINDOW_TOGGLE_PIXEL_INSPECTOR:  togglePixelInspector();                     return true;
    case ACTION_WINDOW_TOGGLE_SCENE_INSPECTOR:  toggleSceneInspector();                     return true;
    case ACTION_WINDOW_TOGGLE_SNAPSHOT:         toggleSnapshotWindow();                     return true;
    case ACTION_WINDOW_TOGGLE_STATUS:           toggleStatusBar();                          return true;
    case ACTION_SNAPSHOT_PREV:                  mViewport->showPrevSnapshot();              return true;
    case ACTION_SNAPSHOT_NEXT:                  mViewport->showNextSnapshot();              return true;
    case ACTION_PATH_VISUALIZER_ON_OFF:         mViewport->pathVisualizerToggleOn();        return true;
    case ACTION_PATH_VISUALIZER_NEXT_NODE:      mViewport->nextPathVisualizerNode();        return true;
    case ACTION_PATH_VISUALIZER_PREV_NODE:      mViewport->prevPathVisualizerNode();        return true;
    case ACTION_WINDOW_TOGGLE_AXIS_DISPLAY:     toggleAxisDisplay();                        return true;
    case ACTION_COPY_BUFFER_TO_CLIPBOARD:       mViewport->copyCurrentBufferToClipboard();  return true;
    default: break;
    }
    return false;
}

void
Interface::draw()
{
    // Request a new frame
    newFrame(); 

    // Get the current pixel under the mouse
    const ImVec2 currentPixel = getCurrentPixel();

    // Draw all components
    for (Component* component : mComponents) {
        // Get dock offset for this component (widthOffset, heightOffset)
        ImVec2 dockOffset(mRightDock.getOffset(component), 
                          mBottomDock.getOffset(component));
        // Draw the component
        component->draw(mViewport, currentPixel, dockOffset);
    }

    // Resize the viewport if necessary
    resizeViewport();

    // Get how much space is available for the image display
    ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
    const int availWidth = imguiViewport->Size.x - mRightDock.getWidth();
    const int availHeight = imguiViewport->Size.y - mBottomDock.getHeight();
  
    // Draw the image
    mImageDisplay->draw(mViewport, availWidth, availHeight);

    render();
}

void
Interface::render()
{
    // Set up proper OpenGL viewport and clear framebuffer before rendering
    glViewport(0, 0, mViewport->getFramebufferWidth(), mViewport->getFramebufferHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Black background
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void
Interface::newFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void
Interface::sleep(int milliseconds)
{
    ImGui_ImplGlfw_Sleep(milliseconds);
}

// --------------------------- Sizing/Resizing ---------------------------- ///

void
Interface::resizeImage(const int width, const int height)
{
    // Update image size for the display component
    mImageDisplay->setImageSize(width, height);
    glViewport(0, 0, width, height);
}

void
Interface::resizeViewport()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport->Size.x == 0 && viewport->Size.y == 0) {
        // Viewport size is not yet initialized
        return;
    }

    // Calculate the new dock size
    mRightDock.updateWidth();
    mBottomDock.updateHeight();

    // If there's a change, a docked component has been opened/closed
    if (mRightDock.hasChanged() || mBottomDock.hasChanged()) {
        // Update the viewport size
        const int vpWidth = viewport->Size.x + mRightDock.getWidthDiff();
        const int vpHeight = viewport->Size.y + mBottomDock.getHeightDiff();
        glfwSetWindowSize(mViewport->getGLFWWindow(), vpWidth, vpHeight);
    }
}

// ------------------- Toggle open/closed windows ----------------------- ///

void Interface::toggleAxisDisplay() { mAxisDisplay->toggle(); }
void Interface::toggleExposureWindow() { mExposureWindow->toggle(); }
void Interface::toggleGammaWindow() { mGammaWindow->toggle(); }
void Interface::toggleHelpWindow() { mHelpWindow->toggle(); }
void Interface::toggleKeyBindings() { mKeyBindingsWindow->toggle(); }
void Interface::togglePathVisualizerWindow() { mPathVisualizerWindow->toggle(); }
void Interface::togglePixelInspector() { mPixelInspector->toggle(); }
void Interface::toggleSceneInspector() { mSceneInspector->toggle(); }
void Interface::toggleSnapshotWindow() { mSnapshotWindow->toggle(); }
void Interface::toggleStatusBar() { mStatusBar->toggle(); }

/// ----------------------- Image Operations ---------------------------- ///
ImVec2
Interface::getCurrentPixel() const
{
    ImGuiIO& io = ImGui::GetIO();
    const int mousePosX = io.MousePos.x;
    const int mousePosY = io.MousePos.y;

    return mImageDisplay->viewportToImageCoords(mousePosX, mousePosY);
}

void
Interface::zoom(const int xoffset, const int yoffset)
{
    mImageDisplay->zoom(xoffset, yoffset);
}

void
Interface::pan(const int xoffset, const int yoffset)
{
    mImageDisplay->pan(xoffset, yoffset);
}

} // namespace moonray_gui_v2
