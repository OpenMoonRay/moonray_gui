// Copyright 2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "KeyBindingsWindow.h"

#include "../Viewport.h"

namespace moonray_gui_v2 {

static constexpr const char* KEY_BINDINGS_FILE = "keybindings.cfg";

// ----------------------------------- Place actions into categories for display ---------------------------------------
namespace {

const std::set<Action> CAMERA_ACTIONS = {
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
    ACTION_CAM_ROLL
};

const std::set<Action> VIEWPORT_ACTIONS = {
    ACTION_IMAGE2D_PAN,
    ACTION_IMAGE2D_ZOOM,
    ACTION_WINDOW_TOGGLE_AXIS_DISPLAY,
    ACTION_WINDOW_TOGGLE_EXPOSURE,
    ACTION_WINDOW_TOGGLE_GAMMA,
    ACTION_WINDOW_TOGGLE_KEY_BINDINGS,
    ACTION_WINDOW_TOGGLE_PATH_VISUALIZER,
    ACTION_WINDOW_TOGGLE_PIXEL_INSPECTOR,
    ACTION_WINDOW_TOGGLE_SCENE_INSPECTOR,
    ACTION_WINDOW_TOGGLE_SNAPSHOT
};

const std::set<Action> DENOISING_ACTIONS = {
    ACTION_DENOISE_TOGGLE_ON_OFF,
    ACTION_DENOISE_TOGGLE_MODE,
    ACTION_DENOISE_SELECT_BUFFERS
};

const std::set<Action> OUTPUT_ACTIONS = {
    ACTION_RENDER_OUTPUT_PREV,
    ACTION_RENDER_OUTPUT_NEXT,
    ACTION_SAVE_IMAGE,
    ACTION_SNAPSHOT_TAKE,
    ACTION_SNAPSHOT_PREV,
    ACTION_SNAPSHOT_NEXT
};

const std::set<Action> COLOR_MANAGEMENT_ACTIONS = {
    ACTION_EXPOSURE_INCREASE,
    ACTION_EXPOSURE_DECREASE,
    ACTION_EXPOSURE_ADJUST,
    ACTION_EXPOSURE_RESET,
    ACTION_GAMMA_ADJUST,
    ACTION_GAMMA_RESET
};

const std::set<Action> VISUALIZATION_ACTIONS = {
    ACTION_CHANNEL_TOGGLE_RGB,
    ACTION_CHANNEL_TOGGLE_RED,
    ACTION_CHANNEL_TOGGLE_GREEN,
    ACTION_CHANNEL_TOGGLE_BLUE,
    ACTION_CHANNEL_TOGGLE_ALPHA,
    ACTION_CHANNEL_TOGGLE_LUMINANCE,
    ACTION_CHANNEL_TOGGLE_RGB_NORMALIZED,
    ACTION_CHANNEL_TOGGLE_NUM_SAMPLES,
    ACTION_FAST_PROGRESSIVE_TOGGLE,
    ACTION_FAST_PROGRESSIVE_NEXT_MODE,
    ACTION_FAST_PROGRESSIVE_PREV_MODE,
    ACTION_TILE_PROGRESS_TOGGLE,
    ACTION_PATH_VISUALIZER_ON_OFF,
    ACTION_PICK_PATH_VISUALIZER_PIXEL,
    ACTION_PATH_VISUALIZER_PREV_NODE,
    ACTION_PATH_VISUALIZER_NEXT_NODE
};
} // end anonymous namespace

// -------------------------------------- Key Capture Class ------------------------------------------------------------

void
KeyCapture::startCapture(Viewport* viewport, const Action action)
{
    if (mState == State::IDLE) {
        // Initialize capture state
        mState = State::CAPTURING;
        mPendingAction = action;
        mPendingKey = -1;
        mPendingMod = 0;
        mCapturedKeys.clear();
        viewport->blockInput();
    }
}

void
KeyCapture::update(Viewport* viewport)
{
    if (mState != State::CAPTURING) {
        return;
    }

    // Check for ESC key to cancel capture
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        mState = State::CANCELLED;
        mCapturedKeys.clear();
        viewport->unblockInput();
        return;
    }

