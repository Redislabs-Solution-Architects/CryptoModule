
#include "../redismodule.h"

#include <mcrypt.h>

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int keysize = 16; /* 128 bits */
static char * IV = "qwdf*32NaR1)92<k";
static char * key = "GH^738ahE12NHa*gw";
static int blocksize;

int encrypt(
    void* buffer,
    int buffer_len, /* Because the plaintext could include null bytes*/
    char* IV, 
    char* key,
    int key_len 
){
  MCRYPT td = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
  int blocksize = mcrypt_enc_get_block_size(td);
  if( buffer_len % blocksize != 0 ){return 1;}

  mcrypt_generic_init(td, key, key_len, IV);
  mcrypt_generic(td, buffer, buffer_len);
  mcrypt_generic_deinit (td);
  mcrypt_module_close(td);
  
  return 0;
}

int decrypt(
    void* buffer,
    int buffer_len,
    char* IV, 
    char* key,
    int key_len 
){
  MCRYPT td = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
  int blocksize = mcrypt_enc_get_block_size(td);
  if( buffer_len % blocksize != 0 ){return 1;}
  
  mcrypt_generic_init(td, key, key_len, IV);
  mdecrypt_generic(td, buffer, buffer_len);
  mcrypt_generic_deinit (td);
  mcrypt_module_close(td);
  
  return 0;
}

int CryptoGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 2) return RedisModule_WrongArity(ctx);

    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"GET","s",argv[1]);

    size_t bufsize;
    const char * repVal = RedisModule_CallReplyStringPtr(reply, &bufsize);
    char * cryptVal = RedisModule_Alloc(bufsize);
    memcpy(cryptVal, repVal, bufsize);
    decrypt(cryptVal, bufsize, IV, key, keysize);
    
    size_t len = *(size_t *)&cryptVal[0];
    
    RedisModule_ReplyWithStringBuffer(ctx, cryptVal + sizeof(size_t), len);
    RedisModule_FreeCallReply(reply);
    RedisModule_Free(cryptVal);
    return REDISMODULE_OK;
}

int CryptoSet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);
    
    size_t len;
    const char * inVal = RedisModule_StringPtrLen(argv[2], &len);
    size_t bufsize = len + sizeof(size_t) + blocksize - ((len + sizeof(size_t)) % blocksize);
    char * cryptVal = RedisModule_Alloc(bufsize);
    memset(cryptVal, 0, bufsize);
    memcpy(cryptVal, &len, sizeof(size_t));
    memcpy(cryptVal + sizeof(size_t), inVal, len);
    encrypt(cryptVal, bufsize, IV, key, keysize);
    
    RedisModuleString* value = RedisModule_CreateString(ctx, cryptVal, bufsize);
    
    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"SET","ss", argv[1], value);
    RedisModule_ReplyWithCallReply(ctx,reply);
    RedisModule_FreeCallReply(reply);
    RedisModule_FreeString(ctx, value);
    RedisModule_Free(cryptVal);
     
    return REDISMODULE_OK;
}

/* This function must be present on each Redis module. It is used in order to
 * register the commands into the Redis server. */
int RedisModule_OnLoad(RedisModuleCtx *ctx) {
    if (RedisModule_Init(ctx,"crypto",1,REDISMODULE_APIVER_1)
        == REDISMODULE_ERR) return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.get",
        CryptoGet_RedisCommand,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.set",
        CryptoSet_RedisCommand,"write",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    MCRYPT td = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
    blocksize = mcrypt_enc_get_block_size(td);

    return REDISMODULE_OK;
}
