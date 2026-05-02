# Edda

**A Correct-by-Construction Framework for Designing High Utilization Real-Time Networks**

> This repository is the artifact accompanying a paper currently under review.

---

## Overview

Edda is a framework for designing real-time networks (RTNs) with provably correct temporal guarantees. It addresses the core challenge of admitting as many flows as possible while ensuring all strict end-to-end deadlines are met.

Edda combines key contributions:

- **Tighter delay bounds** — a new mathematical model that estimates temporal constraints more precisely than prior work
- **Flexible runtime path assignment** — runtime schemes that adapt to network conditions while preserving guarantees

Edda admits **20% more flows** than the state-of-the-art while discovering **10% more paths** than competing systems.

---

## Repository Structure

```
edda/
├── data/                                 # topologies   
├── src/
│   ├── main.cpp                          # Entry point
│   ├── CMakeLists.txt                    # Build configuration
│   ├── util.cpp                          # Utility functions
│   ├── delay_modeling/                   # Delay bound computation
│   ├── design_space_exp/                 # Design space exploration
│   │   ├── design_space_exp.cpp
│   │   ├── design_space_exp.h
│   │   ├── flow_gen.cpp                  # Flow generation
│   │   └── flow_gen.h
│   ├── routing_strategies/               # Path assignment schemes
│   │   ├── adaptive_routing.cpp
│   │   └── adaptive_routing.h
│   ├── modules/                          # Factory implementations
│   │   ├── flow_factory.cpp
│   │   ├── netdesign_factory.cpp
│   │   ├── netview_factory.cpp
│   │   └── topo_factory.cpp
├── tools/                                # Python analysis and plotting scripts
├── external/                             # Third-party dependencies
└── README.md
```

---

## Requirements

- C++17 or later
- CMake 3.15+
- Python 3.8+ (for analysis tools)
- pugixml
- Python packages: `numpy`, `matplotlib`, `pandas`

---

## Building

```bash
cd src/
cmake . && make
```

---

## Running

```bash
Usage: ./edda [OPTIONS]

Options:
 -f, --file <path> Process a single XML topology file with specified number of flows
      Requires: -n <num_flows>
 -d, --directory <path> Process all XML files in directory (design space exploration)
      Requires: -n <num_flows>
 -a, --fullspec <path> Process a single XML topology file with in-built info on flow specs
 -n, --numflows <number> Number of flows to generate
 -h, --help Display this help message
```
---