    // Detect any new key presses
    detectKeyPresses();
    // Detect mouse button clicks
    detectMouseButtons();
    // Detect modifier keys
    detectModifiers();

    // Start timer when first key is pressed
    if (!mTimer.wasStarted()) {
        if (!mCapturedKeys.empty()) {
            mTimer.start();
        }
    } else {
        // Check if capture duration has elapsed
        if (mTimer.getDurationMs() >= mCaptureDurationMs) {
            mTimer.end();
            finishCapture(viewport);
        }
    }
}

void
KeyCapture::detectKeyPresses()
{
    // Check each key mapping to see what key(s) the user pressed
    for (const auto& mapping : KEY_MAPPINGS) {
        if (ImGui::IsKeyPressed(mapping.imGuiKey)) {
            captureKey(mapping.glfwKey);
        }
    }
}

void
KeyCapture::detectMouseButtons()
{
    // Check if the mouse was clicked
    if (ImGui::IsMouseClicked(GLFW_MOUSE_BUTTON_LEFT)) {
        captureKey(GLFW_MOUSE_BUTTON_LEFT);
    } else if (ImGui::IsMouseClicked(GLFW_MOUSE_BUTTON_RIGHT)) {
        captureKey(GLFW_MOUSE_BUTTON_RIGHT);
    } else if (ImGui::IsMouseClicked(GLFW_MOUSE_BUTTON_MIDDLE)) {
        captureKey(GLFW_MOUSE_BUTTON_MIDDLE);
    }
}

void
KeyCapture::detectModifiers()
{
    // Get any modifiers
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl)     mPendingMod |= GLFW_MOD_CONTROL;
    if (io.KeyShift)    mPendingMod |= GLFW_MOD_SHIFT;
    if (io.KeyAlt)      mPendingMod |= GLFW_MOD_ALT;
    if (io.KeySuper)    mPendingMod |= GLFW_MOD_SUPER;
}

void
KeyCapture::finishCapture(Viewport* viewport)
{
    viewport->unblockInput();
    if (mCapturedKeys.empty()) {
        mState = State::CANCELLED;
        return;
    }

    // Process the captured keys
    mPendingKey = *(mCapturedKeys.begin());
    
    if (!hasModifier() && mCapturedKeys.size() > 1) {
        // Use secondary key as the primary, and first key acts as a special modifier
        mPendingKey = mCapturedKeys[1];
        mPendingMod = mCapturedKeys[0];
    }

    mState = State::COMPLETED;
}

void
KeyCapture::reset()
{
    mState = State::IDLE;
    mPendingKey = -1;
    mPendingMod = 0;
    mPendingAction = ACTION_NONE;
    mCapturedKeys.clear();
    
    // Reset timer if it was started
    if (mTimer.wasStarted()) {
        mTimer.end();
    }
}


// -------------------------------------- Key Bindings Window ----------------------------------------------------------

std::string
KeyBindingsWindow::formatKeyBinding(int key, int mod) const
{
    if (key == -1) return "";
    return getModifierName(mod) + getKeyName(key);
}

void
KeyBindingsWindow::updateKeyBindingString(const Action action, const KeyModPair& keyMod, const bool isDefault)
{
    std::string keyStr = formatKeyBinding(keyMod.first, keyMod.second);
    mKeyBindingStrings[action] = std::pair<bool, std::string>(isDefault, keyStr);
}

