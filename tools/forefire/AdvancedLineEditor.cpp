#include "AdvancedLineEditor.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

namespace advanced_editor {

// --- Helper functions to set/restore terminal mode ---
static void setRawMode(termios &oldt) {
    termios newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

static void restoreMode(const termios &oldt) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

// --- Build the command dictionary (commandMan) ---
static const std::unordered_map<std::string, std::string>& buildCommandMan() {
    static std::unordered_map<std::string, std::string> cmdMan = []() -> std::unordered_map<std::string, std::string> {
        std::unordered_map<std::string, std::string> m;
        m["FireDomain"] =
            "FireDomain[Psw;Pne;t;bmapdx;bmapdy]\n"
            " - 'Psw' and 'Pne' are the southwest and northwest points of the boundaries\n"
            "   and should be affected as in 'Psw=(0.,0.,0.)';\n"
            " - 't' is the time associated with fire domain ('t=0.');\n"
            " - 'bmapdx' and 'bmapdy' are the resolution of the burning map in each direction\n"
            "   and should be affected as in 'bmapdx=0.1';";
        m["FireNode"] =
            "FireNode[loc;vel;t]\n"
            " - 'loc' is the spatial point where the firenode is created ('loc=(0.,0.,0.)');\n"
            " - 'vel' is the initial velocity of the firenode ('vel=(0.,0.,0.)');\n"
            " - 't' is the time associated with fire node ('t=0.');";
        m["FireFront"] =
            "FireFront[]\n create a fire front with the following fire nodes";
        m["startFire"] =
            "startFire[loc=(0.,0.,0.),t=0.]\n create a triangular fire front with 3 nodes around the location at time t";
        m["step"] =
            "step[dt]\n - 'dt' is the duration for which the simulation will run ('dt=5.')";
        m["trigger"] =
            "trigger\n triggers changes runtime in simulation data, fuel of dynamic wind \n"
            "- fuelIndice;loc=(x,y,z);fuelType=int or wind;loc=(x,y,z);vel=(vx,vy,vz);t=f"; 
        m["goTo"] =
            "goTo[t]\n - 't' is the desired time till which the simulation will run ('t=56.2')";
        m["print"] =
            "print[output]\n prints a representation of the simulation in the file 'output'";
        m["save"] =
            "save\n saves the simulation in hdf format";
        m["load"] =
            "load\n loads the simulation arrival_times in netcdf format";
        m["plot"] =
            "plot\n generates a png or jpg of the simulation\n parameter=speed or arrival_time_of_front;filename=outfname.png/jpg;opt:range=(0,0.1);opt:cmap=viridis;opt:histogram=true";
        m["computeSpeed"] =
            "computeSpeed\n uses the first registered propp model to activate and get result from an array of values separated by ;";
        m["setParameter"] =
            "setParameter[param=value]\n - sets parameter 'param' to the given 'value'";
        m["setParameters"] =
            "setParameters[param1=val1;param2=val2;...;paramn=valn]\n - sets a given list of parameters to the desired values";
        m["getParameter"] =
            "getParameter[key=value]\n gets parameter 'key'";
        m["include"] =
            "include[input]\n - executes the commands in the file 'input'";
        m["help"] =
            "help\n displays messages about the usage of commands";
        m["loadData"] =
            "loadData\n load a NC data file";
        m["clear"] =
            "clear\n clear all the simulation data";
        m["systemExec"] =
            "systemExec\n run a [system] command";
        m["listenHTTP"] = 
            "listenHTTP\n [host:port] launches an HTTP server to listen for commands";
        m["quit"] =
            "quit\n terminates the simulation";
        return m;
    }();
    return cmdMan;
}

const std::unordered_map<std::string, std::string>& LineEditor::getCommandMan() {
    return buildCommandMan();
}

// --- Syntax coloring ---
// This function colors any occurrence of a recognized command (from our dictionary) in green.
std::string LineEditor::applySyntaxColoring(const std::string &line) {
    std::string result = line;
    const std::string colorOn = "\033[32m"; // green
    const std::string colorOff = "\033[0m";  // reset
    const auto &cmdMan = LineEditor::getCommandMan();
    for (const auto &pair : cmdMan) {
        size_t pos = 0;
        while ((pos = result.find(pair.first, pos)) != std::string::npos) {
            result.replace(pos, pair.first.length(), colorOn + pair.first + colorOff);
            pos += colorOn.length() + pair.first.length() + colorOff.length();
        }
    }
    return result;
}

// --- Redraw ---
// Clears the current line and reprints the prompt (in orange) plus the syntax-colored line.
// Then, moves the cursor left by (line.size() - cursorPos) positions.
void LineEditor::redraw(const std::string &prompt, const std::string &line, int cursorPos) {
    // Clear the current line.
    std::cout << "\33[2K\r";
    // Print the prompt in orange.
    std::cout << "\033[38;5;208m" << prompt << "\033[0m";
    // Print the syntax-colored line.
    std::cout << applySyntaxColoring(line);
    // Calculate how many characters from the end we need to move left.
    int visibleLength = static_cast<int>(line.size());
    int moveLeft = visibleLength - cursorPos;
    if (moveLeft > 0) {
        std::cout << "\033[" << moveLeft << "D";
    }
    std::cout.flush();
}

// --- Auto-completion ---
// This function looks for command dictionary keys starting with the current line.
bool LineEditor::processAutoCompletion(std::string &line, const std::string &prompt) {
    const auto &cmdMan = LineEditor::getCommandMan();
    std::vector<std::string> matches;
    for (const auto &pair : cmdMan) {
        if (pair.first.compare(0, line.size(), line) == 0) {
            matches.push_back(pair.first);
        }
    }
    if (matches.empty())
        return false;
    if (matches.size() == 1) {
        line = matches[0];
        return true;
    } else {
        std::cout << "\n";
        for (const auto &match : matches) {
            std::cout << "\033[32m  " << match << "\033[0m" << "\n";
        }
        std::cout << "\033[38;5;208m" << prompt << "\033[0m" << applySyntaxColoring(line);
        std::cout.flush();
        return false;
    }
}

// --- Main input function ---
// This function now maintains a cursor position (cursorPos) and supports left/right navigation.
std::string LineEditor::getLine(const std::string &prompt) {
    std::string line;
    int cursorPos = 0; // Position within the line.
    std::cout << "\033[38;5;208m" << prompt << "\033[0m";
    std::cout.flush();

    termios oldt;
    setRawMode(oldt);

    static std::vector<std::string> history;
    static int history_index = -1; // -1 means no active history navigation

    static bool lastWasTab = false;

    while (true) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n <= 0)
            break;

        if (c == '\n' || c == '\r') {
            std::cout << std::endl;
            break;
        }
        // Backspace (or DEL): delete character before cursor.
        if (c == 127 || c == '\b') {
            if (cursorPos > 0) {
                line.erase(cursorPos - 1, 1);
                cursorPos--;
            }
            redraw(prompt, line, cursorPos);
            lastWasTab = false;
            continue;
        }
        // Tab key (ASCII 9) for auto-completion.
        if (c == '\t') {
            if (lastWasTab) {
                std::string key = line;
                while (!key.empty() && key.back() == ' ')
                    key.pop_back();
                size_t pos = key.find('[');
                if (pos != std::string::npos)
                    key = key.substr(0, pos);
                const auto &cmdMan = LineEditor::getCommandMan();
                auto it = cmdMan.find(key);
                if (it != cmdMan.end()) {
                    std::cout << "\n\033[36m[" << it->second << "]\033[0m" << std::endl;
                    std::cout << "\033[38;5;208m" << prompt << "\033[0m" << applySyntaxColoring(line);
                    // After printing help, reposition the cursor.
                    int moveLeft = static_cast<int>(line.size()) - cursorPos;
                    if (moveLeft > 0) {
                        std::cout << "\033[" << moveLeft << "D";
                    }
                    std::cout.flush();
                }
                lastWasTab = false;
            } else {
                bool updated = processAutoCompletion(line, prompt);
                if (updated) {
                    cursorPos = static_cast<int>(line.size());
                    redraw(prompt, line, cursorPos);
                }
                lastWasTab = true;
            }
            continue;
        }
        // Reset double-Tab flag if other keys are pressed.
        lastWasTab = false;
        
        // Check for escape sequences (arrow keys).
        if (c == 27) { // ESC
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 0)
                continue;
            if (read(STDIN_FILENO, &seq[1], 1) == 0)
                continue;
            if (seq[0] == '[') {
                if (seq[1] == 'A') { // Up arrow (history)
                    if (!history.empty() && history_index < static_cast<int>(history.size()) - 1) {
                        history_index++;
                        line = history[history.size() - 1 - history_index];
                        cursorPos = static_cast<int>(line.size());
                        redraw(prompt, line, cursorPos);
                    }
                    continue;
                } else if (seq[1] == 'B') { // Down arrow (history)
                    if (history_index > 0) {
                        history_index--;
                        line = history[history.size() - 1 - history_index];
                        cursorPos = static_cast<int>(line.size());
                        redraw(prompt, line, cursorPos);
                    } else if (history_index == 0) {
                        history_index = -1;
                        line.clear();
                        cursorPos = 0;
                        redraw(prompt, line, cursorPos);
                    }
                    continue;
                } else if (seq[1] == 'D') { // Left arrow
                    if (cursorPos > 0) {
                        cursorPos--;
                        redraw(prompt, line, cursorPos);
                    }
                    continue;
                } else if (seq[1] == 'C') { // Right arrow
                    if (cursorPos < static_cast<int>(line.size())) {
                        cursorPos++;
                        redraw(prompt, line, cursorPos);
                    }
                    continue;
                }
            }
        }
        // Regular character: insert at the current cursor position.
        line.insert(cursorPos, 1, c);
        cursorPos++;
        redraw(prompt, line, cursorPos);
    }

    restoreMode(oldt);

    if (!line.empty()) {
        history.push_back(line);
        history_index = -1;
    }
    return line;
}

} // namespace advanced_editor