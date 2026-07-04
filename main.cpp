// ============================================================================
// PROJECT: Motion2Mouse (UEVR Profile Plugin)
// AUTHOR: Frederic1323
// DESCRIPTION: Converts VR Controller positioning into smooth virtual mouse 
//              movements.
// ============================================================================

#include "pch.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include "uevr/Plugin.hpp"

using namespace uevr;

class Motion2Mouse : public uevr::Plugin {
public:
    Motion2Mouse() = default;

    void on_initialize() override {
        API::UObjectHook::activate();

        // 1. Standardwerte setzen
        m_plane_width = 1.6f;     
        m_plane_height = 0.9f;    
        m_smoothing = 0.75f; 
        m_stick_deadzone = 8000;
        m_scroll_speed = 1.5f;
        m_invert_buttons = false; 

        // 2. Einstellungen aus Textdatei laden
        load_config();

        m_initialized = false;
        m_center_was_pressed = false;
        m_center_yaw = 0.0f;
        m_center_pitch = 0.0f;
        m_last_sent_x = 32768;
        m_last_sent_y = 32768;
        m_buttons = 0;
        m_was_trigger = false;
        m_was_grip = false;
        
        m_w_pressed = m_a_pressed = m_s_pressed = m_d_pressed = false;
        m_f_pressed = m_g_pressed = false;
    }

    void on_xinput_get_state(uint32_t* retval, uint32_t user_index, XINPUT_STATE* state) override {
        if (user_index == 0) {
            m_buttons = state->Gamepad.wButtons;
            m_thumb_lx = state->Gamepad.sThumbLX;
            m_thumb_ly = state->Gamepad.sThumbLY;
            m_thumb_rx = state->Gamepad.sThumbRX;
            m_thumb_ry = state->Gamepad.sThumbRY;
        }
    }

    void on_pre_engine_tick(API::UGameEngine* engine, float delta) override {
        auto vr = API::get()->param()->vr;
        int right_idx = vr->get_right_controller_index();
        if (right_idx == -1) return;

        // ============================================================
        // POSITION & ORIENTATION EXTRACTION
        // ============================================================
        UEVR_Vector3f hmd_pos;
        UEVR_Quaternionf hmd_q;
        vr->get_pose(0, &hmd_pos, &hmd_q); 

        UEVR_Vector3f hand_pos;
        UEVR_Quaternionf hand_q;
        vr->get_aim_pose(right_idx, &hand_pos, &hand_q);

        float hmd_norm = std::sqrt(hmd_q.w*hmd_q.w + hmd_q.x*hmd_q.x + hmd_q.y*hmd_q.y + hmd_q.z*hmd_q.z);
        if (hmd_norm > 0.0001f) {
            hmd_q.w /= hmd_norm; hmd_q.x /= hmd_norm; hmd_q.y /= hmd_norm; hmd_q.z /= hmd_norm;
        }
        float hmd_yaw = std::atan2(2.0f * (hmd_q.w * hmd_q.y + hmd_q.x * hmd_q.z), 1.0f - 2.0f * (hmd_q.y * hmd_q.y + hmd_q.z * hmd_q.z));

        float hand_norm = std::sqrt(hand_q.w*hand_q.w + hand_q.x*hand_q.x + hand_q.y*hand_q.y + hand_q.z*hand_q.z);
        if (hand_norm < 0.0001f) return;
        hand_q.w /= hand_norm; hand_q.x /= hand_norm; hand_q.y /= hand_norm; hand_q.z /= hand_norm;

        float current_yaw = std::atan2(2.0f * (hand_q.w * hand_q.y + hand_q.x * hand_q.z), 1.0f - 2.0f * (hand_q.y * hand_q.y + hand_q.z * hand_q.z));
        float current_pitch = std::asin(std::max(-1.0f, std::min(1.0f, 2.0f * (hand_q.w * hand_q.x - hand_q.y * hand_q.z))));

        float dx_h = hand_pos.x - hmd_pos.x;
        float dy_h = hand_pos.y - hmd_pos.y;
        float dz_h = hand_pos.z - hmd_pos.z;
        float real_distance = std::sqrt(dx_h*dx_h + dy_h*dy_h + dz_h*dz_h);

        if (real_distance < 0.1f) real_distance = 0.5f;
        float dynamic_plane_distance = real_distance * 2.5f; 

        if (!m_initialized) {
            m_center_yaw = hmd_yaw; 
            m_center_pitch = current_pitch;
            m_smooth_x = 32768.0f;
            m_smooth_y = 32768.0f;
            m_initialized = true;
            return;
        }

        // ============================================================
        // TRIGO-PROJECTION & MOUSE MOVEMENT
        // ============================================================
        float diff_yaw = current_yaw - m_center_yaw;
        float diff_pitch = current_pitch - m_center_pitch;

        if (diff_yaw > 3.14159f) diff_yaw -= 6.28318f;
        if (diff_yaw < -3.14159f) diff_yaw += 6.28318f;

        float x_proj = dynamic_plane_distance * std::tan(diff_yaw);
        float y_proj = dynamic_plane_distance * std::tan(diff_pitch);

        float raw_target_x = 0.5f - (x_proj / m_plane_width);
        float raw_target_y = 0.5f - (y_proj / m_plane_height);

        float target_absolute_x = raw_target_x * 65535.0f;
        float target_absolute_y = raw_target_y * 65535.0f;

        m_smooth_x = (m_smoothing * m_smooth_x) + ((1.0f - m_smoothing) * target_absolute_x);
        m_smooth_y = (m_smoothing * m_smooth_y) + ((1.0f - m_smoothing) * target_absolute_y);

        LONG final_x = static_cast<LONG>(std::max(0.0f, std::min(65535.0f, m_smooth_x)));
        LONG final_y = static_cast<LONG>(std::max(0.0f, std::min(65535.0f, m_smooth_y)));

        if (std::abs(final_x - m_last_sent_x) > 25 || std::abs(final_y - m_last_sent_y) > 25) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dx = final_x;
            input.mi.dy = final_y;
            input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            SendInput(1, &input, sizeof(INPUT));
            m_last_sent_x = final_x;
            m_last_sent_y = final_y;
        }

