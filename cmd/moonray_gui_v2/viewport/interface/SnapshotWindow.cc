// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "SnapshotWindow.h"

#include "ConfirmDialog.h"
#include "InputTextDialog.h"
#include "../SnapshotManager.h"
#include "../Viewport.h"

#include "imgui_internal.h"

namespace moonray_gui_v2 {

SnapshotWindow::SnapshotWindow()
    : Component(/*isOpen*/ false)
{
    mLabelDialog = std::make_unique<InputTextDialog>("Update Snapshot Label", [this](const std::string& newLabel) {
        if (mLabelDialogSnapshotIdx != -1) {
            mSnapshotManager->setSnapshotLabelAtIdx(mLabelDialogSnapshotIdx, newLabel);
            mLabelDialogSnapshotIdx = -1;
        }
    });

    mDeleteDialog = std::make_unique<ConfirmDialog>("Delete Snapshot", [this](bool confirmed) {
        if (confirmed && mDeleteDialogSnapshotIdx != -1) {
            mSnapshotManager->deleteSnapshotAtIdx(mDeleteDialogSnapshotIdx);
        }
        mDeleteDialogSnapshotIdx = -1;
    });
}

SnapshotWindow::~SnapshotWindow() = default;

void
SnapshotWindow::highlightSnapshot() const
{
    // Highlight the image with a border
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 rectMin = ImGui::GetItemRectMin();
    ImVec2 rectMax = ImGui::GetItemRectMax();
    drawList->AddRect(rectMin, rectMax, mSelectedBorderColor, 0.0f, 0, mSelectedBorderThickness);
}

void
SnapshotWindow::handleSnapshotHover(const std::string& label) const
{
    if (ImGui::IsItemHovered()) {
        // Change mouse cursor to hand when hovering over image
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        // Highlight the image with a border
        highlightSnapshot();

        // Show tooltip with label
        ImGui::BeginTooltip();
        ImGui::Text("%s", label.c_str());
        ImGui::EndTooltip();
    }
}

void 
SnapshotWindow::handleSnapshotClick(Viewport *viewport,
                                    const int selectedSnapshotIdx,
                                    const int clickedSnapshotIdx,
                                    const std::string& clickedSnapshotLabel)
{
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // Select the current snapshot to display in main viewport
        // if the selected image was already chosen, unselect it
        if (selectedSnapshotIdx == clickedSnapshotIdx) {
            mSnapshotManager->deselectSnapshot();
        } else {
            mSnapshotManager->selectSnapshotAtIdx(clickedSnapshotIdx);
        }
    }

    // Use a unique popup ID per snapshot to avoid all snapshots sharing one context menu.
    std::string popupId = "snapshot_context_" + std::to_string(clickedSnapshotIdx);
    if (ImGui::BeginPopupContextItem(popupId.c_str())) {
        if (ImGui::MenuItem("Update Label")) {
            mLabelDialogSnapshotIdx = clickedSnapshotIdx;
            mLabelDialog->setDefaultValue(clickedSnapshotLabel.c_str());
            mLabelDialog->open(viewport);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        if (ImGui::MenuItem("Delete Snapshot")) {
            mDeleteDialogSnapshotIdx = clickedSnapshotIdx;
            mDeleteDialog->setDescription("Are you sure you want to delete snapshot \"" + 
                                          mSnapshotManager->getSnapshotLabelAtIdx(clickedSnapshotIdx) + "\"?");
            mDeleteDialog->open(viewport);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        ImGui::EndPopup();
    }
}

void
SnapshotWindow::handleDeleteHotkey(Viewport* viewport)
{
    // Scoped to this window so the key doesn't delete snapshots while the user is working elsewhere
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) { return; }
    if (!ImGui::IsKeyPressed(ImGuiKey_Delete, /*repeat*/ false)) { return; }

    const int selectedSnapshotIdx = mSnapshotManager->getSelectedSnapshotIdx();
    if (selectedSnapshotIdx == -1) { return; }

    mDeleteDialogSnapshotIdx = selectedSnapshotIdx;
    mDeleteDialog->setDescription("Are you sure you want to delete snapshot \"" + 
                                  mSnapshotManager->getSelectedSnapshotLabel() + "\"?");
    mDeleteDialog->open(viewport);
}

void
SnapshotWindow::configureWindow(const ImVec2& dockOffset) const
{
    ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
    // Position at bottom of viewport, above any other horizontally docked components
    ImVec2 pos = ImVec2(0, imguiViewport->Size.y - dockOffset.y - mHeight);
    ImVec2 size = ImVec2(imguiViewport->Size.x, mHeight);
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
}

void
SnapshotWindow::draw(Viewport* viewport, const ImVec2& currentPixel, const ImVec2& dockOffset)
{
    // There must be a valid snapshot manager to draw the window
    if (!mSnapshotManager) {
        mSnapshotManager = viewport->getSnapshotManager();
        if (!mSnapshotManager) {
            return;
        }
    }

    if (!mOpen) { return; }

    configureWindow(dockOffset);

    float aspectRatio = static_cast<float>(viewport->getFramebufferWidth()) / 
                        static_cast<float>(viewport->getFramebufferHeight());
    int snapshotWidth = mSnapshotHeight * aspectRatio;   // width for snapshot display

    ImGui::Begin("Snapshot Window", &mOpen, ImGuiWindowFlags_NoMove | 
                                            ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_HorizontalScrollbar |
                                            ImGuiWindowFlags_NoCollapse |
                                            ImGuiWindowFlags_MenuBar);
    
    const std::vector<SnapshotManager::Snapshot>& snapshots = mSnapshotManager->getSnapshots();
    const int selectedSnapshotIdx = mSnapshotManager->getSelectedSnapshotIdx();

    // Draw help bar
    if (ImGui::BeginMenuBar()) {
        ImGui::Text("Help");
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + mHelpTooltipWrapWidth);
            ImGui::TextUnformatted("Click on a snapshot to display it in the viewport. Once a snapshot has been "
                                   "selected, you may use the left and right arrows to toggle between snapshots. "
                                   "Click on the currently selected snapshot to unselect it. Right click on a snapshot "
                                   "for more options.");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::EndMenuBar();
    }

    const float textHeight = ImGui::GetTextLineHeight();
                                            
    // Render the snapshot(s)
    for (int i = 0; i < snapshots.size(); ++i) {
        // Ensure all ImGui items/popups in this iteration get a unique ID
        ImGui::PushID(i);

        if (i > 0) {
            // Add spacing between images
            ImGui::SameLine(0.0f, mSnapshotMargin);
        }

        // Display image (upside down because our images are stored upside down -- this is 
        // because the main texture display in ImageWindow expects an upside down image, and if 
        // a snapshot image is selected, we want it to be displayed right-side-up)
        ImGui::Image((ImTextureID)snapshots[i].mTextureHandle, ImVec2(snapshotWidth, mSnapshotHeight),
                      ImVec2(0, 1), ImVec2(1, 0));

        // if the snapshot is currently chosen,
        // add a border to indicate it
        if (i == selectedSnapshotIdx) {
            highlightSnapshot();
        }

        handleSnapshotHover(snapshots[i].mLabel);
        handleSnapshotClick(viewport, selectedSnapshotIdx, i, snapshots[i].mLabel);

        ImGui::PopID();
    }

    handleDeleteHotkey(viewport);

    mLabelDialog->draw(viewport);
    mDeleteDialog->draw(viewport);

    ImGui::SameLine(ImGui::GetCursorScreenPos().x + mBottomPadding); // add padding at end
    ImGui::End();
}

} // namespace moonray_gui_v2
