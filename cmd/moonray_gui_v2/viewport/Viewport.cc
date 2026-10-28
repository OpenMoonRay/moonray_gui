// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "Viewport.h"

#include "../camera/FreeCam.h"
#include "../camera/OrbitCam.h"
#include "../DenoiserManager.h"
#include "../FileUtil.h"
#include "imgui.h"
#include "interface/Interface.h"
#include "Keyboard.h"
#include "SnapshotManager.h"

#include <moonray/rendering/rndr/PathVisualizerManager.h>
#include <moonray/rendering/rndr/RenderOutputDriver.h>

#include <GLFW/glfw3.h>

#ifdef MOONRAY_GUI_CLIPBOARD
#include <clip.h>
#include <algorithm>
#endif

using namespace moonray;

namespace {

rndr::FastRenderMode
nextFastMode(rndr::FastRenderMode const &mode) {
    const int numModes = static_cast<int>(rndr::FastRenderMode::NUM_MODES);
    return static_cast<moonray::rndr::FastRenderMode>((static_cast<int>(mode) + 1) % numModes);
}

rndr::FastRenderMode
prevFastMode(rndr::FastRenderMode const &mode) {
    const int numModes = static_cast<int>(rndr::FastRenderMode::NUM_MODES);
    return static_cast<rndr::FastRenderMode>((static_cast<int>(mode) + numModes - 1) % numModes);
}

}

namespace moonray_gui_v2 {
static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void 
keyCallback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods)
{
    // Get the Window instance from the user pointer
    Viewport* viewportInstance = static_cast<Viewport*>(glfwGetWindowUserPointer(window));
    if (viewportInstance) {
        // Call the member function to handle the key press
        viewportInstance->handleKeyEvent(key, scancode, action, mods);
    }
}

void
mouseCallback(GLFWwindow* window, const int button, const int action, const int mods)
{
    // Only call our viewport's handler if the imgui UI
    // doesn't need to capture the mouse event
    if (!ImGui::GetIO().WantCaptureMouse) {
        // Get the Viewport instance from the user pointer
        Viewport* viewportInstance = static_cast<Viewport*>(glfwGetWindowUserPointer(window));
        if (viewportInstance) {
            // Call the member function to handle the mouse press
            viewportInstance->handleMouseEvent(button, action, mods);
        }
    }
}

void
mouseMoveCallback(GLFWwindow* window, const double xpos, const double ypos)
{
    // Only call our viewport's handler if the imgui UI
    // doesn't need to capture the mouse event
    if (!ImGui::GetIO().WantCaptureMouse) {
        // Get the Viewport instance from the user pointer
        Viewport* viewportInstance = static_cast<Viewport*>(glfwGetWindowUserPointer(window));
        if (viewportInstance) {
            // Call the member function to handle the key press
            viewportInstance->handleMouseMove(xpos, ypos);
        }
    }
}

void
scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Only call our viewport's handler if the imgui UI
    // doesn't need to capture the mouse event
    if (!ImGui::GetIO().WantCaptureMouse) {
        // Get the Viewport instance from the user pointer
        Viewport* viewportInstance = static_cast<Viewport*>(glfwGetWindowUserPointer(window));
        if (viewportInstance) {
            // Call the member function to handle the key press
            viewportInstance->handleScrollEvent(xoffset, yoffset);
        }
    }
}

void setGlfwWindowHints()
{
#if defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
}

Viewport::Viewport(CameraType initialCamType, 
                   const std::string& snapPath,
                   const bool showTileProgress,
                   DenoiserManager* denoiserManager)
