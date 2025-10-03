#ifndef IODICIUM_COMMON_DIALOG_H
#define IODICIUM_COMMON_DIALOG_H

#include <string>
#include <vector>

namespace Iodicium {
    namespace Common {

        // Standard button IDs, matching Windows for consistency
        constexpr int DLG_OK = 1;
        constexpr int DLG_CANCEL = 2;
        constexpr int DLG_YES = 6;
        constexpr int DLG_NO = 7;

        struct CustomButton {
            std::wstring text;
            int id;
        };

        enum class DialogStyle {
            Info,
            Warning,
            Error,
            Question
        };

        struct DialogOptions {
            std::wstring title = L"Iodicium";
            std::wstring main_instruction;
            std::wstring message;
            DialogStyle style = DialogStyle::Info;
            
            // For custom buttons. If empty, standard OK/Cancel etc. will be used.
            std::vector<CustomButton> custom_buttons;

            // New constructor to allow passing std::string literals, converting them internally
            DialogOptions(const std::string& title_str, const std::string& message_str, const std::string& main_instruction_str, DialogStyle s = DialogStyle::Info)
                : title(title_str.begin(), title_str.end()),
                  message(message_str.begin(), message_str.end()),
                  main_instruction(main_instruction_str.begin(), main_instruction_str.end()),
                  style(s) {}

            // Default constructor
            DialogOptions() = default;
        };

        // Shows a modal dialog and returns the ID of the button that was clicked.
        int ShowDialog(const DialogOptions& options);

    }
}

#endif //IODICIUM_COMMON_DIALOG_H
