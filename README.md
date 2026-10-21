# Rock Paper Scissors with Markov-Chain AI

A console Rock Paper Scissors game written in C++ where the computer doesn't play randomly. It learns your habits using a **Markov chain** and tries to predict your next move. Your move history is saved to a JSON file, so the AI keeps learning across sessions.

## Features

- Play Rock, Paper, Scissors against an AI in the terminal
- Markov-chain move prediction based on your past moves
- Persistent player history stored in JSON (using [nlohmann/json](https://github.com/nlohmann/json))
- Running score display during a session

## How the AI Works

People are rarely truly random. For example, someone who just played Rock may be more likely to play Paper next.

1. The game records every move you make.
2. It builds a table of **transitions**: how often each move follows another (e.g., Rock → Paper, Rock → Rock, Rock → Scissors).
3. Using your last move, it looks up the most likely next move.
4. It then plays the move that beats that prediction.

The more you play, the more data it has, and the better its predictions get. Because the history is saved to disk, it remembers you next time.

## Tech Stack

- **Language:** C++ (C++17)
- **Library:** [nlohmann/json](https://github.com/nlohmann/json) for reading and writing history
- **Interface:** Console (terminal)

## Getting Started

### Prerequisites

- A C++ compiler with C++17 support (g++ or clang++)
- nlohmann/json

On Ubuntu/Debian:

```bash
sudo apt install g++ nlohmann-json3-dev
```

Or download `json.hpp` from the [nlohmann/json releases](https://github.com/nlohmann/json/releases) and place it next to the source files.

### Build

```bash
git clone https://github.com/Noordeepsinghv/rock-paper-scissors.git
cd rock-paper-scissors
g++ -std=c++17 main.cpp -o rps
```

> Replace `main.cpp` with your actual source file name(s) if different, e.g. `g++ -std=c++17 *.cpp -o rps`.

### Run

```bash
./rps
```

## Usage

Follow the on-screen prompts to choose Rock, Paper, or Scissors each round. Your moves are saved automatically so the AI can learn from them in later sessions.

## Project Structure

```
Will add soon!
```

## Roadmap

- [ ] Terminal UI using [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
- [ ] Web UI version
- [ ] Higher-order Markov chains (use the last 2-3 moves for prediction)
- [ ] Win-rate statistics across sessions

## What I Learned

- Applying Markov chains to a real problem
- Reading and writing JSON in C++ with an external library
- Structuring a C++ project and debugging logic errors in game and prediction code

## Author

**Noordeep Singh**
GitHub: [@Noordeepsinghv](https://github.com/Noordeepsinghv)
