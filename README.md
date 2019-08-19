# ratelimit

PoC code for rate limit by socket

Just insert sleep into receive loop

## Build environment

Ubuntu 18.04 + kernel 4.15

## Server

### Usage

`usage: server [chunksize(in KiB)(default=100)]`

### Example

```
make server
./server 200
```

## Client

### Usage

`usage: client <destaddr> <duration(in sec)> <rate(in Kibps)> [chunksize(in KiB)(default=10)]`

### Example

```
make client
./client localhost 10 500 20
```

## License

MIT