        // ============================================================
        // MANUAL RE-CENTERING (Klick auf rechten Stick / R3)
        // ============================================================
        bool center_pressed = (m_buttons & 0x0010) != 0;
        if (center_pressed && !m_center_was_pressed) {
            m_center_yaw = current_yaw; 
            m_center_pitch = current_pitch;
            m_smooth_x = 32768.0f;
            m_smooth_y = 32768.0f;
            m_center_was_pressed = true;

            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dx = 32768;
            input.mi.dy = 32768;
            input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            SendInput(1, &input, sizeof(INPUT));
        }
        if (!center_pressed) m_center_was_pressed = false;

        // ============================================================
        // BUTTON MAPPING
        // ============================================================
        DWORD trigger_down = m_invert_buttons ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_LEFTDOWN;
        DWORD trigger_up   = m_invert_buttons ? MOUSEEVENTF_RIGHTUP   : MOUSEEVENTF_LEFTUP;
        DWORD grip_down    = m_invert_buttons ? MOUSEEVENTF_LEFTDOWN  : MOUSEEVENTF_RIGHTDOWN;
        DWORD grip_up      = m_invert_buttons ? MOUSEEVENTF_LEFTUP    : MOUSEEVENTF_RIGHTUP;

        bool trigger_pressed = (m_buttons & 0x0200) != 0;
        if (trigger_pressed && !m_was_trigger) send_mouse_click(trigger_down);
        if (!trigger_pressed && m_was_trigger) send_mouse_click(trigger_up);
        m_was_trigger = trigger_pressed;

        bool grip_pressed = (m_buttons & 0x8000) != 0;
        if (grip_pressed && !m_was_grip) send_mouse_click(grip_down);
        if (!grip_pressed && m_was_grip) send_mouse_click(grip_up);
        m_was_grip = grip_pressed;

