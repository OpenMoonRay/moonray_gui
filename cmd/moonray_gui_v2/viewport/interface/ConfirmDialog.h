// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "imgui.h"
#include <array>
#include <cstdio>
#include <functional>
#include <string>

namespace moonray_gui_v2 {

class Viewport;

class ConfirmDialog {
public:
    ConfirmDialog(const std::string& title, const std::function<void(bool)>& callback);
    ~ConfirmDialog() = default;

    void setDescription(const std::string& desc) { mDesc = desc; }
    void draw(Viewport* viewport);
    void open(Viewport* viewport);

private:
    void configureWindow() const;
    void close(Viewport* viewport);
    
    bool mOpen {false};                                 // Whether the dialog is currently open
    std::string mTitle {""};                            // Title of the dialog window
    std::string mDesc {""};                             // Description text displayed above the input text field
    std::function<void(bool)> mCallback;                // Callback function to use when the user confirms or cancels
    ImGuiWindowFlags mWindowFlags       {ImGuiWindowFlags_AlwaysAutoResize};
};

} // end namespace moonray_gui_v2