void
KeyBindingsWindow::initializeKeyBindingStrings(const Keyboard* keyboard)
{
    mKeyBindingStrings.clear();

    // Get all bindable actions and insert them into the unordered map,
    // with their key bindings formatted for display
    for (int i = ACTION_NONE + 1; i < ACTION_COUNT; ++i) {
        Action action = static_cast<Action>(i);
        KeyModPair keyModPair = keyboard->getKeyModPair(action);

        // Special case for scroll action
        if (keyboard->getScrollAction() == action) {
            mKeyBindingStrings[action] = std::pair<bool, std::string>(/*isDefault*/ true, "SCROLL");
            continue;
        }
        bool isDefault = keyboard->isDefaultBinding(action, keyModPair);
        updateKeyBindingString(action, keyModPair, isDefault);
    }
}

void
KeyBindingsWindow::initialize(const Keyboard* keyboard)
{
    initializeKeyBindingStrings(keyboard);
    mInitialized = true;
}

void
KeyBindingsWindow::processKeyCapture(Viewport* viewport)
{
    // Update the capture state - this must be called every frame
    mKeyCapture.update(viewport);

    // Check if capture just completed
    if (mKeyCapture.isCompleted() && !mShowOverrideConfirm) {
        applyKeyBinding(viewport, /*force key binding override*/ false);
    }
    
    // If capture was cancelled, just reset
    if (mKeyCapture.isCancelled()) {
        mKeyCapture.reset();
    }
}

void
KeyBindingsWindow::applyKeyBinding(Viewport* viewport, bool forceOverride)
{
    Keyboard* keyboard = viewport->getKeyboard();

    int pendingKey = mKeyCapture.getPendingKey();
    int pendingMod = mKeyCapture.getPendingMod();
    Action pendingAction = mKeyCapture.getPendingAction();

    // Set new key binding, if there are no conflicts (or if the user has overridden the conflict)
    if (keyboard->addCustomBinding(pendingKey, pendingMod, pendingAction, mConflictingAction, forceOverride)) {
        // Update the display
        KeyModPair pendingKeyMod(pendingKey, pendingMod);
        bool isDefault = keyboard->isDefaultBinding(pendingAction, pendingKeyMod);
        updateKeyBindingString(pendingAction, pendingKeyMod, isDefault);

        // Unbind the conflicting action display string
        if (mConflictingAction != ACTION_NONE) {
            updateKeyBindingString(mConflictingAction, INVALID_KEYMOD);
        }

        clearPendingBinding();

    } else {
        // Show the key binding override confirmation modal
        mShowOverrideConfirm = true;
    }
}

void
KeyBindingsWindow::clearPendingBinding()
{
    mKeyCapture.reset();
    mPendingReset = false;
    mConflictingAction = ACTION_NONE;
}

void
KeyBindingsWindow::resetBindingToDefault(Keyboard* keyboard, const Action action, bool forceOverride)
{
    KeyModPair defaultKeyModPair;
    if (keyboard->resetBindingToDefault(action, mConflictingAction, defaultKeyModPair, forceOverride)) {
        updateKeyBindingString(action, defaultKeyModPair);
        updateKeyBindingString(mConflictingAction, INVALID_KEYMOD);

        clearPendingBinding();
    }  else {
        // Show the key binding override confirmation modal
        mShowOverrideConfirm = true;
        mPendingReset = true;
        mKeyCapture.setPendingAction(action);
        mKeyCapture.setPendingKey(defaultKeyModPair.first);
        mKeyCapture.setPendingMod(defaultKeyModPair.second);
    }
}

void
KeyBindingsWindow::drawHeaderText() const {
    if (mKeyCapture.isCapturing()) {
        std::string actionName = getActionName(mKeyCapture.getPendingAction());
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.f), "Press a key combination for '%s'", actionName.c_str());
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.f), "(Press ESC to cancel)");
    } else {
        ImGui::Text("Click on a key binding to change it.");
        ImGui::Text("Right-click to reset to default.");
    }
}

void
KeyBindingsWindow::drawControlButtons(Keyboard* keyboard) {
    if (ImGui::Button("Reset All")) {
        keyboard->resetToDefaults();
        initialize(keyboard);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save Settings")) {
        keyboard->saveKeyBindings(KEY_BINDINGS_FILE);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Settings")) {
        if (keyboard->loadKeyBindings(KEY_BINDINGS_FILE)) {
            initialize(keyboard);
        }
    }
}

