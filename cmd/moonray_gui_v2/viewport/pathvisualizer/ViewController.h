// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <scene_rdl2/common/math/Mat4.h>

namespace moonray { namespace rndr { class PathVisualizerManager; } }
using PathVisualizerManager = moonray::rndr::PathVisualizerManager;

namespace moonray_gui_v2 {

class NavigationCam;

namespace pathvisualizer {

// Specifies different camera views that can be used
// when viewing the visualization
enum class CameraMode {
    PATH_VISUALIZER,    // Align with the camera view used to draw the Path Visualizer rays 
    INTERACTIVE         // Free-form interactive camera
};

// Small helper class for Viewport to help manage GUI-side Path Visualizer camera state.
class ViewController
{
public:

    ViewController() = default;

    // Uses the current state of the navigation camera to determine the appropriate
    // camera mode and potentially update the stored interactive camera.
    void update(const PathVisualizerManager * const pathVisualizer,
                NavigationCam* navigationCamera);

    // Changes the active view based on the requested camera mode.
    void changeCameraMode(const PathVisualizerManager * const pathVisualizer, 
                          CameraMode mode,
                          NavigationCam* navigationCamera);

    bool cameraIsInteractive() const { return mCameraMode == CameraMode::INTERACTIVE; }

    // Checks if the cached interactive camera matches the current path visualizer camera.
    bool camerasMatch(const PathVisualizerManager * const pathVisualizer) const;

    void reset()
    {
        mCameraMode = CameraMode::PATH_VISUALIZER;
        mInitialized = false;
    }

private:
    CameraMode mCameraMode {CameraMode::PATH_VISUALIZER};   // Current active camera mode
    scene_rdl2::math::Mat4f mInteractiveCameraXform;        // Transform for the interactive camera
    bool mInitialized {false};                              // Indicates if the interactive camera has been initialized
};

} // namespace pathvisualizer
} // namespace moonray_gui_v2
