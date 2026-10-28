// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../GuiTypes.h"
#include "MouseTimer.h"

#include <mcrt_denoise/denoiser/Denoiser.h>
#include <moonray/rendering/rndr/rndr.h>
#include <scene_rdl2/common/math/Math.h>

#include <GLFW/glfw3.h>

using namespace moonray::rndr;

namespace moonray_gui_v2 {

class DenoiserManager;
class Interface;
class FreeCam;
class Keyboard;
class NavigationCam;
class OrbitCam;
class SnapshotManager;

/* The Viewport class holds anything related to the viewport rendering and interaction. It also interfaces with 
   the Imgui GUI management class, which handles any user menus, windows, or frontend tools. */
class Viewport
{
public:
    Viewport(const CameraType initialCamType, 
             const std::string& snapPath,
             const bool showTileProgress,
             DenoiserManager* denoiserManager);
    ~Viewport();

    // Executes the main event loop
    void exec();

    // Updates the framebuffer data and type
    void update(const fb_util::Rgb888Buffer* rgbBuffer);

    // Keyboard and mouse event handlers
    // They need to be public so that the static GLFW callback functions can call them
    void handleKeyEvent(int key, int scancode, int type, int mods);
    void handleMouseEvent(int button, int type, int mods);
    void handleMouseMove(double xpos, double ypos);
    void handleScrollEvent(const double xoffset, const double yoffset);

    // Inspects the current pixel, based on the current inspector mode
    // and returns a string containing relevant info
    std::string inspect(const int x, const int y) const;

    // Show previous/next snapshot
    void showPrevSnapshot();
    void showNextSnapshot();

    // Toggle path visualizer on/off
    void pathVisualizerToggleOn();

    // Select the next/prev node
    void prevPathVisualizerNode();
    void nextPathVisualizerNode();

    // -------------------------------------- Getters --------------------------------------- //
    // Checks if the GLFW window has been closed
    bool isWindowOpen() const;

    // Gets exposure
    float getExposure() const { return mExposure; }

    // Gets gamma
    float getGamma() const { return mGamma; }

    // Gets current debug mode (i.e. channel being viewed)
    DebugMode getDebugMode() const { return mDebugMode; }

    // Gets whether to show tile progress overlay
    bool getShowTileProgress() const { return mShowTileProgress; }

    // Gets whether fast progressive mode is on
    bool isFastProgressive() const { return mProgressiveFast; }

    // Gets current fast mode
    FastRenderMode getFastMode() const { return mFastMode; }

    // Gets whether the viewport texture needs to be refreshed
    bool getNeedsRefresh() const { return mNeedsRefresh; }

    // Gets framebuffer dimensions
    int getFramebufferWidth() const { return mFramebufferWidth; }
    int getFramebufferHeight() const { return mFramebufferHeight; }

    // Gets OpenGL texture handle for framebuffer
    GLuint getFramebufferTexture() const { return mTextureHandle; }

    // Gets the texture to display (either framebuffer or snapshot)
    GLuint getDisplayTexture() const;

    // Gets the GLFW window pointer
    GLFWwindow* getGLFWWindow() const { return mGLFWWindow; }

    // Gets the current framebuffer
    const fb_util::Rgb888Buffer* getRgbBuffer() const { return mRgbBuffer; }

    // Gets the active camera type (either free or orbit)
    CameraType getActiveCameraType() const { return mActiveCameraType; }

    // Gets the current navigation camera (either orbit or free)
    NavigationCam* getNavigationCam();

    // Gets pointer to snapshot manager
    SnapshotManager* getSnapshotManager() { return mSnapshotManager.get(); }

    // Gets pointer to keyboard state
    const Keyboard* getKeyboard() const { return mKeyboard.get(); }
    Keyboard* getKeyboard() { return mKeyboard.get(); }

    // Gets the current render output index
    int getRenderOutputIndex() const { return mRenderOutputIndex; }

    // Gets a human-readable name for the current render output
    std::string getRenderOutputName() const;