:   mSnapshotManager(std::make_unique<SnapshotManager>(snapPath, this))
,   mKeyboard(std::make_unique<Keyboard>())
,   mDenoiserManager(denoiserManager)
,   mActiveCameraType(initialCamType)
,   mOrbitCam(std::make_unique<OrbitCam>())
,   mFreeCam(std::make_unique<FreeCam>())
,   mShowTileProgress(showTileProgress)
{    
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        throw std::runtime_error("ERROR: Failed to initialize GLFW");
    }

    setGlfwWindowHints();

    // ---- Create window with graphics context ----
    // The viewport will be resized later when we get the first framebuffer from the renderer.
    // GLFW DOCS: "To create a full screen window, you need to specify the monitor the 
    // window will cover. If no monitor is specified, the window will be windowed mode", and also
    // "...you can specify another window whose context the new one should share its objects 
    // (textures, vertex and element buffers, etc.) with"
    mGLFWWindow = glfwCreateWindow(1, 1, "MoonRay GUI", /*monitor*/ nullptr, /*window to share with*/ nullptr);
    if (!mGLFWWindow) {
        throw std::runtime_error("ERROR: Failed to create GLFW window");
    }
    // Make mGLFWWindow the current context
    glfwMakeContextCurrent(mGLFWWindow);
    glfwSwapInterval(1); // Enable vsync

    // Set this Viewport instance as user pointer so it can be accessed in callbacks
    glfwSetWindowUserPointer(mGLFWWindow, this);
    glfwSetKeyCallback(mGLFWWindow, keyCallback);
    glfwSetMouseButtonCallback(mGLFWWindow, mouseCallback);
    glfwSetCursorPosCallback(mGLFWWindow, mouseMoveCallback);
    glfwSetScrollCallback(mGLFWWindow, scrollCallback);

    // Need to wait until we configure the GLFW window to
    // initialize the user interface
    mInterface = std::make_unique<Interface>(this);
}

Viewport::~Viewport()
{
    // Clean up OpenGL resources
    if (mTextureHandle != 0) {
        glDeleteTextures(1, &mTextureHandle);
    }

    // We must destroy the Interface before terminating glfw
    // because Interface needs to do some glfw cleanup first
    mInterface.reset();
    
    // Clean up GLFW resources
    if (mGLFWWindow) {
        glfwDestroyWindow(mGLFWWindow);
    }
    glfwTerminate();
}

bool
Viewport::isWindowOpen() const
{
    return mOpen;
}

void
Viewport::exec()
{
    // Main loop
    while (!glfwWindowShouldClose(mGLFWWindow)) {
        // Poll and handle user events (key presses, window resize, etc.)
        glfwPollEvents();

        // If the window is minimized, we skip the rendering to save resources.
        // The ImGui UI will still be responsive and can restore the window.
        if (glfwGetWindowAttrib(mGLFWWindow, GLFW_ICONIFIED) != 0) {
            mInterface->sleep();
            continue;
        }

        // Update the texture that imgui renders (do this before ImGui layout)
        updateFrame();

        // Update the axis lines, if necessary
        if (mUpdateAxis) {
            mRenderContext->getCameraAxesScreenSpace(mAxisXDir, mAxisYDir, mAxisZDir);
            mUpdateAxis = false;
        }

        // Draws the imgui UI
        mInterface->draw();

        // Ensure context is current first
        glfwMakeContextCurrent(mGLFWWindow);
        // Swap for the recently drawn buffer (allows for seamless refresh transition)
        glfwSwapBuffers(mGLFWWindow);
    }
    mOpen = false;
}

/// ---------------------------- Camera Methods -------------------------------------- ///
void
Viewport::setCameraRenderContext(moonray::rndr::RenderContext &context)
{
    mOrbitCam->setRenderContext(context);
    mFreeCam->setRenderContext(context);

    // save it, we'll use it for picking
    mRenderContext = &context;
}

void
Viewport::setDefaultCameraTransform(const scene_rdl2::math::Mat4f &xform)
{
    mOrbitCam->resetTransform(xform, true);
    mFreeCam->resetTransform(xform, true);
}

NavigationCam*
Viewport::getNavigationCam()
{
    MNRY_ASSERT(mActiveCameraType < NUM_CAMERA_TYPES);
    return (mActiveCameraType == ORBIT_CAM) ? static_cast<NavigationCam*>(mOrbitCam.get()) :
                                              static_cast<NavigationCam*>(mFreeCam.get());
}

void Viewport::resize(const int width, const int height)
{
    if ((width != mFramebufferWidth || height != mFramebufferHeight) && (width > 0 && height > 0)) {
        // Set the width and height
        mFramebufferWidth = width;
        mFramebufferHeight = height;

        if (!mInitialResizeDone) {
            // Resize the window to fit the image on the first resize only
            // The size of the viewport should initially be the size of the 
            // render, plus room for any stationary ui elements
            int vpWidth = width + mInterface->getRightDockWidth();
            int vpHeight = height + mInterface->getBottomDockHeight();
            glfwSetWindowSize(mGLFWWindow, vpWidth, vpHeight);
            mInitialResizeDone = true;
        }

        // Update the opengl render size
        glViewport(0, 0, width, height);
        
        // Resize the actual image display
        mInterface->resizeImage(width, height);
    }
}

