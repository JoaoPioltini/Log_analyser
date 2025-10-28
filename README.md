#  Log Analyzer (Sequential and Parallel Versions)

##  Overview
This project implements a **HTTP log analyzer** that processes an access log file and extracts:
- The total number of requests that returned **HTTP 404 (Not Found)**.
- The total number of **bytes transferred** in responses with **HTTP 200 (OK)**.

Two versions were developed:
- **Sequential version (`log_analyser_seq.c`)** – processes all lines in a single thread.
- **Parallel version (`log_analyser_par.c`)** – divides the workload among multiple threads using **POSIX Threads (Pthreads)**.

---

##  How It Works
Both programs load the entire log file into memory and process it line by line.

Each line of the log is parsed to extract the HTTP **status code** and **response size** (in bytes).  
The algorithms then:
1. Count all responses with status `404`.
2. Sum the bytes from responses with status `200`.

###  Parallel Version
The parallel implementation follows a **data decomposition** model:
1. The vector of log lines is split into *N* chunks (one per thread).
2. Each thread processes its own chunk independently.
3. Local results are accumulated in **global counters** protected by a **mutex**.
4. After all threads finish, the final global statistics are displayed.

This model allows scalable performance with different thread counts and file sizes.

---

##  Performance Example
| Threads | Execution Time (s) | Speedup | Efficiency |
|----------|--------------------|----------|-------------|
| 1 | 0.0774 | 1.00 | 100% |
| 2 | 0.0396 | 1.95 | 97.7% |
| 4 | 0.0232 | 3.34 | 83.4% |
| 8 | 0.0169 | 4.58 | 57.2% |
| 16 | 0.0160 | 4.84 | 30.2% |

> **Observation:** The speedup is significant up to 4 threads.  
Beyond that, synchronization overhead and Amdahl’s Law limit performance gains.

---

##  Files
| File | Description |
|------|--------------|
| `log_analyser_seq.c` | Sequential implementation. |
| `log_analyser_par.c` | Parallel implementation using Pthreads. |
| `makefile` | Compilation automation for both versions. |
| `access_log_sample.txt` | Small example log file (optional for tests). |
| `.gitignore` | Prevents large log files and compiled binaries from being tracked. |

---

##  Compilation
Run the provided **makefile**:

```bash
make
