// Copyright 2023-2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "RenderGui.h"

#include "camera/NavigationCam.h"
#include "DenoiserManager.h"
#include "Draw2D.h"
#include "viewport/Viewport.h"

#include <moonray/rendering/rndr/PathVisualizerManager.h>
#include <moonray/rendering/rndr/RenderContextConsoleDriver.h>
#include <moonray/rendering/rndr/RenderOutputDriver.h>

namespace moonray_gui_v2 {
using namespace scene_rdl2::math;

RenderGui::RenderGui(Viewport* viewport, DenoiserManager* denoiserManager)
    : mViewport(viewport), mDenoiserManager(denoiserManager)
{
    mColorManager.setupConfig();
    mShmFbOutput = std::make_shared<scene_rdl2::grid_util::ShmFbOutput>();
    moonray::rndr::RenderContextConsoleDriver::setShmFbOutput(mShmFbOutput);
}

void
RenderGui::updateFrame(scene_rdl2::fb_util::RenderBuffer *renderBuffer,
                       const scene_rdl2::fb_util::VariablePixelBuffer *renderOutputBuffer,
                       bool showProgress,
                       bool parallel)
{
    /// Don't update frame while we're running the visualizer simulation
    const moonray::rndr::PathVisualizerManager* visualizer = mRenderContext->getPathVisualizerManager().get();
    if (visualizer->isProcessing()) { return; }

    const DebugMode mode = mViewport->getDebugMode();

    // Apply denoising whilst frame is in linear HDR format.
    if (mode != NUM_SAMPLES && mRenderOutput < 0) { 
        mDenoiserManager->denoiseFrame(renderBuffer, mRenderContext); 
    }

    scene_rdl2::fb_util::PixelBufferUtilOptions options = parallel?
            scene_rdl2::fb_util::PIXEL_BUFFER_UTIL_OPTIONS_PARALLEL :
            scene_rdl2::fb_util::PIXEL_BUFFER_UTIL_OPTIONS_NONE;

    // Apply color render transform
    mColorManager.applyCRT(mViewport,
                           mRenderOutput,
                           *renderBuffer,
                           *renderOutputBuffer,
                           &mDisplayBuffer,
                           options,
                           parallel);


    if (mShmFbOutput && mShmFbOutput->getActive()) {
        mShmFbOutput->updateFbRGB888(mDisplayBuffer.getWidth(),
                                     mDisplayBuffer.getHeight(),
                                     static_cast<const void* const>(mDisplayBuffer.getData()),
                                     false /* top2BottomFlag */ );

    }

    if (showProgress) {
        showTileProgress(DISPLAY_BUFFER_IS_DISPLAY_BUFFER);
    }

    // Send the frame directly to the viewport to display
    mViewport->update(&mDisplayBuffer);
}

void
RenderGui::snapshotFrame(scene_rdl2::fb_util::RenderBuffer *renderBuffer,
                         scene_rdl2::fb_util::HeatMapBuffer *heatMapBuffer,
                         scene_rdl2::fb_util::FloatBuffer *weightBuffer,
                         scene_rdl2::fb_util::RenderBuffer *renderBufferOdd,
                         scene_rdl2::fb_util::VariablePixelBuffer *renderOutputBuffer,
                         bool untile, bool parallel)
{
    // Don't snapshot frame while visualizer is gathering data
    moonray::rndr::PathVisualizerManager* visualizer = mRenderContext->getPathVisualizerManager().get();
    if (visualizer->isProcessing()) { return; }

    DebugMode mode = mViewport->getDebugMode();

    // Special case if debug mode is set to NUM_SAMPLES, in which case we want to display
    // the weights buffer directly with some transform applied to aid visualization.
    if (mode == NUM_SAMPLES) {
        mRenderContext->snapshotWeightBuffer(renderOutputBuffer, untile, parallel);
        return;
    }

    if (mRenderOutput < 0) {
        // snapshot the plain old render buffer output
        mRenderContext->snapshotRenderBuffer(renderBuffer, untile, parallel, true);
        return;
    }

    // snapshot something other than the render buffer
    const auto *rod = mRenderContext->getRenderOutputDriver();

    // If we have had a scene change but have not yet started rendering, the
    // progressive update might call us anyway.  This works for the render
    // buffer, since the render driver referenced by RenderContext is
    // a singleton that persists across RenderContext tear-downs.  But
    // the render output driver does not - and it is only setup during
    // start frame based on scene data.  We should be called
    // again shortly after the frame is started.
    if (!rod) return;

    MNRY_ASSERT(mRenderOutput < static_cast<int>(rod->getNumberOfRenderOutputs()));

    if (rod->requiresRenderBuffer(mRenderOutput)) {
        mRenderContext->snapshotRenderBuffer(renderBuffer, untile, parallel, /* usePrimaryAov */ true);
    }
    if (rod->requiresHeatMap(mRenderOutput)) {
        mRenderContext->snapshotHeatMapBuffer(heatMapBuffer, untile, parallel);
    }
    if (rod->requiresWeightBuffer(mRenderOutput)) {
        mRenderContext->snapshotWeightBuffer(weightBuffer, untile, parallel);
    }
    if (rod->requiresRenderBufferOdd(mRenderOutput)) {
        mRenderContext->snapshotRenderBufferOdd(renderBufferOdd, untile, parallel);
    }

    scene_rdl2::fb_util::RenderBuffer beautyBuffer;
    mRenderContext->snapshotRenderBuffer(&beautyBuffer, untile, parallel, /* usePrimaryAov */ false);
    mRenderContext->snapshotRenderOutput(renderOutputBuffer, mRenderOutput,
                                         renderBuffer, &beautyBuffer, heatMapBuffer, weightBuffer, renderBufferOdd,
                                         untile, parallel);
}

void
RenderGui::beginInteractiveRendering(const Mat4f& cameraXform,
                                     bool makeDefaultXform)
{
    mRenderTimestamp = 0;
    mLastSnapshotTimestamp = 0;
    mLastSnapshotTime = 0.0;
    mLastFilmActivity = 0;
    mLastCameraUpdateTime = -1.0;
    mLastCameraXform = cameraXform;

    // Give the navigation camera access to the scene in case it needs to run
    // collision checks.
    mViewport->setCameraRenderContext(*mRenderContext);

    if (makeDefaultXform) {
        mViewport->setDefaultCameraTransform(cameraXform);
    }

    // Update the camera.
    computeCameraMotionXformOffset();
    NavigationCam *cam = mViewport->getNavigationCam();
    if (cam) {
        Mat4f conditionedXform = cam->resetTransform(cameraXform, false);
        if (!isEqual(mLastCameraXform, conditionedXform)) {
            setCameraXform(conditionedXform);
        }
    }
}

uint32_t
RenderGui::updateInteractiveRendering()
{
    switch (mRenderContext->getRenderMode())
    {
    case moonray::rndr::RenderMode::PROGRESSIVE:
    case moonray::rndr::RenderMode::PROGRESSIVE_FAST:
    case moonray::rndr::RenderMode::PROGRESS_CHECKPOINT:
    case moonray::rndr::RenderMode::BATCH:
        return updateProgressiveRendering();

    case moonray::rndr::RenderMode::REALTIME:
        return updateRealTimeRendering();

    default:
        MNRY_ASSERT(0);
    }

    return 0;
}

Mat4f 
RenderGui::endInteractiveRendering()
{
    if (mRenderContext->isFrameRendering()) {
        mRenderContext->stopFrame();
    }

    return updateNavigationCam(util::getSeconds());
}

uint32_t
RenderGui::updateProgressiveRendering()
{
    const double currentTime = util::getSeconds();
    bool updated = false;

    // RenderOutputDriver must exist to render a frame
    const auto *rod = mRenderContext->getRenderOutputDriver();

    moonray::rndr::PathVisualizerManager* visualizer = mRenderContext->getPathVisualizerManager().get();

    // This block of code won't get executed on the first iteration after
    // beginInteractiveRendering is called but will be for all subsequent 
    // iterations.

    if (mRenderContext->isFrameRendering() || mRenderContext->isFrameComplete() && rod) {

        // Throttle rendering to the specified frames per second.
        float fps = mRenderContext->getSceneContext().getSceneVariables().get(rdl2::SceneVariables::sFpsKey);
        if (fps < 0.000001f) {
            fps = 24.0f;
        }

        // Have we elapsed enough time to show another part of the frame?
        const bool snapshotIntervalElapsed = (currentTime - mLastSnapshotTime) >= ((1.0 / fps) - 0.001f);   // 1 ms slop

        const unsigned filmActivity = mRenderContext->getFilmActivity();
        const bool renderSamplesPending = (filmActivity != mLastFilmActivity);
        bool roChanged = updateRenderOutput();

        // In NORMAL view mode, we want to check if we have a complete frame.  In
        // SNOOP mode, we allow partial frames
        const bool readyForDisplay = mRenderContext->isFrameReadyForDisplay();

        // All these conditions must be met before we push another new frame up.
        if (readyForDisplay && ((snapshotIntervalElapsed && renderSamplesPending) || roChanged)) {
            mLastSnapshotTimestamp = mRenderTimestamp;
            mLastSnapshotTime = currentTime;
            mLastFilmActivity = filmActivity;

            snapshotFrame(&mRenderBuffer, &mHeatMapBuffer, &mWeightBuffer, &mRenderBufferOdd,
                          &mRenderOutputBuffer, true, false);
            updateFrame(&mRenderBuffer, &mRenderOutputBuffer,
                        !mRenderContext->isFrameComplete() &&
                        mViewport->getShowTileProgress(),
                        false);

            updated = true;
        }
    }

    // Special case for when we want to resend the frame buffer even after it
    // has completed rendering. One current example is if you toggle the show
    // alpha mode after rendering has completed. Another is when the tile
    // overlays are fading out right after the frame completes.
    bool needsRefresh = mViewport->getNeedsRefresh();
    if (!updated && needsRefresh && mRenderContext->isFrameComplete()) {
        snapshotFrame(&mRenderBuffer, &mHeatMapBuffer, &mWeightBuffer,
                      &mRenderBufferOdd, &mRenderOutputBuffer, true, false);
        updateFrame(&mRenderBuffer, &mRenderOutputBuffer, false, true);
        mViewport->setNeedsRefresh(false);
    }

    // This check forces us to wait on the previous frame being displayed at least once
    // before triggering the next frame. If we didn't do this, we may never see
    // anything displayed, or motion may be jerky.
    if (mLastSnapshotTimestamp >= mRenderTimestamp) {

        // The user specified we should start the visualizer
        if (visualizer->isInStartRecordState()) {

            // If we're rendering, we need to stop in order to run the ray simulation
            if (mRenderContext->isFrameRendering()) {
                mRenderContext->stopFrame();
                // If we stopped a rendering process, specify that we need
                // to re-start it after running the visualizer
                visualizer->setNeedsRenderRefresh(true);
            }
            visualizer->setRecordState();
            mRenderContext->startFrame(/*simulationMode*/ true);
        }
        
        // Check if there have been any scene changes since the last render.
        // If so, we need to kick off a new rendering process.
        Mat4f cameraXform = updateNavigationCam(currentTime);
        SceneChangeFlags changeFlags = processSceneChanges(cameraXform);
        if (changeFlags.any()) {

            // Stop the previous frame (if we were rendering one).
            if (mRenderContext->isFrameRendering()) {
                mRenderContext->stopFrame();
            }

            mRenderTimestamp = ++mMasterTimestamp;
            mLastFilmActivity = 0;

            //
            // Here is the point in the frame where we've stopped all render threads
            // and it's safe to update the scene.
            //

            // Update the camera.
            setCameraXform(cameraXform);

            // Kick off a new frame with the updated camera/progressive mode
            mRenderContext->startFrame();

            // if the camera changes, we need to re-generate the lines from the new camera perspective
            if (changeFlags.mCamera && visualizer->isInDrawState()) {
                visualizer->generateLines();
            }

            // Update the tile progress rendering state.
            mOkToRenderTiles = false;
            unsigned numTiles = unsigned(mRenderContext->getTiles()->size());
            if (mFadeLevels[0].getNumBits() != numTiles) {
                for (unsigned i = 0; i < NUM_TILE_FADE_STEPS; ++i) {
                    mFadeLevels[i].init(numTiles);
                }
            }
        }
    }

    return mRenderContext->isFrameRendering() ? mRenderTimestamp : 0;
}

uint32_t
RenderGui::updateRealTimeRendering()
{
    const double currentTime = util::getSeconds();

    // This block of code won't get executed on the first iteration after
    // beginInteractiveRendering is called but will be for all subsequent 
    // iterations.
    if (mRenderContext->isFrameRendering()) {

        if (mRenderContext->isFrameReadyForDisplay()) {
            mRenderContext->stopFrame();

            mRenderTimestamp = ++mMasterTimestamp;

            updateRenderOutput();
            snapshotFrame(&mRenderBuffer, &mHeatMapBuffer, &mWeightBuffer, &mRenderBufferOdd,
                          &mRenderOutputBuffer, true, true);

            updateFrame(&mRenderBuffer, &mRenderOutputBuffer, false, true);

            // Here is the point in the frame where we've stopped all render
            // threads and it's safe to update the scene.

            // ...

            // Update realtime frame statistics.
            mRenderContext->commitCurrentRealtimeStats();

            // Update the camera.
            Mat4f cameraXform = updateNavigationCam(currentTime);
            setCameraXform(cameraXform);

            mRenderContext->startFrame();
        }

    } else {

        // Kick off the first frame.
         
        // Check if there have been any scene changes since the last render.
        Mat4f cameraXform = updateNavigationCam(currentTime);

        // Update the camera.
        setCameraXform(cameraXform);

        mRenderTimestamp = ++mMasterTimestamp;

        // Kick off a new frame with the updated camera.
        mRenderContext->startFrame();
    }

    return mRenderContext->isFrameRendering() ? mRenderTimestamp : 0;
}

RenderGui::SceneChangeFlags
RenderGui::processSceneChanges(const Mat4f& cameraXform)
{
    RenderGui::SceneChangeFlags flags;
    // Check if there have been any scene changes since the last render.
    flags.mCamera = !math::isEqual(mLastCameraXform, cameraXform);
    flags.mTimestamp = (mMasterTimestamp != mRenderTimestamp);
    if (flags.mCamera || flags.mTimestamp) { return flags; }

    // Check if the progressive mode changed
    moonray::rndr::RenderMode currentMode = mViewport->isFastProgressive() ?
        moonray::rndr::RenderMode::PROGRESSIVE_FAST :
        moonray::rndr::RenderMode::PROGRESSIVE;
    if (mRenderContext->getRenderMode() != currentMode) {
        flags.mFastProgressiveActive = true;
        mRenderContext->setRenderMode(currentMode);
        return flags;
    }

    // Check if the fast progressive mode changed
    moonray::rndr::FastRenderMode currentFastMode = mViewport->getFastMode();
    if (mRenderContext->getFastRenderMode() != currentFastMode) {
        flags.mFastProgressiveMode = true;
        mRenderContext->setFastRenderMode(currentFastMode);
        return flags;
    }

    return flags;
}

void
RenderGui::computeCameraMotionXformOffset()
{
    const rdl2::Camera* camera = mRenderContext->getCamera();
    MNRY_ASSERT(camera);
    MNRY_ASSERT(rdl2::Node::sNodeXformKey.isBlurrable());

    // To preserve any camera motion xform, we need to compute the existing
    // "offset" xform to go from TIMESTEP_END to TIMESTEP_BEGIN
    // We'll accept the double to float precision loss for gui maniupulations
    Mat4f c02w = toFloat(camera->get(rdl2::Node::sNodeXformKey, rdl2::TIMESTEP_BEGIN));
    Mat4f c12w = toFloat(camera->get(rdl2::Node::sNodeXformKey, rdl2::TIMESTEP_END));
    Mat4f w2c0 = c02w.inverse();
    mC12C0 = c12w * w2c0;
}

void
RenderGui::setCameraXform(const Mat4f& c2w)
{
    rdl2::Camera* camera = const_cast<rdl2::Camera*>(mRenderContext->getCamera());
    MNRY_ASSERT(camera);
    MNRY_ASSERT(rdl2::Node::sNodeXformKey.isBlurrable());

    // We then add the offset to the given camera xform to set the corresponding
    // motion transform
    camera->beginUpdate();
    camera->set(rdl2::Node::sNodeXformKey, toDouble(c2w), rdl2::TIMESTEP_BEGIN);
    camera->set(rdl2::Node::sNodeXformKey, toDouble(mC12C0 * c2w), rdl2::TIMESTEP_END);
    camera->endUpdate();
    mRenderContext->setSceneUpdated();
    mViewport->setUpdateAxis(true);

    mLastCameraXform = c2w;
}

Mat4f
RenderGui::updateNavigationCam(double currentTime)
{
    NavigationCam *cam = mViewport->getNavigationCam();
    if (!cam) {
        return Mat4f(math::one);
    }

    double dt = (mLastCameraUpdateTime < 0.0) ? 0.0 : (currentTime - mLastCameraUpdateTime);
    mLastCameraUpdateTime = currentTime;

    mViewport->pathVisualizerUpdateViewController();
    return cam->update(static_cast<float>(dt));
}

void
RenderGui::drawTileOutlines(DisplayBuffer buf, const std::vector<scene_rdl2::fb_util::Tile> &tiles,
                            float tileColor, int fadeLevelIdx)
{
    switch (buf) {
    case DISPLAY_BUFFER_IS_DISPLAY_BUFFER:
        {
            const uint8_t byteColor = convertToByteColor(tileColor);
            mFadeLevels[fadeLevelIdx].forEachBitSet([&](unsigned idx) {
                MNRY_ASSERT(idx < tiles.size());
                drawTileOutline(&mDisplayBuffer, tiles[idx], byteColor);
            });
        }
        break;
    case DISPLAY_BUFFER_IS_RENDER_BUFFER:
        mFadeLevels[fadeLevelIdx].forEachBitSet([&](unsigned idx) {
            MNRY_ASSERT(idx < tiles.size());
            drawTileOutline(&mRenderBuffer, tiles[idx], tileColor);
        });
        break;
    case DISPLAY_BUFFER_IS_RENDER_OUTPUT_BUFFER:
        switch (mRenderOutputBuffer.getFormat()) {
        case scene_rdl2::fb_util::VariablePixelBuffer::FLOAT3:
            mFadeLevels[fadeLevelIdx].forEachBitSet([&](unsigned idx) {
                MNRY_ASSERT(idx < tiles.size());
                drawTileOutline(&mRenderOutputBuffer.getFloat3Buffer(), tiles[idx], tileColor);
            });
            break;
        case scene_rdl2::fb_util::VariablePixelBuffer::FLOAT4:
            mFadeLevels[fadeLevelIdx].forEachBitSet([&](unsigned idx) {
                MNRY_ASSERT(idx < tiles.size());
                drawTileOutline(&mRenderOutputBuffer.getFloat4Buffer(), tiles[idx], tileColor);
            });
            break;
        default:
            MNRY_ASSERT(0 && "tile progress in render output buffer unhandled");
        }
        break;
    default:
        MNRY_ASSERT(0 && "unknown display buffer");
    }
}

void
RenderGui::showTileProgress(DisplayBuffer buf)
{
    // Color of new tiles, additive on framebuffer.
    static const float refTileColor = 0.2f;

    // Initial passes essentially try and render something to all tiles as fast
    // as possible so we have an image to extrapolate. This is problematic if
    // rendering diagnostic tiles on top since they cover the entire image
    // making it harder to see, especially if the camera is in constant motion.
    // The solution here is to only start rendering tiles when less than a
    // certain percentage of the screen is covered with them.
    // Here we set that threshold at 10%.
    static const float tileRatioThreshold = 0.1f;

    // Render all the tiles which we are are currently submitting primary rays
    // for over all threads.

    const std::vector<scene_rdl2::fb_util::Tile> &tiles =
        *(mRenderContext->getTiles());
    mRenderContext->getTilesRenderedTo(mFadeLevels[0]);

    if (!mOkToRenderTiles) {
        auto totalTiles = tiles.size();
        float ratio = float(double(mFadeLevels[0].getNumBitsSet()) / double(totalTiles));

        if (ratio < tileRatioThreshold) {
            mOkToRenderTiles = true;
        } else {
            // Early return.
            return;
        }
    }

    // Render full bright tiles we've rendered this frame.
    drawTileOutlines(buf, tiles, refTileColor, 0);

    // Render the tiles for each different fade level.
    for (unsigned i = 1; i < NUM_TILE_FADE_STEPS; ++i) {

        // Ensure each bit is only set to on for a single list, with lower indexed
        // lists getting priority over higher indexed lists.
        mFadeLevels[i].combine(mFadeLevels[0], [](uint32_t &a, uint32_t b) {
            a &= ~b;
        });

        // Compute fade amount.
        float t = (1.f - (float(i) / float(NUM_TILE_FADE_STEPS))) * 0.6f;
        const float fadeColor = refTileColor * t;

        drawTileOutlines(buf, tiles, fadeColor, i);
    }

    // Do actual fade. (TODO: use std::move instead of the logic below)
    // Note: mFadeLevels[0] is cleared next time around.
    for (int i = NUM_TILE_FADE_STEPS - 1; i > 0; --i) {
        mFadeLevels[i].combine(mFadeLevels[i - 1], [](uint32_t &a, uint32_t b) {
            a = b;
        });
    }
}

bool
RenderGui::updateRenderOutput()
{
    bool updated = false;
    int guiIndx = mViewport->getRenderOutputIndex();
    const auto *rod = mRenderContext->getRenderOutputDriver();

    // rod can be null if we have not yet called startFrame()
    // this will happen in progressive rendering when we have a scene change
    if (!rod) {
        return false;
    }
    const int numRenderOutputs = rod->getNumberOfRenderOutputs();

    if (guiIndx != mLastRenderOutputGuiIndx) {
        if (guiIndx > mLastRenderOutputGuiIndx) {
            // find next active output
            if (mRenderOutput + 1 < numRenderOutputs) {
                mRenderOutput++;
                updated = true;
            }
        } else if (guiIndx < mLastRenderOutputGuiIndx) {
            // find previous active output
            if (mRenderOutput >= 0) {
                mRenderOutput--;
                updated = true;
                // -1 means to use the render buffer
            }
        }
        mLastRenderOutputGuiIndx = guiIndx;
    }

    if (mLastTotalRenderOutputs != numRenderOutputs) {
        // the scene changed - our mRenderOutput index is potentially out
        // of range or invalid.  first try to match the render output name
        for (int i = 0; i < numRenderOutputs; ++i) {
            if (mLastRenderOutputName == rod->getRenderOutput(i)->getName()) {
                // found it, no update needed
                mRenderOutput = i;
                break;
            }
        }

        // if we didn't find it and we are out of range, put
        // us at the last render output - this implies an update
        if (!(mRenderOutput < numRenderOutputs)) {
            mRenderOutput = numRenderOutputs - 1;
        }

        // if we have some kind of change, but our index is
        // in range, just flag this as an update
        const std::string outputName = mRenderOutput > 0? rod->getRenderOutput(mRenderOutput)->getName() : "";
        if (mLastRenderOutputName != outputName) {
            updated = true;
        }

        mLastTotalRenderOutputs = numRenderOutputs;
    }

    if (updated) {
        if (mRenderOutput < 0) {
            std::cerr << "switch output to render buffer\n";
            mLastRenderOutputName = "";
        } else {
            const std::string outputName = rod->getRenderOutput(mRenderOutput)->getName();
            std::cerr << "switch output to "
                      << outputName
                      << '\n';
            mLastRenderOutputName = outputName;
        }
    }
    return updated;
}

bool
RenderGui::isFastProgressive() const
{
    return mViewport->isFastProgressive();
}

moonray::rndr::FastRenderMode
RenderGui::getFastRenderMode() const
{
    return mViewport->getFastMode();
}

void
RenderGui::setContext(moonray::rndr::RenderContext *ctx)
{
    mRenderContext = ctx;

    if (mViewport) {
        // Keep the viewport's RenderContext pointer in sync with the current context
        mViewport->updateRenderContext(ctx);
    }
}


} // namespace moonray_gui_v2
