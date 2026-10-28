// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "Keyboard.h"

#include <fstream>

namespace moonray_gui_v2 {

#define MOD_NONE 0

// ----------------------------------------- KeyboardBindings Class ------------------------------------------------- //
KeyboardBindings::KeyboardBindings()
{
    // ------------- Key mappings ------------------
    mKeyBindings.insert(KeyModPair(GLFW_KEY_1, MOD_NONE), ACTION_CHANNEL_TOGGLE_RED);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_2, MOD_NONE), ACTION_CHANNEL_TOGGLE_GREEN);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_3, MOD_NONE), ACTION_CHANNEL_TOGGLE_BLUE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_4, MOD_NONE), ACTION_CHANNEL_TOGGLE_ALPHA);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_5, MOD_NONE), ACTION_CHANNEL_TOGGLE_LUMINANCE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_6, MOD_NONE), ACTION_CHANNEL_TOGGLE_RGB_NORMALIZED);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_7, MOD_NONE), ACTION_CHANNEL_TOGGLE_NUM_SAMPLES);

    mKeyBindings.insert(KeyModPair(GLFW_KEY_A, MOD_NONE), ACTION_CAM_LEFT);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_B, MOD_NONE), ACTION_DENOISE_SELECT_BUFFERS);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_C, MOD_NONE), ACTION_CAM_DOWN);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_D, MOD_NONE), ACTION_CAM_RIGHT);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_E, MOD_NONE), ACTION_CAM_SPEED_UP);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_F, MOD_NONE), ACTION_CAM_RECENTER);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_G, MOD_NONE), ACTION_WINDOW_TOGGLE_KEY_BINDINGS);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_H, MOD_NONE), ACTION_PRINT_KEY_BINDINGS);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_I, MOD_NONE), ACTION_WINDOW_TOGGLE_SCENE_INSPECTOR);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_J, MOD_NONE), ACTION_COPY_BUFFER_TO_CLIPBOARD);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_K, MOD_NONE), ACTION_SNAPSHOT_TAKE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_L, MOD_NONE), ACTION_FAST_PROGRESSIVE_TOGGLE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_M, MOD_NONE), ACTION_CAM_PRINT_MATRICES);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_N, MOD_NONE), ACTION_DENOISE_TOGGLE_ON_OFF);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_O, MOD_NONE), ACTION_CAM_TOGGLE_ACTIVE_TYPE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_P, MOD_NONE), ACTION_WINDOW_TOGGLE_PIXEL_INSPECTOR);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_Q, MOD_NONE), ACTION_CAM_SLOW_DOWN);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_R, MOD_NONE), ACTION_CAM_RESET);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_S, MOD_NONE), ACTION_CAM_BACKWARD);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_T, MOD_NONE), ACTION_TILE_PROGRESS_TOGGLE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_U, MOD_NONE), ACTION_CAM_SET_UP_VECTOR);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_V, MOD_NONE), ACTION_WINDOW_TOGGLE_PATH_VISUALIZER);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_W, MOD_NONE), ACTION_CAM_FORWARD);

    mKeyBindings.insert(KeyModPair(GLFW_KEY_PERIOD, MOD_NONE), ACTION_RENDER_OUTPUT_NEXT);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_COMMA, MOD_NONE), ACTION_RENDER_OUTPUT_PREV);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_GRAVE_ACCENT, MOD_NONE), ACTION_CHANNEL_TOGGLE_RGB);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_LEFT, MOD_NONE), ACTION_SNAPSHOT_PREV);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_RIGHT, MOD_NONE), ACTION_SNAPSHOT_NEXT);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_UP, MOD_NONE), ACTION_EXPOSURE_INCREASE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_DOWN, MOD_NONE), ACTION_EXPOSURE_DECREASE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_SPACE, MOD_NONE), ACTION_CAM_UP);

    mKeyBindings.insert(KeyModPair(GLFW_KEY_A, GLFW_MOD_SHIFT), ACTION_WINDOW_TOGGLE_AXIS_DISPLAY);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_F, GLFW_MOD_SHIFT), ACTION_CAM_FRAME_SCENE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_N, GLFW_MOD_SHIFT), ACTION_DENOISE_TOGGLE_MODE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_V, GLFW_MOD_SHIFT), ACTION_PATH_VISUALIZER_ON_OFF);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_X, GLFW_MOD_SHIFT), ACTION_EXPOSURE_RESET);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_Y, GLFW_MOD_SHIFT), ACTION_GAMMA_RESET);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_LEFT, GLFW_MOD_SHIFT), ACTION_PATH_VISUALIZER_PREV_NODE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_RIGHT, GLFW_MOD_SHIFT), ACTION_PATH_VISUALIZER_NEXT_NODE);

    mKeyBindings.insert(KeyModPair(GLFW_KEY_UP, GLFW_MOD_ALT), ACTION_FAST_PROGRESSIVE_NEXT_MODE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_DOWN, GLFW_MOD_ALT), ACTION_FAST_PROGRESSIVE_PREV_MODE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_K, GLFW_MOD_ALT), ACTION_WINDOW_TOGGLE_SNAPSHOT);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_S, GLFW_MOD_ALT), ACTION_WINDOW_TOGGLE_STATUS);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_X, GLFW_MOD_ALT), ACTION_WINDOW_TOGGLE_EXPOSURE);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_Y, GLFW_MOD_ALT), ACTION_WINDOW_TOGGLE_GAMMA);

    mKeyBindings.insert(KeyModPair(GLFW_KEY_S, GLFW_MOD_CONTROL), ACTION_SAVE_IMAGE);

    // ---------------- Mouse mappings ------------------
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_LEFT, MOD_NONE), ACTION_PICK_PATH_VISUALIZER_PIXEL);
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_MIDDLE, MOD_NONE), ACTION_IMAGE2D_PAN);

    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_LEFT, GLFW_MOD_ALT), ACTION_CAM_ROTATE);
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOD_ALT), ACTION_CAM_TRACK);
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOD_ALT), ACTION_CAM_DOLLY);
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_LEFT, GLFW_MOD_CONTROL), ACTION_CAM_ROLL);

    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_LEFT, GLFW_KEY_X), ACTION_EXPOSURE_ADJUST);
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_LEFT, GLFW_KEY_Y), ACTION_GAMMA_ADJUST);
    // Since GLFW does not natively support keys like 'X' and 'Y' as modifiers,
    // we need to keep track of these special modifier keys separately.
    mSpecialMods.insert(GLFW_KEY_X);
    mSpecialMods.insert(GLFW_KEY_Y);
    
    // Copy the default bindings to mKeyDefaults for future reference
    mKeyDefaults = mKeyBindings;
}

