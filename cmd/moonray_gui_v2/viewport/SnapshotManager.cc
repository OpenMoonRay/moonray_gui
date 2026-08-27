// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "SnapshotManager.h"
#include "Viewport.h"

#include "imgui.h"

#include <moonray/rendering/rndr/rndr.h>

#include <filesystem>
#include <fstream>
#include <GLFW/glfw3.h>

#include <OpenImageIO/imageio.h>

using namespace moonray;
using namespace scene_rdl2::logging;

namespace {

bool validateSnapPath(const std::string& snapPath)
{
    auto warn = [snapPath] (const std::string& msg) {
        Logger::warn(msg + " '" + snapPath + "'. Snapshotting has been disabled.\n");
        return false;
    };
    
    // Check if path is a directory and exists
    if (!std::filesystem::is_directory(snapPath) || !std::filesystem::exists(snapPath)) {
        return warn("Invalid snapshot path");
    } else {
        // Create a temporary file in the given directory to test write permissions
        std::filesystem::path testFilePath = std::filesystem::path(snapPath) / ".__test_write_permission__";
        std::ofstream testFile(testFilePath.string());
        if (!testFile.is_open()) {
            return warn("Invalid permissions for snapshot path");
        }
        // Clean up the temp file
        testFile.close();
        std::filesystem::remove(testFilePath);
    }
    return true;
}

} // end anonymous namespace

namespace moonray_gui_v2 {

SnapshotManager::SnapshotManager(const std::string& snapPath,
                                 const Viewport* viewport) 
    : mSnapPath(snapPath)
    , mViewport(viewport)
{
    // If path is empty, use current directory
    if (snapPath.empty()) {
        mSnapPath = std::filesystem::current_path().string();
    }

    // Ensure the path is valid
    mValid = validateSnapPath(mSnapPath);
    if (!mValid) { return; }

    // Append directory separator
    mSnapPath = (std::filesystem::path(mSnapPath) / "").string();

    // Get index for snapshot
    // NOTE: This assumes snapshots are named sequentially without gaps.
    while (true) {
        std::stringstream ss;
        ss << mSnapPath << "snapshot.";
        ss << std::setw(4) << std::setfill('0') << mSnapIdx << ".exr";
        if (!std::filesystem::exists(ss.str())) {
            break;
        }
        mSnapIdx++;
    }
}

void
SnapshotManager::takeASnapshot(const moonray::rndr::RenderContext* renderContext)
{
    // If snapshot path was not valid, we can't take any new snapshots
    if (!mValid) { 
        Logger::error("Failed to take snapshot. Snapshotting is disabled.");
        return; 
    }
    // Ensure the render context exists and can be displayed.
    // Key bindings can call this function before everything is fully ready.
    if (!renderContext || !renderContext->isFrameReadyForDisplay()) return;

    std::stringstream ss;
    ss << "snapshot." << std::setw(4) << std::setfill('0') << mSnapIdx << ".exr";
    const std::string outputFilename = ss.str();
    // Write the image
    scene_rdl2::fb_util::RenderBuffer outputBuffer;
    const scene_rdl2::rdl2::SceneObject *metadata = renderContext->getSceneContext().getSceneVariables()
                                                                  .getExrHeaderAttributes();
    const scene_rdl2::math::HalfOpenViewport aperture = renderContext->getRezedApertureWindow();
    const scene_rdl2::math::HalfOpenViewport region = renderContext->getRezedRegionWindow();
    renderContext->snapshotRenderBuffer(&outputBuffer, /*untile*/ true, /*parallel*/ true, /*usePrimaryAov*/ true);
    try {
        moonray::rndr::writePixelBuffer(outputBuffer, mSnapPath + outputFilename, metadata, aperture, region);
        std::cout << "Snapshot " << outputFilename << " taken and saved to " << mSnapPath << std::endl;
        mSnapIdx++;
        loadNewSnapshot();
    } catch (...) {
        Logger::error("Failed to write out snapshot");
    }
}

void
SnapshotManager::createTexture(float* pixels, const int width, const int height)
{
    // Apply color grading to the snapshot pixels
    scene_rdl2::fb_util::Rgb888Buffer displayBuffer;
    displayBuffer.init(width, height);

    mColorManager.applySnapshotCRT(mViewport, pixels, width, height, &displayBuffer);

    // Create OpenGL texture
    unsigned int& texture = mSnapshotTextures.emplace_back();

    // Create a OpenGL texture identifier
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
}

void
SnapshotManager::loadSnapshotAtIdx(const int idx)
{
    std::stringstream ss;
    ss << "snapshot." << std::setw(4) << std::setfill('0') << idx << ".exr";
    const std::string filename = ss.str();
    const std::string filepath = mSnapPath + filename;

    int width, height, channels;
    auto inp = OIIO::ImageInput::open(filepath);
    if (!inp) {
        std::cerr << "Failed to open snapshot image: " << filepath << std::endl;
        return;
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
        createTexture(pixels.data(), width, height);
        // Save the filename
        mSnapshotFilenames.emplace_back(filename);
    } else {
        std::cerr << "Failed to load snapshot image: " << filename << std::endl;
    }
}

void 
SnapshotManager::loadNewSnapshot()
{
    loadSnapshotAtIdx(mSnapIdx - 1);
}

void 
SnapshotManager::load()
{
    mColorManager.setupConfig();
    for (int i = 1; i < mSnapIdx; ++i) {
        loadSnapshotAtIdx(i);
    }
    mSnapshotsAreLoaded = true;
}

void
SnapshotManager::clear()
{
    for (unsigned int texture : mSnapshotTextures) {
        glDeleteTextures(1, &texture);
    }
    mSnapshotTextures.clear();
    mSnapshotFilenames.clear();
    mSnapshotsAreLoaded = false;
    mChosenSnapshotIdx = -1;
}

void
SnapshotManager::prev()
{
    if (mChosenSnapshotIdx != -1 && !mSnapshotTextures.empty()) {
        mChosenSnapshotIdx = (mChosenSnapshotIdx - 1 + mSnapshotTextures.size()) % mSnapshotTextures.size();
    }
}

void 
SnapshotManager::next()
{ 
    if (mChosenSnapshotIdx != -1 && !mSnapshotTextures.empty()) {
        mChosenSnapshotIdx = (mChosenSnapshotIdx + 1) % mSnapshotTextures.size();
    }
}

int 
SnapshotManager::getChosenSnapshotTexture() const 
{ 
    return mChosenSnapshotIdx == -1 ? -1 : mSnapshotTextures[mChosenSnapshotIdx]; 
}

} // end namespace moonray_gui_v2