void
KeyBindingsWindow::drawKeyboardModeDropdown(Keyboard* keyboard)
{
    // Keyboard Mode Selection
    const char* keyboardModes[] = { "Default", "Maya", "Houdini" };
    int currentMode = static_cast<int>(mKeyboardMode);
    
    ImGui::Text("Keyboard Mode:");
    ImGui::SameLine();
    if (ImGui::Combo("##KeyboardMode", &currentMode, keyboardModes, IM_ARRAYSIZE(keyboardModes))) {
        // Mode changed - update the keyboard
        KeyboardMode newMode = static_cast<KeyboardMode>(currentMode);
        if (newMode != mKeyboardMode) {
            mKeyboardMode = newMode;
            
            // Call the appropriate keyboard mode change function
            switch (mKeyboardMode) {
                case KEYBOARD_MODE_DEFAULT:
                    keyboard->setDefaultKeyboardMode();
                    break;
                case KEYBOARD_MODE_MAYA:
                    keyboard->setMayaKeyboardMode();
                    break;
                case KEYBOARD_MODE_HOUDINI:
                    keyboard->setHoudiniKeyboardMode();
                    break;
            }
            
            // Reinitialize the key binding strings for the new mode
            initialize(keyboard);
        }
    }
}

void
KeyBindingsWindow::drawKeyBindingButton(Viewport* viewport, const Action action)
{
    // Make the key binding display clickable
    ImGui::PushID(action);

    // Visual style for capturing state
    bool isCapturing = mKeyCapture.isCapturing() && mKeyCapture.getPendingAction() == action;
    if (isCapturing) {
        ImGui::PushStyleColor(ImGuiCol_Button, mKeyButtonColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, mKeyButtonHoverColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, mKeyButtonActiveColor);
    }

    const std::string& keyBindingStr = mKeyBindingStrings[action].second;
    const bool keyIsDefault = mKeyBindingStrings[action].first;

    if (!keyIsDefault) { ImGui::PushStyleColor(ImGuiCol_Text, mKeyOverrideTextColor); }

    // ------ Create a button that looks like text but can be clicked -------
    ImVec2 textSize = ImGui::CalcTextSize(keyBindingStr.c_str());
    ImVec2 buttonSize = ImVec2(/*take remaining width*/-1, textSize.y + mKeyHeightPadding);

    // We currently can't bind the scroll input, so that button is disabled for now
    if (action == ACTION_IMAGE2D_ZOOM) { ImGui::BeginDisabled(); }

    if (ImGui::Button(keyBindingStr.c_str(), buttonSize)) {
        mKeyCapture.startCapture(viewport, action);
    }

    if (action == ACTION_IMAGE2D_ZOOM) { ImGui::EndDisabled(); }
    // ----------------------------------------------------------------------

    if (!keyIsDefault) { ImGui::PopStyleColor(1); }
    if (isCapturing) { ImGui::PopStyleColor(3); }

    // Right-click context menu
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !mKeyCapture.isCapturing()) {
        ImGui::OpenPopup("key_context_menu");
    }
            
    if (ImGui::BeginPopup("key_context_menu")) {
        if (ImGui::MenuItem("Reset to Default")) {
            resetBindingToDefault(viewport->getKeyboard(), action, /*forceOverride*/ false);
        }
        ImGui::EndPopup();
    }     
    ImGui::PopID();
}

