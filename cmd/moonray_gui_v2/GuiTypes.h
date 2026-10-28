// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "imgui.h"

#include <scene_rdl2/common/fb_util/FbTypes.h>

#include <GLFW/glfw3.h>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace scene_rdl2;

namespace moonray_gui_v2 {

enum CameraType
{
    ORBIT_CAM,
    FREE_CAM,
    NUM_CAMERA_TYPES,
};

enum DebugMode
{
    RGB,
    RED,
    GREEN,
    BLUE,
    ALPHA,
    LUMINANCE,
    SATURATION,
    RGB_NORMALIZED,
    NUM_SAMPLES,

    NUM_DEBUG_MODES,
};

enum InspectorMode
{
    INSPECT_NONE,
    INSPECT_LIGHT_CONTRIBUTIONS,
    INSPECT_GEOMETRY,
    INSPECT_GEOMETRY_PART,
    INSPECT_MATERIAL,
    NUM_INSPECTOR_MODES
};

// Which additional buffers do we want to use for denoising.
enum DenoisingBufferMode
{
    DN_BUFFERS_BEAUTY,
    DN_BUFFERS_BEAUTY_ALBEDO,
    DN_BUFFERS_BEAUTY_ALBEDO_NORMALS,
    NUM_DENOISING_BUFFER_MODES,
};

// Define all bindable actions that can be triggered by keyboard/mouse input
enum Action
{
    ACTION_NONE = 0,

    // Camera movement actions
    ACTION_CAM_TOGGLE_ACTIVE_TYPE,
    ACTION_CAM_FORWARD,
    ACTION_CAM_BACKWARD,
    ACTION_CAM_LEFT,
    ACTION_CAM_RIGHT,
    ACTION_CAM_UP,
    ACTION_CAM_DOWN,
    ACTION_CAM_SLOW_DOWN,
    ACTION_CAM_SPEED_UP,
    ACTION_CAM_RESET,
    ACTION_CAM_RECENTER,
    ACTION_CAM_FRAME_SCENE,
    ACTION_CAM_PRINT_MATRICES,
    ACTION_CAM_SET_UP_VECTOR,
    ACTION_CAM_ROTATE,
    ACTION_CAM_DOLLY,
    ACTION_CAM_TRACK,
    ACTION_CAM_ROLL,

    // Image 2D Nav
    ACTION_IMAGE2D_PAN,
    ACTION_IMAGE2D_ZOOM,

    // Denoising actions
    ACTION_DENOISE_TOGGLE_ON_OFF,
    ACTION_DENOISE_TOGGLE_MODE,
    ACTION_DENOISE_SELECT_BUFFERS,    

    // Channel actions
    ACTION_CHANNEL_TOGGLE_RGB,
    ACTION_CHANNEL_TOGGLE_RED,
    ACTION_CHANNEL_TOGGLE_GREEN,
    ACTION_CHANNEL_TOGGLE_BLUE,
    ACTION_CHANNEL_TOGGLE_ALPHA,
    ACTION_CHANNEL_TOGGLE_LUMINANCE,
    ACTION_CHANNEL_TOGGLE_RGB_NORMALIZED,
    ACTION_CHANNEL_TOGGLE_NUM_SAMPLES,

    // Image adjustment actions
    ACTION_EXPOSURE_INCREASE,
    ACTION_EXPOSURE_DECREASE,
    ACTION_EXPOSURE_ADJUST,
    ACTION_EXPOSURE_RESET,
    ACTION_GAMMA_ADJUST,
    ACTION_GAMMA_RESET,

    // Fast progressive mode actions
    ACTION_FAST_PROGRESSIVE_TOGGLE,
    ACTION_FAST_PROGRESSIVE_NEXT_MODE,
    ACTION_FAST_PROGRESSIVE_PREV_MODE,

    // Window toggles
    ACTION_WINDOW_TOGGLE_EXPOSURE,
    ACTION_WINDOW_TOGGLE_GAMMA,
    ACTION_WINDOW_TOGGLE_KEY_BINDINGS,
    ACTION_WINDOW_TOGGLE_SCENE_INSPECTOR,
    ACTION_WINDOW_TOGGLE_PATH_VISUALIZER,
    ACTION_WINDOW_TOGGLE_PIXEL_INSPECTOR,
    ACTION_WINDOW_TOGGLE_SNAPSHOT,
    ACTION_WINDOW_TOGGLE_STATUS,
    ACTION_WINDOW_TOGGLE_AXIS_DISPLAY,

    // Output actions
    ACTION_RENDER_OUTPUT_PREV,
    ACTION_RENDER_OUTPUT_NEXT,
    ACTION_SAVE_IMAGE,
    ACTION_SNAPSHOT_TAKE,
    ACTION_SNAPSHOT_PREV,
    ACTION_SNAPSHOT_NEXT,

    // Path visualizer actions
    ACTION_PICK_PATH_VISUALIZER_PIXEL,
    ACTION_PATH_VISUALIZER_ON_OFF,
    ACTION_PATH_VISUALIZER_PREV_NODE,
    ACTION_PATH_VISUALIZER_NEXT_NODE,

