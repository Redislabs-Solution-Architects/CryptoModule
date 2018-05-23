# Crypto Mudule

A module for data encryption in Redis

## Overview
The crypto module encrypts values just before storing them in redis and decrypts them just after reading them from redis. 
The values are encrypted while they are stored in redis, including when persisted to disk or replicated to slaves.
It supports dynamic loading of a crypto library, so users can use any encryption library they prefer.
The module supports strings and hashes. It uses the same syntax as the original commands, but the command name is prepended by "CRYPTO.".

##Compile
Move to the src directory and run
```make```

## Loading
The crypto module can be loaded, just like any other module, through a command or configuration.
It accept a single parameter which is the shared object of the crypto library to use.
To load through the configuration file, add the following line to redis.conf:
```
loadmodule cryptomodule.so mcrypto.so
```

You need to provide the full path for the .so files.

## Loading in Redis Enterprise
Create a zip file containing the cryptomodule.so and the module.json file.
Load it through the Redis Enterprise UI.
Copy the mcrypto.so to /opt/redislabs/lib/modules/ on all nodes.
Create a database and choose the Crypto Module from the list of modules.

## Crypto library
The crypto module supports multiple crypto libraries and and through that, supports multiple encryption methods.
The module dynamically loads the crypto library provided as a parameter.
In order for a crypto library to be used by the module, it needs to export the following funcrtions:

```
int encrypt(void* buffer, int buffer_len)
```
Performs an inplace encryption of the input buffer with the input size

```
int decrypt(void* buffer, int buffer_len)
```
Performs an inplace decryption of the input buffer with the input size

```
int blocksize()
```
Returns the cypher block size. It will make sure that the buffer for encryption is allocated according to the block size.

The module comes with 2 crypto libraries:
* simplecrypto - This library encrypts data by performing a bitwise XOR operation with some random data. It mainly serves as an example of a crypto library.
* mcrypto - This library uses the mcrypt library. You need to make sure that mcrypt is installed on the server in order to use it.

## Commands
The following commands are supported by the module:

```
crypto.get
crypto.set
crypto.mget
crypto.mset
crypto.append
crypto.getset
crypto.strlen
crypto.setnx
crypto.hget
crypto.hset
crypto.hmget
crypto.hmset
crypto.hgetall
crypto.hsetnx
crypto.hstrlen
crypto.hvals
```

The above commands are the same as the matching regular commands in terms of functionality and parameters. The difference is that the stored values are encrypted and the read values are decrypted.






