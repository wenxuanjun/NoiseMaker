#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

static int bpm = 120;
static const int swing = 15;

typedef struct {
    string tone[16];
    int length;
} pitch;

inline int get_time(int time) {
    return 4000000 / time * 60 / bpm;
}

inline void rest(int time) {
    usleep(get_time(time));
}

double get_freq(string tone) {
    int midi = 0;
    switch (tone[0]) {
        case 'C': midi += 0; break;
        case 'D': midi += 2; break;
        case 'E': midi += 4; break;
        case 'F': midi += 5; break;
        case 'G': midi += 7; break;
        case 'A': midi += 9; break;
        case 'B': midi += 11; break;
    }
    midi += ((int)tone[1] - 47) * 12;
    if (tone[2] == '#') midi++;
    if (tone[2] == 'b') midi--;
    double ratio = pow(2, 1 / 12.0);
    double base = 220.0 * pow(ratio, 3) * pow(0.5, 5);
    return base * pow(ratio, midi);
}

void play_pitch(int console, pitch pitch) {
    int magical_number = 1190000 / get_freq(pitch.tone[0]);
    ioctl(console, 0x4B2F, magical_number);
    rest(pitch.length);
    ioctl(console, 0x4B2F, 0);
}

void play_chord(int console, pitch pitch, int length) {
    for (int i = 0; i < get_time(pitch.length) / (1000 * swing); i++) {
        int magical_number = 1190000 / get_freq(pitch.tone[i % length]);
        ioctl(console, 0x4B2F, magical_number);
        usleep(1000 * swing);
        ioctl(console, 0x4B2F, 0);
    }
}

void play(int console, pitch pitch) {
    if (pitch.tone[1][0]) {
        cout << "chord" << ' ';
        int size = 0; 
        for (int o = 0; o < 16; o++) {
            if (pitch.tone[o][0])
                size++;
            else
                break;
        }
        for (int o = 0; o < size; o++)
            cout << pitch.tone[o % size] << ' ';
        cout << endl;
        play_chord(console, pitch, size);
    } else {
        cout << "pitch" << ' ';
        cout << pitch.tone[0] << endl;
        if (pitch.tone[0][0] == 'R')
            rest(pitch.length);
        else
            play_pitch(console, pitch);
    }
}

void read(int console, string str) {
    string result;
    vector<string> res;
    stringstream input;
    input << str;
    while (input >> result) res.push_back(result);
    if (res[0] == "BPM") {
        bpm = atoi(res[1].c_str());
        return;
    }
    if (res[0] == "R") {
        play(console, {{"R"}, atoi(res[1].c_str())});
    } else {
        pitch lin;
        int key = 0;
        for (key; key < res.size(); key++)
            if (isalpha(res[key][0])) lin.tone[key] = res[key];
        lin.length = atoi(res[key - 1].c_str());
        play(console, lin);
    }
}

void line_process(string &line, string comment_str = "%") {
    for (char &ch : line)
        if (ch == ';' || ch == '\r' || ch == '\n') ch = ' ';
    line.erase(0, line.find_first_not_of(" "));
    line.erase(line.find_last_not_of(" ") + 1);
    int comment_start = line.find_first_of(comment_str);
    if (comment_start != string::npos)
        line.erase(comment_start);
}

int main(int argc, char *argv[]) {
    if (!argv[1]) {
        cout << "Command: ./sound.o <data.txt>" << endl;
        exit(1);
    }
    ifstream infile;
    infile.open(argv[1]);
    if (!infile.is_open()) {
        cout << "Unable to read this file!" << endl;
        exit(1);
    }
    int console;
    if ((console = open("/dev/console", O_WRONLY)) == -1) {
        fprintf(stderr, "You need to run it with Root permission\n");
        exit(1);
    }
    string str;
    while (getline(infile, str)) {
        line_process(str);
        if (str.empty()) continue;
        read(console, str);
    }
    infile.close();
    return 0;
}
