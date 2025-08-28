# Redis-Server
Redis Server built from scratch

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
SET
Use case: Caching user session information or configuration settings. For example, store a user token with SET session:123 "user_data".

GET
Use case: Retrieve stored configuration or session data. For instance, fetch the user session with GET session:123.

KEYS
Use case: List all keys or keys matching a pattern (e.g., KEYS session:*) to analyze cache usage or to perform maintenance tasks.

TYPE
Use case: Check what type of value is stored at a key—useful when the data structure can vary, such as determining if a key holds a string, list, or hash.

DEL / UNLINK
Use case: Remove keys that are no longer valid. This might be used to evict a stale cache entry after a user logs out or when cleaning up expired data.

EXPIRE
Use case: Set a timeout on keys for caching. For example, cache product details for 3600 seconds so that the cache automatically evicts old data.

RENAME
Use case: When restructuring keys during a migration or data reorganization, use RENAME to change the key’s name without losing its data.

## Lists
LGET: LGET <key> → all elements
LLEN: LLEN <key> → length
LPUSH/RPUSH: LPUSH <key> <v1> [v2 ...] / RPUSH → push multiple
LPOP/RPOP: LPOP <key> / RPOP <key> → pop one
LREM: LREM <key> <count> <value> → remove occurrences
LINDEX: LINDEX <key> <index> → get element
LSET: LSET <key> <index> <value> → set element


## Hashes
HSET: HSET <key> <field> <value>
HGET: HGET <key> <field>
HEXISTS: HEXISTS <key> <field>
HDEL: HDEL <key> <field>
HLEN: HLEN <key> → field count
HKEYS: HKEYS <key> → all fields
HVALS: HVALS <key> → all values
HGETALL: HGETALL <key> → field/value pairs
HMSET: HMSET <key> <f1> <v1> [f2 v2 ...]