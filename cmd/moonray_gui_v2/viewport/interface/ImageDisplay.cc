// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "ImageDisplay.h"

#include "DrawUtil.h"
#include "../Viewport.h"

#include <moonray/rendering/rndr/PathVisualizerManager.h>

#include <cmath>

namespace moonray_gui_v2 {

constexpr float MAX_ZOOM = 30.0f;   // max zoom in (30x)
constexpr float MIN_ZOOM = 0.1f;    // max zoom out (0.1x)
constexpr float ZOOM_STEP = 1.05f;  // zoom step factor

void
ImageDisplay::configureWindow(const int availWidth, const int availHeight)
{
    // --------------- Set position/size ----------------
    // Get the scaled image size to account for zoom in/out
    ImVec2 scaledSize = getScaledSize();
    ImGui::SetNextWindowSize(scaledSize);

    // Center image in the viewport
    mPosition = ImVec2((availWidth - scaledSize.x) * 0.5f, (availHeight - scaledSize.y) * 0.5f);

    // Apply any panning offset
    mPosition.x += mPositionOffset.x;
    mPosition.y += mPositionOffset.y;
    ImGui::SetNextWindowPos(mPosition);
    // -------------------------------------------------

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
}

void
ImageDisplay::draw(const Viewport* viewport, const int availWidth, const int availHeight)
{
    const int textureHandle = viewport->getDisplayTexture();

    // Only show if we have valid data
    if (textureHandle == 0 || mImageSize.x <= 0 || mImageSize.y <= 0 || availWidth <= 0 || availHeight <= 0) {
        return;
    }

    configureWindow(availWidth, availHeight);
    
    // This window will just contain the image, no decorations or inputs
    if (ImGui::Begin("ImageViewport", nullptr, 
                    ImGuiWindowFlags_NoDecoration | 
                    ImGuiWindowFlags_NoMove | 
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoInputs |
                    ImGuiWindowFlags_NoBringToFrontOnFocus)) {
        
        // Ensure cursor is at origin for precise positioning
        ImGui::SetCursorPos(ImVec2(0, 0));
        
        // Draw the image (Note that ImGui uses top-left origin for UVs, OpenGL uses bottom-left)
        ImGui::Image((ImTextureID)(intptr_t)textureHandle, getScaledSize(), ImVec2(0, 1), ImVec2(1, 0));

        /// Draw path visualizer lines on top of the image
        moonray::rndr::PathVisualizerManager* manager = viewport->getPathVisualizerManager();
        drawPathVisualizerLines(manager, viewport->pathVisualizerShowOnlyEndpoints());
    }
    ImGui::End();
    ImGui::PopStyleVar(2); 
}

// -------------------------------------- Path Visualizer Line Drawing ---------------------------------------------- //

void
ImageDisplay::drawLine(const int x1, const int y1, const int x2, const int y2, const scene_rdl2::math::Color& color, 
                       const float a, const float w, const bool drawEndPoint, const bool selected, 
                       const bool isSample, const bool showEndpointsOnly)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Given the pixel coordinates, convert to viewport coordinates for drawing
    const ImVec2 start = imageToViewportCoords(x1, y1);
    const ImVec2 end = imageToViewportCoords(x2, y2);

    ImU32 lineColor = IM_COL32(color.r * 255, color.g * 255, color.b * 255, a * 255);
    if (!selected) {
        lineColor = IM_COL32(color.r * 255, color.g * 255, color.b * 255, a * 0.3 * 255);
    }

    if (!showEndpointsOnly) {
        if (isSample) {
            addDashedLine(start, end, w, lineColor);
        } else {
            drawList->AddLine(start, end, lineColor, w);
        }
    }

    if (drawEndPoint) {
        drawList->AddCircleFilled(end, w + 3, lineColor, /*circle segments*/ 16);
    }
}

