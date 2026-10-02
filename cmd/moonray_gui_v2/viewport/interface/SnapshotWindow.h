// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Component.h"

#include <array>
#include <memory>
#include <string>

namespace moonray_gui_v2 {

class ConfirmDialog;
class InputTextDialog;
class SnapshotManager;

class SnapshotWindow : public Component {
public:
    SnapshotWindow();
    ~SnapshotWindow() override;

    void draw(Viewport* viewport, const ImVec2& currentPixel, const ImVec2& dockOffset) override;

    int getWidth() const override { return -1; }  // -1 means auto-size to full width
    int getHeight() const override { return mHeight; }

private:
    // Define the window settings (size, position, style, etc)
    void configureWindow(const ImVec2& dockOffset) const;

    void highlightSnapshot() const;
    void handleSnapshotHover(const std::string& label) const;
    void handleSnapshotClick(Viewport *viewport,
                             const int selectedSnapshotIdx,
                             const int clickedSnapshotIdx,
                             const std::string& clickedSnapshotLabel);
    void handleDeleteHotkey(Viewport* viewport);

    SnapshotManager* mSnapshotManager {nullptr};    // Pointer to the snapshot manager

    int mHeight {205};                              // height of the window; width is auto-sized to the viewport
    int mBottomPadding {50};                        // padding between the bottom of the window and the snapshots

    int mSnapshotHeight {150};                      // height of the snapshot display; width is then calculated
                                                    // based on the aspect ratio
    float mSnapshotMargin {10.0f};                  // margin between snapshots
    float mHelpTooltipWrapWidth {400.0f};           // width of the "help" tooltip; determines where text will wrap

    float mSelectedBorderThickness {3.f};                     // thickness of the border around the current snapshot
    ImU32 mSelectedBorderColor {IM_COL32(0, 255, 255, 255)};  // color of the border around the current snapshot

    int mLabelDialogSnapshotIdx {-1};                   // Index for snapshot for which label dialog is open; -1 if none
    std::unique_ptr<InputTextDialog> mLabelDialog;      // Dialog for updating the label of a snapshot

    int mDeleteDialogSnapshotIdx {-1};                  // Index of snapshot pending deletion; -1 if none
    std::unique_ptr<ConfirmDialog> mDeleteDialog;       // Confirmation dialog for deleting a snapshot
};

} // end namespace moonray_gui_v2
