# mqtt-test

All the tests mentioned were run on a PC with the following specs:

```
AMD Ryzen 1700X 8C 16T @3.4GHx
Nvidia GTX 1080Ti 11GB
16GB DDR4 RAM 2600MHz
Mushkin 1TB SATA SSD
```

For each model, I ran the client-server program at least once, each time with 100,000 iterations (100,000 round-trips)


## TCP test (for reference)

`TCP_NODELAY` were enabled so every message was put in a separate TCP packet. Below is the result of a single test run with 100,000 round trips.

#### C client to C server

```
Total time elapsed: 0.758910122 seconds
Average time is 0.000007589 seconds(s) for 100000 iterations
Average round-trip per second: 131769 (rt/s)
```

## UDP test (for reference)


#### C client to C server

```
Total time elapsed: 0.681790973 seconds
Average time is 0.000006817 seconds(s) for 100000 iterations
Average round-trip per second: 146692 (rt/s)
```

## MMAP test

#### C client to C server
```
Total time elapsed: 0.057626285 seconds
Average time is 0.000000576 seconds(s) for 100000 iterations
Average round-trip per second: 1736111 (rt/s)
```

