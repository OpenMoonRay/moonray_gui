// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "GuiTypes.h"

namespace moonray_gui_v2 {

std::string getActionName(const Action action) {
    switch (action) {
    case ACTION_CAM_FORWARD:                    return "Camera Forward";
    case ACTION_CAM_BACKWARD:                   return "Camera Backward";
    case ACTION_CAM_LEFT:                       return "Camera Left";
    case ACTION_CAM_RIGHT:                      return "Camera Right";
    case ACTION_CAM_UP:                         return "Camera Up";
    case ACTION_CAM_DOWN:                       return "Camera Down";
    case ACTION_CAM_SLOW_DOWN:                  return "Camera Slow Down";
    case ACTION_CAM_SPEED_UP:                   return "Camera Speed Up";
    case ACTION_CAM_RESET:                      return "Camera Reset";
    case ACTION_CAM_RECENTER:                   return "Camera Recenter";
    case ACTION_CAM_FRAME_SCENE:                return "Camera Frame Scene";
    case ACTION_CAM_PRINT_MATRICES:             return "Camera Print Matrices";
    case ACTION_CAM_SET_UP_VECTOR:              return "Camera Set Up Vector";
    case ACTION_CAM_ROTATE:                     return "Camera Rotation";
    case ACTION_CAM_DOLLY:                      return "Camera Dolly";
    case ACTION_CAM_TRACK:                      return "Camera Track";
    case ACTION_CAM_ROLL:                       return "Camera Roll";
    case ACTION_IMAGE2D_PAN:                    return "Image 2D Pan";
    case ACTION_IMAGE2D_ZOOM:                   return "Image 2D Zoom";
    case ACTION_DENOISE_TOGGLE_ON_OFF:          return "Denoise Toggle On/Off";
    case ACTION_DENOISE_TOGGLE_MODE:            return "Denoise Toggle Mode";
    case ACTION_DENOISE_SELECT_BUFFERS:         return "Denoise Select Buffers";
    case ACTION_SNAPSHOT_TAKE:                  return "Snapshot Take";
    case ACTION_SNAPSHOT_PREV:                  return "Snapshot Previous";
    case ACTION_SNAPSHOT_NEXT:                  return "Snapshot Next";
    case ACTION_CHANNEL_TOGGLE_RGB:             return "Channel Toggle RGB";
    case ACTION_CHANNEL_TOGGLE_RED:             return "Channel Toggle Red";
    case ACTION_CHANNEL_TOGGLE_GREEN:           return "Channel Toggle Green";
    case ACTION_CHANNEL_TOGGLE_BLUE:            return "Channel Toggle Blue";
    case ACTION_CHANNEL_TOGGLE_ALPHA:           return "Channel Toggle Alpha";
    case ACTION_CHANNEL_TOGGLE_LUMINANCE:       return "Channel Toggle Luminance";
    case ACTION_CHANNEL_TOGGLE_RGB_NORMALIZED:  return "Channel Toggle RGB Normalized";
    case ACTION_CHANNEL_TOGGLE_NUM_SAMPLES:     return "Channel Toggle Num Samples";
    case ACTION_EXPOSURE_INCREASE:              return "Exposure Increase";
    case ACTION_EXPOSURE_DECREASE:              return "Exposure Decrease";
    case ACTION_EXPOSURE_ADJUST:                return "Exposure Adjust";
    case ACTION_EXPOSURE_RESET:                 return "Exposure Reset";
    case ACTION_GAMMA_ADJUST:                   return "Gamma Adjust";
    case ACTION_GAMMA_RESET:                    return "Gamma Reset";
    case ACTION_FAST_PROGRESSIVE_TOGGLE:        return "Fast Progressive Toggle";
    case ACTION_FAST_PROGRESSIVE_NEXT_MODE:     return "Fast Progressive Next Mode";
    case ACTION_FAST_PROGRESSIVE_PREV_MODE:     return "Fast Progressive Previous Mode";
    case ACTION_WINDOW_TOGGLE_EXPOSURE:         return "Window Toggle Exposure";
    case ACTION_WINDOW_TOGGLE_GAMMA:            return "Window Toggle Gamma";
    case ACTION_WINDOW_TOGGLE_KEY_BINDINGS:     return "Window Toggle Key Bindings";
    case ACTION_WINDOW_TOGGLE_SCENE_INSPECTOR:  return "Window Toggle Scene Inspector";
    case ACTION_WINDOW_TOGGLE_PATH_VISUALIZER:  return "Window Toggle Path Visualizer";
    case ACTION_WINDOW_TOGGLE_PIXEL_INSPECTOR:  return "Window Toggle Pixel Inspector";
    case ACTION_WINDOW_TOGGLE_SNAPSHOT:         return "Window Toggle Snapshot";
    case ACTION_WINDOW_TOGGLE_STATUS:           return "Window Toggle Status";
    case ACTION_WINDOW_TOGGLE_AXIS_DISPLAY:     return "Window Toggle Axis Display";
    case ACTION_SAVE_IMAGE:                     return "Save Image";
    case ACTION_PICK_PATH_VISUALIZER_PIXEL:     return "Pick Path Visualizer Pixel";
    case ACTION_PATH_VISUALIZER_ON_OFF:         return "Turn Path Visualizer On/Off";
    case ACTION_PATH_VISUALIZER_PREV_NODE:      return "Path Visualizer Previous Node";
    case ACTION_PATH_VISUALIZER_NEXT_NODE:      return "Path Visualizer Next Node";
    case ACTION_CAM_TOGGLE_ACTIVE_TYPE:         return "Camera Toggle Active Type";
    case ACTION_TILE_PROGRESS_TOGGLE:           return "Tile Progress Toggle";
    case ACTION_RENDER_OUTPUT_PREV:             return "Render Output Previous";
    case ACTION_RENDER_OUTPUT_NEXT:             return "Render Output Next";
    case ACTION_PRINT_KEY_BINDINGS:             return "Print Key Bindings";
    case ACTION_COPY_BUFFER_TO_CLIPBOARD:       return "Copy current snapshot/buffer to clipboard";
    default:                                    return "";
    }
}

// Gets the description of the action for display in the UI (help window)
std::string getDescription(const Action action) {
    switch (action) {
    case ACTION_CAM_FORWARD:                    return "Move forward";
    case ACTION_CAM_BACKWARD:                   return "Move backward";
    case ACTION_CAM_LEFT:                       return "Move left";
    case ACTION_CAM_RIGHT:                      return "Move right";
    case ACTION_CAM_UP:                         return "Move up";
    case ACTION_CAM_DOWN:                       return "Move down";
    case ACTION_CAM_SLOW_DOWN:                  return "Reduce movement speed";
    case ACTION_CAM_SPEED_UP:                   return "Increase movement speed";
    case ACTION_CAM_RESET:                      return "Reset camera to initial position and orientation";
    case ACTION_CAM_RECENTER:                   return "Recenter camera on point of interest";
    case ACTION_CAM_FRAME_SCENE:                return "Reorient and zoom camera to frame the entire scene";
    case ACTION_CAM_PRINT_MATRICES:             return "Print camera matrix to console";
    case ACTION_CAM_SET_UP_VECTOR:              return "Set camera upright (remove roll)";
    case ACTION_CAM_ROTATE:                     return "Rotate camera based on mouse movement";
    case ACTION_CAM_DOLLY:                      return "Dolly/zoom camera based on mouse movement";
    case ACTION_CAM_TRACK:                      return "Track camera based on mouse movement";
    case ACTION_CAM_ROLL:                       return "Roll camera based on mouse movement";
    case ACTION_IMAGE2D_PAN:                    return "Pan the image";
    case ACTION_IMAGE2D_ZOOM:                   return "Zoom the image in/out";
    case ACTION_DENOISE_TOGGLE_ON_OFF:          return "Toggle denoising on/off";
    case ACTION_DENOISE_TOGGLE_MODE:            return "Toggle denoising mode (optix, open image denoise, etc.)";
    case ACTION_DENOISE_SELECT_BUFFERS:         return "Select denoising buffers (beauty, albedo, normal, etc.)";
    case ACTION_SNAPSHOT_TAKE:                  return "Take a snapshot and save to the given snap_path (command line argument)";
    case ACTION_SNAPSHOT_PREV:                  return "Show previous snapshot";
    case ACTION_SNAPSHOT_NEXT:                  return "Show next snapshot";
    case ACTION_CHANNEL_TOGGLE_RGB:             return "Full RGB color";
    case ACTION_CHANNEL_TOGGLE_RED:             return "Red channel only";
    case ACTION_CHANNEL_TOGGLE_GREEN:           return "Green channel only";
    case ACTION_CHANNEL_TOGGLE_BLUE:            return "Blue channel only";
    case ACTION_CHANNEL_TOGGLE_ALPHA:           return "Alpha channel only";
    case ACTION_CHANNEL_TOGGLE_LUMINANCE:       return "Luminance channel only";
    case ACTION_CHANNEL_TOGGLE_RGB_NORMALIZED:  return "RGB Normalized channel only";
    case ACTION_CHANNEL_TOGGLE_NUM_SAMPLES:     return "Samples per pixel (SPP) view";
    case ACTION_EXPOSURE_INCREASE:              return "Increase exposure";
    case ACTION_EXPOSURE_DECREASE:              return "Decrease exposure";
    case ACTION_EXPOSURE_ADJUST:                return "Adjust exposure with mouse drag";
    case ACTION_EXPOSURE_RESET:                 return "Reset exposure";
    case ACTION_GAMMA_ADJUST:                   return "Adjust gamma with mouse drag";
    case ACTION_GAMMA_RESET:                    return "Reset gamma";
    case ACTION_FAST_PROGRESSIVE_TOGGLE:        return "Toggle fast progressive rendering on/off";
    case ACTION_FAST_PROGRESSIVE_NEXT_MODE:     return "Next fast progressive rendering mode";
    case ACTION_FAST_PROGRESSIVE_PREV_MODE:     return "Previous fast progressive rendering mode";
    case ACTION_WINDOW_TOGGLE_EXPOSURE:         return "Open/close exposure adjustment window";
    case ACTION_WINDOW_TOGGLE_GAMMA:            return "Open/close gamma adjustment window";
    case ACTION_WINDOW_TOGGLE_KEY_BINDINGS:     return "Open/close key bindings window";
    case ACTION_WINDOW_TOGGLE_SCENE_INSPECTOR:  return "Open/close scene inspector window";
    case ACTION_WINDOW_TOGGLE_PATH_VISUALIZER:  return "Open/close path visualizer window";
    case ACTION_WINDOW_TOGGLE_PIXEL_INSPECTOR:  return "Open/close pixel inspector window";
    case ACTION_WINDOW_TOGGLE_SNAPSHOT:         return "Open/close snapshot window";
    case ACTION_WINDOW_TOGGLE_STATUS:           return "Open/close status bar";
    case ACTION_SAVE_IMAGE:                     return "Save image";
    case ACTION_PICK_PATH_VISUALIZER_PIXEL:     return "Select pixel under mouse cursor for path visualization";
    case ACTION_CAM_TOGGLE_ACTIVE_TYPE:         return "Toggle active camera type (orbit/freecam)";
    case ACTION_TILE_PROGRESS_TOGGLE:           return "Turn tile progress visualization on/off";
    case ACTION_RENDER_OUTPUT_PREV:             return "Show previous render output";
    case ACTION_RENDER_OUTPUT_NEXT:             return "Show next render output";
    case ACTION_PRINT_KEY_BINDINGS:             return "Print key bindings to console";
    case ACTION_COPY_BUFFER_TO_CLIPBOARD:       return "Copy displayed image to clipboard";
    default:                                    return "";
    }
}

std::string getKeyName(const int& glfwKey)
{
    const char* keyName = glfwGetKeyName(glfwKey, 0);
    if (keyName != nullptr) {
        std::string result(keyName);
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
    
    // Handle special keys that glfwGetKeyName doesn't cover
    switch (glfwKey) {
        case GLFW_MOUSE_BUTTON_LEFT:    return "LMB";
        case GLFW_MOUSE_BUTTON_RIGHT:   return "RMB";
        case GLFW_MOUSE_BUTTON_MIDDLE:  return "MMB";
        case GLFW_KEY_SPACE:            return "SPACE";
        case GLFW_KEY_APOSTROPHE:       return "'";
        case GLFW_KEY_COMMA:            return ",";
        case GLFW_KEY_MINUS:            return "-";
        case GLFW_KEY_PERIOD:           return ".";
        case GLFW_KEY_SLASH:            return "/";
        case GLFW_KEY_SEMICOLON:        return ";";
        case GLFW_KEY_EQUAL:            return "=";
        case GLFW_KEY_LEFT_BRACKET:     return "[";
        case GLFW_KEY_BACKSLASH:        return "\\";
        case GLFW_KEY_RIGHT_BRACKET:    return "]";
        case GLFW_KEY_GRAVE_ACCENT:     return "`";
        case GLFW_KEY_ESCAPE:           return "ESC";
        case GLFW_KEY_ENTER:            return "ENTER";
        case GLFW_KEY_TAB:              return "TAB";
        case GLFW_KEY_BACKSPACE:        return "BACKSPACE";
        case GLFW_KEY_INSERT:           return "INSERT";
        case GLFW_KEY_DELETE:           return "DELETE";
        case GLFW_KEY_RIGHT:            return "RIGHT";
        case GLFW_KEY_LEFT:             return "LEFT";
        case GLFW_KEY_DOWN:             return "DOWN";
        case GLFW_KEY_UP:               return "UP";
        case GLFW_KEY_PAGE_UP:          return "PAGE_UP";
        case GLFW_KEY_PAGE_DOWN:        return "PAGE_DOWN";
        case GLFW_KEY_HOME:             return "HOME";
        case GLFW_KEY_END:              return "END";
        case GLFW_KEY_CAPS_LOCK:        return "CAPS_LOCK";
        case GLFW_KEY_SCROLL_LOCK:      return "SCROLL_LOCK";
        case GLFW_KEY_NUM_LOCK:         return "NUM_LOCK";
        case GLFW_KEY_PRINT_SCREEN:     return "PRINT_SCREEN";
        case GLFW_KEY_PAUSE:            return "PAUSE";
        case GLFW_KEY_F1:               return "F1";
        case GLFW_KEY_F2:               return "F2";
        case GLFW_KEY_F3:               return "F3";
        case GLFW_KEY_F4:               return "F4";
        case GLFW_KEY_F5:               return "F5";
        case GLFW_KEY_F6:               return "F6";
        case GLFW_KEY_F7:               return "F7";
        case GLFW_KEY_F8:               return "F8";
        case GLFW_KEY_F9:               return "F9";
        case GLFW_KEY_F10:              return "F10";
        case GLFW_KEY_F11:              return "F11";
        case GLFW_KEY_F12:              return "F12";
        case GLFW_KEY_LEFT_SHIFT:       return "LEFT_SHIFT";
        case GLFW_KEY_LEFT_CONTROL:     return "LEFT_CTRL";
        case GLFW_KEY_LEFT_ALT:         return "LEFT_ALT";
        case GLFW_KEY_LEFT_SUPER:       return "LEFT_SUPER";
        case GLFW_KEY_RIGHT_SHIFT:      return "RIGHT_SHIFT";
        case GLFW_KEY_RIGHT_CONTROL:    return "RIGHT_CTRL";
        case GLFW_KEY_RIGHT_ALT:        return "RIGHT_ALT";
        case GLFW_KEY_RIGHT_SUPER:      return "RIGHT_SUPER";
        default:                        return "UNKNOWN";
    }
}

std::string getModifierName(const int& mod)
{
    if (mod == 0) {
        return "";
    }

    // First, check to see if any secondary keys are acting as modifiers
    std::string keyName = getKeyName(mod);
    /// NOTE: currently, mouse buttons cannot act as modifiers
    /// since their mapped values overlap with actual key mods.
    if (keyName != "UNKNOWN" && keyName != "LMB" && keyName != "RMB" && keyName != "MMB") {
        return keyName + "+";
    }

    // Then, check for actual key modifiers
    if (mod & GLFW_MOD_CONTROL)     { return "CTRL+"; }
    else if (mod & GLFW_MOD_SHIFT)  { return "SHIFT+"; }
    else if (mod & GLFW_MOD_ALT)    { return "ALT+"; }
    return "";
}

} // end namespace moonray_gui_v2