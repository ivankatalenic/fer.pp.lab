# Tests

## Task (work per thread) count benchmark

Parameters:
- Work size (total tasks) = 1e8
- Work group size = Left to the implementation to decide

Results:
| Work per thread | Duration (ms) |
| ---:            | ---:          |
| 1               | 985           |
| 10              | 460           |
| 100             | 377           |
| 1000            | 374           |
| 1e4             | 377           |
| 1e5             | 1185          |
| 1e6             | 4992          |
| 1e7             | 28477         |

## Work group size benchmark
Parameters:
- Work size = 1e8
- Work per thread = 100

Results:
| Work group size | Duration (ms) |
| ---:            | ---:          |
| 1               | 6273          |
| 10              | 836           |
| 100             | 443           |
| 1000            | 382           |
| 1e4             | Error         |
| 32              | 370           |
| 64              | 389           |
| 128             | Error         |

## GPU vs. CPU
Parameters:
- Work per thread = 100
- Work group size = Left to the implementation to decide

Results:
| Work size | CPU (ms) | GPU (ms) | CPU/GPU |
| ---:      | :---:    | :---:    | ---:    |
| 1e3       | 17       | 174      | 0.10x   |
| 1e4       | 17       | 172      | 0.10x   |
| 1e5       | 23       | 184      | 0.13x   |
| 1e6       | 72       | 202      | 0.36x   |
| 1e7       | 564      | 214      | 2.64x   |
| 1e8       | 5483     | 378      | 14.5x   |
| 1e9       | 54644    | 2237     | 24.4x   |
| 1e10      | -        | 19619    | -       |