void
Viewport::update(const fb_util::Rgb888Buffer* rgbBuffer)
{
    // Store the new frame data
    mRgbBuffer = rgbBuffer;
}

void
Viewport::updateTexture(const int width, const int height, const unsigned char* pixelData)
{
    // Load existing snapshots on first frame update (deferred from constructor)
    if (!mSnapshotsLoaded) {
        mSnapshotManager->load();
        mSnapshotsLoaded = true;
    }

    // Delete previous texture if it exists
    if (mTextureHandle != 0) {
        glDeleteTextures(1, &mTextureHandle);
    }

    // Create a OpenGL texture identifier
    glGenTextures(1, &mTextureHandle);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    // Set pixel store parameters
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Setup texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Upload processed RGB buffer to texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, mFramebufferWidth, mFramebufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelData);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void
Viewport::updateFrame()
{
    glfwMakeContextCurrent(mGLFWWindow);

    if (mRgbBuffer != nullptr) {
        int width = static_cast<int>(mRgbBuffer->getWidth());
        int height = static_cast<int>(mRgbBuffer->getHeight());
        const unsigned char* pixelData = reinterpret_cast<const unsigned char*>(mRgbBuffer->getData());

        resize(width, height);
        updateTexture(width, height, pixelData);
    }
}

/// ---------------------------- Camera Methods -------------------------------------- ///
void
Viewport::toggleActiveCameraType()
{
    if (mActiveCameraType == ORBIT_CAM) {
        // switch from orbit cam to free cam
        auto xform = mOrbitCam->update(0.0f);
        mOrbitCam->clearMovementState();
        mFreeCam->resetTransform(xform, false);
        mActiveCameraType = FREE_CAM;
    } else {
        // switch from free cam to orbit cam
        auto xform = mFreeCam->update(0.0f);
        mFreeCam->clearMovementState();
        mOrbitCam->resetTransform(xform, false);
        mActiveCameraType = ORBIT_CAM;
    }
}

/// ---------------------------- Denoising Methods -------------------------------------- ///

void Viewport::selectDenoisingBuffers() { mDenoiserManager->selectBuffers(mRenderContext); }
void Viewport::toggleDenoising() { mDenoiserManager->toggleDenoising(); }
void Viewport::toggleDenoisingMode() { mDenoiserManager->toggleDenoisingMode(); }

/// ---------------------------- Inspector Methods -------------------------------------- ///

std::string
Viewport::inspect(int x, int y) const
{
    // Ensure the render context exists before trying to fetch info
    if (!mRenderContext || x < 0 || y < 0) { return ""; }

    switch (mInspectorMode) {
    case INSPECT_LIGHT_CONTRIBUTIONS:
    {
        std::string lightPickResults;
        moonray::shading::LightContribArray rdlLights;
        mRenderContext->handlePickLightContributions(x, y, rdlLights);
        std::sort(rdlLights.begin(), rdlLights.end(),
                    [&](const moonray::shading::LightContrib &l0, const moonray::shading::LightContrib &l1) {
                        return l0.second < l1.second;
                    });
        for (unsigned int i = 0; i < rdlLights.size(); ++i) {
            lightPickResults += rdlLights[i].first->getName() + ": " + 
                                std::to_string(rdlLights[i].second) + ", " + 
                                rdlLights[i].first->getSceneClass().getName() + "\n";
        }

        return lightPickResults;
    }
    break;
    case INSPECT_GEOMETRY:
        {
            const rdl2::Geometry *geometry = mRenderContext->handlePickGeometry(x, y);
            if (geometry) { return geometry->getName() + "\n" + geometry->getSceneClass().getName(); }
        }
        break;
    case INSPECT_GEOMETRY_PART:
        {
            std::string parts;
            const rdl2::Geometry* geometry = mRenderContext->handlePickGeometryPart(x, y, parts);
            if (geometry) { return geometry->getName() + ", " + parts + "\n" + geometry->getSceneClass().getName(); }
        }
        break;
    case INSPECT_MATERIAL:
        {
            const rdl2::Material *material = mRenderContext->handlePickMaterial(x, y);
            if (material) { return material->getName() + "\n" + material->getSceneClass().getName(); }
        }
        break;
    }

    return "";
}

