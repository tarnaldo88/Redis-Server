# Redis-Server
Redis Server built from scratch

Redis-Server is a Redis-like server implemented in modern C++ (C++17). It speaks the Redis Serialization Protocol (RESP), supports a practical subset of Redis commands across Strings, Lists, and Hashes, and persists data periodically to a simple dump file.

## Overview
This project is an educational implementation of a Redis-style in-memory data store:
- Single binary `my_redis_server` built with a portable Makefile
- TCP server with a multithreaded per-connection model
- RESP parsing for compatibility with `redis-cli`
- In-memory data structures: strings, lists, and hashes
- Basic persistence: load on startup and background dump every 5 minutes to `dump.my_rdb`
- Graceful shutdown with SIGINT (Ctrl+C) triggers a final dump

## Features
- RESP protocol support (arrays and bulk strings) with a whitespace fallback for testing
- Keys: set/get/delete/type/expire/rename
- Lists: push/pop, index/set/remove, fetch all elements
- Hashes: set/get/exists/del/len/keys/vals/getall/mset
- Background persistence to `dump.my_rdb`

## Build
Prerequisites: a C++17-capable compiler and `make` (tested on Linux with g++ and pthreads).

Commands:
- Build: `make`
- Clean: `make clean`
- Rebuild: `make rebuild`
- Run: `make run`

The build produces the `my_redis_server` binary in the repository root.

## Run
Run the server on the default port 6379:

```bash
./my_redis_server
```

Or specify a custom port:

```bash
./my_redis_server 6380
```

On startup the server attempts to load `dump.my_rdb` (if present). A background thread persists the database every 300 seconds. On shutdown (SIGINT/Ctrl+C), a final dump is performed.

## Using with redis-cli
Because the server speaks RESP, you can use `redis-cli` to interact with it.

```bash
redis-cli -p 6379 PING
redis-cli -p 6379 ECHO "hello"
redis-cli -p 6379 SET user:1 "alice"
redis-cli -p 6379 GETSET user:1 "alice"
redis-cli -p 6379 GET user:1
redis-cli -p 6379 KEYS "*"      # Note: pattern filtering is not implemented; returns all keys
redis-cli -p 6379 TYPE user:1
redis-cli -p 6379 DEL user:1     # Single key
redis-cli -p 6379 EXPIRE user:1 60
redis-cli -p 6379 RENAME old new

# Lists
redis-cli -p 6379 LPUSH mylist a b c
redis-cli -p 6379 LLEN mylist
redis-cli -p 6379 LGET mylist        # Non-standard: returns all elements
redis-cli -p 6379 LINDEX mylist 1
redis-cli -p 6379 LSET mylist 1 x
redis-cli -p 6379 LREM mylist 1 x
redis-cli -p 6379 LPOP mylist
redis-cli -p 6379 RPOP mylist

# Hashes
redis-cli -p 6379 HSET user:1 name alice
redis-cli -p 6379 HGET user:1 name
redis-cli -p 6379 HEXISTS user:1 name
redis-cli -p 6379 HDEL user:1 name
redis-cli -p 6379 HLEN user:1
redis-cli -p 6379 HKEYS user:1
redis-cli -p 6379 HVALS user:1
redis-cli -p 6379 HGETALL user:1
redis-cli -p 6379 HMSET user:1 name alice city paris
```

## Supported commands
Common:
- `PING`
- `ECHO <message>`
- `FLUSHALL`

Strings (key/value):
- `SET <key> <value>`
- `GETSET <key> <value>`
- `GET <key>`
- `KEYS` (returns all keys; pattern matching is not implemented)
- `TYPE <key>`
- `DEL <key>` / `UNLINK <key>`
- `EXPIRE <key> <seconds>`
- `RENAME <old> <new>`

Lists:
- `LLEN <key>`
- `LGET <key>` (non-standard; returns all elements)
- `LINDEX <key> <index>`
- `LSET <key> <index> <value>`
- `LREM <key> <count> <value>`
- `LPUSH <key> v1 [v2 ...]`
- `RPUSH <key> v1 [v2 ...]`
- `LPOP <key>` / `RPOP <key>`

