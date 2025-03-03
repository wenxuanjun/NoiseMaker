#include <chrono>
#include <cmath>
#include <fstream>
#include <iterator>
#include <print>
#include <ranges>
#include <iostream>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace std::chrono;
using namespace std::this_thread;

constexpr int swing = 35;
constexpr int clock_frequency = 1193180;

using Pitch = struct {
    std::vector<std::string> tones;
    int length;
};

struct MusicState {
    int bpm = 120;
    
    [[nodiscard]] constexpr auto to_ms(int time) const -> int {
        return 4000000.0 / time * 60 / bpm;
    }
};

inline MusicState music_state;

auto note_to_midi = [](char note) -> int {
    switch (note) {
        case 'C': return 0;
        case 'D': return 2;
        case 'E': return 4;
        case 'F': return 5;
        case 'G': return 7;
        case 'A': return 9;
        case 'B': return 11;
        default: throw std::out_of_range("Invalid note");
    }
};

auto get_freq(std::string_view tone) -> double {
    double ratio = std::pow(2.0, 1.0 / 12.0);
    double base = 220.0 * std::pow(ratio, 3) * std::pow(0.5, 5);
    
    int midi = note_to_midi(tone.at(0));
    midi += (tone.at(1) - '0' + 1) * 12;
    
    if (tone.size() > 2) {
        midi += (tone.at(2) == '#') - (tone.at(2) == 'b');
    }
    
    return base * std::pow(ratio, midi);
}

auto beep(int console, std::string_view pitch, int ms) {
    if (pitch == "R") {
        sleep_for(microseconds(ms));
        return;
    }
    int magical_number = clock_frequency / get_freq(pitch);
    ioctl(console, 0x4B2F, magical_number);
    sleep_for(microseconds(ms));
    ioctl(console, 0x4B2F, 0);
}

auto play(int console, const Pitch& pitch) {
    const auto duration = music_state.to_ms(pitch.length);

    if (pitch.tones.size() == 1) {
        std::println("pitch: {}", pitch.tones.at(0));
        beep(console, pitch.tones.at(0), duration);
    } else {
        auto tones = pitch.tones | std::views::join_with(' ') |
                     std::ranges::to<std::string>();
        std::println("chord: {}", tones);
        const auto time_per_note = swing * 1000;
        const int iterations = duration / time_per_note;

        for (int i = 0; i < iterations; ++i) {
            const auto note_idx = i % pitch.tones.size();
            beep(console, pitch.tones.at(note_idx), time_per_note);
        }
    }
}

auto read(int console, const std::string &str) {
    auto result = str | std::views::split(' ') |
                  std::ranges::to<std::vector<std::string>>();

    if (result.at(0) == "BPM") {
        music_state.bpm = std::stoi(result.at(1));
        return;
    }
    if (result.at(0) == "R") {
        play(console, {{"R"}, std::stoi(result.at(1))});
        return;
    }

    auto is_tone = [](const std::string &s) { return std::isalpha(s.at(0)); };
    auto tones = result | std::views::filter(is_tone) |
                 std::ranges::to<std::vector<std::string>>();
    play(console, {tones, std::stoi(result.back())});
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::println("Command: ./sound.o <data.txt>");
        exit(1);
    }

    int console;
    if ((console = open("/dev/console", O_WRONLY)) == -1) {
        fprintf(stderr, "You need to run it with Root permission\n");
        exit(1);
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::println(std::cerr, "Unable to read this file!");
        throw std::runtime_error("Failed to open file");
    }

    for (std::string line; std::getline(file, line);) {
        auto rule = [](char c) { return c == '\r' || c == '\n'; };
        std::replace_if(line.begin(), line.end(), rule, ' ');

        line.erase(0, line.find_first_not_of(" "));
        line.erase(line.find_last_not_of(" ") + 1);

        if (!line.empty() && !line.starts_with("%")) {
            read(console, line);
        }
    }

    return 0;
}