scene_rdl2::math::Color 
Viewport::getPixelColor(const int x, const int y) const
{
    // Bounds check
    if (x < 0 || x >= mFramebufferWidth || y < 0 || y >= mFramebufferHeight) {
        return scene_rdl2::math::Color(0.0f);
    }

    // Read directly from display framebuffer
    // NOTE: This framebuffer data includes any color grading applied for display.
    // If we want the raw render data, we would need to read from the renderer's framebuffer instead.
    // However, reading from the renderer's framebuffer here could be problematic if
    // the renderer is still writing to it (we would need to synchronize access)
    // So for now, we read from the display framebuffer.
    // TODO: Consider either reading from the renderer's framebuffer with proper synchronization,
    // or reverse applying color grading to get the raw data.
    if (mRgbBuffer) {
        const auto& pixelColor = mRgbBuffer->getPixel(x, y);
        return scene_rdl2::math::Color(pixelColor.r / 255.f, pixelColor.g / 255.f, pixelColor.b / 255.f);
    }

    // Fallback: No processed data available
    std::cout << "No buffer available for pixel reading" << std::endl;
    return scene_rdl2::math::Color(0.0f);
}

/// ---------------------------- Color Grading Methods -------------------------------------- ///

void Viewport::toggleExposureWindow() { mInterface->toggleExposureWindow(); }
void Viewport::toggleGammaWindow() { mInterface->toggleGammaWindow(); }

void Viewport::startAdjustExposure() { mUpdateExposure = true; }
void Viewport::startAdjustGamma() { mUpdateGamma = true; }

void Viewport::endAdjustExposure() { mUpdateExposure = false; }
void Viewport::endAdjustGamma() { mUpdateGamma = false; }

void Viewport::resetExposure() { mExposure = 0.f; }
void Viewport::resetGamma() { mGamma = 1.f; }

void Viewport::increaseExposure() { mExposure = math::floor(mExposure) + 1.f; }
void Viewport::decreaseExposure() { mExposure = math::floor(mExposure) - 1.f; }

/// ------------------------------- Other Methods -------------------------------------- ///

void Viewport::takeASnapshot() { mSnapshotManager->takeASnapshot(mRenderContext); }
void Viewport::showPrevSnapshot() { mSnapshotManager->prev(); }
void Viewport::showNextSnapshot() { mSnapshotManager->next(); }

void Viewport::toggleShowTileProgress() { mShowTileProgress = !mShowTileProgress; }

void Viewport::toggleFastProgressiveMode() { mProgressiveFast = !mProgressiveFast; }

void
Viewport::toggleDebugMode(const DebugMode& debugMode)
{
    mDebugMode = (mDebugMode == debugMode) ? RGB : debugMode;
}

void
Viewport::prevRenderOutput()
{
    // move to previous render input
    int numOutputs = mRenderContext->getRenderOutputDriver()->getNumberOfRenderOutputs();
    mRenderOutputIndex = scene_rdl2::math::clamp(mRenderOutputIndex - 1, -1, numOutputs - 1);
}

void
Viewport::nextRenderOutput()
{
    // move to next render input
    int numOutputs = mRenderContext->getRenderOutputDriver()->getNumberOfRenderOutputs();
    mRenderOutputIndex = scene_rdl2::math::clamp(mRenderOutputIndex + 1, -1, numOutputs - 1);
}

std::string 
Viewport::getRenderOutputName() const 
{
    if (!mRenderContext) { return ""; }
    if (mRenderOutputIndex == -1) { return "render buffer"; }

    const moonray::rndr::RenderOutputDriver* roDriver = mRenderContext->getRenderOutputDriver();
    if (!roDriver || mRenderOutputIndex >= roDriver->getNumberOfRenderOutputs() || mRenderOutputIndex < 0) {
        return "";
    }
    return roDriver->getRenderOutput(mRenderOutputIndex)->getName();
}

void
Viewport::setPathVisualizerPixel()
{
    moonray::rndr::PathVisualizerManager* manager = getPathVisualizerManager();
    if (manager && manager->isOn()) {
        const ImVec2 pixel = mInterface->getCurrentPixel();

        if (pixel.x >= 0 && pixel.y >= 0) {
            manager->setPixel(pixel.x, pixel.y, /*update*/ true);
        }
    }
}

