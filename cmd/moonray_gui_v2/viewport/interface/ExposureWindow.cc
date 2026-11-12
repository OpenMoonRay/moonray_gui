// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "ExposureWindow.h"

#include "../Viewport.h"

namespace moonray_gui_v2 {

void
ExposureWindow::configureWindow() const
{
    // Center the window in the viewport when first opened
    ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(imguiViewport->GetCenter(), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(mWidth, mHeight), ImGuiCond_Once);
}

void
ExposureWindow::draw(Viewport* viewport, const ImVec2& /*currentPixel*/, const ImVec2& /*dockOffset*/)
{
    if (!mOpen) { return; }

    configureWindow();

    ImGui::Begin("Exposure", &mOpen, ImGuiWindowFlags_NoResize);
    // Get ptr to the editable exposure amount
    float* exposure = viewport->getExposurePtr();
    ImGui::SliderFloat("##Exposure", exposure, mMinExposure, mMaxExposure);

    if (ImGui::Button("Reset")) {
        *exposure = mExposureDefault;
    }
    ImGui::End();
}

} // namespace moonray_gui_v2
