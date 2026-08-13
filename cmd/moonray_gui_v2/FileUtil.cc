// Copyright 2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "FileUtil.h"

#include <moonray/application/RaasApplication.h>
#include <moonray/rendering/rndr/RenderContext.h>
#include <scene_rdl2/scene/rdl2/SceneContext.h>
#include <scene_rdl2/scene/rdl2/SceneObject.h>
#include <scene_rdl2/scene/rdl2/SceneClass.h>
#include <scene_rdl2/scene/rdl2/Attribute.h>
#include <scene_rdl2/scene/rdl2/Types.h>
#include <scene_rdl2/scene/rdl2/Light.h>
#include <scene_rdl2/scene/rdl2/LightFilter.h>
#include <scene_rdl2/scene/rdl2/Map.h>

#include <regex>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>

namespace moonray_gui_v2 {

void saveEXR(moonray::rndr::RenderContext* renderContext)
{
    // Create temporary buffers to snapshot into
    scene_rdl2::fb_util::RenderBuffer        renderBuffer;
    scene_rdl2::fb_util::HeatMapBuffer       heatMapBuffer;
    scene_rdl2::fb_util::FloatBuffer         weightBuffer;
    scene_rdl2::fb_util::RenderBuffer        renderBufferOdd;
    scene_rdl2::fb_util::VariablePixelBuffer renderOutputBuffer;

    std::vector<scene_rdl2::fb_util::VariablePixelBuffer> aovBuffers;
    std::vector<scene_rdl2::fb_util::VariablePixelBuffer> displayFilterBuffers;

    // Snapshot the main render buffer
    renderContext->snapshotRenderBuffer(&renderBuffer, /*untile*/ true, /*parallel*/ true, /*usePrimaryAov*/ true);

    // Write out the main beauty image
    const scene_rdl2::rdl2::SceneVariables& sceneVars = renderContext->getSceneContext().getSceneVariables();
    const std::string& outputFilename = sceneVars.get(scene_rdl2::rdl2::SceneVariables::sOutputFile);
    const scene_rdl2::rdl2::SceneObject* metadata = sceneVars.getExrHeaderAttributes();

    const scene_rdl2::math::HalfOpenViewport& aperture = renderContext->getRezedApertureWindow();
    const scene_rdl2::math::HalfOpenViewport& region = renderContext->getRezedRegionWindow();

    moonray::writeImageWithMessage(&renderBuffer, outputFilename, metadata, aperture, region);

    // Write any arbitrary RenderOutput objects
    renderContext->snapshotHeatMapBuffer(&heatMapBuffer, /*untile*/ true, /*parallel*/ true);
    renderContext->snapshotWeightBuffer(&weightBuffer, /*untile*/ true, /*parallel*/ true);
    renderContext->snapshotAovBuffers(aovBuffers, /*untile*/ true, /*parallel*/ true);
    renderContext->snapshotRenderBufferOdd(&renderBufferOdd, /*untile*/ true, /*parallel*/ true);
    renderContext->snapshotDisplayFilterBuffers(displayFilterBuffers, /*untile*/ true, /*parallel*/ true);

    const moonray::pbr::DeepBuffer* deepBuffer = renderContext->getDeepBuffer();
    moonray::pbr::CryptomatteBuffer* cryptomatteBuffer = renderContext->getCryptomatteBuffer();
    moonray::writeRenderOutputsWithMessages(renderContext->getRenderOutputDriver(),
                                            deepBuffer, cryptomatteBuffer, &heatMapBuffer,
                                            &weightBuffer, &renderBufferOdd, aovBuffers,
                                            displayFilterBuffers);
}

void parseRdlaFileForReferences (
    const std::string &sceneFile,
    std::set<std::string> &referencedRdlaFiles,
    std::map<std::string, std::string> &luaVariables
){
    // Keep the newly found rdla files separate for recursion
    std::set<std::string> newReferencedRdlaFiles;

    // Open sceneFile
    std::ifstream fin;
    fin.open(sceneFile);
    if (!fin.good()) {
        std::cerr << "Failed to load scenefile: " << sceneFile
            << std::endl;
        return;
    }

    // Find rdla references with variable concatenation like the following:
    // dofile(asset_lib_dir .. "char/astrid/skin/rdla/astrid_skin.rdla")
    // TODO: Make this work with more than one variable preceding the path
    //std::regex rdlaWithVariableRegex (R"(.*\((\w+)\s+\.\.\s+\"+(.*rdla)\"+.*)");
    std::regex rdlaWithVariableRegex (R"([^-]*\((\w+)\s+\.\.\s+\"+(.*rdla)\"+.*)");

    // Find straight rdla references with no variables lines like the following:
    // dofile("/work/gshad/moonshine/lib/char/astrid/skin/rdla/astrid_skin.rdla")
    std::regex rdlaWithoutVariableRegex (R"([^-]*\"+(.*rdla)\"+.*)");

    // Find lua variable assignment lines like the following:
    // asset_lib_dir = "/work/gshad/moonshine/lib/"
    std::regex variableAssignmentRegex (R"(^\s*(\w+)\s*=\s*\"+(.*)\"+)");

    std::string buf;
    while (std::getline(fin, buf)) {
        // Really long lines can cause issues for the regex parser.
        // Long lines are usually layer entries, or rdl mesh attributes, and
        // certainly do not contain rdla file references.
        const unsigned int maxLineSize = 1024;
        if (buf.size() > maxLineSize) continue;

        std::cmatch cm;
        if (std::regex_match(buf.c_str(), cm, rdlaWithVariableRegex)) {
            // Found line with .rdla file with variable used.
            // Try to find the lua variable in the map and then
            // add it to the found rdla path.
            std::string luaVariable = cm[1];
            std::string rdlaPath = cm[2];
            if (luaVariables.find(luaVariable) != luaVariables.end()) {
                std::string &luaVariableValue = luaVariables[luaVariable];
                if (luaVariableValue.back() == '/')
                    newReferencedRdlaFiles.insert(luaVariableValue + rdlaPath);
                else
                    newReferencedRdlaFiles.insert(luaVariableValue + '/' + rdlaPath);
            }
        } else if (std::regex_match(buf.c_str(), cm, rdlaWithoutVariableRegex)) {
            // Found line with .rdla file and no variable used.
            // Just add the rdla file to the set.
            newReferencedRdlaFiles.insert(cm[1]);
        } else if (std::regex_match(buf.c_str(), cm, variableAssignmentRegex)) {
            // Found line with variable assignment.
            // Add it to the map.
            luaVariables[cm[1]] = cm[2];
        }
        // Add other regex situations here
    }
    fin.close();

    // Parse referenced rdla files recursively
    for (auto &rdlaFile : newReferencedRdlaFiles) {
        parseRdlaFileForReferences (rdlaFile, referencedRdlaFiles, luaVariables);
    }

    // Concatenate the new set onto the set passed in
    referencedRdlaFiles.insert(newReferencedRdlaFiles.begin(), newReferencedRdlaFiles.end());
}

static bool isTextureFile(const std::string& path)
{
    // Compare extension case-insensitively so .EXR and .TX are also matched.
    auto lowerN = [&](size_t n) {
        if (path.size() < n) return std::string{};
        std::string ext = path.substr(path.size() - n);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    };
    if (lowerN(4) == ".exr") return true;
    if (lowerN(3) == ".tx")  return true;
    return false;
}

static void collectTexturesFromObject(const scene_rdl2::rdl2::SceneObject& obj,
                                      std::set<std::string>& textureFiles)
{
    const scene_rdl2::rdl2::SceneClass& sc = obj.getSceneClass();
    for (auto it = sc.beginAttributes(); it != sc.endAttributes(); ++it) {
        const scene_rdl2::rdl2::Attribute* attr = *it;

        // Only consider attributes explicitly declared as file paths.
        // This excludes output paths, stats files, etc. that also happen
        // to have .exr/.tx extensions but are not texture inputs.
        if (!(attr->getFlags() & scene_rdl2::rdl2::FLAGS_FILENAME)) continue;

        if (attr->getType() == scene_rdl2::rdl2::TYPE_STRING) {
            const scene_rdl2::rdl2::AttributeKey<scene_rdl2::rdl2::String> key(*attr);
            const std::string& val = obj.get(key);
            if (!val.empty() && isTextureFile(val)) {
                textureFiles.insert(val);
            }
        } else if (attr->getType() == scene_rdl2::rdl2::TYPE_STRING_VECTOR) {
            const scene_rdl2::rdl2::AttributeKey<scene_rdl2::rdl2::StringVector> key(*attr);
            const scene_rdl2::rdl2::StringVector& vec = obj.get(key);
            for (const auto& val : vec) {
                if (!val.empty() && isTextureFile(val)) {
                    textureFiles.insert(val);
                }
            }
        }
    }
}

void collectTextureFiles(const moonray::rndr::RenderContext& renderContext,
                         int mode,
                         std::set<std::string>& textureFiles)
{
    // mode 0 = off, nothing to do
    if (mode == 0) return;

    const scene_rdl2::rdl2::SceneContext& ctx = renderContext.getSceneContext();

    for (auto it = ctx.beginSceneObject(); it != ctx.endSceneObject(); ++it) {
        const scene_rdl2::rdl2::SceneObject* obj = it->second;
        if (!obj) continue;

        if (mode == 1) {
            // Lights and light filters only
            if (!obj->isA<scene_rdl2::rdl2::Light>() &&
                !obj->isA<scene_rdl2::rdl2::LightFilter>()) {
                continue;
            }
        }
        // mode == 2 (full): collect from all objects

        collectTexturesFromObject(*obj, textureFiles);
    }
}

void touchTextureObjects(moonray::rndr::RenderContext& renderContext,
                         const std::set<std::string>& changedFiles)
{
    scene_rdl2::rdl2::SceneContext& ctx = renderContext.getSceneContext();

    for (auto it = ctx.beginSceneObject(); it != ctx.endSceneObject(); ++it) {
        scene_rdl2::rdl2::SceneObject* obj = it->second;
        if (!obj) continue;

        const scene_rdl2::rdl2::SceneClass& sc = obj->getSceneClass();
        bool touched = false;

        for (auto attrIt = sc.beginAttributes(); attrIt != sc.endAttributes(); ++attrIt) {
            const scene_rdl2::rdl2::Attribute* attr = *attrIt;

            // Only touch attributes declared as file paths (same guard as collect).
            if (!(attr->getFlags() & scene_rdl2::rdl2::FLAGS_FILENAME)) continue;

            if (attr->getType() == scene_rdl2::rdl2::TYPE_STRING) {
                const scene_rdl2::rdl2::AttributeKey<scene_rdl2::rdl2::String> key(*attr);
                // Copy the value - get() returns a reference into attribute storage
                // which would be invalidated by the set() calls below.
                const std::string val = obj->get(key);
                if (!val.empty() && changedFiles.count(val)) {
                    if (!touched) {
                        obj->beginUpdate();
                        touched = true;
                    }
                    obj->set(key, std::string());
                    obj->set(key, val);
                }
            } else if (attr->getType() == scene_rdl2::rdl2::TYPE_STRING_VECTOR) {
                const scene_rdl2::rdl2::AttributeKey<scene_rdl2::rdl2::StringVector> key(*attr);
                const scene_rdl2::rdl2::StringVector& vec = obj->get(key);
                bool vecChanged = false;
                for (const auto& val : vec) {
                    if (!val.empty() && changedFiles.count(val)) { vecChanged = true; break; }
                }
                if (vecChanged) {
                    if (!touched) {
                        obj->beginUpdate();
                        touched = true;
                    }
                    scene_rdl2::rdl2::StringVector copy = vec;
                    obj->set(key, scene_rdl2::rdl2::StringVector());
                    obj->set(key, copy);
                }
            }
        }

        if (touched) {
            obj->endUpdate();
        }
    }
}

std::string getConfigDirectory()
{
    std::string configDir;

#ifdef _WIN32
    // Windows: Use APPDATA environment variable
    const char* appData = std::getenv("APPDATA");
    if (appData) {
        configDir = std::string(appData) + "/moonray_gui";
    } else {
        std::cerr << "Warning: APPDATA environment variable not set. "
                  << "Unable to create config directory." << std::endl;
        return "";
    }
#else
    // Linux/macOS: Use XDG_CONFIG_HOME or fall back to ~/.config
    const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfigHome && xdgConfigHome[0] != '\0') {
        configDir = std::string(xdgConfigHome) + "/moonray_gui";
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            configDir = std::string(home) + "/.config/moonray_gui";
        } else {
            std::cerr << "Warning: HOME environment variable not set. "
                      << "Unable to create config directory." << std::endl;
            return "";
        }
    }
#endif

    // Create the directory if it doesn't exist
    std::filesystem::create_directory(configDir);

    return configDir;
}

} // end namespace moonray_gui_v2