#ifdef MOONRAY_GUI_CLIPBOARD
void Viewport::copyCurrentBufferToClipboard() const
{
    const GLuint textureHandle = getDisplayTexture();
    if (textureHandle == 0) {
        Logger::error("Could not get texture handle");
        return;
    }

    int width = 0, height = 0;

    glfwMakeContextCurrent(mGLFWWindow);

    // Flush unrelated GL errors
    while (glGetError() != GL_NO_ERROR) {}

    glBindTexture(GL_TEXTURE_2D, textureHandle);

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    if (height <= 0 || width <= 0) {
        Logger::error("Invalid dimensions");
        return;
    }

    std::vector<uint32_t> pixels;
    pixels.resize(width * height);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);


    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::error("GL reported an error after copying the display texture: 0x", std::hex, err);
        return;
    }

    // flipping pixels vertically in-place
    for (int y = 0; y < height / 2; ++y) {
        auto top_it = pixels.begin() + y * width;
        auto bottom_it = pixels.begin() + (height - 1 - y) * width;
        std::swap_ranges(top_it, top_it + width, bottom_it);
    }

    clip::image_spec spec;
    spec.width = width;
    spec.height = height;
    spec.bits_per_pixel = 32;
    spec.bytes_per_row = width * sizeof(uint32_t);
    spec.red_mask = 0xff;
    spec.green_mask = 0xff00;
    spec.blue_mask = 0xff0000;
    spec.alpha_mask = 0xff000000;
    spec.red_shift = 0;
    spec.green_shift = 8;
    spec.blue_shift = 16;
    spec.alpha_shift = 24;

    clip::image snapshot(pixels.data(), spec);
    if (!clip::set_image(snapshot)) {
        Logger::error("Failed to copy image data to clipboard");
        return;
    }

    std::cout << "Copied displayed image to clipboard" << std::endl;
}
#else
void Viewport::copyCurrentBufferToClipboard() const {
    Logger::error("Clipboard is not enabled for this build");
}
#endif

void
Viewport::pathVisualizerToggleOn()
{
    moonray::rndr::PathVisualizerManager* manager = getPathVisualizerManager();
    if (manager) {
        if (manager->isOn()) {
            manager->turnOff();
        } else {
            manager->turnOn();
        }
    }
}

void
Viewport::prevPathVisualizerNode()
{
    moonray::rndr::PathVisualizerManager* manager = getPathVisualizerManager();
    if (manager && manager->isOn()) {
        manager->prevNode();
    }
}

void
Viewport::nextPathVisualizerNode()
{
    moonray::rndr::PathVisualizerManager* manager = getPathVisualizerManager();
    if (manager && manager->isOn()) {
        manager->nextNode();
    }
}

