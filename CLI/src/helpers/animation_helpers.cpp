#include "animation_helpers.hpp"
#include 
#include 
#include 

// Helper to clear the 5 lines of the previous frame
static void clear_animation_lines() {
    for (int i = 0; i < 5; ++i) {
        std::cout << "\033[A\033[2K"; // Move cursor up one line and clear it
    }
}

void play_add_animation(const std::string& filename) {
    // Frame 1: Stick figure holding the file above the open box container
    std::cout << "  O        [" << filename << "] \n";
    std::cout << " /|\\       |     |   A D D I N G . . .\n";
    std::cout << " / \\       |     |                    \n";
    std::cout << "           |     |                    \n";
    std::cout << "           +-----+\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    clear_animation_lines();

    // Frame 2: Stick figure dropping the file halfway into the open box
    std::cout << "  O                             \n";
    std::cout << " \\|/       |     |   A D D I N G . . .\n";
    std::cout << " / \\       |[" << filename << "]|\n";
    std::cout << "           |     |                    \n";
    std::cout << "           +-----+\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    clear_animation_lines();

    // Frame 3: File settled at the bottom of the open box, success state
    std::cout << "  O                             \n";
    std::cout << " /|\\       |     |   A D D E D !      \n";
    std::cout << " / \\       |     |                    \n";
    std::cout << "           |[" << filename << "]|\n";
    std::cout << "           +-----+\n";
    
    // Print a trailing newline so subsequent terminal logs don't overwrite the final frame
    std::cout << "\n";
}