    // Getters for exposure and gamma pointers (for ImGui sliders)
    float* getExposurePtr() { return &mExposure; }
    float* getGammaPtr() { return &mGamma; }

    // Get whether denoising is enabled
    bool getDenoisingEnabled() const;

    // Get the current denoiser mode
    moonray::denoiser::DenoiserMode getDenoiserMode() const;

    // Get the current denoising buffers used
    DenoisingBufferMode getDenoisingBufferMode() const;

    // Gets an editable pointer to the current scene inspector mode
    int* getSceneInspectorModePtr() { return &mInspectorMode; }

    // Gets the color of the pixel at the given pixel coordinates
    scene_rdl2::math::Color getPixelColor(const int x, const int y) const;

    // Get pointer to path visualizer manager
    moonray::rndr::PathVisualizerManager* getPathVisualizerManager() const { 
        if (!mRenderContext) return nullptr;
        return mRenderContext->getPathVisualizerManager().get(); 
    };

    bool getPVShowOnlyEndpoints() const { return mPVShowOnlyEndpoints; }

    // Get whether to update the navigation lines
    bool getUpdateAxis() const { return mUpdateAxis; }

    // Get the screen-space display vector for the x, y, and z axes
    const scene_rdl2::math::Vec2f& getAxisXDir() const { return mAxisXDir; }
    const scene_rdl2::math::Vec2f& getAxisYDir() const { return mAxisYDir; }
    const scene_rdl2::math::Vec2f& getAxisZDir() const { return mAxisZDir; }

    /// ---------------------------------- Setters ----------------------------------------- ///
    // Sets the render context for the cameras
    void setCameraRenderContext(RenderContext &context);

    // Updates the render context pointer (called when RenderContext is recreated)
    void updateRenderContext(RenderContext* context) { mRenderContext = context; }

    // Sets the default camera transform
    void setDefaultCameraTransform(const scene_rdl2::math::Mat4f &xform);

    // Sets whether to show tile progress overlay
    bool setShowTileProgress(const bool show) { return mShowTileProgress = show; }

    // Sets whether fast progressive mode is on
    void setFastProgressive(const bool fast) { mProgressiveFast = fast; }

    // Sets current fast mode
    void setFastMode(const FastRenderMode mode) { mFastMode = mode; }

    // Sets whether the viewport texture needs to be refreshed
    void setNeedsRefresh(const bool refresh) { mNeedsRefresh = refresh; }

    // Sets whether we need to update the navigation lines
    void setUpdateAxis(const bool update) { mUpdateAxis = update; }

    // Sets the path visualizer setting to show only endpoints
    void setPVShowOnlyEndpoints(const bool show) { mPVShowOnlyEndpoints = show; }

    // Copy snapshot/buffer to clipboard
    void copyCurrentBufferToClipboard() const;

private:
    // Uses the framebuffer data to update the viewport display
    void updateFrame();

    // Updates the opengl texture used for imgui display
    void updateTexture(const int width, const int height, const unsigned char* pixelData);

    // Resizes the viewport
    void resize(const int width, const int height);

    /// Action handlers
    void handlePressAction(const Action action);
    void handleReleaseAction(const Action action, bool wasQuickPress);

    /// Keyboard event handlers
    void handleKeyPressEvent(const Action action);
    void handleKeyReleaseEvent(const Action action);

    /// Mouse event handlers
    void handleMousePressEvent(const Action action);
    void handleMouseReleaseEvent(const Action action);

    /// ------------ Functions corresponding to hotkeys -------------- ///
    // Toggle active camera type (free or orbit)
    void toggleActiveCameraType();

    // Select the denoising buffer(s) to use, based on available render outputs
    void selectDenoisingBuffers();

    // Toggles denoising on/off
    void toggleDenoising();

    // Toggles type of denoiser used
    void toggleDenoisingMode();

    // Open/close exposure window
    void toggleExposureWindow();

    // Open/close gamma window
    void toggleGammaWindow();

    // Start adjusting the exposure
    void startAdjustExposure();

    // Start adjusting the gamma
    void startAdjustGamma();

    // End adjusting the exposure
    void endAdjustExposure();

