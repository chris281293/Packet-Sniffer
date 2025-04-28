# Network Package Sniffer
A lightweight C-based packet sniffer with a minimalistic TUI for real-time packet inspection. 

In this project, the following features are provided:

1. Captures packets from the default network interface using pcap library.
2. Implements a simple TUI (Text User Interface) with separate output and input windows.
3. Extracts and displays packet information in the TUI output window.
2. Supports basic commands entered in the input window to control the output and sniffer behavior.

## Demo
![](demo.gif)

## Dependencies
- Environment:
  - Linux
  - GCC
  - Make
- Libraries:
  - pcap
  - ncurses

## Build and Run
```bash
git clone https://github.com/chris281293/Packet-Sniffer.git

cd <project_dir>

make

sudo ./sniffer
```
