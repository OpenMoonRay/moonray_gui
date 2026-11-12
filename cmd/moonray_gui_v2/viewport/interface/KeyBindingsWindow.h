
// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>

#include "Component.h"
#include "../Keyboard.h"
#include "../MouseTimer.h"

namespace moonray_gui_v2 {

class Keyboard;
class Viewport;

// ------------------------------------------------------------------------------------------------------------------ //
//
// Manages the process of capturing user key input for binding actions.
//
// Usage Flow:
// 1. Call startCapture(action) to begin listening for keys
// 2. Call update() each frame while isCapturing() returns true
// 3. After capture completes, check hasKeyBinding() and retrieve via getPendingKey/Mod/Action
// 4. Call reset() to clear state and prepare for next capture
//
class KeyCapture {
public:
    enum class State {
        IDLE,       // Not capturing
        CAPTURING,  // Actively listening for key presses
        COMPLETED,  // Capture finished successfully
        CANCELLED   // Capture cancelled (ESC pressed)
    };

    KeyCapture() = default;
    ~KeyCapture() = default;

    // Starts listening for any user input
    void startCapture(Viewport* viewport, const Action action);

    // Updates the capture state - MUST be called every frame when capturing
    // Handles key detection, timer management, and ESC key cancellation
    void update(Viewport* viewport);

    // Whether we are currently in capturing mode
    bool isCapturing() const { return mState == State::CAPTURING; }

    // Whether we are in any active state (capturing or completed)
    bool isActive() const { return mState != State::IDLE; }

    // Whether capture was completed successfully
    bool isCompleted() const { return mState == State::COMPLETED; }

    // Whether capture was cancelled
    bool isCancelled() const { return mState == State::CANCELLED; }

    // Resets to default state
    void reset();

    // Gets the pending key. This will be either:
    // 1. The first key pressed, if only one key is pressed, OR
    // 2. The second key pressed. If no modifier is pressed, and more 
    //    than one key is pressed, the first key acts as a special modifier (see finishCapture)
    int getPendingKey() const { return mPendingKey; }

    // Gets the pending modifier. This will be either 
    // the pressed modifier, or the first key pressed, if more
    // than one key is selected. 
    int getPendingMod() const { return mPendingMod; }

    // Gets the pending action (i.e. the action for which we 
    // are recording a potential new key binding)
    Action getPendingAction() const { return mPendingAction; }

    // Whether we've captured a valid key binding
    bool hasKeyBinding() const { return mPendingKey != -1 && mPendingAction != ACTION_NONE; }

    // Whether we've captured a modifier
    bool hasModifier() const { return mPendingMod != 0; }

    // Gets the current capture state
    State getState() const { return mState; }

    // Gets the capture duration (in milliseconds)
    int getCaptureDurationMs() const { return mCaptureDurationMs; }

    // Sets the capture duration (in milliseconds)
    void setCaptureDurationMs(int durationMs) { mCaptureDurationMs = durationMs; }

    // Sets the pending action
    void setPendingAction(Action action) { mPendingAction = action; }

    // Sets the pending key
    void setPendingKey(int key) { mPendingKey = key; }

    // Sets the pending modifier
    void setPendingMod(int mod) { mPendingMod = mod; }

    // Captures a key press
    void captureKey(int key) {
        if (std::find(mCapturedKeys.begin(), mCapturedKeys.end(), key) == mCapturedKeys.end()) {
            mCapturedKeys.push_back(key);
        }
    }

private:
    // Processes captured keys and determines the final key/mod pair
    void finishCapture(Viewport* viewport);

    // Checks for new key presses and adds them to mCapturedKeys
    void detectKeyPresses();

    // Checks for modifier key states and updates mPendingMod
    void detectModifiers();

    // Checks for mouse button presses
    void detectMouseButtons();

    State mState {State::IDLE};         // Current capture state

    int mPendingKey {-1};               // Candidate key (can't be 0 because LeftMouseButton is mapped to 0)
    int mPendingMod {0};                // Candidate modifier (can be 0 because LeftMouseButton is not a valid modifier)
    Action mPendingAction {ACTION_NONE};// Action to assign new binding to

