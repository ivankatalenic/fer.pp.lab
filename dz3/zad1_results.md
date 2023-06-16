# Tests

# CPU vs. GPU vs. GPU optimized code

Parameters:
- Work size = 100 000
- Work per thread = 2

Results:
| Implementation | Time      | Multiplier |
| ---            | ---:      | ---:       |
| CPU            | 446.775 s | 190.36x    |
| GPU suboptimal | 2.570 s   | 1.10x      |
| GPU optimized  | 2.347 s   | 1.00x      |

# GPU vs. GPU optimized code

Parameters:
- Work size = 100 000

Results:
| Work per thread | GPU suboptimal | GPU optimized | Diff    |
| ---             | ---            | ---           | ---:    |
| 1               | 2373 ms        | 2389 ms       | -0.67 % |
| 2               | 2488 ms        | 2386 ms       | 4.10 %  |
| 4               | 3134 ms        | 2397 ms       | 23.52 % |
| 8               | 3894 ms        | 2833 ms       | 27.24 % |
| 16              | 4181 ms        | 3653 ms       | 12.62 % |
| 32              | 4440 ms        | 3647 ms       | 17.86 % |
| 64              | 8924 ms        | 4582 ms       | 48.66 % |
| 128             | 12663 ms       | 5992 ms       | 52.68 % |
| 256             | 38706 ms       | 23714 ms      | 38.73 % |
| 512             | 58868 ms       | 47307 ms      | 19.64 % |
| 1024            | 90619 ms       | 56464 ms      | 37.69 % |
| 2048            | 163543 ms      | 81676 ms      | 50.06 % |
| 4096            | 301230 ms      | 152480 ms     | 49.38 % |
