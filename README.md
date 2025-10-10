# RAM Memory Simulator

This project implements a simple RAM memory simulator with the following specifications:
- Capacity: 512 positions × 64 bits = 4096 bytes (4 KB)
- Organization: Linear array
- Addressing: 0x0000 to 0x01FF (9 bits effective address)
- Data type: double (64 bits, IEEE 754)

## Building the Project

To build the project, simply run:

```bash
make
```

## Running the Tests

To run the tests without verbose output:
```bash
./ram_simulator
```

To run the tests with verbose output (debug mode):
```bash
./ram_simulator -v
```

The verbose mode will print detailed information about memory operations, including:
- Write operations and their values
- Read operations and their values
- Invalid address access attempts
- Memory clearing operations

## Project Structure

```
project/
├── src/
│   └── ram/
│       ├── ram.hpp
│       └── ram.cpp
├── test/
│   ├── ram_test.hpp
│   └── ram_test.cpp
├── Makefile
├── main.cpp
└── README.md
```

## Requirements

- C++20 compatible compiler
- Make build system