KeyModPair
KeyboardBindings::getKeyModPair(const Action action) const
{
    if (mKeyBindings.hasValue(action)) {
        return mKeyBindings.getKey(action);
    }
    return INVALID_KEYMOD;
}

Action
KeyboardBindings::getActionFromInput(GLFWwindow* window, const KeyModPair& keyModPair)
{
    // First, check for any currently pressed glfw keys acting as modifiers
    if (window) {
        for (int specialMod : mSpecialMods) {
            if (glfwGetKey(window, specialMod) == GLFW_PRESS) {
                if (mKeyBindings.hasKey(KeyModPair(keyModPair.first, specialMod))) {
                    return mKeyBindings.getValue(KeyModPair(keyModPair.first, specialMod));
                }
            }
        }
    }
    // Otherwise, if the key/mod combination exists in the map, return the action
    return getAction(keyModPair);
}

Action
KeyboardBindings::getAction(const KeyModPair& keyModPair) const
{
    if (mKeyBindings.hasKey(keyModPair)) {
        return mKeyBindings.getValue(keyModPair);
    }
    return ACTION_NONE;
}

KeyModPair
KeyboardBindings::getDefaultKeyModPair(const Action action) const
{
    if (mKeyDefaults.hasValue(action)) {
        return mKeyDefaults.getKey(action);
    }
    return INVALID_KEYMOD;
}

void
KeyboardBindings::removeBinding(const Action action)
{
    if (mKeyBindings.hasValue(action)) {
        mKeyBindings.removeByValue(action);
    }
}

void
KeyboardBindings::addCustomBinding(const KeyModPair& keyModPair, const Action action)
{
    // In order to add a new custom binding, we need to remove the key/mod pair
    // currently bound to the given action, and save the new binding.

    // 1. Delete the current binding
    removeBinding(action);

    // 2. Add the custom binding
    mKeyBindings.insert(keyModPair, action);

    // 3. If the modifier is non-standard, add it to the special mods list
    if (keyModPair.second != 0 && !isStandardModifier(keyModPair.second)) {
        mSpecialMods.insert(keyModPair.second);
    }
}

void
KeyboardBindings::resetBindingToDefault(const Action action)
{
    // In order to restore the default binding, we need to remove the custom key binding for the
    // given action, and reassign that action to the retrieved default key/mod pair. 

    if (mKeyDefaults.hasValue(action)) {
        // Delete the custom binding
        removeBinding(action);

        // Restore the default binding
        KeyModPair defaultKeyModPair = mKeyDefaults.getKey(action);
        mKeyBindings.insert(defaultKeyModPair, action);
    }
}

