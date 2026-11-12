// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "StatusBar.h"

#include "../SnapshotManager.h"
#include "../Viewport.h"

#include "imgui.h"

namespace moonray_gui_v2 {

void
StatusBar::configureWindow(const ImVec2& dockOffset) const
{
    // Set position & size
    const ImGuiViewport* imguiViewport = ImGui::GetMainViewport();

    // Position at bottom of viewport by finding the size of the viewport
    // and subtracting the height of the status bar and any offset from bottom
    ImGui::SetNextWindowPos(ImVec2(0, imguiViewport->Size.y - dockOffset.y - mHeight));
    ImGui::SetNextWindowSize(ImVec2(imguiViewport->WorkSize.x - dockOffset.x, mHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(mWindowPadding, mWindowPadding));
}

void
StatusBar::draw(Viewport* viewport, const ImVec2& currentPixel, const ImVec2& dockOffset)
{
    if (!mOpen) { return; }

    configureWindow(dockOffset);

    ImGui::Begin("##Status Bar", nullptr, ImGuiWindowFlags_NoTitleBar |
                                            ImGuiWindowFlags_NoDecoration |
                                            ImGuiWindowFlags_NoMove |
                                            ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoBringToFrontOnFocus |
                                            ImGuiWindowFlags_NoNavInputs |
                                            ImGuiWindowFlags_NoNavFocus |
                                            ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoScrollbar);

    // Get the current pixel under the mouse
    const int currentPixelX = static_cast<int>(currentPixel.x);
    const int currentPixelY = static_cast<int>(currentPixel.y);

    ImGui::Text("(%d, %d)", currentPixelX, currentPixelY);
    ImGui::SameLine(0.0f, mLargeSpace);
    ImGui::Text("%s", getDebugModeStr(viewport->getDebugMode()).c_str());
    ImGui::SameLine(0.0f, mLargeSpace);
    ImGui::Text("%s", getFastProgressiveModeStr(viewport->isFastProgressive(), viewport->getFastMode()).c_str());
    ImGui::SameLine(0.0f, mLargeSpace);
    ImGui::Text("%s", getDenoiseModeStr(viewport->getDenoisingEnabled(), viewport->getDenoiserMode()).c_str());
    ImGui::SameLine(0.0f, mSmallSpace);
    ImGui::Text("%s", getDenoiseBufferStr(viewport->getDenoisingEnabled(), viewport->getDenoisingBufferMode()).c_str());
    ImGui::SameLine(0.0f, mLargeSpace);
    ImGui::Text("%s", getCameraTypeStr(viewport->getActiveCameraType()).c_str());
    ImGui::SameLine(0.0f, mLargeSpace);
    ImGui::Text("%s", getRenderOutputStr(viewport->getRenderOutputName()).c_str());

    if (viewport->getSnapshotManager()->getSelectedSnapshotIdx() != -1) {
        ImGui::SameLine(0.0f, mLargeSpace);
        ImGui::Text("SNAP: %s", viewport->getSnapshotManager()->getSelectedSnapshotLabel().c_str());
    }
    
    // Add help button at the end (right side) of the status bar
    ImGui::SameLine(0.0f, mButtonSpace);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 20);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    if (ImGui::Button("?", ImVec2(20, 16))) {
        if (mHelpCallback) {
            mHelpCallback();
        }
    }
    ImGui::PopStyleVar();
    
    ImGui::End();
    ImGui::PopStyleVar();
}

std::string
StatusBar::getDebugModeStr(const DebugMode& mode) const
{
    std::string title = "CH:";
    switch (mode) {
    case DebugMode::RGB:            return title + "RGB";
    case DebugMode::RED:            return title + "RED";
    case DebugMode::GREEN:          return title + "GREEN";
    case DebugMode::BLUE:           return title + "BLUE";
    case DebugMode::ALPHA:          return title + "ALPHA";
    case DebugMode::LUMINANCE:      return title + "LUM";
    case DebugMode::SATURATION:     return title + "SAT";
    case DebugMode::RGB_NORMALIZED: return title + "NRGB";
    case DebugMode::NUM_SAMPLES:    return title + "SPP";
    default:                        return title + "???";
    }
}

std::string
StatusBar::getFastProgressiveModeStr(const bool isFastProgressive, const moonray::rndr::FastRenderMode& mode) const
{
    std::string title = "FAST:";
    if (!isFastProgressive) {
        return title + "OFF";
    }
    switch (mode) {
    case moonray::rndr::FastRenderMode::NORMALS:              return title + "NORMALS";
    case moonray::rndr::FastRenderMode::NORMALS_SHADING:      return title + "SHADING_NORMALS";
    case moonray::rndr::FastRenderMode::FACING_RATIO:         return title + "FACING_RATIO";
    case moonray::rndr::FastRenderMode::FACING_RATIO_INVERSE: return title + "FACING_RATIO_INVERSE";
    case moonray::rndr::FastRenderMode::UVS:                  return title + "UVS";
    default:                                                  return title + "???";
    }
}

std::string
StatusBar::getDenoiseModeStr(const bool denoisingEnabled, const moonray::denoiser::DenoiserMode& mode) const
{
    if (!denoisingEnabled) {
        return "DENOISE:OFF";
    }
    switch (mode) {
    case moonray::denoiser::DenoiserMode::OPTIX:                    return "OPTIX:";
    case moonray::denoiser::DenoiserMode::METAL:                    return "METAL:";
    case moonray::denoiser::DenoiserMode::OPEN_IMAGE_DENOISE:       return "OIDN:";
    case moonray::denoiser::DenoiserMode::OPEN_IMAGE_DENOISE_CPU:   return "OIDN_CPU:";
    case moonray::denoiser::DenoiserMode::OPEN_IMAGE_DENOISE_CUDA:  return "OIDN_CUDA:";
    default:                                                        return "???";
    }
}

std::string
StatusBar::getDenoiseBufferStr(const bool denoisingEnabled, const DenoisingBufferMode& mode) const
{
    if (!denoisingEnabled) {
        return "";
    }
    switch (mode) {
    case DenoisingBufferMode::DN_BUFFERS_BEAUTY:                    return "BEAUTY";
    case DenoisingBufferMode::DN_BUFFERS_BEAUTY_ALBEDO:             return "BEAUTY+ALBEDO";
    case DenoisingBufferMode::DN_BUFFERS_BEAUTY_ALBEDO_NORMALS:     return "BEAUTY+ALBEDO+NORMALS";
    default:                                                        return "???";
    }
}

std::string
StatusBar::getCameraTypeStr(const CameraType& type) const
{
    switch (type) {
    case CameraType::ORBIT_CAM:      return "ORBIT_CAM";
    case CameraType::FREE_CAM:       return "FREE_CAM";
    default:                         return "???";
    }
}

std::string
StatusBar::getRenderOutputStr(const std::string& roName) const
{
    return "RO: " + roName;
}

} // namespace moonray_gui_v2