    std::vector<int> mCapturedKeys;     // All of the unique keys captured within the capture duration, sorted by
                                        // the order in which they were pressed
    MouseTimer mTimer;                  // Key capture timer 
    int mCaptureDurationMs {300};       // Amount of time to wait (in ms) before processing the captured keys
};
// ------------------------------------------------------------------------------------------------------------------ //


using KeyBindingStringMap = std::unordered_map<Action, std::pair<bool, std::string>>;

class KeyBindingsWindow : public Component {
    public:
        KeyBindingsWindow() : Component(/*isOpen*/ false) {}

        void draw(Viewport* viewport, const ImVec2& currentPixel, const ImVec2& dockOffset) override;

        // Get the width and height of the window
        // -1 indicates dynamic height based on viewport size
        int getWidth() const override { return mWidth; }
        int getHeight() const override { return -1; }

        // Whether we are currently capturing a key binding
        bool isCapturing() const { return mKeyCapture.isCapturing(); }

    private:
        enum KeyboardMode {
            KEYBOARD_MODE_DEFAULT,
            KEYBOARD_MODE_MAYA,
            KEYBOARD_MODE_HOUDINI
        };

        // Define the window settings (size, position, style, etc)
        void configureWindow() const;

        // Initialize the keybinding display
        void initialize(const Keyboard* keyboard);

        // Set up the initial key binding strings
        void initializeKeyBindingStrings(const Keyboard* keyboard);

        // Update key binding string for the given action
        void updateKeyBindingString(const Action action, const KeyModPair& keyMod, const bool isDefault = true);

        // Format the given key/mod pair and return a human-readable string
        std::string formatKeyBinding(int key, int mod) const;

        // Process ImGui key inputs when capturing
        void processKeyCapture(Viewport* viewport);

        // Apply the pending key binding
        void applyKeyBinding(Viewport* viewport, bool forceOverride);

        // Clears the pending key binding state
        void clearPendingBinding();

        // Resets the key binding for the given action to its default value
        void resetBindingToDefault(Keyboard* keyboard, const Action action, bool forceOverride);

        // Draw the header text for the key bindings window
        void drawHeaderText() const;

        // Draw the control buttons (Reset All, Save Settings, Load Settings)
        void drawControlButtons(Keyboard* keyboard);

        // Draw the dropdown for selecting the keyboard mode
        void drawKeyboardModeDropdown(Keyboard* keyboard);

        // Draw button that displays the current key and 
        // allows for the user to change the key binding
        void drawKeyBindingButton(Viewport* viewport, const Action action);

        // Draw the table displaying actions and their key bindings
        void drawKeyBindingTable(std::string header, const std::set<Action>& actions, Viewport* viewport);

        // Draw the modal popup for key binding conflicts
        void drawConflictModal(Viewport* viewport);

        bool mInitialized {false};      // Whether the window has been initialized
        int mWidth {450};               // Width of the window
        int mHeightPadding {50};        // The window is the same height as the viewport, minus this total padding
                                        // (so mHeightPadding / 2 on the top and bottom)
        int mWindowPadding {15};        // Amt of padding inside the window

        int mKeyColumnWidth {100};      // Width of the column containing the key binding buttons
        int mKeyHeightPadding {6};      // Height padding for key binding buttons

        ImVec4 mKeyButtonColor {ImVec4(0.3f, 0.8f, 0.3f, 0.3f)};        // Color of the key binding button
        ImVec4 mKeyButtonHoverColor {ImVec4(0.3f, 0.8f, 0.3f, 0.5f)};   // Color of key binding button during hover
        ImVec4 mKeyButtonActiveColor {ImVec4(0.3f, 0.8f, 0.3f, 0.7f)};  // Color of key binding button when clicked
        ImVec4 mKeyOverrideTextColor {ImVec4(0.9f, 0.6f, 0.2f, 1.0f)};  // Color of text for overridden keys

        KeyBindingStringMap mKeyBindingStrings;             // Map of actions, along with strings representing 
                                                            // their key bindings
        bool mShowOverrideConfirm {false};                  // Modal popup state for conflicts
        Action mConflictingAction {ACTION_NONE};            // Conflicting action for the pending key binding override
        bool mPendingReset {false};                         // Whether a pending reset to default binding is in progress

        KeyCapture mKeyCapture;                             // Object responsible for capturing user input
        KeyboardMode mKeyboardMode {KEYBOARD_MODE_DEFAULT}; // Current keyboard mode
};

} // end namespace moonray_gui_v2