        // ============================================================
        // STICKS (WASD auf Links, Scrollen/Keys auf Rechts)
        // ============================================================
        if (m_thumb_ly > m_stick_deadzone) { if (!m_w_pressed) send_key(0x57, true); m_w_pressed = true; } else { if (m_w_pressed) send_key(0x57, false); m_w_pressed = false; }
        if (m_thumb_ly < -m_stick_deadzone) { if (!m_s_pressed) send_key(0x53, true); m_s_pressed = true; } else { if (m_s_pressed) send_key(0x53, false); m_s_pressed = false; }
        if (m_thumb_lx < -m_stick_deadzone) { if (!m_a_pressed) send_key(0x41, true); m_a_pressed = true; } else { if (m_a_pressed) send_key(0x41, false); m_a_pressed = false; }
        if (m_thumb_lx > m_stick_deadzone) { if (!m_d_pressed) send_key(0x44, true); m_d_pressed = true; } else { if (m_d_pressed) send_key(0x44, false); m_d_pressed = false; }

        if (std::abs((int)m_thumb_ry) > m_stick_deadzone) {
            float scroll = -(float)m_thumb_ry / 32768.0f * 120.0f * m_scroll_speed;
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = static_cast<DWORD>(static_cast<LONG>(scroll));
            SendInput(1, &input, sizeof(INPUT));
        }

        if (m_thumb_rx > m_stick_deadzone) { if (!m_f_pressed) send_key(0x46, true); m_f_pressed = true; } else { if (m_f_pressed) send_key(0x46, false); m_f_pressed = false; }
        if (m_thumb_rx < -m_stick_deadzone) { if (!m_g_pressed) send_key(0x47, true); m_g_pressed = true; } else { if (m_g_pressed) send_key(0x47, false); m_g_pressed = false; }
    }

private:
    void send_mouse_click(DWORD flags) {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = flags;
        SendInput(1, &input, sizeof(INPUT));
    }

    void send_key(WORD vk, bool down) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    void load_config() {
        const std::string filename = "motion2mouse_config.txt";
        std::ifstream infile(filename);
        
        if (!infile.good()) {
            // Datei existiert nicht -> Standarddatei erstellen
            std::ofstream outfile(filename);
            outfile << "# Motion2Mouse Konfiguration\n";
            outfile << "width=1.6\n";
            outfile << "height=0.9\n";
            outfile << "smoothing=0.75\n";
            outfile << "deadzone=8000\n";
            outfile << "scroll_speed=1.5\n";
            outfile << "invert_buttons=0\n";
            outfile.close();
            return;
        }

        std::string line;
        while (std::getline(infile, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                if (key == "width") m_plane_width = std::stof(value);
                else if (key == "height") m_plane_height = std::stof(value);
                else if (key == "smoothing") m_smoothing = std::stof(value);
                else if (key == "deadzone") m_stick_deadzone = static_cast<SHORT>(std::stoi(value));
                else if (key == "scroll_speed") m_scroll_speed = std::stof(value);
                else if (key == "invert_buttons") m_invert_buttons = (std::stoi(value) != 0);
            }
        }
        infile.close();
    }

    // Variablen
    bool m_initialized = false;
    bool m_center_was_pressed = false;
    float m_center_yaw = 0.0f;
    float m_center_pitch = 0.0f;

    float m_plane_width;
    float m_plane_height;
    float m_smoothing;
    float m_scroll_speed;
    bool m_invert_buttons;

    float m_smooth_x = 32768.0f;
    float m_smooth_y = 32768.0f;
    LONG m_last_sent_x = 32768;
    LONG m_last_sent_y = 32768;
    WORD m_buttons = 0;
    bool m_was_trigger = false;
    bool m_was_grip = false;
    SHORT m_thumb_lx = 0;
    SHORT m_thumb_ly = 0;
    SHORT m_thumb_rx = 0;
    SHORT m_thumb_ry = 0;
    SHORT m_stick_deadzone = 8000;

    bool m_w_pressed, m_a_pressed, m_s_pressed, m_d_pressed, m_f_pressed, m_g_pressed;
};

std::unique_ptr<Motion2Mouse> g_plugin{ new Motion2Mouse() };

extern "C" __declspec(dllexport) uevr::Plugin* create_plugin() {
    return g_plugin.get();
}
