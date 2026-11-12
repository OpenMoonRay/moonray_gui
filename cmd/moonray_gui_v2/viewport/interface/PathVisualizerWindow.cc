// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "PathVisualizerWindow.h"

#include "DrawUtil.h"
#include "../pathvisualizer/ViewController.h"
#include "../Viewport.h"

#include <moonray/rendering/rndr/PathVisualizerManager.h>

namespace moonray_gui_v2 {

void
PathVisualizerWindow::drawEnableButton(PathVisualizerManager* manager)
{
    const bool enabled = manager->isOn();
    const char* buttonText = enabled ? "Disable" : "Enable";
    const ImVec2 buttonSize(60, 30);
    
    if (ImGui::Button(buttonText, buttonSize)) {
        // When clicked, toggle on/off path visualizer
        if (enabled) { manager->turnOff(); } 
        else         { manager->turnOn();  }
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
}

void
PathVisualizerWindow::drawPixelSelector(Viewport* viewport)
{
    PathVisualizerManager* manager = viewport->getPathVisualizerManager();
    MNRY_ASSERT(manager);

    // Default to open
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Configuration")) {

        // ------ Camera Selection Dropdown ------------------------------------------------------------
        const bool cameraIsInteractive = viewport->pathVisualizerCamIsInteractive();
        const bool camerasMatch = viewport->pathVisualizerCamerasMatch();
        const char* selectedCamera = cameraIsInteractive ? "Interactive" : "Path Visualizer";
        const ImVec4 navigationBlue(0.35f, 0.65f, 1.0f, 1.0f);

        ImGui::TextUnformatted("Camera:");
        ImGui::SameLine();

        const ImVec4 dropdownTextColor = cameraIsInteractive ? navigationBlue : ImGui::GetStyleColorVec4(ImGuiCol_Text);
        ImGui::PushStyleColor(ImGuiCol_Text, dropdownTextColor);

        // When camera mode is PATH_VISUALIZER, we have two possibilities:
        // 1. The path visualizer and cached interactive camera do not match. We are currently viewing the path
        //    visualizer camera, but we have the option to switch to the interactive camera.
        // 2. The path visualizer and cached interactive camera match. We have not yet moved the interactive camera, 
        //    or we've reset it such that it's identical to the path visualizer camera. In this case, we should 
        //    disable the dropdown, as there is nothing to switch to.
        ImGui::BeginDisabled(!cameraIsInteractive && camerasMatch);
        if (ImGui::BeginCombo("##PathVisualizerCamera", selectedCamera)) {
            ImGui::PopStyleColor();

            if (ImGui::Selectable("Path Visualizer", !cameraIsInteractive)) {
                viewport->pathVisualizerChangeCameraMode(pathvisualizer::CameraMode::PATH_VISUALIZER);
            }
            if (cameraIsInteractive) {
                ImGui::PushStyleColor(ImGuiCol_Text, navigationBlue);
            }
            if (ImGui::Selectable("Interactive", cameraIsInteractive)) {
                viewport->pathVisualizerChangeCameraMode(pathvisualizer::CameraMode::INTERACTIVE);
            }
            if (cameraIsInteractive) {
                ImGui::PopStyleColor();
            }
            ImGui::EndCombo();
        } else {
            ImGui::PopStyleColor();
        }
        ImGui::EndDisabled();

        // ------ Pixel Selection Inputs --------------------------------------------------------------

        const int pixelX = static_cast<int>(manager->getPixelX());
        const int pixelY = static_cast<int>(manager->getPixelY());

        // If the camera is interactive, we should disable pixel selection inputs, because it would be
        // confusing to the user which camera would be used for generating path visualizer data. The only 
        // way to select a pixel from the interactive camera is by clicking the pixel in the viewport, which will
        // update the path visualizer recording camera.
        ImGui::BeginDisabled(cameraIsInteractive);
        if (cameraIsInteractive) {
            ImGui::PushStyleColor(ImGuiCol_Text, navigationBlue);
        }
        drawInputInt("Pixel X", pixelX, mMinPixelX, mMaxPixelX, [&](int value) {
            manager->setPixelX(static_cast<uint32_t>(std::max(0, value)), /*update*/ true);
        });

        drawInputInt("Pixel Y", pixelY, mMinPixelY, mMaxPixelY, [&](int value) {
            manager->setPixelY(static_cast<uint32_t>(std::max(0, value)), /*update*/ true);
        });
        if (cameraIsInteractive) {
            ImGui::PopStyleColor();
        }
        ImGui::EndDisabled();
    }
    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
}

void
PathVisualizerWindow::drawSamplingSettingsMenu(PathVisualizerManager* manager)
{
    // Default to open
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Sampling Settings")) {
        bool useSceneSamples = manager->getUseSceneSamples();

        if (ImGui::Checkbox("Use Scene Sampling Settings", &useSceneSamples)) {
            manager->setUseSceneSamples(useSceneSamples, /*update*/ true);
        }
        
        // Disable other sampling settings if useSceneSamples is checked
        ImGui::BeginDisabled(useSceneSamples);

        // -------------------- Pixel Samples Input --------------------------- //
        drawInputInt("Pixel Samples", manager->getPixelSamples(), mMinPixelSamples, mMaxPixelSamples, [&](int value) {
            manager->setPixelSamples(value, /*update*/ true);
        });
        
        // -------------------- Light Samples Input --------------------------- //
        drawInputInt("Light Samples", manager->getLightSamples(), mMinLightSamples, mMaxLightSamples, [&](int value) {
            manager->setLightSamples(value, /*update*/ true);
        });
        
        // -------------------- BSDF Samples Input --------------------------- //
        drawInputInt("BSDF  Samples", manager->getBsdfSamples(), mMinBsdfSamples, mMaxBsdfSamples, [&](int value) {
            manager->setBsdfSamples(value, /*update*/ true);
        });

        ImGui::EndDisabled();
    }
    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);}
}