void
KeyBindingsWindow::drawKeyBindingTable(std::string header, const std::set<Action>& actions, Viewport* viewport)
{
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable(header.c_str(), 2, ImGuiTableFlags_Borders | 
                                                 ImGuiTableFlags_Resizable | 
                                                 ImGuiTableFlags_RowBg)) {
            // Set up columns
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Key Binding", ImGuiTableColumnFlags_WidthFixed, mKeyColumnWidth);

            ImGui::TableHeadersRow();

            for (const Action action : actions) {
                ImGui::TableNextRow();

                // First column (name of action)
                ImGui::TableSetColumnIndex(0);
                std::string actionStr = getActionName(action);
                ImGui::Text("%s", actionStr.c_str());

                // Second column (key binding)
                ImGui::TableSetColumnIndex(1);
                drawKeyBindingButton(viewport, action);

            }
            ImGui::EndTable();
        }
    }
}

void
KeyBindingsWindow::drawConflictModal(Viewport* viewport) {
    if (mShowOverrideConfirm) {
        ImGui::OpenPopup("Key Binding Conflict##Override_Confirm");
        mShowOverrideConfirm = false;
    }

    if (ImGui::BeginPopupModal("Key Binding Conflict##Override_Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

        std::string keyBindingStr = formatKeyBinding(mKeyCapture.getPendingKey(), mKeyCapture.getPendingMod());
        ImGui::Text("The key combination '%s' is already bound to:", keyBindingStr.c_str());
        ImGui::Text("  '%s'", getActionName(mConflictingAction).c_str());
        ImGui::Text("");
        if (mPendingReset) {
            ImGui::Text("Resetting '%s' to its default binding ", getActionName(mKeyCapture.getPendingAction()).c_str());
            ImGui::Text("will cause '%s' to be unbound.", getActionName(mConflictingAction).c_str());
            ImGui::Text("Do you wish to proceed?");
        } else {
            ImGui::Text("Do you want to reassign it to:");
            ImGui::Text("  '%s'", getActionName(mKeyCapture.getPendingAction()).c_str());
        }
        ImGui::Separator();

        if (ImGui::Button("Yes, Override", ImVec2(120, 0))) {
            if (mPendingReset) {
                resetBindingToDefault(viewport->getKeyboard(), mKeyCapture.getPendingAction(), /*forceOverride*/ true);
            } else {
                applyKeyBinding(viewport, /*force key binding override*/ true);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            clearPendingBinding();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void
KeyBindingsWindow::configureWindow() const
{
    ImGuiViewport* imguiViewport = ImGui::GetMainViewport();

    // ---- Set permanent position & size ----
    ImVec2 size = ImVec2(mWidth, imguiViewport->Size.y - mHeightPadding);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(imguiViewport->GetCenter(), ImGuiCond_Once, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(mWindowPadding, mWindowPadding));
}

void
KeyBindingsWindow::draw(Viewport* viewport, const ImVec2& currentPixel, const ImVec2& dockOffset)
{
    if (!viewport || !viewport->getKeyboard()) { return; }

    Keyboard* keyboard = viewport->getKeyboard();
    if (!mInitialized) {
        initialize(keyboard);
    }

    // Process any key inputs if we're in capturing mode
    processKeyCapture(viewport);

    if (mOpen) {
        configureWindow();

        if (ImGui::Begin("Key Bindings", &mOpen, ImGuiWindowFlags_NoResize)) {
            // Fixed header section at the top
            if (ImGui::BeginChild("HeaderSection", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY, 
                                                                 ImGuiWindowFlags_NoScrollbar)) {
                drawHeaderText();
                ImGui::Separator();
    
                drawKeyboardModeDropdown(keyboard);
                ImGui::Separator();
    
                drawControlButtons(keyboard);
                ImGui::Separator();
                ImGui::EndChild();
            }

            // Scrollable content section
            if (ImGui::BeginChild("ContentSection", ImVec2(0, 0), ImGuiChildFlags_None)) {
                for (const auto& [categoryName, categoryActions] : ACTION_CATEGORIES) {
                    drawKeyBindingTable(categoryName.c_str(), categoryActions, viewport);
                }
                ImGui::EndChild();
            }
            ImGui::End();
        }
        ImGui::PopStyleVar(1);
    }

    drawConflictModal(viewport);
}

} // namespace moonray_gui_v2
