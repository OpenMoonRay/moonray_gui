// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "ViewController.h"

#include "../../camera/NavigationCam.h"

#include <moonray/rendering/rndr/PathVisualizerManager.h>

namespace moonray_gui_v2 {

namespace pathvisualizer {

bool
ViewController::camerasMatch(const PathVisualizerManager * const pathVisualizer) const
{
    assert(pathVisualizer);
    if (!pathVisualizer->isOn() || !mInitialized) {
        return true;
    }

    const scene_rdl2::math::Mat4f pathVisXform(pathVisualizer->getRecorderCameraXform());
    return scene_rdl2::math::isEqual(pathVisXform, mInteractiveCameraXform);
}

void
ViewController::update(const PathVisualizerManager * const pathVisualizer,
                       NavigationCam* navigationCamera)
{
    assert(pathVisualizer);
    if (!pathVisualizer->isOn()) {
        return;
    }

    // If the current navigation camera matches the path visualizer camera, we are in 
    // PATH_VISUALIZER mode. Otherwise, we are viewing the visualization from the INTERACTIVE camera.
    const scene_rdl2::math::Mat4f pathVisXform(pathVisualizer->getRecorderCameraXform());
    bool xformsAreEqual = scene_rdl2::math::isEqual(pathVisXform, navigationCamera->update(0.0f));
    CameraMode newMode = xformsAreEqual ? CameraMode::PATH_VISUALIZER : CameraMode::INTERACTIVE;

    // If the mode is interactive OR if we haven't initialized the 
    // interactive camera, update its transform
    if (cameraIsInteractive() || !mInitialized) {
        mInteractiveCameraXform = navigationCamera->update(0.0f);
        mInitialized = true;
    }
    // Note that we should update the camera mode after updating the interactive camera
    // to ensure the transform is stored before switching modes.
    mCameraMode = newMode;
}

void
ViewController::changeCameraMode(const PathVisualizerManager * const pathVisualizer,
                                 const CameraMode newMode,
                                 NavigationCam* navigationCamera)
{
    assert(pathVisualizer);
    if (!pathVisualizer->isOn() || !navigationCamera) {
        return;
    }

    // If the user requests a change in camera mode, we need to update the navigation camera accordingly.
    if (newMode == CameraMode::PATH_VISUALIZER && mCameraMode != CameraMode::PATH_VISUALIZER) {
        const scene_rdl2::math::Mat4f pathVisXform(pathVisualizer->getRecorderCameraXform());
        navigationCamera->clearMovementState();
        navigationCamera->resetTransform(pathVisXform, false);
    } else if (newMode == CameraMode::INTERACTIVE && mCameraMode != CameraMode::INTERACTIVE) {
        navigationCamera->clearMovementState();
        navigationCamera->resetTransform(mInteractiveCameraXform, false);
    }

    mCameraMode = newMode;
}

} // namespace pathvisualizer
} // namespace moonray_gui_v2
