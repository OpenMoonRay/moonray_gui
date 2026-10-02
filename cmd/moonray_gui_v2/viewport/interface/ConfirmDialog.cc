// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "ConfirmDialog.h"
#include "../Viewport.h"

namespace moonray_gui_v2 {

// The most minimal constructor requires a title and a callback function. Users can then 
// set the description using the provided setter method.
ConfirmDialog::ConfirmDialog(const std::string& title, const std::function<void(bool)>& callback)
    : mTitle(title)
    , mCallback(callback)
{}

void ConfirmDialog::open(Viewport *viewport)
{
    mOpen = true;
    viewport->blockInput();
}

void
ConfirmDialog::configureWindow() const
{
    ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
    // Always center the window on the main viewport
    ImGui::SetNextWindowPos(imguiViewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
}

void ConfirmDialog::draw(Viewport* viewport)
{
    if (mOpen) {
        configureWindow();
        ImGui::OpenPopup(mTitle.c_str());
        mOpen = false;
    }

    if (ImGui::BeginPopupModal(mTitle.c_str(), nullptr, mWindowFlags)) {

        if (!mDesc.empty()) {
            ImGui::Text("%s", mDesc.c_str());
        }

        if (ImGui::Button("Confirm")) {
            mCallback(true);
            close(viewport);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            mCallback(false);
            close(viewport);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        ImGui::EndPopup();
    }
}

void ConfirmDialog::close(Viewport* viewport)
{
    ImGui::CloseCurrentPopup();
    // Unblock input when the dialog is closed
    viewport->unblockInput();
}

} // end namespace moonray_gui_v2
