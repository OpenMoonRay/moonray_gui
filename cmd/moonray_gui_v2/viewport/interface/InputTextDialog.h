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

class InputTextDialog {
public:
    InputTextDialog(const std::string& title, const std::function<void(const std::string&)>& callback);
    ~InputTextDialog() = default;

    void setDescription(const std::string& desc) { mDesc = desc; }
    void setDefaultValue(const std::string& defaultVal) { mDefaultValue = defaultVal; }
    void draw(Viewport* viewport);
    void open(Viewport* viewport);

private:
    void configureWindow() const;
    void close(Viewport* viewport);
    
    bool mOpen {false};                                 // Whether the dialog is currently open
    std::string mTitle {""};                            // Title of the dialog window
    std::string mDesc {""};                             // Description text displayed above the input text field
    std::string mDefaultValue {""};                     // Default value for the input text field
    std::array<char, 256> mInputBuffer {};              // Buffer to hold the input text
    std::function<void(const std::string&)> mCallback;  // Callback function to use when the user submits the input text
    bool mNeedsFocus {true};                            // Whether the input text field needs to be focused when 
                                                        // the dialog is opened
    ImGuiWindowFlags mWindowFlags       {ImGuiWindowFlags_AlwaysAutoResize};
    ImGuiInputTextFlags mInputTextFlags {ImGuiInputTextFlags_EnterReturnsTrue | 
                                         ImGuiInputTextFlags_AutoSelectAll};
};

} // end namespace moonray_gui_v2
