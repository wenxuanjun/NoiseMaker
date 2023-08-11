#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <iterator>
#include <algorithm>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace std::chrono;
using namespace std::this_thread;

constexpr int swing = 35;
constexpr int max_pitch_length = 16;
constexpr int clock_frequency = 1190000;

static int current_bpm = 120;

using pitch = struct {
    std::string tone[max_pitch_length];
    int length;
};

inline static int duration_to_microseconds(int time) {
    return 4000000 / time * 60 / current_bpm;
}

static double get_freq(const std::string& tone) {
    int midi = 0;
    const auto note = tone[0];
    const auto octave = tone[1] - '0';
    const auto accidental = (tone.size() > 2) ? tone[2] : ' ';
    const std::map<char, int> note_to_midi {
        {'C', 0}, {'D', 2}, {'E', 4}, {'F', 5}, {'G', 7}, {'A', 9}, {'B', 11}
    };
    midi += note_to_midi.at(note);
    midi += (octave + 1) * 12;
    if (accidental == '#') midi++;
    if (accidental == 'b') midi--;
    const double ratio = std::pow(2, 1.0 / 12.0);
    const double base = 220.0 * std::pow(ratio, 3) * std::pow(0.5, 5);
    return base * std::pow(ratio, midi);
}

static void play_beep(int console, std::string pitch, int time) {
    int magical_number = clock_frequency / get_freq(pitch);
    ioctl(console, 0x4B2F, magical_number);
    sleep_for(microseconds(time));
    ioctl(console, 0x4B2F, 0);
}

static void play_pitch(int console, pitch pitch) {
    play_beep(console, pitch.tone[0], duration_to_microseconds(pitch.length));
}

static void play_chord(int console, pitch pitch, int length) {
    auto time_ms = duration_to_microseconds(pitch.length) / (1000 * swing);
    for (int i = 0; i < time_ms; ++i) {
        play_beep(console, pitch.tone[i % length], 1000 * swing);
    }
}

static void play(int console, pitch pitch) {
    if (pitch.tone[1][0]) {
        std::cout << "chord" << ' ';
        int size = std::count_if(
        	std::begin(pitch.tone),
        	std::end(pitch.tone),
        	[](const std::string& s) { return !s.empty(); }
        );
        std::copy_n(
        	std::begin(pitch.tone), size,
        	std::ostream_iterator<std::string>(std::cout, " ")
        );
        std::cout << std::endl;
        play_chord(console, pitch, size);
    } else {
        std::cout << "pitch" << ' ';
        std::cout << pitch.tone[0] << std::endl;
        if (pitch.tone[0][0] == 'R')
            sleep_for(microseconds(duration_to_microseconds(pitch.length)));
        else
            play_pitch(console, pitch);
    }
}

static void read(int console, std::string str) {
    std::vector<std::string> result {
    	std::istream_iterator<std::string>(
    		std::istringstream(str) >> std::ws
    	),
    	std::istream_iterator<std::string>()
    };
    if (result[0] == "BPM") {
        current_bpm = std::stoi(result[1]);
        return;
    }
    if (result[0] == "R") {
        play(console, {{"R"}, std::stoi(result[1])});
    } else {
        pitch lin;
        int key = 0;
        for (const auto &note : result)
            if (std::isalpha(note[0]))
                lin.tone[key++] = note;
        lin.length = std::stoi(result.back());
        play(console, lin);
    }
}

static void line_process(std::string &line, std::string comment_str = "%") {
    std::replace_if(line.begin(), line.end(), [](char ch){
        return ch == ';' || ch == '\r' || ch == '\n';
    }, ' ');
    line.erase(0, line.find_first_not_of(" "));
    line.erase(line.find_last_not_of(" ") + 1);
    auto comment_start = line.find_first_of(comment_str);
    if (comment_start != std::string::npos)
        line.erase(comment_start);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Command: ./sound.o <data.txt>" << std::endl;
        exit(1);
    }
    int console;
    if ((console = open("/dev/console", O_WRONLY)) == -1) {
        fprintf(stderr, "You need to run it with Root permission\n");
        exit(1);
    }
    std::ifstream infile(argv[1]);
    if (!infile) {
        std::cerr << "Unable to read this file!\n";
        throw std::runtime_error("Failed to open file");
    }
    std::string str;
    while (std::getline(infile, str)) {
        line_process(str);
        if (str.empty()) continue;
        read(console, str);
    }
    infile.close();
    return 0;
}
