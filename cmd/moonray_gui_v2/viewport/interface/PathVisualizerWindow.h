// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Component.h"

#include <scene_rdl2/common/math/Color.h>

#include <functional>
#include <string>

namespace moonray { namespace rndr { class PathVisualizerManager; }}

namespace moonray_gui_v2 {

using PathVisualizerManager = moonray::rndr::PathVisualizerManager;

class PathVisualizerWindow : public Component {
/* This window manages the UI for the path visualizer settings and controls. It is by 
 * default docked on the righthand side of the viewport, and can be opened using the hotkey
 * in the Keyboard class. Whenever a setting is changed that might require the visualizer to 
 * re-process, we need to set "update" to true in the setter. Any purely aesthetic changes 
 * (e.g. line width/color) do not require restarting the simulation. Once the path visualizer 
 * has finished processing, it will generate line data, which will be drawn as an overlay in 
 * the ImageDisplay class.
 */
    public:
        PathVisualizerWindow() : Component(/*isOpen*/ false) {}

        ~PathVisualizerWindow() override {}

        void draw(Viewport* viewport, const ImVec2& currentPixel, const ImVec2& dockOffset) override;

        int getWidth() const override { return mWidth; }
        int getHeight() const override { return -1; } // auto-size height

    private:
        // Define the window settings (size, position, style, etc)
        void configureWindow(const ImVec2& dockOffset) const;

        // Draw enable/disable button
        void drawEnableButton(PathVisualizerManager* manager);

        // Draw various submenus
        void drawPixelSelector(Viewport* viewport);
        void drawSamplingSettingsMenu(PathVisualizerManager* manager);
        void drawMaxDepthMenu(PathVisualizerManager* manager);
        void drawVisibilityTogglesMenu(PathVisualizerManager* manager);
        void drawStyleMenu(Viewport* viewport);
        void drawMiscMenu(PathVisualizerManager* manager);
        void drawNodeInfo(PathVisualizerManager* manager);

        int mWidth {250};                   // Width of the window

        // Slider limits
        float mItemSpacing {8.f};
        int mWindowPadding {10};

        int mMinLineWidth {1};
        int mMaxLineWidth {8};

        int mMinMaxDepth {1};
        int mMaxMaxDepth {20};

        int mMinPixelSamples {1};
        int mMaxPixelSamples {32};

        int mMinLightSamples {1};
        int mMaxLightSamples {32};

        int mMinBsdfSamples {1};
        int mMaxBsdfSamples {32};

        int mMinPixelX {0};
        int mMaxPixelX {1000000};

        int mMinPixelY {0};
        int mMaxPixelY {1000000};
};

} // end namespace moonray_gui_v2
