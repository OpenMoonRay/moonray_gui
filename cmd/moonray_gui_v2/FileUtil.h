// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <set>
#include <string>

namespace moonray { namespace rndr { class RenderContext; }}

namespace moonray_gui_v2 {

// Save the current render buffer to an exr
void saveEXR(moonray::rndr::RenderContext* renderContext);

// Parses an rdla file and adds references to other
// rdla files (via the lua language) to the
// referencedRdlaFiles set.  Recursive.
void parseRdlaFileForReferences (
    const std::string &sceneFile,
    std::set<std::string> &referencedRdlaFiles,
    
    // Holds values of rdla file lua variables which are
    // possibly referenced in dofile() or other such type
    // file inclusion mechanisms
    std::map<std::string, std::string> &luaVariables
);

// Scan the scene objects in renderContext and populate textureFiles with paths
// to all .exr and .tx textures found in string attributes.
// mode corresponds to scene_rdl2::rdl2::TextureUpdateMode:
//   0 = off (do nothing)
//   1 = lights and light filters only
//   2 = full (all scene objects)
void collectTextureFiles(const moonray::rndr::RenderContext& renderContext,
                         int mode,
                         std::set<std::string>& textureFiles);

// For each scene object whose string attributes reference one of the changed
// texture files, call beginUpdate/set/endUpdate to mark it dirty so that
// renderPrep's applyUpdates() calls the DSO update() within the proper
// rendering pipeline context.
void touchTextureObjects(moonray::rndr::RenderContext& renderContext,
                         const std::set<std::string>& changedFiles);

// Get the user config directory path for moonray_gui.
// On Linux/macOS: ~/.config/moonray_gui/
// On Windows: %APPDATA%/moonray_gui/
// Creates the directory if it doesn't exist.
// Returns empty string on failure.
std::string getConfigDirectory();

} // end namespace moonray_gui_v2