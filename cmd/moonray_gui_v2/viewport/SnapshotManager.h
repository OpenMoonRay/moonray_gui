// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../ColorManager.h"

#include <string>
#include <unordered_map>
#include <vector>

struct ImGuiContext;
struct ImGuiSettingsHandler;
struct ImGuiTextBuffer;

namespace moonray { namespace rndr { class RenderContext; }}

namespace moonray_gui_v2 {

class Viewport;

class SnapshotManager {

public:

    struct Snapshot {
        std::string mFilename;
        std::string mLabel;
        unsigned int mTextureHandle;

        bool operator==(const Snapshot& other) const {
            return mFilename == other.mFilename && mLabel == other.mLabel && mTextureHandle == other.mTextureHandle;
        }
    };

    SnapshotManager(const std::string& snapPath, const Viewport* viewport);
    ~SnapshotManager();

    // Loads all snapshot textures into OpenGL. If a snapshot fails to load, it will be removed from the list.
    // This function should be called after the OpenGL context has been initialized.
    void loadSnapshotTextures();

    // ---- User Utilities ----------------------------------------------

    // Takes a snapshot of the current render buffer and saves it to disk.
    void takeASnapshot(const moonray::rndr::RenderContext* renderContext);

    // Selects the previous snapshot in the list.
    void prev();

    // Selects the next snapshot in the list.
    void next();

    // Deletes the snapshot at the given index, including its OpenGL texture and the file on disk. 
    // If the deleted snapshot is currently selected, the selection will be updated to a valid snapshot if available.
    // If the snapshot idx is invalid, no action will be taken.
    void deleteSnapshotAtIdx(const int idx);

    // ---- Getters/Setters --------------------------------------------

    void setSnapshotLabelAtIdx(const int idx, const std::string& label)
    {
        MNRY_ASSERT(snapshotIdxIsValid(idx));
        mSnapshots[idx].mLabel = label;
    }

    void selectSnapshotAtIdx(const int idx)
    {
        MNRY_ASSERT(snapshotIdxIsValid(idx));
        mSelectedSnapshotIdx = idx;
    }

    void deselectSnapshot()
    {
        mSelectedSnapshotIdx = -1;
    }

    int getSelectedSnapshotIdx() const
    {
        return mSelectedSnapshotIdx;
    }

    unsigned int getSelectedSnapshotTextureHandle() const
    {
        // Returns 0 (no texture) when nothing is selected
        if (!snapshotIdxIsValid(mSelectedSnapshotIdx)) { return 0; }
        return mSnapshots[mSelectedSnapshotIdx].mTextureHandle;
    }

    std::string getSelectedSnapshotLabel() const
    {
        return mSelectedSnapshotIdx == -1 ? "" : mSnapshots[mSelectedSnapshotIdx].mLabel;
    }

    std::string getSnapshotLabelAtIdx(const int idx) const
    {
        MNRY_ASSERT(snapshotIdxIsValid(idx));
        return mSnapshots[idx].mLabel;
    }

    bool getSnapshotTexturesLoaded() const { return mSnapshotTexturesLoaded; }

    const std::vector<Snapshot>& getSnapshots() const { return mSnapshots; }

private:
    void deserializeSnapshots();
    void serializeSnapshots();

    /**
     * Creates an OpenGL texture from the given pixel data.
     * 
     * @param pixels Pointer to the pixel data.
     * @param width Width of the image.
     * @param height Height of the image.
     * @return The OpenGL texture handle.
     */
    unsigned int createTexture(float* pixels, const int width, const int height);

    /**
     * Reads the snapshot image from disk and creates an OpenGL texture.
     * 
     * @param snapshot The snapshot object containing the filename and other metadata.
     * @return True if the snapshot was successfully loaded and the texture created, false otherwise.
     */
    bool loadSnapshotTexture(Snapshot& snapshot);

    bool snapshotIdxIsValid(const int idx) const { return idx >= 0 && idx < static_cast<int>(mSnapshots.size()); }

    bool mValid {true};                             // whether the snapshot path given is valid
    std::string mSnapshotPath;                      // path to the user's snapshot directory
    bool mSnapshotTexturesLoaded {false};           // have snapshot textures been loaded into OpenGL?

    std::vector<Snapshot> mSnapshots;               // Vector of snapshots
    int mSelectedSnapshotIdx {-1};                  // Index of currently selected snapshot; -1 if none selected

    ColorManager mColorManager;                     // Color manager for applying CRT to snapshots
    const Viewport* mViewport;                      // Pointer to the viewport for color management
};

} // end namespace moonray_gui_v2