void 
PathVisualizerWindow::drawMaxDepthMenu(PathVisualizerManager* manager)
{
    // Default to open
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Max Depth")) {

        drawInputInt("Max Depth", manager->getMaxDepth(), mMinMaxDepth, mMaxMaxDepth, [&](int value) {
            manager->setMaxDepth(value, /*update*/ true);
        });
    }
    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
}

void
PathVisualizerWindow::drawVisibilityTogglesMenu(PathVisualizerManager* manager)
{
    // Default to open
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Visibility Toggles")) {
        bool showDirectRays = manager->getShowDirectRays();
        bool showIndirectRays = manager->getShowIndirectRays();
        bool showSamples = manager->getShowSamples();

        // Indirect rays (continuing rays)
        bool showIndirectDiffuse = manager->getShowIndirectDiffuseRays();
        bool showIndirectSpecular = manager->getShowIndirectSpecularRays();
        
        // Direct rays (occlusion rays)
        bool showDirectDiffuse = manager->getShowDirectDiffuseRays();
        bool showDirectSpecular = manager->getShowDirectSpecularRays();
        bool showDirectLight = manager->getShowDirectLightRays();
        
        // Samples
        bool showDiffuseSamples = manager->getShowDiffuseSamples();
        bool showSpecularSamples = manager->getShowSpecularSamples();
        bool showLightSamples = manager->getShowLightSamples();

        // Parent checkbox for Indirect Rays
        if (ImGui::Checkbox("Indirect Rays (Continuing)", &showIndirectRays)) {
            manager->setShowIndirectRays(showIndirectRays);
        }
        
        // Indent child checkboxes
        ImGui::Indent();
        if (ImGui::Checkbox("Diffuse", &showIndirectDiffuse)) {
            manager->setShowIndirectDiffuseRays(showIndirectDiffuse);
        }
        if (ImGui::Checkbox("Specular", &showIndirectSpecular)) {
            manager->setShowIndirectSpecularRays(showIndirectSpecular);
        }
        ImGui::Unindent();

        ImGui::Spacing();
        
        // Parent checkbox for Direct Rays
        if (ImGui::Checkbox("Direct Rays (Occlusion)", &showDirectRays)) {
            manager->setShowDirectRays(showDirectRays);
        }
        
        // Indent child checkboxes
        ImGui::Indent();
        if (ImGui::Checkbox("Diffuse##Direct", &showDirectDiffuse)) {
            manager->setShowDirectDiffuseRays(showDirectDiffuse);
        }
        if (ImGui::Checkbox("Specular##Direct", &showDirectSpecular)) {
            manager->setShowDirectSpecularRays(showDirectSpecular);
        }
        if (ImGui::Checkbox("Light", &showDirectLight)) {
            manager->setShowDirectLightRays(showDirectLight);
        }
        ImGui::Unindent();

        ImGui::Spacing();
        
        // Parent checkbox for Samples
        if (ImGui::Checkbox("Samples", &showSamples)) {
            manager->setShowSamples(showSamples);
        }
        
        // Indent child checkboxes
        ImGui::Indent();
        if (ImGui::Checkbox("Diffuse##Samples", &showDiffuseSamples)) {
            manager->setShowDiffuseSamples(showDiffuseSamples);
        }
        if (ImGui::Checkbox("Specular##Samples", &showSpecularSamples)) {
            manager->setShowSpecularSamples(showSpecularSamples);
        }
        if (ImGui::Checkbox("Light##Samples", &showLightSamples)) {
            manager->setShowLightSamples(showLightSamples);
        }
        ImGui::Unindent();
    }
    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
}

