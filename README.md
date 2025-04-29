# MemTrace - Linux Memory Tracer

MemTrace is a tiny routine(I wish@_@) for monitoring memory usage of processes in Linux systems. It can track both user and kernel processes, displaying real-time memory information and generating visualizations of memory usage patterns over time.
But kernel process judge maybe not so accurate, and monitoring depends on /proc filesystem update frequency.


## Features

- **Process Memory Monitoring**: Tracks key memory metrics including:
  - Virtual Memory Size (VSZ)
  - Physical Memory Usage (RSS)
  - Data Segment Size
  - Stack Space Usage

- **Kernel vs User Process Detection**: Automatically identifies and handles different process types
- **Continuous Monitoring**: Option to monitor processes over time
- **Memory Visualization**: Command-line charts of memory usage history
- **Logging**: Comprehensive logging of memory statistics for later analysis


## How It Works

MemTrace accesses the `/proc` filesystem in Linux to read memory statistics for any specified process. It differentiates between user and kernel processes and displays the appropriate metrics for each.


## Usage

parameters usage:
- -i: interval in seconds, only works in continuous monitoring mode.
- -c: continuous monitoring mode: time interval in seconds, default is 1 second.
- -l: write log to file.
example:
```bash
$ ./memtrc trace 1234 -c -i 5 -l xxx.log(or xxx.txt)
```
NOTE: log file will be created and saved in the current directory, or you could to use absolute path, like: /log/xxx.log. 
second,wriete_log() dose not enforce the specifiction of text file type.recommend to use *.log or *.txt as the file extension.


## Building and Running

- make install - need root permission, it will be installed to /usr/local/bin/
- make uninstall - remove the project from local machine.
- make clean - remove all object files and executables.
- make test - build test executable.

## append instructions

your system should be Linux, and glibc version >= 2.0, and gcc version >= 4.0. I give up some analysis in Makefile, so don't need clang dependency or others. sure, you might already have it :P


### Design Philosophy

This project represents a systematic approach to process memory tracing, developed as a learning journey in C programming. It demonstrates:

- Modular architecture
- Comprehensive testing
- Advanced build system (Makefile)
- Practical system programming techniques

### source files achitecture
memtrc/
├── include/        # header files
│   ├── chart.h     # chart related declarations
│   └── memtrc.h    # core functionality declarations
├── chart.c         # chart related functions
├── main.c          # main function
├── memtrc.c        # core functionality
├── test.c          # test units
├── Makefile        # makefile for building the project
├── README.md       # project description


#### Experience

This small project was created as a learning exercise in C programming, focusing on system-level programming and process'memory information retrieval in Linux. of course, it's so little and non-ready for production use, therefore, it doesn't any seem like lincences. just for paractice and learning purpose.

I'm still striving to learn and improve my C programming and Linux system programming skills. certainly, it's not easy,I would to enjoy that process and find funny. Thanks for your reading, and hope you gain something from it. :-)
