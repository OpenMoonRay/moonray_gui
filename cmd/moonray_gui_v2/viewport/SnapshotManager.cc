// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "SnapshotManager.h"
#include "Viewport.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <moonray/rendering/rndr/rndr.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <GLFW/glfw3.h>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <unistd.h>

#include <OpenImageIO/imageio.h>

using namespace moonray;
using namespace scene_rdl2::logging;

namespace {

bool validateSnapPath(const std::string& snapPath)
{
    auto error = [snapPath] (const std::string& msg) {
        Logger::error(msg + " '" + snapPath + "'. Snapshotting has been disabled.\n");
        return false;
    };

    const std::filesystem::path snapshotDir(snapPath);
    std::error_code errorCode;

    // Create missing snapshot directories, but reject existing paths that are not directories.
    if (!std::filesystem::exists(snapshotDir, errorCode)) {
        if (errorCode) {
            return error("Unable to access snapshot path");
        }
        if (!std::filesystem::create_directories(snapshotDir, errorCode) && errorCode) {
            return error("Unable to create snapshot path");
        }
    }

    if (!std::filesystem::is_directory(snapshotDir, errorCode) || errorCode) {
        return error("Invalid snapshot path");
    }

    // Check to ensure we have write and execute permissions for the snapshot directory.
    if (::access(snapshotDir.c_str(), W_OK | X_OK) != 0) {
        return error("Invalid permission for snapshot path");
    }

    return true;
}

} // end anonymous namespace

