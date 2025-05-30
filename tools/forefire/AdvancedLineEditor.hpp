#ifndef ADVANCED_LINE_EDITOR_HPP
#define ADVANCED_LINE_EDITOR_HPP

#include <string>
#include <unordered_map>

namespace advanced_editor {

class LineEditor {
public:
    /// Reads a line from standard input with a colored prompt.
    /// Supports history (arrow up/down), left/right navigation,
    /// syntax coloring, auto‐completion (Tab), and on a second Tab press prints help info.
    static std::string getLine(const std::string &prompt = "");

    // Internal: Returns the built‐in command dictionary mapping command name to help text.
    static const std::unordered_map<std::string, std::string>& getCommandMan();

private:
    // Applies syntax coloring to the given line.
    // In this example, any occurrence of a recognized command is shown in green.
    static std::string applySyntaxColoring(const std::string &line);

    // Redraws the prompt and the (colored) line, and positions the cursor.
    // The parameter cursorPos is the position (0-based) in the uncolored string.
    static void redraw(const std::string &prompt, const std::string &line, int cursorPos);

    // Processes auto‐completion based on the built‐in command dictionary.
    // Returns true if the line was updated.
    static bool processAutoCompletion(std::string &line, const std::string &prompt);

};

} // namespace advanced_editor

#endif // ADVANCED_LINE_EDITOR_HPP