    // Misc actions
    ACTION_TILE_PROGRESS_TOGGLE,
    ACTION_PRINT_KEY_BINDINGS,

    // Output actions cont.
    ACTION_COPY_BUFFER_TO_CLIPBOARD,

    // Add more actions here as needed
    ACTION_COUNT // Keep this as the last element to count the number of actions
};

// Map common ImGui keys back to GLFW keys
struct KeyMapping {
    ImGuiKey imGuiKey;
    int glfwKey;
};

constexpr KeyMapping KEY_MAPPINGS[] = {
    {ImGuiKey_Space, GLFW_KEY_SPACE},
    {ImGuiKey_Apostrophe, GLFW_KEY_APOSTROPHE},
    {ImGuiKey_Comma, GLFW_KEY_COMMA},
    {ImGuiKey_Minus, GLFW_KEY_MINUS},
    {ImGuiKey_Period, GLFW_KEY_PERIOD},
    {ImGuiKey_Slash, GLFW_KEY_SLASH},
    {ImGuiKey_0, GLFW_KEY_0}, {ImGuiKey_1, GLFW_KEY_1}, {ImGuiKey_2, GLFW_KEY_2},
    {ImGuiKey_3, GLFW_KEY_3}, {ImGuiKey_4, GLFW_KEY_4}, {ImGuiKey_5, GLFW_KEY_5},
    {ImGuiKey_6, GLFW_KEY_6}, {ImGuiKey_7, GLFW_KEY_7}, {ImGuiKey_8, GLFW_KEY_8},
    {ImGuiKey_9, GLFW_KEY_9},
    {ImGuiKey_Semicolon, GLFW_KEY_SEMICOLON},
    {ImGuiKey_Equal, GLFW_KEY_EQUAL},
    {ImGuiKey_A, GLFW_KEY_A}, {ImGuiKey_B, GLFW_KEY_B}, {ImGuiKey_C, GLFW_KEY_C},
    {ImGuiKey_D, GLFW_KEY_D}, {ImGuiKey_E, GLFW_KEY_E}, {ImGuiKey_F, GLFW_KEY_F},
    {ImGuiKey_G, GLFW_KEY_G}, {ImGuiKey_H, GLFW_KEY_H}, {ImGuiKey_I, GLFW_KEY_I},
    {ImGuiKey_J, GLFW_KEY_J}, {ImGuiKey_K, GLFW_KEY_K}, {ImGuiKey_L, GLFW_KEY_L},
    {ImGuiKey_M, GLFW_KEY_M}, {ImGuiKey_N, GLFW_KEY_N}, {ImGuiKey_O, GLFW_KEY_O},
    {ImGuiKey_P, GLFW_KEY_P}, {ImGuiKey_Q, GLFW_KEY_Q}, {ImGuiKey_R, GLFW_KEY_R},
    {ImGuiKey_S, GLFW_KEY_S}, {ImGuiKey_T, GLFW_KEY_T}, {ImGuiKey_U, GLFW_KEY_U},
    {ImGuiKey_V, GLFW_KEY_V}, {ImGuiKey_W, GLFW_KEY_W}, {ImGuiKey_X, GLFW_KEY_X},
    {ImGuiKey_Y, GLFW_KEY_Y}, {ImGuiKey_Z, GLFW_KEY_Z},
    {ImGuiKey_LeftBracket, GLFW_KEY_LEFT_BRACKET},
    {ImGuiKey_Backslash, GLFW_KEY_BACKSLASH},
    {ImGuiKey_RightBracket, GLFW_KEY_RIGHT_BRACKET},
    {ImGuiKey_GraveAccent, GLFW_KEY_GRAVE_ACCENT},
    {ImGuiKey_Enter, GLFW_KEY_ENTER},
    {ImGuiKey_Tab, GLFW_KEY_TAB},
    {ImGuiKey_Backspace, GLFW_KEY_BACKSPACE},
    {ImGuiKey_Insert, GLFW_KEY_INSERT},
    {ImGuiKey_Delete, GLFW_KEY_DELETE},
    {ImGuiKey_RightArrow, GLFW_KEY_RIGHT},
    {ImGuiKey_LeftArrow, GLFW_KEY_LEFT},
    {ImGuiKey_DownArrow, GLFW_KEY_DOWN},
    {ImGuiKey_UpArrow, GLFW_KEY_UP},
    {ImGuiKey_PageUp, GLFW_KEY_PAGE_UP},
    {ImGuiKey_PageDown, GLFW_KEY_PAGE_DOWN},
    {ImGuiKey_Home, GLFW_KEY_HOME},
    {ImGuiKey_End, GLFW_KEY_END},
    {ImGuiKey_F1, GLFW_KEY_F1}, {ImGuiKey_F2, GLFW_KEY_F2}, {ImGuiKey_F3, GLFW_KEY_F3},
    {ImGuiKey_F4, GLFW_KEY_F4}, {ImGuiKey_F5, GLFW_KEY_F5}, {ImGuiKey_F6, GLFW_KEY_F6},
    {ImGuiKey_F7, GLFW_KEY_F7}, {ImGuiKey_F8, GLFW_KEY_F8}, {ImGuiKey_F9, GLFW_KEY_F9},
    {ImGuiKey_F10, GLFW_KEY_F10}, {ImGuiKey_F11, GLFW_KEY_F11}, {ImGuiKey_F12, GLFW_KEY_F12}
};

// Action category structure for organizing hotkeys in UI
struct ActionCategory {
    const char* name;
    const std::set<Action> actions;
};

// Centralized action categorization for use in KeyBindingsWindow and HelpWindow
// Maps category names to their associated actions for easy retrieval
inline const std::map<std::string, std::set<Action>> ACTION_CATEGORIES = {
    {"General Camera Controls", {
        ACTION_CAM_TOGGLE_ACTIVE_TYPE,
        ACTION_CAM_RESET,
        ACTION_CAM_RECENTER,
        ACTION_CAM_FRAME_SCENE,
        ACTION_CAM_PRINT_MATRICES,
        ACTION_CAM_SET_UP_VECTOR
    }},
    
    {"Camera Movement Keys", {
        ACTION_CAM_FORWARD,
        ACTION_CAM_BACKWARD,
        ACTION_CAM_LEFT,
        ACTION_CAM_RIGHT,
        ACTION_CAM_UP,
        ACTION_CAM_DOWN,
        ACTION_CAM_SLOW_DOWN,
        ACTION_CAM_SPEED_UP
    }},
    
    {"Camera Mouse Controls", {
        ACTION_CAM_ROTATE,
        ACTION_CAM_DOLLY,
        ACTION_CAM_TRACK,
        ACTION_CAM_ROLL
    }},
    
    {"Exposure Controls", {
        ACTION_WINDOW_TOGGLE_EXPOSURE,
        ACTION_EXPOSURE_INCREASE,
        ACTION_EXPOSURE_DECREASE,
        ACTION_EXPOSURE_ADJUST,
        ACTION_EXPOSURE_RESET
    }},
    
    {"Gamma Controls", {
        ACTION_WINDOW_TOGGLE_GAMMA,
        ACTION_GAMMA_ADJUST,
        ACTION_GAMMA_RESET
    }},
    
    {"Denoising", {
        ACTION_DENOISE_TOGGLE_ON_OFF,
        ACTION_DENOISE_TOGGLE_MODE,
        ACTION_DENOISE_SELECT_BUFFERS
    }},
    
    {"Fast Progressive", {
        ACTION_FAST_PROGRESSIVE_TOGGLE,
        ACTION_FAST_PROGRESSIVE_NEXT_MODE,
        ACTION_FAST_PROGRESSIVE_PREV_MODE
    }},
    
    {"Path Visualizer", {
        ACTION_WINDOW_TOGGLE_PATH_VISUALIZER,
        ACTION_PICK_PATH_VISUALIZER_PIXEL
    }},
    
    {"Output", {
        ACTION_SAVE_IMAGE,
        ACTION_RENDER_OUTPUT_PREV,
        ACTION_RENDER_OUTPUT_NEXT,
        ACTION_SNAPSHOT_TAKE,
        ACTION_SNAPSHOT_PREV,
        ACTION_SNAPSHOT_NEXT,
        ACTION_COPY_BUFFER_TO_CLIPBOARD
    }},
    
    {"Image Controls", {
        ACTION_IMAGE2D_PAN,
        ACTION_IMAGE2D_ZOOM,
        ACTION_TILE_PROGRESS_TOGGLE
    }},
    
    {"Window Toggles", {
        ACTION_WINDOW_TOGGLE_KEY_BINDINGS,
        ACTION_WINDOW_TOGGLE_PIXEL_INSPECTOR,
        ACTION_WINDOW_TOGGLE_SCENE_INSPECTOR,
        ACTION_WINDOW_TOGGLE_SNAPSHOT,
        ACTION_WINDOW_TOGGLE_STATUS
    }},
    
    {"Channels", {
        ACTION_CHANNEL_TOGGLE_RGB,
        ACTION_CHANNEL_TOGGLE_RED,
        ACTION_CHANNEL_TOGGLE_GREEN,
        ACTION_CHANNEL_TOGGLE_BLUE,
        ACTION_CHANNEL_TOGGLE_ALPHA,
        ACTION_CHANNEL_TOGGLE_LUMINANCE,
        ACTION_CHANNEL_TOGGLE_RGB_NORMALIZED,
        ACTION_CHANNEL_TOGGLE_NUM_SAMPLES
    }}
};

// Get the human-readable action name
std::string getActionName(const Action action);

// Get the human-readable action description
std::string getDescription(const Action action);

// Get the human-readable key name
std::string getKeyName(const int& glfwKey);

// Get the human-readable modifier name, in the format: NAME+
std::string getModifierName(const int& mod);

} // namespace moonray_gui_v2

