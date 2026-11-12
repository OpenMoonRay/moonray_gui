// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "InputTextDialog.h"
#include "../Viewport.h"

namespace moonray_gui_v2 {

// The most minimal constructor requires a title and a callback function. Users can choose to then set the 
// description and default value using the provided setter methods.
InputTextDialog::InputTextDialog(const std::string& title, const std::function<void(const std::string&)>& callback)
    : mTitle(title)
    , mCallback(callback)
{}

void InputTextDialog::open(Viewport *viewport)
{
    // seed the input buffer with the default value so it is shown on the very first open
    std::snprintf(mInputBuffer.data(), mInputBuffer.size(), "%s", mDefaultValue.c_str());
    mOpen = true;
    mNeedsFocus = true;
    viewport->blockInput();
}

void
InputTextDialog::configureWindow() const
{
    ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
    // Always center the window on the main viewport
    ImGui::SetNextWindowPos(imguiViewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
}

void InputTextDialog::draw(Viewport* viewport)
{
    if (mOpen) {
        configureWindow();
        ImGui::OpenPopup(mTitle.c_str());
        mOpen = false;
    }

    if (ImGui::BeginPopupModal(mTitle.c_str(), nullptr, mWindowFlags)) {

        // ---- Draw description text ----
        if (!mDesc.empty()) {
            ImGui::Text("%s", mDesc.c_str());
        }

        // ---- Draw input text field ----
        if (mNeedsFocus) {
            ImGui::SetKeyboardFocusHere();
            mNeedsFocus = false;
        }
        const std::string inputLabel = "##input_" + mTitle;
        if (ImGui::InputText(inputLabel.c_str(), mInputBuffer.data(), mInputBuffer.size(), mInputTextFlags)) {
            mCallback(mInputBuffer.data());
            close(viewport);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
        }

        // ---- Draw "OK" Button ----
        if (ImGui::Button("OK")) {
            mCallback(mInputBuffer.data());
            close(viewport);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        // ---- Draw "Cancel" Button ----
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            close(viewport);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            close(viewport);
        }

        ImGui::EndPopup();
    }
}

void InputTextDialog::close(Viewport* viewport)
{
    mNeedsFocus = false;
    mInputBuffer.fill('\0');  // Clear the input buffer for the next time the dialog is opened
    ImGui::CloseCurrentPopup();
    // Unblock input when the dialog is closed
    viewport->unblockInput();
}

} // end namespace moonray_gui_v2