    // End adjusting the gamma
    void endAdjustGamma();

    // Reset exposure levels
    void resetExposure();

    // Reset gamma levels
    void resetGamma();

    // Increase/decrease exposure by a set amount
    void increaseExposure();
    void decreaseExposure();

    // Turn opencolorio management on/off
    void toggleOCIO();

    // Take a snapshot and save to user's indicated path
    void takeASnapshot();

    // Turn on/off tile progress overlay
    void toggleShowTileProgress();

    // Turn on/off fast progressive rendering
    void toggleFastProgressiveMode();

    // Select debug mode (i.e. channel to display)
    void toggleDebugMode(const DebugMode& mode);

    // Show the next/prev render output
    void prevRenderOutput();
    void nextRenderOutput();

    // Select the pixel for the path visualizer
    void setPathVisualizerPixel();

    /// ---------------------------------------------------------------///

    GLFWwindow* mGLFWWindow {nullptr};                          // Pointer to the GLFW window
    std::unique_ptr<Interface> mInterface {nullptr};            // Pointer to the Imgui UI manager
    RenderContext *mRenderContext {nullptr};                    // Pointer to the render context

    std::unique_ptr<SnapshotManager> mSnapshotManager;          // Ptr to manager for snapshot taking and saving
    const std::unique_ptr<Keyboard> mKeyboard {};               // Ptr to key/mouse binding information
    DenoiserManager* mDenoiserManager {nullptr};                // Ptr to denoiser manager
    
    // Frame data
    const fb_util::Rgb888Buffer* mRgbBuffer {nullptr};          // Pointer to the current RGB888 framebuffer
    GLuint mTextureHandle {0};                                  // OpenGL texture handle for framebuffer display
    int mFramebufferWidth {-1};                                 // Framebuffer width
    int mFramebufferHeight {-1};                                // Framebuffer height

    // Camera variables
    CameraType mActiveCameraType {ORBIT_CAM};                   // Current active camera type (either orbit or free)
    std::unique_ptr<OrbitCam> mOrbitCam {nullptr};              // Orbit camera instance
    std::unique_ptr<FreeCam> mFreeCam {nullptr};                // Free camera instance

    bool mNeedsRefresh {true};                                  // Does the viewport need to be refreshed?
    bool mInitialResizeDone {false};                            // Whether the viewport has been sized to fit the render

    DebugMode mDebugMode {RGB};                                 // Current debug mode (i.e. channel being viewed)
    bool mShowTileProgress {true};                              // Whether to show tile progress overlay
    bool mApplyColorRenderTransform;                            // Whether to apply color render transform

    bool mUpdateExposure {false};                               // is exposure being updated?
    bool mUpdateGamma {false};                                  // is gamma being updated?
    float mExposure {0.f};                                      // current exposure value
    float mGamma {1.f};                                         // current gamma value

    int mRenderOutputIndex {-1};                                // Current render output index
    bool mProgressiveFast {false};                              // Whether fast progressive mode is on
    FastRenderMode mFastMode {FastRenderMode::NORMALS};         // Current fast progressive mode
    int mInspectorMode {INSPECT_NONE};                          // Current inspector mode

    int mMousePosX {0};                                         // x position of the mouse
    int mMousePosY {0};                                         // y position of the mouse
    MouseTimer mMouseTimer {};                                  // calculates time between clicks

    bool mPanImage {false};                                     // is image panning active

    bool mSnapshotsLoaded {false};                              // have we loaded existing snapshots yet?

    bool mOpen {true};                                          // whether the viewport is open

    bool mUpdateAxis {false};                                   // whether to update axis direction vectors
    scene_rdl2::math::Vec2f mAxisXDir;                          // Navigation direction vector for the X axis
    scene_rdl2::math::Vec2f mAxisYDir;                          // Navigation direction vector for the Y axis
    scene_rdl2::math::Vec2f mAxisZDir;                          // Navigation direction vector for the Z axis

    bool mPVShowOnlyEndpoints {false};                          // if path visualizer should only display endpoints
};
   
} // namespace moonray_gui_v2