KeyBindingMap
KeyboardBindings::getCustomBindings() const
{
    KeyBindingMap customBindings;

    // If the action has a different entry in mKeyBindings than in mKeyDefaults,
    // it means the action has a custom binding, so we add it to the customBindings map.
    mKeyBindings.forEach([this, &customBindings](const KeyModPair& keyModPair, const Action action) {
        KeyModPair defaultKeyModPair = getDefaultKeyModPair(action);
        if (keyModPair != defaultKeyModPair) {
            customBindings.insert(keyModPair, action);
        }
    });
    return customBindings;
}

void
KeyboardBindings::printCustomBindings() const
{
    std::cout << "Custom Bindings:" << std::endl;
    std::cout << "----------------" << std::endl;

    auto customBindings = getCustomBindings();
    customBindings.forEach([](const KeyModPair& keyModPair, const Action action) {
        const int key = keyModPair.first;
        const int mod = keyModPair.second;

        std::string actionStr = getActionName(action);
        std::string keyStr = getKeyName(key);
        std::string modStr = getModifierName(mod);

        std::cout << actionStr << ": " << modStr << keyStr << std::endl;
    });
}

void 
KeyboardBindings::printKeyBindings() const
{
    std::cout << "\nKey Bindings:" << std::endl;
    std::cout <<   "-------------" << std::endl;
    
    std::vector<std::pair<KeyModPair, Action>> entries;
    mKeyBindings.forEach([&entries](const KeyModPair& keyModPair, const Action action) {
        entries.push_back({keyModPair, action});
    });
    std::sort(entries.begin(), entries.end(), [](const std::pair<KeyModPair, Action>& a, 
        const std::pair<KeyModPair, Action>& b) {
            return a.second < b.second;
        });
        
        for (const std::pair<KeyModPair, Action>& entry : entries) {
            KeyModPair pair = entry.first;
            Action action = entry.second;
            
            const int glfwKey = pair.first;
            const int glfwMod = pair.second;
            
            std::string actionStr = getActionName(action);
            std::string keyStr = getKeyName(glfwKey);
            std::string modStr = getModifierName(glfwMod);
            
            std::cout << actionStr << ": " << modStr << keyStr << std::endl;
        }
    std::cout << " ----------------------------\n";
}

// ---------------------------------------- MayaKeyboardBindings Class ---------------------------------------------- //
MayaKeyboardBindings::MayaKeyboardBindings()
{
    // Override any key mappings for Maya keyboard mode
    mKeyBindings.insert(KeyModPair(GLFW_KEY_HOME, GLFW_MOD_ALT), ACTION_CAM_RESET);
    // Add more key mappings here as needed

    // Image 2D Nav mouse actions
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_MIDDLE, GLFW_KEY_BACKSLASH), ACTION_IMAGE2D_PAN);
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_RIGHT, GLFW_KEY_BACKSLASH), ACTION_IMAGE2D_ZOOM);
    // We need to track BACKSLASH as a special modifier
    mSpecialMods.insert(GLFW_KEY_BACKSLASH);
    // Add more mouse mappings here as needed

    // Copy the default bindings to mKeyDefaults for future reference
    mKeyDefaults = mKeyBindings;
}

// -------------------------------------- HoudiniKeyboardBindings Class --------------------------------------------- //
HoudiniKeyboardBindings::HoudiniKeyboardBindings()
{
    // Override any key mappings for Houdini keyboard mode
    mKeyBindings.insert(KeyModPair(GLFW_KEY_H, GLFW_KEY_SPACE), ACTION_CAM_RESET);
    mKeyBindings.insert(KeyModPair(GLFW_KEY_Z, GLFW_KEY_SPACE), ACTION_CAM_RECENTER);
    // Add more key mappings here as needed

    // Override mouse mappings for Houdini keyboard mode
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_LEFT, GLFW_KEY_SPACE), ACTION_CAM_ROTATE);
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_MIDDLE, GLFW_KEY_SPACE), ACTION_CAM_TRACK);
    mKeyBindings.insert(KeyModPair(GLFW_MOUSE_BUTTON_RIGHT, GLFW_KEY_SPACE), ACTION_CAM_DOLLY);
    // We need to track SPACE as a special modifier
    mSpecialMods.insert(GLFW_KEY_SPACE);
    // Add more mouse mappings here as needed

    // Copy the default bindings to mKeyDefaults for future reference
    mKeyDefaults = mKeyBindings;
}

// ------------------------------------------ Keyboard Class -------------------------------------------------------- //
Keyboard::Keyboard() : 
    mBindings(std::make_unique<KeyboardBindings>())
{}

// -------------------------- Key Binding Lookup ---------------------------- //

