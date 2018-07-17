# Crypto Module

A module for data encryption in Redis

## Overview
The crypto module encrypts values just before storing them in redis and decrypts them just after reading them from redis. 
The values are encrypted while they are stored in redis, including when persisted to disk or replicated to slaves.
It supports dynamic loading of a crypto library, so users can use any encryption library they prefer.
The module supports strings and hashes. It uses the same syntax as the original commands, but the command name is prepended by "CRYPTO.".

## Compile
Move to the src directory and run
```
make
```

## Loading
The crypto module can be loaded, just like any other module, through a command or configuration.
It accept a single parameter which is the shared object of the crypto library to use.
To load through the configuration file, add the following line to redis.conf:
```
loadmodule cryptomodule.so mcrypto.so
```

You need to provide the full path for the .so files.
You can also specify additional parameters to initialize the crypt library.

## Loading in Redis Enterprise
Create a zip file containing the cryptomodule.so and the module.json file.
Load it through the Redis Enterprise UI.
Copy the mcrypto.so to /opt/redislabs/lib/modules/ on all nodes.
Create a database and choose the Crypto Module from the list of modules.

## Crypto library
The crypto module supports multiple crypto libraries and and through that, supports multiple encryption methods.
The module dynamically loads the crypto library provided as a parameter.
In order for a crypto library to be used by the module, it needs to export the following functions:

```
int init(int argc, const char ** argv)
```
Initialize the crypt library. Return 0 on success or anything else on failure. Receives additional parameters to initialize the crypt library

```
int encrypt(void* buffer, int buffer_len)
```
Performs an in place encryption of the input buffer with the input size

```
int decrypt(void* buffer, int buffer_len)
```
Performs an in place decryption of the input buffer with the input size

```
int blocksize()
```
Returns the cypher block size. It will make sure that the buffer for encryption is allocated according to the block size.

The module comes with 2 crypto libraries:
* simplecrypto - This library encrypts data by performing a bitwise XOR operation with some random data. It mainly serves as an example of a crypto library.
* mcrypto - This library uses the mcrypt library. You need to make sure that mcrypt is installed on the server in order to use it.

## The mcrypto library
mcrypto is used as the main encryption library. It can be initialized with the following parameters:
* Algorithm - The encryption algorithm to use. The default algorithm is `rijndael-128`.
* Mode - The encryption mode. The default mode is `cbc`.
* key - The encryption key. If not provided, a key hardcoded in the module is used.
* IV - A random sequence of bytes used for block encryption. If not provided, an IV that matches the `rijndael-128` algorithm is used. If you cahnge the algorith, you need to provide an IV in the size of the algorithm encryption block.

The following table shows the supported algorithms with their block size and supported modes:

| Algorithm       | Block size (bytes) | Supported modes               |
|:---------------:|:------------------:|:-----------------------------:|
| cast-128        | 16         	       | cbc cfb ctr ecb ncfb nofb ofb
| gost            | 32                 | cbc cfb ctr ecb ncfb nofb ofb
| rijndael-128    | 32                 | cbc cfb ctr ecb ncfb nofb ofb
| twofish         | 32                 | cbc cfb ctr ecb ncfb nofb ofb
| arcfour         | 256                | stream
| cast-256        | 32                 | cbc cfb ctr ecb ncfb nofb ofb
| loki97          | 32                 | cbc cfb ctr ecb ncfb nofb ofb
| rijndael-192    | 32                 | cbc cfb ctr ecb ncfb nofb ofb
| saferplus       | 32                 | cbc cfb ctr ecb ncfb nofb ofb
| wake            | 32                 | stream
| blowfish-compat | 56                 | cbc cfb ctr ecb ncfb nofb ofb
| des             | 8                  | cbc cfb ctr ecb ncfb nofb ofb
| rijndael-256    | 32                 | cbc cfb ctr ecb ncfb nofb ofb
| serpent         | 32                 | cbc cfb ctr ecb ncfb nofb ofb
| xtea            | 16                 | cbc cfb ctr ecb ncfb nofb ofb
| blowfish        | 56                 | cbc cfb ctr ecb ncfb nofb ofb
| enigma          | 13                 | stream
| rc2             | 128                | cbc cfb ctr ecb ncfb nofb ofb
| tripledes       | 24                 | cbc cfb ctr ecb ncfb nofb ofb

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