/// ------------------------------- Action Handling ---------------------------------------------------------------- ///
void
Viewport::handlePressAction(const Action action)
{
    switch(action) {
        // Denoising actions
        case ACTION_DENOISE_TOGGLE_ON_OFF:          toggleDenoising();                      mNeedsRefresh = true; break;
        case ACTION_DENOISE_TOGGLE_MODE:            toggleDenoisingMode();                  mNeedsRefresh = true; break;
        case ACTION_DENOISE_SELECT_BUFFERS:         selectDenoisingBuffers();               mNeedsRefresh = true; break;

        // Snapshot actions
        case ACTION_SNAPSHOT_TAKE:                  takeASnapshot();                                              break;
        case ACTION_COPY_BUFFER_TO_CLIPBOARD:       copyCurrentBufferToClipboard();                               break;

        // Channel actions
        case ACTION_CHANNEL_TOGGLE_RGB:             toggleDebugMode(RGB);                   mNeedsRefresh = true; break;
        case ACTION_CHANNEL_TOGGLE_RED:             toggleDebugMode(RED);                   mNeedsRefresh = true; break;
        case ACTION_CHANNEL_TOGGLE_GREEN:           toggleDebugMode(GREEN);                 mNeedsRefresh = true; break;
        case ACTION_CHANNEL_TOGGLE_BLUE:            toggleDebugMode(BLUE);                  mNeedsRefresh = true; break;
        case ACTION_CHANNEL_TOGGLE_ALPHA:           toggleDebugMode(ALPHA);                 mNeedsRefresh = true; break;
        case ACTION_CHANNEL_TOGGLE_LUMINANCE:       toggleDebugMode(LUMINANCE);             mNeedsRefresh = true; break;
        case ACTION_CHANNEL_TOGGLE_RGB_NORMALIZED:  toggleDebugMode(RGB_NORMALIZED);        mNeedsRefresh = true; break;
        case ACTION_CHANNEL_TOGGLE_NUM_SAMPLES:     toggleDebugMode(NUM_SAMPLES);           mNeedsRefresh = true; break;
        
        // Image adjustment actions
        case ACTION_EXPOSURE_ADJUST:                startAdjustExposure();                                        break;
        case ACTION_EXPOSURE_INCREASE:              increaseExposure();                     mNeedsRefresh = true; break;
        case ACTION_EXPOSURE_DECREASE:              decreaseExposure();                     mNeedsRefresh = true; break;
        case ACTION_EXPOSURE_RESET:                 resetExposure();                        mNeedsRefresh = true; break;
        case ACTION_GAMMA_ADJUST:                   startAdjustGamma();                                           break;
        case ACTION_GAMMA_RESET:                    resetGamma();                           mNeedsRefresh = true; break;
        case ACTION_IMAGE2D_PAN:                    mPanImage = true;                                             break;

        // Fast progressive mode actions
        case ACTION_FAST_PROGRESSIVE_TOGGLE:        toggleFastProgressiveMode();            mNeedsRefresh = true; break;
        case ACTION_FAST_PROGRESSIVE_NEXT_MODE:     mFastMode = nextFastMode(mFastMode);    mNeedsRefresh = true; break;
        case ACTION_FAST_PROGRESSIVE_PREV_MODE:     mFastMode = prevFastMode(mFastMode);    mNeedsRefresh = true; break;

        // Misc actions
        case ACTION_SAVE_IMAGE:                     saveEXR(mRenderContext);                                      break;
        case ACTION_CAM_TOGGLE_ACTIVE_TYPE:         toggleActiveCameraType();               mNeedsRefresh = true; break;
        case ACTION_PATH_VISUALIZER_PREV_NODE:      getPathVisualizerManager()->prevNode(); mNeedsRefresh = true; break;
        case ACTION_PATH_VISUALIZER_NEXT_NODE:      getPathVisualizerManager()->nextNode(); mNeedsRefresh = true; break;
        case ACTION_TILE_PROGRESS_TOGGLE:           toggleShowTileProgress();               mNeedsRefresh = true; break;
        case ACTION_PICK_PATH_VISUALIZER_PIXEL:     setPathVisualizerPixel();                                     break;
        case ACTION_RENDER_OUTPUT_PREV:             prevRenderOutput();                     mNeedsRefresh = true; break;
        case ACTION_RENDER_OUTPUT_NEXT:             nextRenderOutput();                     mNeedsRefresh = true; break;
        case ACTION_PRINT_KEY_BINDINGS:             mKeyboard->printKeyBindings();                                break;
        default: break;
    }
}

void
Viewport::handleReleaseAction(const Action action, bool wasQuickPress)
{
    switch (action) {
        case ACTION_EXPOSURE_ADJUST:
            if (wasQuickPress) { resetExposure(); }
            endAdjustExposure();
            mNeedsRefresh = true;
            break;
        case ACTION_GAMMA_ADJUST:
            if (wasQuickPress) { resetGamma(); }
            endAdjustGamma();
            mNeedsRefresh = true;
            break;
        case ACTION_IMAGE2D_PAN:
            mPanImage = false;
            break;
        default: 
            break;
    }
}

/// ---------------------------- Key Press Event Handlers ----------------------------- ///

void
Viewport::handleKeyPressEvent(const Action action)
{
    // Try having the camera handle the action
    if (getNavigationCam()->processKeyPressEvent(mGLFWWindow, action)) { return; }

    // Otherwise, handle it in the viewport
    handlePressAction(action);
}

void
Viewport::handleKeyReleaseEvent(const Action action)
{
    // Try having the camera handle the action
    if (getNavigationCam()->processKeyReleaseEvent(mGLFWWindow, action)) { return; }

    // Otherwise, handle it in the viewport
    handleReleaseAction(action, true);
}