KeyModPair 
Keyboard::getKeyModPair(const Action action) const
{
    return mBindings->getKeyModPair(action);
}

Action 
Keyboard::getActionFromInput(GLFWwindow* glfwWindow, const int glfwKey, const int glfwMod)
{
    return mBindings->getActionFromInput(glfwWindow, KeyModPair(glfwKey, glfwMod));
}

Action 
Keyboard::getScrollAction() const
{
    return mBindings->getScrollAction();
}

bool 
Keyboard::hasBindingConflict(const int glfwKey, const int glfwMod, const Action targetAction) const
{
    // Find the action (if any) currently bound to the key/mod combination
    // If it doesn't match the target action, we have a conflict
    Action boundAction = mBindings->getAction(KeyModPair(glfwKey, glfwMod));
    return boundAction != ACTION_NONE && boundAction != targetAction;
}

Action 
Keyboard::getBindingConflict(const int glfwKey, const int glfwMod, const Action targetAction) const
{
    // Find the action (if any) currently bound to the key/mod combination
    // If it doesn't match the target action, we have a conflict
    Action boundAction = mBindings->getAction(KeyModPair(glfwKey, glfwMod));
    if (boundAction != targetAction) {
        return boundAction;
    }
    return ACTION_NONE;
}

bool 
Keyboard::addCustomBinding(int glfwKey, int glfwMod, Action action, Action& conflictingAction, bool forceOverride)
{
    // Check for conflicts unless we're forcing the override
    conflictingAction = getBindingConflict(glfwKey, glfwMod, action);
    if (!forceOverride && conflictingAction != ACTION_NONE) {
        return false;
    }

    // Apply the key binding.
    KeyModPair keyModPair(glfwKey, glfwMod);
    mBindings->addCustomBinding(keyModPair, action);
    return true;
}

bool
Keyboard::resetBindingToDefault(const Action action, Action& conflictingAction, 
                                KeyModPair& defaultKeyModPair, bool forceOverride)
{
    defaultKeyModPair = mBindings->getDefaultKeyModPair(action);
    conflictingAction = getBindingConflict(defaultKeyModPair.first, defaultKeyModPair.second, action);

    // If the default will conflict with a custom binding, 
    // return false so we can notify the user on the frontend (unless forceOverride is true)
    if (!forceOverride && conflictingAction != ACTION_NONE) {
        return false;
    }
    mBindings->resetBindingToDefault(action);
    return true;
}

void
Keyboard::resetToDefaults()
{
    switch (mKeyboardMode) {
    case KeyboardMode::Houdini: mBindings = std::make_unique<HoudiniKeyboardBindings>(); break;
    case KeyboardMode::Maya:    mBindings = std::make_unique<MayaKeyboardBindings>();    break;
    default:                    mBindings = std::make_unique<KeyboardBindings>();        break;
    }
}

void
Keyboard::changeKeyboardMode(const KeyboardMode newMode)
{
    // 1: Extract all custom bindings from the current mode. This way, we can
    // preserve them when switching to the new mode.
    KeyBindingMap customBindings = mBindings->getCustomBindings();

    // 2: Switch to the new keyboard mode, changing all of the default bindings.
    mKeyboardMode = newMode;
    resetToDefaults();

    // 3: Re-apply the custom bindings to the new mode
    customBindings.forEach([this](const KeyModPair& keyModPair, const Action action) {
        mBindings->addCustomBinding(keyModPair, action);
    });
}

bool 
Keyboard::saveKeyBindings(const std::string filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << "# MoonRay GUI Key Bindings\n";
    file << "# Format: key,modifier,action\n";

    mBindings->getCustomBindings().forEach([&file](const KeyModPair& keyModPair, const Action action) {
        file << keyModPair.first << "," << keyModPair.second << "," << action << "\n";
    });

    return true;
}

bool 
Keyboard::loadKeyBindings(const std::string filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    resetToDefaults();

    auto parseInt = [](const std::string& str) -> int {
        try {
            return std::stoi(str);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing key binding: " << e.what() << std::endl;
            return -1;
        }
    };

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse: key,modifier,action
        std::istringstream iss(line);
        std::string token;
        
        if (!std::getline(iss, token, ',')) continue;
        int key = parseInt(token);
        if (key == -1) continue;

        if (!std::getline(iss, token, ',')) continue;
        int mod = parseInt(token);
        if (mod == -1) continue;

        if (!std::getline(iss, token, ',')) continue;
        int action = parseInt(token);
        if (action == -1) continue;

        mBindings->addCustomBinding(KeyModPair(key, mod), static_cast<Action>(action));
    }
    return true;
}

} // end namespace moonray_gui_v2