namespace moonray_gui_v2 {

SnapshotManager::SnapshotManager(const std::string& snapPath,
                                 const Viewport* viewport) 
    : mSnapshotPath(snapPath)
    , mViewport(viewport)
{
    // If path is empty, use current directory
    if (snapPath.empty()) {
        mSnapshotPath = std::filesystem::current_path().string();
    }

    // Ensure the path is valid. Missing directories are created, but invalid paths disable snapshotting.
    mValid = validateSnapPath(mSnapshotPath);
    if (!mValid) {
        return;
    }

    // Append directory separator
    mSnapshotPath = (std::filesystem::path(mSnapshotPath) / "").string();

    // Deserialize snapshot data from snapshots.txt file (if it exists)
    deserializeSnapshots();
}

SnapshotManager::~SnapshotManager()
{
    // Write out the snapshot labels to file
    serializeSnapshots();
}

// ---- Snapshot Serializing to/Deserializing from Disk ----------------------------------------------------------------

void
SnapshotManager::deserializeSnapshots()
{
    if (!mValid) {
        return;
    }

    std::string snapshotsFilePath = mSnapshotPath + "snapshots.txt";
    // If file does not exist, return
    if (!std::filesystem::exists(snapshotsFilePath)) {
        return;
    }

    std::ifstream infile(snapshotsFilePath);
    std::string line;
    while (std::getline(infile, line)) {
        // Each line is expected to be in the format: filename|label
        // This allows users to use labels with spaces.
        std::istringstream iss(line);
        std::string filename, label;

        size_t delimiterPos = line.find("|");
        if (delimiterPos == std::string::npos) {
            std::cerr << "Malformed line in snapshots.txt: " << line << std::endl;
            continue; // Skip malformed lines
        }
        filename = line.substr(0, delimiterPos);
        label = line.substr(delimiterPos + 1);

        mSnapshots.emplace_back(Snapshot{filename, label, 0}); // We'll load the texture later
    }
}

void
SnapshotManager::serializeSnapshots()
{
    if (!mValid) {
        return;
    }
    std::string snapshotsFilePath = mSnapshotPath + "snapshots.txt";
    std::ofstream outfile(snapshotsFilePath);    for (const Snapshot& snapshot : mSnapshots) {
        outfile << snapshot.mFilename << "|" << snapshot.mLabel << "\n";
    }
}

// ---- Loading Snapshot Textures --------------------------------------------------------------------------------------

unsigned int
SnapshotManager::createTexture(float* pixels, const int width, const int height)
{
    // Apply color grading to the snapshot pixels
    scene_rdl2::fb_util::Rgb888Buffer displayBuffer;
    displayBuffer.init(width, height);

    mColorManager.applySnapshotCRT(mViewport, pixels, width, height, &displayBuffer);

    // Create a OpenGL texture identifier
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set pixel store parameters
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Setup texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Upload processed RGB buffer to texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, displayBuffer.getData());

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

bool
SnapshotManager::loadSnapshotTexture(Snapshot& snapshot)
{
    MNRY_ASSERT(!snapshot.mFilename.empty());
    const std::string filepath = mSnapshotPath + snapshot.mFilename;

    int width, height, channels;
    auto inp = OIIO::ImageInput::open(filepath);
    if (!inp) {
        std::cerr << "Failed to open snapshot image: " << filepath << std::endl;
        return false;
    }
    const OIIO::ImageSpec &spec = inp->spec();
    width = spec.width;
    height = spec.height;
    channels = spec.nchannels;
    
    // Allocate buffer for float data
    std::vector<float> pixels(width * height * channels);
    const int scanlineSize = width * channels * sizeof(float);
    
    // Read image flipped (start from last scanline and go backwards)
    // We do this because imgui reads our final texture upside down
    inp->read_image(0, 0, 0, channels, OIIO::TypeDesc::FLOAT, 
                    (char*)pixels.data() + (height-1)*scanlineSize, 
                    OIIO::AutoStride, -scanlineSize, OIIO::AutoStride);
    inp->close();

    if (pixels.data()) {
        // Create the opengl texture
        snapshot.mTextureHandle = createTexture(pixels.data(), width, height);
        return true;
    } else {
        std::cerr << "Failed to load snapshot image: " << filepath << std::endl;
        return false;
    }
}

void 
SnapshotManager::loadSnapshotTextures()
{
    mColorManager.setupConfig();
    // Try to load each snapshot. If the snapshot fails to load, remove it.
    mSnapshots.erase(std::remove_if(mSnapshots.begin(), mSnapshots.end(), [this](Snapshot& snapshot) {
        return !loadSnapshotTexture(snapshot);
    }), mSnapshots.end());
    mSnapshotTexturesLoaded = true;
}

// ---- User Interaction -----------------------------------------------------------------------------------------------

void
SnapshotManager::takeASnapshot(const moonray::rndr::RenderContext* renderContext)
{
    // If snapshot path was not valid, we can't take any new snapshots
    if (!mValid) { 
        Logger::error("Failed to take snapshot. Snapshot path is invalid.");
        return; 
    }
    // Ensure the render context exists and can be displayed.
    // Key bindings can call this function before everything is fully ready.
    if (!renderContext || !renderContext->isFrameReadyForDisplay()) return;

    // Generate snapshot filename
    int startIdx = mSnapshots.size();
    std::stringstream ss;
    std::string outputFilename;

    // Keep incrementing the snapshot index until we find a filename that does not already exist
    while (true) {
        ss << "snapshot." << std::setw(4) << std::setfill('0') << startIdx << ".exr";
        outputFilename = ss.str();

        // Check that the file does not already exist
        if (!std::ifstream(mSnapshotPath + outputFilename)) {
            break;
        }
        // Otherwise, reset and increment the index
        ss.str("");
        ss.clear();
        startIdx++;
    }

    // Write the image
    scene_rdl2::fb_util::RenderBuffer outputBuffer;
    const scene_rdl2::rdl2::SceneObject *metadata = renderContext->getSceneContext().getSceneVariables()
                                                                  .getExrHeaderAttributes();
    const scene_rdl2::math::HalfOpenViewport aperture = renderContext->getRezedApertureWindow();
    const scene_rdl2::math::HalfOpenViewport region = renderContext->getRezedRegionWindow();
    renderContext->snapshotRenderBuffer(&outputBuffer, /*untile*/ true, /*parallel*/ true, /*usePrimaryAov*/ true);
    try {
        moonray::rndr::writePixelBuffer(outputBuffer, mSnapshotPath + outputFilename, metadata, aperture, region);
        std::cout << "Snapshot " << outputFilename << " taken and saved to " << mSnapshotPath << std::endl;
        Snapshot newSnapshot = {outputFilename, outputFilename, 0};
        if (loadSnapshotTexture(newSnapshot)) {
            mSnapshots.push_back(newSnapshot);
        } else {
            std::cerr << "Failed to create new snapshot: " << outputFilename << std::endl;
        }
    } catch (...) {
        Logger::error("Failed to write out snapshot");
    }
}

void
SnapshotManager::prev()
{
    if (snapshotIdxIsValid(mSelectedSnapshotIdx) && !mSnapshots.empty()) {
        mSelectedSnapshotIdx = (mSelectedSnapshotIdx - 1 + mSnapshots.size()) % mSnapshots.size();
    }
}

void 
SnapshotManager::next()
{ 
    if (snapshotIdxIsValid(mSelectedSnapshotIdx) && !mSnapshots.empty()) {
        mSelectedSnapshotIdx = (mSelectedSnapshotIdx + 1) % mSnapshots.size();
    }
}

void
SnapshotManager::deleteSnapshotAtIdx(const int idx)
{
    if (snapshotIdxIsValid(idx)) {

        // First, delete the OpenGL texture
        glDeleteTextures(1, &mSnapshots[idx].mTextureHandle);

        // Then, delete the snapshot from disk
        const std::string& filename = mSnapshots[idx].mFilename;
        if (std::remove((mSnapshotPath + filename).c_str()) != 0) {
            std::cerr << "Failed to delete snapshot file: " << mSnapshotPath + filename << std::endl;
        }

        // Finally, remove the snapshot from the list
        mSnapshots.erase(mSnapshots.begin() + idx);
        
        // Cycle the selected snapshot index to a valid snapshot if necessary
        if (mSnapshots.empty()) {
             mSelectedSnapshotIdx = -1;
         } else if (mSelectedSnapshotIdx == idx) {
             mSelectedSnapshotIdx = idx % mSnapshots.size();
         } else if (mSelectedSnapshotIdx > idx) {
             --mSelectedSnapshotIdx;
         }
    }
}

} // end namespace moonray_gui_v2