Hashes:
- `HSET <key> <field> <value>`
- `HGET <key> <field>`
- `HEXISTS <key> <field>`
- `HDEL <key> <field>`
- `HLEN <key>`
- `HKEYS <key>`
- `HVALS <key>`
- `HGETALL <key>`
- `HMSET <key> <f1> <v1> [f2 v2 ...]`

## Protocol notes
- Primary input is RESP Arrays and Bulk Strings.
- Whitespace-delimited commands are also accepted for convenience in simple clients.

## Persistence
- On startup: attempts to load `dump.my_rdb`.
- Background save: every 5 minutes.
- On shutdown: a final dump is attempted.

## Project structure
- `src/` server, command handling, and main entrypoint
- `include/` public headers (`RedisServer.h`, `RedisDatabase.h`, `RedisCommandHandler.h`)
- `Makefile` build rules (`make`, `make run`, `make clean`)
- `my_redis_server` compiled binary (after build)
- `UseCases.md` usage notes and examples

## Limitations and notes
- This is an educational project; it is not production-ready.
- `KEYS` returns all keys; glob patterns are not supported.
- `DEL/UNLINK` handle a single key.
- Persistence format is a simple dump, not compatible with Redis RDB/AOF.
- No authentication, clustering, replication, transactions, or pub/sub.

## Roadmap ideas
- Improve RESP parsing robustness and error messages
- Add pattern support to `KEYS` and multi-key operations
- Add additional data structures (sets, sorted sets)
- More comprehensive tests and benchmarking



### Legacy notes
The original notes for planned topics and command summaries are preserved below.
# IDEAS for project and things to implement through the course of building this Redis Server
1. TCP/IP & socket programming
2. Concurrencyt and Multithreading
3. Mutex & synchronization
4. Data Structures - Hash tables, vectors
5. Parsing & RESP protocol
6. File I/O presitance
7. Signal Handling
8. Command Processing & Response formatting
9. Singleton Pattern
10. Bitwise Operators'|='
11. std:: libraries

## Common Commands
*PING*
Use case: Before embarking on any data operations, an application can send a PING to ensure that the Redis server is alive and responsive—like knocking on a door before entering.

*ECHO*
Use case: A debugging tool or simple utility to test network connectivity by having the server repeat a message. It can also be used in logging systems to trace commands.

*FLUSHALL*
Use case: When resetting a cache or starting fresh, FLUSHALL clears all stored keys. This is useful during development or when you need to wipe out stale data completely.

## Key/Value Operations
### SET
Use case: Caching user session information or configuration settings. For example, store a user token with SET session:123 "user_data".

### GET
Use case: Retrieve stored configuration or session data. For instance, fetch the user session with GET session:123.

### GETSET
Use case: Sets key to value and returns the old value stored at key.

### KEYS
Use case: List all keys or keys matching a pattern (e.g., KEYS session:*) to analyze cache usage or to perform maintenance tasks.

### TYPE
Use case: Check what type of value is stored at a key—useful when the data structure can vary, such as determining if a key holds a string, list, or hash.

### DEL / UNLINK
Use case: Remove keys that are no longer valid. This might be used to evict a stale cache entry after a user logs out or when cleaning up expired data.

### EXPIRE
Use case: Set a timeout on keys for caching. For example, cache product details for 3600 seconds so that the cache automatically evicts old data.

### RENAME
Use case: When restructuring keys during a migration or data reorganization, use RENAME to change the key’s name without losing its data.

## Lists
### LGET: 
LGET key → all elements
### LLEN: 
LLEN key → length
### LPUSH/RPUSH: 
LPUSH key v1 [v2 ...] / RPUSH → push multiple
### LPOP/RPOP: 
LPOP key / RPOP key → pop one
### LREM: 
LREM key count value → remove occurrences
### LINDEX: 
LINDEX key index → get element
### LSET: 
LSET key index value → set element


## Hashes
### HSET: 
HSET key field value
### HGET: 
HGET key field
### HEXISTS: 
HEXISTS key field
### HDEL:
HDEL key field
### HLEN:
HLEN key → field count
### HKEYS: 
HKEYS key → all fields
### HVALS: 
HVALS key → all values
### HGETALL: 
HGETALL key → field/value pairs
### HMSET: 
HMSET key f1 v1 [f2 v2 ...]