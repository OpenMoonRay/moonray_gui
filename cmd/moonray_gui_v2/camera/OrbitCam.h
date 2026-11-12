// Copyright 2023-2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "NavigationCam.h"

namespace moonray_gui_v2 {

struct Camera;

class OrbitCam : public NavigationCam
{
public:
                         OrbitCam();
                        ~OrbitCam();

    void                setRenderContext(const moonray::rndr::RenderContext &context) override;

    /// The active render context should be set before calling this function.
    scene_rdl2::math::Mat4f  resetTransform(const scene_rdl2::math::Mat4f &xform, const bool makeDefault) override;

    scene_rdl2::math::Mat4f  update(const float dt) override;

    /// Returns true if the input was used, false if ignored.
    bool                processKeyPressEvent(GLFWwindow* window, const Action action) override;
    bool                processKeyReleaseEvent(GLFWwindow* window, const Action action) override;
    bool                processMouseMoveEvent(const double xpos, const double ypos) override;
    void                clearMovementState() override;

private:
    enum MouseMode
    {
        NONE,
        ROTATE,
        TRACK,
        DOLLY,
        ROLL,
    };

    // Run a center-pixel "pick" operation to compute camera focus
    void                pickFocusPoint();

    // Recenter the camera on the chosen focus point
    void                recenterCamera();

    // Reset the camera to its initial state
    void                resetCamera();

    // Frame the scene by adjusting the camera position and orientation to fit the entire scene within the view.
    // This is a best effort based on the scene bounds and may not account for all camera projection 
    // parameters (e.g. FOV, aspect ratio, near plane, etc).
    void                frameScene();

    bool                pick(const int x, const int y, scene_rdl2::math::Vec3f* hitPoint) const;
    scene_rdl2::math::Mat4f  makeCameraMatrix(const Camera& camera) const;
    void                printCameraMatrices() const;

    const moonray::rndr::RenderContext *mRenderContext {nullptr};
    std::unique_ptr<Camera>  mCamera {nullptr};

    float                    mSpeed {50.0f};
    uint32_t                 mInputState {0};        /// bitfield representing current movement state
    MouseMode                mMouseMode {NONE};      /// current mouse interaction mode (ROTATE, TRACK, DOLLY, ROLL, NONE)
    int                      mMouseX {-1};           /// last mouse x position
    int                      mMouseY {-1};           /// last mouse y position

    bool                     mFocusSet {false};
    bool                     mInitialTransformSet {false};
    scene_rdl2::math::Vec3f  mInitialPosition;
    scene_rdl2::math::Vec3f  mInitialViewDir;
    scene_rdl2::math::Vec3f  mInitialUp;
    float                    mInitialFocusDistance {1.0f};
    bool                     mInitialFocusDistanceNeedsUpdate {false};
};

} // namespace moonray_gui_v2