void
Viewport::handleKeyEvent(const int key, const int scancode, const int type, const int mods)
{
    const Action action = mKeyboard->getActionFromInput(mGLFWWindow, key, mods);

    // First, check if the interface wants to explicitly
    // handle this action (currently, just for opening/closing windows)
    // NOTE: We have automatic ImGui key handling enabled in the Interface,
    //       so we don't need to explicitly handle any window interactions 
    //       (ex: moving a slider) here.
    if (type == GLFW_PRESS && mInterface->handleKeyPressEvent(action)) { return; }

    // Otherwise, unless ImGui wants to automatically 
    // capture this key event, this is a viewport-specific 
    // action, and should be handled here
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (type == GLFW_PRESS) {
            // key press handling
            handleKeyPressEvent(action); 
        } else if (type == GLFW_RELEASE) {
            // key release handling
            handleKeyReleaseEvent(action);
        }
    }
}

/// ---------------------------- Mouse Press Event Handlers ----------------------------- ///

void
Viewport::handleMousePressEvent(const Action action)
{
    // Cache current cursor position for drag operations
    double cursorX, cursorY;
    glfwGetCursorPos(mGLFWWindow, &cursorX, &cursorY);
    mMousePosX = static_cast<int>(cursorX);
    mMousePosY = static_cast<int>(cursorY);

    // Start timing for quick-click detection
    mMouseTimer.start();

    // Try having the camera handle the action
    if (getNavigationCam()->processKeyPressEvent(mGLFWWindow, action)) { return; }

    // Otherwise, handle it in the viewport
    handlePressAction(action);
}

void 
Viewport::handleMouseReleaseEvent(const Action action)
{
    // End timing for quick click detection
    mMouseTimer.end();

    // Try having the camera handle the action
    if (getNavigationCam()->processKeyReleaseEvent(mGLFWWindow, action)) { return; }

    // Otherwise, handle it in the viewport
    handleReleaseAction(action, mMouseTimer.wasQuickClick());
}

void
Viewport::handleMouseEvent(const int button, const int type, const int mods)
{
    const Action action = mKeyboard->getActionFromInput(mGLFWWindow, button, mods);

    if (type == GLFW_PRESS) {
        handleMousePressEvent(action);
    } else if (type == GLFW_RELEASE) {
        handleMouseReleaseEvent(action);
    }
}

/// ---------------------------- Mouse Move Event Handlers ----------------------------- ///
void
Viewport::handleMouseMove(const double xpos, const double ypos)
{
    // Handle exposure/gamma adjustment by mouse drag
    if (mUpdateExposure) {
        const int currentPosX = static_cast<int>(xpos);
        const int deltaX = currentPosX - mMousePosX;

        mExposure += 0.01f * deltaX;
        mMousePosX = currentPosX;
        mNeedsRefresh = true;

    } else if (mUpdateGamma) {
        const int currentPosX = static_cast<int>(xpos);
        const int deltaX = currentPosX - mMousePosX;

        mGamma += 0.005f * deltaX;
        mGamma = std::max(mGamma, 0.1f); // Clamp minimum gamma
        mMousePosX = currentPosX;
        mNeedsRefresh = true;

    } else if (mPanImage) {
        int currentPosX = static_cast<int>(xpos);
        int currentPosY = static_cast<int>(ypos);
        mInterface->pan(currentPosX - mMousePosX, currentPosY - mMousePosY);
        mMousePosX = currentPosX;
        mMousePosY = currentPosY;
    }

    getNavigationCam()->processMouseMoveEvent(xpos, ypos);
}

void
Viewport::handleScrollEvent(const double xoffset, const double yoffset)
{
    mInterface->zoom(xoffset, yoffset);
}

/// ----------------------------------- Getters ----------------------------------------- ///

bool Viewport::getDenoisingEnabled() const { return mDenoiserManager->getDenoisingEnabled(); }

moonray::denoiser::DenoiserMode Viewport::getDenoiserMode() const { return mDenoiserManager->getDenoiserMode(); }

DenoisingBufferMode Viewport::getDenoisingBufferMode() const { return mDenoiserManager->getDenoisingBufferMode(); }

GLuint 
Viewport::getDisplayTexture() const {
    GLuint textureHandle = mTextureHandle;

    // If a snapshot is chosen, return that texture instead
    if (!mSnapshotManager) {
        return textureHandle;
    }
    int snapshotTexture = mSnapshotManager->getChosenSnapshotTexture();
    if (snapshotTexture != -1) {
        textureHandle = snapshotTexture;
    }
    return textureHandle;
}

} // namespace moonray_gui_v2
