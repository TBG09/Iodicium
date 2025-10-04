#include "common/dialog.h"

#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <vector>

#pragma comment(lib, "comctl32.lib")

#elif defined(__APPLE__)


std::string escape_for_shell(const std::wstring& s) {
    std::wstringstream escaped;
    for (wchar_t c : s) {
        if (c == L'\'' || c == L'"') {
            escaped << L"\\" << c;
        } else {
            escaped << c;
        }
    }

    std::wstring wide_str = escaped.str();
    std::string narrow_str(wide_str.begin(), wide_str.end());
    return narrow_str;
}

#endif

namespace Iodicium {
    namespace Common {

        int ShowDialog(const DialogOptions& options) {
#if defined(_WIN32)

            TASKDIALOGCONFIG config = { sizeof(config) };
            config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
            config.pszWindowTitle = options.title.c_str();
            config.pszMainInstruction = options.main_instruction.c_str();
            config.pszContent = options.message.c_str();

            switch (options.style) {
                case DialogStyle::Info:     config.pszMainIcon = TD_INFORMATION_ICON; break;
                case DialogStyle::Warning:  config.pszMainIcon = TD_WARNING_ICON;     break;
                case DialogStyle::Error:    config.pszMainIcon = TD_ERROR_ICON;       break;
                case DialogStyle::Question: config.pszMainIcon = TD_INFORMATION_ICON; break;
            }

            std::vector<TASKDIALOG_BUTTON> buttons;
            if (!options.custom_buttons.empty()) {
                for (const auto& btn : options.custom_buttons) {
                    buttons.push_back({btn.id, btn.text.c_str()});
                }
                config.pButtons = buttons.data();
                config.cButtons = buttons.size();
                config.dwFlags |= TDF_USE_COMMAND_LINKS;
            } else {
                config.dwCommonButtons = TDCBF_OK_BUTTON;
            }

            int button_pressed = 0;
            TaskDialogIndirect(&config, &button_pressed, NULL, NULL);
            return button_pressed;

#elif defined(__APPLE__)

            std::stringstream script;
            script << "osascript -e 'display dialog \"" << escape_for_shell(options.message) << "\"";
            script << " with title \"" << escape_for_shell(options.title) << "\"";

            switch (options.style) {
                case DialogStyle::Info:    script << " with icon note"; break;
                case DialogStyle::Warning: script << " with icon caution"; break;
                case DialogStyle::Error:   script << " with icon stop"; break;
                case DialogStyle::Question: script << " with icon note"; break;
            }

            if (!options.custom_buttons.empty()) {
                script << " buttons {";
                for (size_t i = 0; i < options.custom_buttons.size(); ++i) {
                    script << "\"" << escape_for_shell(options.custom_buttons[i].text) << "\"";
                    if (i < options.custom_buttons.size() - 1) script << ", ";
                }
                script << "}";
            }
            script << " default button 1'";

            FILE* pipe = popen(script.str().c_str(), "r");
            if (!pipe) return 0;

            char buffer[256];
            std::string result_str = "";
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                result_str += buffer;
            }
            pclose(pipe);


            if (result_str.find("button returned:") != std::string::npos) {
                std::string button_text = result_str.substr(result_str.find(":") + 1);
                button_text.pop_back();
                
                for (const auto& btn : options.custom_buttons) {
                    std::string btn_text(btn.text.begin(), btn.text.end());
                    if (button_text == btn_text) {
                        return btn.id;
                    }
                }
            }
            return 0;

#else
            if (std::getenv("DISPLAY") != nullptr) {
                std::stringstream command;
                command << "zenity";

                switch (options.style) {
                    case DialogStyle::Info: command << " --info"; break;
                    case DialogStyle::Warning: command << " --warning"; break;
                    case DialogStyle::Error: command << " --error"; break;
                    case DialogStyle::Question: command << " --question"; break;
                }

                command << " --title=\"" << std::string(options.title.begin(), options.title.end()) << "\" --text=\"" << std::string(options.message.begin(), options.message.end()) << "\"";

                if (!options.custom_buttons.empty()) {
                    for (const auto& btn : options.custom_buttons) {
                        command << " --extra-button=\"" << std::string(btn.text.begin(), btn.text.end()) << "\"";
                    }
                }

                FILE* pipe = popen(command.str().c_str(), "r");
                if (pipe) {
                    char buffer[128];
                    std::string result_str = "";
                    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                        result_str += buffer;
                    }
                    pclose(pipe);
                    if (!result_str.empty()) {
                        result_str.pop_back();
                    }

                    for (const auto& btn : options.custom_buttons) {
                         if (result_str == std::string(btn.text.begin(), btn.text.end())) {
                            return btn.id;
                         }
                    }
                    return 0;
                } else {
                    std::cerr << "[Dialog] Failed to launch zenity. Falling back to console output.\n";
                }
            }

            std::cerr << "\n--- DIALOG ---\n";
            std::cerr << "Title: " << std::string(options.title.begin(), options.title.end()) << "\n";
            std::cerr << "Message: " << std::string(options.message.begin(), options.message.end()) << "\n";
            if (!options.custom_buttons.empty()) {
                std::cerr << "Options: ";
                for (size_t i = 0; i < options.custom_buttons.size(); ++i) {
                    std::cerr << "[" << options.custom_buttons[i].id << "] " << std::string(options.custom_buttons[i].text.begin(), options.custom_buttons[i].text.end());
                    if (i < options.custom_buttons.size() - 1) std::cerr << ", ";
                }
                std::cerr << "\n";
            }
            std::cerr << "------------\n\n";
            return 0;
#endif
        }
    }
}
