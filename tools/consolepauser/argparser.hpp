/*
 *  This file is part of Red Panda C++
 *  Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <map>

template <typename Ch>
struct ArgParser
{
    using String = std::basic_string<Ch>;
    using StringView = std::basic_string_view<Ch>;

    template <size_t N>
    static String StringFromAsciiLiteral(const char (&literal)[N])
    {
        if constexpr (std::is_same_v<Ch, char>) {
            return String{literal};
        } else {
            String result(N - 1, Ch(0));
            for (size_t i = 0; i < N - 1; ++i) {
                result[i] = literal[i];
            }
            return result;
        }
    }

    // translation items: please keep sync with `RedPandaIDE/compiler/compilermanager.cpp`
    enum class TextItem {
        EXIT,
        USAGE_HEADER,
        USAGE_RETURN_VALUE,
        USAGE_CPU_TIME,
        USAGE_MEMORY,
    };

    inline static std::map<TextItem, String> fallbackText = {
        {TextItem::EXIT, StringFromAsciiLiteral("Press ANY key to exit...")},
        {TextItem::USAGE_HEADER, StringFromAsciiLiteral("Process exited after")},
        {TextItem::USAGE_RETURN_VALUE, StringFromAsciiLiteral("Return value")},
        {TextItem::USAGE_CPU_TIME, StringFromAsciiLiteral("CPU time")},
        {TextItem::USAGE_MEMORY, StringFromAsciiLiteral("Memory")},
    };

    struct Args
    {
        StringView program;
        std::vector<StringView> args;
        std::map<StringView, StringView> translatedText;

        String sharedMemory;
        String redirectInput;
        bool pauseConsole = false;
        bool enableVirtualTerminalSeq = false;
        bool runInWsl = false;
    };

    inline static Args gArgs;

    static StringView GetText(StringView key, TextItem fallbackKey)
    {
        if (auto it = gArgs.translatedText.find(key);
            it != gArgs.translatedText.end())
            return it->second;
        return fallbackText[fallbackKey];
    }

    static bool AsciiEqual(StringView lhs, std::string_view rhs)
    {
        if constexpr (std::is_same_v<Ch, char>) {
            return lhs == rhs;
        } else {
            if (lhs.size() != rhs.size())
                return false;
            for (size_t i = 0; i < lhs.size(); ++i) {
                if (lhs[i] != rhs[i])
                    return false;
            }
            return true;
        }
    }

    static bool MatchFlag(StringView arg, std::string_view flag)
    {
        return AsciiEqual(arg, flag);
    }

    static bool MatchOption(StringView arg, std::string_view option, const Ch *&valuePos)
    {
        // --option value
        if (AsciiEqual(arg, option)) {
            valuePos = nullptr;
            return true;
        }
        // --option=value
        if (arg.size() > option.size() && arg[option.size()] == '=' && AsciiEqual(arg.substr(0, option.size()), option)) {
            valuePos = arg.data() + option.size() + 1;
            return true;
        }
        return false;
    }

    static Args ParseArgs(Ch *const *argv)
    {
        Args result;
        std::vector<StringView> positionalArgs;

        argv++; // skip argv[0]

        // phase 1: mixed flags, options, and positional args
        while (*argv != nullptr) {
            StringView arg(*argv);
            const Ch *valuePos;
            if (MatchFlag(arg, "--")) {
                argv++;
                break;
            } else if (MatchFlag(arg, "--pause-console")) {
                result.pauseConsole = true;
            } else if (MatchFlag(arg, "--enable-virtual-terminal-sequence")) {
                result.enableVirtualTerminalSeq = true;
            } else if (MatchFlag(arg, "--run-in-wsl")) {
                result.runInWsl = true;
            } else if (MatchOption(arg, "--shared-memory", valuePos)) {
                if (valuePos) {
                    result.sharedMemory = valuePos;
                } else if (argv[1] != nullptr) {
                    result.sharedMemory = argv[1];
                    argv++;
                } else {
                    throw StringFromAsciiLiteral("missing value for --shared-memory-name.");
                }
            } else if (MatchOption(arg, "--redirect-input", valuePos)) {
                if (valuePos) {
                    result.redirectInput = valuePos;
                } else if (argv[1] != nullptr) {
                    result.redirectInput = argv[1];
                    argv++;
                } else {
                    throw StringFromAsciiLiteral("missing value for --redirect-input.");
                }
            } else if (MatchOption(arg, "--add-translation", valuePos)) {
                StringView raw;
                if (valuePos) {
                    raw = valuePos;
                } else if (argv[1] != nullptr) {
                    raw = argv[1];
                    argv++;
                } else {
                    throw StringFromAsciiLiteral("missing value for --add-translation.");
                }

                size_t pos = raw.find('=');
                if (pos == String::npos) {
                    String message = StringFromAsciiLiteral("invalid value for --add-translation: ");
                    message.insert(message.end(), raw.begin(), raw.end());
                    message.push_back('.');
                    throw message;
                }
                result.translatedText.emplace(raw.substr(0, pos), raw.substr(pos + 1));
            } else if (arg[0] == '-') {
                String message = StringFromAsciiLiteral("unknown option ");
                message.insert(message.end(), arg.begin(), arg.end());
                message.push_back('.');
                throw message;
            } else {
                positionalArgs.push_back(arg);
            }
            argv++;
        }

        // phase 2: positional args after `--`
        while (*argv != nullptr) {
            positionalArgs.push_back(*argv);
            argv++;
        }

        if (positionalArgs.empty())
            throw StringFromAsciiLiteral("missing program.");
        result.program = positionalArgs[0];
        result.args.insert(result.args.end(), positionalArgs.begin() + 1, positionalArgs.end());

        return result;
    }

    static String HelpMessage()
    {
        return StringFromAsciiLiteral(
R"(Red Panda C++ console pauser - run program and pause console after exit.

Usage: consolepauser [options] -- program [args...]

Public options:
  --pause-console                 Pause console after exit.

Internal options:
  --add-translation <key>=<text>
  --enable-virtual-terminal-sequence
  --redirect-input <file>
  --run-in-wsl
  --shared-memory <name>
)"
        );
    }
};