void 
ImageDisplay::drawPathVisualizerLines(moonray::rndr::PathVisualizerManager* manager, const bool showEndpointsOnly)
{
    // If the manager doesn't exist or is turned off, nothing to draw
    if (!manager || !manager->isOn()) { return; }

    // Get the scaled image size to account for zoom in/out
    const ImVec2 size = getScaledSize();
    
    using PosType = scene_rdl2::grid_util::VectorPacketLineStatus::PosType;

    auto lineDrawingCallback = [&](const scene_rdl2::math::Vec2i& s,
                                   const scene_rdl2::math::Vec2i& e,
                                   const uint8_t& flags,
                                   const float a,
                                   const float w,
                                   const bool drawEndPoint,
                                   const unsigned nodeId,
                                   const PosType startPosType,
                                   const PosType endPosType) {
        // Check if the line should be visible based on user settings
        if (!manager->showRay(flags)) { return; }

        // For now, if the given alpha "a" is less than 1, it's a hidden line,
        // so use the given hidden line opacity.
        // TODO: In the future, we might consider replacing the opacity field in the 
        // Line struct to a field like "mHidden", and allow the frontend to draw
        // hidden line segments however it wants
        const float opacity = a < 1.f ? manager->getHiddenLineOpacity() : 1.f;
        const bool selected = manager->isSelectedNode(nodeId);
        const bool isSample = manager->isSample(flags);
        drawLine(s[0], s[1], e[0], e[1], manager->getColorByFlags(flags), opacity, w, drawEndPoint, 
                 selected, isSample, showEndpointsOnly);
    };
    manager->crawlAllLines(lineDrawingCallback);
}

// ------------------------------ Image <--> Viewport coordinate conversions ---------------------------------------- //
ImVec2
ImageDisplay::viewportToImageCoords(const int viewportPosX, const int viewportPosY) const
{
    const ImVec2 scaledImageSize = getScaledSize();
    ImVec2 imagePos = ImVec2(viewportPosX, viewportPosY);

    // Offset by the image position
    // to put viewportPos in image coordinates
    imagePos.x -= mPosition.x;
    imagePos.y -= mPosition.y;

    if (imagePos.x >= 0 && imagePos.y >= 0 && imagePos.x < scaledImageSize.x && imagePos.y < scaledImageSize.y) {
        // Adjust position to account for difference between framebuffer size and image display size
        // i.e. if the image is scaled up in the viewport, we need to scale the position down to get 
        // the correct pixel. Also, since the pixel coord is the center of the pixel, we need to 
        // subtract 0.5 to get the top left corner of the pixel before scaling
        imagePos.x = std::floor((imagePos.x - 0.5f) * (1.f/mZoomScale));
        imagePos.y = std::floor((imagePos.y - 0.5f) * (1.f/mZoomScale));

        // Finally, we need to flip the y coordinate since our pixel coords have 
        // origin at bottom left but the imgui uses top left origin for viewport coords
        return ImVec2(imagePos.x, mImageSize.y - 1 - imagePos.y);
    }
    return ImVec2(-1, -1);
}

ImVec2
ImageDisplay::imageToViewportCoords(const int pixelX, const int pixelY) const
{
    // Scale pixel coordinates by the image zoom factor
    int viewportPosX = static_cast<int>(pixelX * mZoomScale);
    // Have to flip the y coordinate since our pixel coords have origin at bottom left
    // but viewport coords with imgui have origin at top left
    int viewportPosY = static_cast<int>((mImageSize.y - 1 - pixelY) * mZoomScale);

    // Offset by the image position to get viewport coordinates
    viewportPosX += static_cast<int>(mPosition.x);
    viewportPosY += static_cast<int>(mPosition.y);

    // NOTE: We do not need to check for out of bounds here, since
    // imgui will handle that internally.
    return ImVec2(viewportPosX, viewportPosY);
}

// -------------------------------------------- Zooming and Panning ------------------------------------------------- //
void
ImageDisplay::zoom(const int xoffset, const int yoffset)
{
    if (yoffset > 0 && mZoomScale < MAX_ZOOM) {
        mZoomScale *= ZOOM_STEP;
        // Adjust position offset to zoom towards center
        // As the image size increases, the offset point
        // moves further away from the center, so we need to scale it proportionally
        mPositionOffset.x *= ZOOM_STEP;
        mPositionOffset.y *= ZOOM_STEP;
    } else if (yoffset <= 0 && mZoomScale > MIN_ZOOM) {
        mZoomScale /= ZOOM_STEP;
        // Adjust position offset to zoom away from center
        // As the image size decreases, the offset point
        // moves closer to the center, so we need to scale it proportionally
        mPositionOffset.x /= ZOOM_STEP;
        mPositionOffset.y /= ZOOM_STEP;
    }
}

void
ImageDisplay::pan(const int xoffset, const int yoffset)
{
    mPositionOffset.x = mPositionOffset.x + xoffset;
    mPositionOffset.y = mPositionOffset.y + yoffset;
}

} // namespace moonray_gui_v2
