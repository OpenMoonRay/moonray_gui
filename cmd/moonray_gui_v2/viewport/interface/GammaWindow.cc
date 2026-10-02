// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "GammaWindow.h"

#include "../Viewport.h"

namespace moonray_gui_v2 {

void
GammaWindow::configureWindow() const
{
    // Center the window in the viewport when first opened
    ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(imguiViewport->GetCenter(), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(mWidth, mHeight), ImGuiCond_Once);
}

void
GammaWindow::draw(Viewport* viewport, const ImVec2& /*currentPixel*/, const ImVec2& /*dockOffset*/)
{
    if (!mOpen) { return; }

    configureWindow();

    ImGui::Begin("Gamma", &mOpen, ImGuiWindowFlags_NoResize);
    // Get ptr to the editable gamma amount
    float* gamma = viewport->getGammaPtr();
    ImGui::SliderFloat("##Gamma", gamma, mMinGamma, mMaxGamma);

    if (ImGui::Button("Reset")) {
        *gamma = mGammaDefault;
    }
    ImGui::End();
}

} // namespace moonray_gui_v2
