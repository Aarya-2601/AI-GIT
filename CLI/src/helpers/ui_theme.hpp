#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace UI
{
    // Enable ANSI Virtual Terminal processing on Windows console
    inline void initTerminal()
    {
#ifdef _WIN32
        static bool initialized = false;
        if (!initialized)
        {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE)
            {
                DWORD dwMode = 0;
                if (GetConsoleMode(hOut, &dwMode))
                {
                    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    SetConsoleMode(hOut, dwMode);
                }
            }
            SetConsoleOutputCP(CP_UTF8);
            initialized = true;
        }
#endif
    }

    // AI-Git Chameleon Palette ANSI Escape Codes
    namespace Color
    {
        // Resets & Styles
        const std::string RESET       = "\033[0m";
        const std::string BOLD        = "\033[1m";
        const std::string DIM         = "\033[2m";
        const std::string ITALIC      = "\033[3m";
        const std::string UNDERLINE   = "\033[4m";

        // Chameleon Primary Colors (24-bit TrueColor)
        const std::string CYAN        = "\033[38;2;0;240;255m";     // #00f0ff Bright Teal/Cyan
        const std::string TEAL        = "\033[38;2;34;211;238m";    // #22d3ee Soft Teal
        const std::string GREEN       = "\033[38;2;16;185;129m";    // #10b981 Emerald Green
        const std::string BRIGHT_GREEN= "\033[38;2;52;211;153m";    // #34d399 Bright Mint/Green
        const std::string BLUE        = "\033[38;2;56;189;248m";    // #38bdf8 Subtle Blue Accent
        const std::string PURPLE      = "\033[38;2;168;85;247m";    // #a855f7 Purple Accent
        
        // Neutral & High Contrast
        const std::string WHITE       = "\033[38;2;248;250;252m";   // #f8fafc Crisp White
        const std::string SLATE       = "\033[38;2;148;163;184m";   // #94a3b8 Muted Secondary
        const std::string DARK_SLATE  = "\033[38;2;71;85;105m";     // #475569 Dim Border
        const std::string BORDER      = "\033[38;2;51;65;85m";      // #334155 Subtle Box Border
        const std::string AMBER       = "\033[38;2;245;158;11m";    // #f59e0b Warning / Modified
        const std::string RED         = "\033[38;2;239;68;68m";     // #ef4444 Deleted / Error

        // Backgrounds
        const std::string BG_NAVY     = "\033[48;2;10;15;29m";      // #0a0f1d Deep Navy BG
        const std::string BG_PANEL    = "\033[48;2;15;23;42m";      // #0f172a Panel Dark BG
        const std::string BG_CYAN     = "\033[48;2;0;240;255m\033[38;2;10;15;29m";
        const std::string BG_GREEN    = "\033[48;2;16;185;129m\033[38;2;10;15;29m";
        const std::string BG_AMBER    = "\033[48;2;245;158;11m\033[38;2;10;15;29m";
    }

    // Format byte sizes into human readable units (B, KB, MB, GB)
    inline std::string formatBytes(uintmax_t bytes)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1);
        if (bytes < 1024)
        {
            ss << bytes << " B";
        }
        else if (bytes < 1024 * 1024)
        {
            ss << (bytes / 1024.0) << " KB";
        }
        else if (bytes < 1024ULL * 1024 * 1024)
        {
            ss << (bytes / (1024.0 * 1024.0)) << " MB";
        }
        else
        {
            ss << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
        }
        return ss.str();
    }

    // Chameleon Logo Banner
    inline void printLogoBanner()
    {
        initTerminal();
        std::cout << Color::CYAN << Color::BOLD << "  🦎  AI-GIT " 
                  << Color::GREEN << "• "
                  << Color::TEAL << "FastCDC "
                  << Color::SLATE << "Content-Addressable Storage VCS\n"
                  << Color::RESET;
    }

    // Rounded Box Header
    inline void printHeader(const std::string& title, const std::string& subtitle = "")
    {
        initTerminal();
        std::cout << Color::BORDER << "╭─ " 
                  << Color::CYAN << Color::BOLD << title 
                  << Color::RESET;
        if (!subtitle.empty())
        {
            std::cout << Color::SLATE << "  (" << subtitle << ")";
        }
        std::cout << "\n" << Color::RESET;
    }
}
