# ratelimit

PoC code for rate limit by socket

Just insert sleep into receive loop

## Usage
### Server

No command line options

```
make server
./server
```

### Client

`usage: client destaddr duration(in sec) rate(in Kibps)`

```
make client
./client localhost 10 500
```

## License
MIT