void
PathVisualizerWindow::drawStyleMenu(Viewport* viewport)
{
    PathVisualizerManager* manager = viewport->getPathVisualizerManager();
    MNRY_ASSERT(manager);

    // Default to open
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Style Options")) {
        // Line width control
        int roundedLineWidth = static_cast<uint32_t>(manager->getLineWidth() + 0.5f);
        drawInputInt("Line Width", roundedLineWidth, mMinLineWidth, mMaxLineWidth, [&](int value) {
            manager->setLineWidth(static_cast<float>(value));
        });

        // Hidden line opacity
        drawSliderFloat("Hidden Line Opacity", manager->getHiddenLineOpacity(), 0.0f, 1.0f, [&](float value) {
            manager->setHiddenLineOpacity(value);
        });

        // Toggle off/on showing endpoints only
        ImGui::Checkbox("Show Endpoints Only", &viewport->pathVisualizerShowOnlyEndpoints());

        // Separator between display options and color pickers
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Color pickers for different ray types
        drawColorEdit("Camera Ray", manager->getCameraRayColor(), [&](scene_rdl2::math::Color color) {
            manager->setCameraRayColor(color);
        });
        
        ImGui::Spacing();
        ImGui::Text("Indirect Rays (Continuing):");
        ImGui::Indent();
        drawColorEdit("Diffuse", manager->getIndirectDiffuseRayColor(), [&](scene_rdl2::math::Color color) {
            manager->setIndirectDiffuseRayColor(color);
        });
        drawColorEdit("Specular", manager->getIndirectSpecularRayColor(), [&](scene_rdl2::math::Color color) {
            manager->setIndirectSpecularRayColor(color);
        });
        ImGui::Unindent();
        
        ImGui::Spacing();
        ImGui::Text("Direct Rays (Occlusion):");
        ImGui::Indent();
        drawColorEdit("Diffuse##Direct", manager->getDirectDiffuseRayColor(), [&](scene_rdl2::math::Color color) {
            manager->setDirectDiffuseRayColor(color);
        });
        drawColorEdit("Specular##Direct", manager->getDirectSpecularRayColor(), [&](scene_rdl2::math::Color color) {
            manager->setDirectSpecularRayColor(color);
        });
        drawColorEdit("Light", manager->getDirectLightRayColor(), [&](scene_rdl2::math::Color color) {
            manager->setDirectLightRayColor(color);
        });
        ImGui::Unindent();
    }
    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
}

void
PathVisualizerWindow::drawMiscMenu(PathVisualizerManager* manager)
{
    // Draw miscellaneous menu (including max ray length)
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Miscellaneous")) {
        drawInputFloat("Max Ray Length", manager->getMaxRayLength(), [&](float value) {
            manager->setMaxRayLength(value, /*update*/ true);
        });
    }
    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
}

void
PathVisualizerWindow::drawNodeInfo(PathVisualizerManager* manager)
{
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Node Info")) {
        const int nodeIndex = manager->getSelectedNode();
        if (nodeIndex < 0) {
            const std::string infoStr = "No node selected. In order to select a node, use the LEFT/RIGHT arrow "
                                        "keys while holding SHIFT";
            ImGui::TextWrapped("%s", infoStr.c_str());
            return;
        }

        const std::string nodeInfo = manager->getNodeInfo(static_cast<size_t>(nodeIndex));
        ImGui::TextWrapped("%s", nodeInfo.c_str());
    }
}

void
PathVisualizerWindow::configureWindow(const ImVec2& dockOffset) const
{
    // Establish size and position
    ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(imguiViewport->Size.x - mWidth - dockOffset.x, 0));
    ImGui::SetNextWindowSize(ImVec2(mWidth, imguiViewport->Size.y));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(mWindowPadding, mWindowPadding));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, mItemSpacing));
}

void
PathVisualizerWindow::draw(Viewport* viewport, const ImVec2& currentPixel, const ImVec2& dockOffset)
{
    if (!mOpen) { return; }

    // If path visualizer manager doesn't exist, we can't
    // perform any of the actions in the UI
    PathVisualizerManager* manager = viewport->getPathVisualizerManager();
    if (!manager) {
        return;
    }

    configureWindow(dockOffset);

    // Create window
    ImGui::Begin("Path Visualizer", &mOpen, ImGuiWindowFlags_NoMove | 
                                            ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoCollapse);
    // Create "on/off" button
    drawEnableButton(manager);
    // Create pixel selector
    drawPixelSelector(viewport);
    // Create sampling settings menu
    drawSamplingSettingsMenu(manager);
    // Create max depth menu
    drawMaxDepthMenu(manager);
    // Create visibility toggles menu
    drawVisibilityTogglesMenu(manager);
    // Create style options menu
    drawStyleMenu(viewport);
    // Create miscellaneous menu
    drawMiscMenu(manager);
    // Create node information display
    drawNodeInfo(manager);

    ImGui::End();
    ImGui::PopStyleVar(2);
}

} // namespace moonray_gui_v2
