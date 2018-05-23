
#include "../redismodule.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

int (*encrypt)(void *,int);
int (*decrypt)(void *,int);
int (*blocksize)();

size_t getCryptSize(size_t size)
{
   size_t cryptSize = size + sizeof(size_t); 
   int bsize = (*blocksize)();
   if ( bsize )
        cryptSize += bsize - (cryptSize % bsize);
   return cryptSize;
}

size_t get(RedisModuleCtx *ctx, const RedisModuleString * key, char **value)
{
    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"GET","s", key);
    if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR) 
    {
	RedisModule_ReplyWithCallReply(ctx, reply);
        RedisModule_FreeCallReply(reply);
        return -1;
    }

    if ( RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_NULL )
    {
        RedisModule_FreeCallReply(reply);
	*value = NULL;
	return 0;
    }

    size_t bufsize;
    const char * repVal = RedisModule_CallReplyStringPtr(reply, &bufsize);
    char * cryptVal = RedisModule_Alloc(bufsize);
    memcpy(cryptVal, repVal, bufsize);
    (*decrypt)(cryptVal, bufsize);

    size_t len = *(size_t *)&cryptVal[0];
    memmove(cryptVal, cryptVal + sizeof(size_t), len);
    *value = cryptVal;
    RedisModule_FreeCallReply(reply);
    return len;
}

RedisModuleCallReply * set(RedisModuleCtx *ctx, const RedisModuleString *key, const RedisModuleString * value)
{
    size_t len;
    const char * inVal = RedisModule_StringPtrLen(value, &len);
    size_t bufsize = getCryptSize(len);

    char * cryptVal = RedisModule_Alloc(bufsize);
    memset(cryptVal, 0, bufsize);
    memcpy(cryptVal, &len, sizeof(size_t));
    memcpy(cryptVal + sizeof(size_t), inVal, len);
    (*encrypt)(cryptVal, bufsize);

    RedisModuleString* cryptValue = RedisModule_CreateString(ctx, cryptVal, bufsize);

    RedisModuleCallReply *reply;
    reply = RedisModule_Call(ctx,"SET","!ss", key, cryptValue);
    RedisModule_FreeString(ctx, cryptValue);
    RedisModule_Free(cryptVal);
    return reply;
}

size_t mget(RedisModuleCtx *ctx, RedisModuleString ** keys, size_t len)
{
    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"MGET","v", keys, len);
  
    size_t size = RedisModule_CallReplyLength(reply);
    RedisModule_ReplyWithArray(ctx, size);
    size_t i = 0;
    size_t bufsize;
    char * cryptVal;
    for (;i < size; i++)
    {
        RedisModuleCallReply * strReply = RedisModule_CallReplyArrayElement(reply, i);
        if ( RedisModule_CallReplyType(strReply) == REDISMODULE_REPLY_NULL )
    	{
	    RedisModule_ReplyWithNull(ctx);
            RedisModule_FreeCallReply(strReply);
	    continue;
        }

        const char * repVal = RedisModule_CallReplyStringPtr(strReply, &bufsize);
        cryptVal = RedisModule_Alloc(bufsize);
        memcpy(cryptVal, repVal, bufsize);
        (*decrypt)(cryptVal, bufsize);

        size_t len = *(size_t *)&cryptVal[0];
        memmove(cryptVal, cryptVal + sizeof(size_t), len);
        RedisModule_ReplyWithStringBuffer(ctx, cryptVal, len);
        RedisModule_FreeCallReply(strReply);
        RedisModule_Free(cryptVal);
    }

    RedisModule_FreeCallReply(reply);
    return size;
}

int CryptoGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 2) return RedisModule_WrongArity(ctx);

    char * cryptVal;
    size_t len = get(ctx, argv[1], &cryptVal);
    if ( len ==  (size_t) -1 )
    {
	return REDISMODULE_OK;
    }
	
    if (cryptVal == NULL)
    {
    	RedisModule_ReplyWithNull(ctx);
    }
    else
    {
    	RedisModule_ReplyWithStringBuffer(ctx, cryptVal, len);
    	RedisModule_Free(cryptVal);
    }
    return REDISMODULE_OK;
}

int CryptoSet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);
    
    RedisModuleCallReply *reply = set(ctx, argv[1], argv[2]); 
    RedisModule_ReplyWithCallReply(ctx,reply);
    RedisModule_FreeCallReply(reply);
     
    return REDISMODULE_OK;
}

int CryptoMGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc < 2) return RedisModule_WrongArity(ctx);

    mget(ctx, &argv[1], argc - 1);
    return REDISMODULE_OK;
}

int CryptoMSet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc < 2 || (argc % 2) == 0) return RedisModule_WrongArity(ctx);

    int i = 1;
    while(i < argc)
    {
    	RedisModuleCallReply *reply = set(ctx, argv[i], argv[i + 1]);
    	RedisModule_FreeCallReply(reply);
        i += 2;
    }

    RedisModule_ReplyWithSimpleString(ctx,"OK");
    return REDISMODULE_OK;
}

int CryptoAppend_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);

    char * origVal = NULL;
    size_t origLen = get(ctx, argv[1], &origVal);
    if ( origLen ==  (size_t) -1 )
    {
        return REDISMODULE_OK;
    }

    size_t appLen;
    const char * appVal = RedisModule_StringPtrLen(argv[2], &appLen);
    origVal = RedisModule_Realloc(origVal, origLen + appLen);
    memcpy(origVal + origLen, appVal, appLen);
    
    RedisModuleString * appendVal = RedisModule_CreateString(ctx, origVal, origLen + appLen);
    RedisModuleCallReply *reply = set(ctx, argv[1], appendVal);
    RedisModule_ReplyWithCallReply(ctx,reply);
    RedisModule_FreeCallReply(reply);
    RedisModule_Free(origVal);
    RedisModule_FreeString(ctx, appendVal);

    return REDISMODULE_OK;
}

int CryptoGetSet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);

    char * cryptVal;
    size_t len = get(ctx, argv[1], &cryptVal);
    if ( len ==  (size_t) -1 )
    {
        return REDISMODULE_OK;
    }
    RedisModuleCallReply *reply = set(ctx, argv[1], argv[2]);
    RedisModule_FreeCallReply(reply);
    if (cryptVal == NULL)
    {
        RedisModule_ReplyWithNull(ctx);
    }
    else
    {
        RedisModule_ReplyWithStringBuffer(ctx, cryptVal, len);
        RedisModule_Free(cryptVal);
    }
 
    return REDISMODULE_OK;
}

int CryptoStrlen_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 2) return RedisModule_WrongArity(ctx);

    char * cryptVal;
    size_t len = get(ctx, argv[1], &cryptVal);
    if ( len ==  (size_t) -1 )
    {
        return REDISMODULE_OK;
    }

    if (cryptVal == NULL)
    {
        RedisModule_ReplyWithLongLong(ctx, 0);
    }
    else
    {
        RedisModule_ReplyWithLongLong(ctx, len);
        RedisModule_Free(cryptVal);
    }
    return REDISMODULE_OK;
}

int CryptoSetNX_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);

    char * cryptVal;
    size_t len = get(ctx, argv[1], &cryptVal);
    if ( len ==  (size_t) -1 )
    {
        return REDISMODULE_OK;
    }

    if (cryptVal == NULL)
    {
    	RedisModuleCallReply *reply = set(ctx, argv[1], argv[2]);
    	RedisModule_FreeCallReply(reply);
        RedisModule_ReplyWithLongLong(ctx, 1);
    }
    else
    {
        RedisModule_ReplyWithLongLong(ctx, 0);
        RedisModule_Free(cryptVal);
    }

    return REDISMODULE_OK;
}

size_t hget(RedisModuleCtx *ctx, const RedisModuleString * key, const RedisModuleString * field, char **value)
{
    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"HGET","ss", key, field);
    if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR)
    {
        RedisModule_ReplyWithCallReply(ctx, reply);
        RedisModule_FreeCallReply(reply);
        return -1;
    }

    if ( RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_NULL )
    {
        RedisModule_FreeCallReply(reply);
        *value = NULL;
        return 0;
    }

    size_t bufsize;
    const char * repVal = RedisModule_CallReplyStringPtr(reply, &bufsize);
    char * cryptVal = RedisModule_Alloc(bufsize);
    memcpy(cryptVal, repVal, bufsize);
    (*decrypt)(cryptVal, bufsize);

    size_t len = *(size_t *)&cryptVal[0];
    memmove(cryptVal, cryptVal + sizeof(size_t), len);
    *value = cryptVal;
    RedisModule_FreeCallReply(reply);
    return len;
}

RedisModuleCallReply * hset(RedisModuleCtx *ctx, const RedisModuleString *key, const RedisModuleString *field, const RedisModuleString * value)
{
    size_t len;
    const char * inVal = RedisModule_StringPtrLen(value, &len);
    size_t bufsize = getCryptSize(len);

    char * cryptVal = RedisModule_Alloc(bufsize);
    memset(cryptVal, 0, bufsize);
    memcpy(cryptVal, &len, sizeof(size_t));
    memcpy(cryptVal + sizeof(size_t), inVal, len);
    (*encrypt)(cryptVal, bufsize);

    RedisModuleString* cryptValue = RedisModule_CreateString(ctx, cryptVal, bufsize);

    RedisModuleCallReply *reply;
    reply = RedisModule_Call(ctx,"HSET","!sss", key, field, cryptValue);
    RedisModule_FreeString(ctx, cryptValue);
    RedisModule_Free(cryptVal);
    return reply;
}

size_t hmget(RedisModuleCtx *ctx, const RedisModuleString * key, RedisModuleString ** fields, size_t len)
{
    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"HMGET","sv", key, fields, len);
     if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR)
    {
        RedisModule_ReplyWithCallReply(ctx, reply);
        RedisModule_FreeCallReply(reply);
        return -1;
    }

    size_t size = RedisModule_CallReplyLength(reply);
    RedisModule_ReplyWithArray(ctx, size);
    size_t i = 0;
    size_t bufsize;
    char * cryptVal;
    for (;i < size; i++)
    {
        RedisModuleCallReply * strReply = RedisModule_CallReplyArrayElement(reply, i);
        if ( RedisModule_CallReplyType(strReply) == REDISMODULE_REPLY_NULL )
        {
            RedisModule_ReplyWithNull(ctx);
            RedisModule_FreeCallReply(strReply);
            continue;
        }

        const char * repVal = RedisModule_CallReplyStringPtr(strReply, &bufsize);
        cryptVal = RedisModule_Alloc(bufsize);
        memcpy(cryptVal, repVal, bufsize);
        (*decrypt)(cryptVal, bufsize);

        size_t len = *(size_t *)&cryptVal[0];
        memmove(cryptVal, cryptVal + sizeof(size_t), len);
        RedisModule_ReplyWithStringBuffer(ctx, cryptVal, len);
        RedisModule_FreeCallReply(strReply);
        RedisModule_Free(cryptVal);
    }

    RedisModule_FreeCallReply(reply);
    return size;
}

size_t hgetall(RedisModuleCtx *ctx, const RedisModuleString * key)
{
    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"HGETALL","s", key);
    if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR)
    {
        RedisModule_ReplyWithCallReply(ctx, reply);
        RedisModule_FreeCallReply(reply);
        return -1;
    }

    if ( RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_NULL )
    {
        RedisModule_FreeCallReply(reply);
        return 0;
    }

    size_t size = RedisModule_CallReplyLength(reply);
    RedisModule_ReplyWithArray(ctx, size);
    size_t i = 0;
    size_t bufsize;
    char * cryptVal;
    for (;i < size; i++)
    {
	RedisModuleCallReply * strReply = RedisModule_CallReplyArrayElement(reply, i);
	if ((i % 2) == 0)
    	{
 	     RedisModule_ReplyWithCallReply(ctx, strReply);		
             RedisModule_FreeCallReply(strReply);
             continue;
        }

	const char * repVal = RedisModule_CallReplyStringPtr(strReply, &bufsize);
    	cryptVal = RedisModule_Alloc(bufsize);
    	memcpy(cryptVal, repVal, bufsize);
    	(*decrypt)(cryptVal, bufsize);

    	size_t len = *(size_t *)&cryptVal[0];
    	memmove(cryptVal, cryptVal + sizeof(size_t), len);
	RedisModule_ReplyWithStringBuffer(ctx, cryptVal, len);
    	RedisModule_FreeCallReply(strReply);
	RedisModule_Free(cryptVal);
    }
   
    RedisModule_FreeCallReply(reply);
    return size;
}

size_t hvals(RedisModuleCtx *ctx, const RedisModuleString * key)
{
    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"HVALS","s", key);
    if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR)
    {
        RedisModule_ReplyWithCallReply(ctx, reply);
        RedisModule_FreeCallReply(reply);
        return -1;
    }

    if ( RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_NULL )
    {
        RedisModule_FreeCallReply(reply);
        return 0;
    }

    size_t size = RedisModule_CallReplyLength(reply);
    RedisModule_ReplyWithArray(ctx, size);
    size_t i = 0;
    size_t bufsize;
    char * cryptVal;
    for (;i < size; i++)
    {
        RedisModuleCallReply * strReply = RedisModule_CallReplyArrayElement(reply, i);
        const char * repVal = RedisModule_CallReplyStringPtr(strReply, &bufsize);
        cryptVal = RedisModule_Alloc(bufsize);
        memcpy(cryptVal, repVal, bufsize);
        (*decrypt)(cryptVal, bufsize);

        size_t len = *(size_t *)&cryptVal[0];
        memmove(cryptVal, cryptVal + sizeof(size_t), len);
        RedisModule_ReplyWithStringBuffer(ctx, cryptVal, len);
        RedisModule_FreeCallReply(strReply);
        RedisModule_Free(cryptVal);
    }

    RedisModule_FreeCallReply(reply);
    return size;
}

int CryptoHGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);

    char * cryptVal;
    size_t len = hget(ctx, argv[1], argv[2], &cryptVal);
    if ( len ==  (size_t) -1 )
    {
        return REDISMODULE_OK;
    }

    if (cryptVal == NULL)
    {
        RedisModule_ReplyWithNull(ctx);
    }
    else
    {
        RedisModule_ReplyWithStringBuffer(ctx, cryptVal, len);
        RedisModule_Free(cryptVal);
    }
    return REDISMODULE_OK;
}

int CryptoHSet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 4) return RedisModule_WrongArity(ctx);

    RedisModuleCallReply *reply = hset(ctx, argv[1], argv[2], argv[3]);
    RedisModule_ReplyWithCallReply(ctx,reply);
    RedisModule_FreeCallReply(reply);

    return REDISMODULE_OK;
}

int CryptoHMGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc < 3) return RedisModule_WrongArity(ctx);

    hmget(ctx, argv[1], &argv[2], argc - 2);
    return REDISMODULE_OK;
}

int CryptoHMSet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc < 4 || (argc % 2) == 1) return RedisModule_WrongArity(ctx);

    int i = 2;
    while(i < argc)
    {
        RedisModuleCallReply *reply = hset(ctx, argv[1], argv[i], argv[i + 1]);
        if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR)
        {
        	RedisModule_ReplyWithCallReply(ctx, reply);
        	RedisModule_FreeCallReply(reply);
        	return REDISMODULE_OK;
    	}

        RedisModule_FreeCallReply(reply);
        i += 2;
    }

    RedisModule_ReplyWithSimpleString(ctx,"OK");
    return REDISMODULE_OK;
}

int CryptoHGetAll_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 2) return RedisModule_WrongArity(ctx);

    size_t size = hgetall(ctx, argv[1]);
    if ( size ==  (size_t) -1 )
    {
        return REDISMODULE_OK;
    }

    if (!size)
    {
        RedisModule_ReplyWithNull(ctx);
    }
    return REDISMODULE_OK;
}

int CryptoHSetNX_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 4) return RedisModule_WrongArity(ctx);

    char * cryptVal;
    size_t len = hget(ctx, argv[1], argv[2], &cryptVal);
    if ( len ==  (size_t) -1 )
    {
        return REDISMODULE_OK;
    }

    if (cryptVal == NULL)
    {
        RedisModuleCallReply *reply = hset(ctx, argv[1], argv[2], argv[3]);
        RedisModule_FreeCallReply(reply);
        RedisModule_ReplyWithLongLong(ctx, 1);
    }
    else
    {
        RedisModule_ReplyWithLongLong(ctx, 0);
        RedisModule_Free(cryptVal);
    }

    return REDISMODULE_OK;
}

int CryptoHVals_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 2) return RedisModule_WrongArity(ctx);

    size_t size = hvals(ctx, argv[1]);
    if ( size ==  (size_t) -1 )
    {
        return REDISMODULE_OK;
    }

    if (!size)
    {
        RedisModule_ReplyWithNull(ctx);
    }
    return REDISMODULE_OK;
}

int CryptoHStrlen_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);

    char * cryptVal;
    size_t len = hget(ctx, argv[1], argv[2], &cryptVal);
    if ( len ==  (size_t) -1 )
    {
        return REDISMODULE_OK;
    }

    if (cryptVal == NULL)
    {
        RedisModule_ReplyWithLongLong(ctx, 0);
    }
    else
    {
        RedisModule_ReplyWithLongLong(ctx, len);
        RedisModule_Free(cryptVal);
    }
    return REDISMODULE_OK;
}

/* This function must be present on each Redis module. It is used in order to
 * register the commands into the Redis server. */
int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (RedisModule_Init(ctx,"crypto",1,REDISMODULE_APIVER_1)
        == REDISMODULE_ERR) return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.get",
        CryptoGet_RedisCommand,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.set",
        CryptoSet_RedisCommand,"write",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.mget",
        CryptoMGet_RedisCommand,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.mset",
        CryptoMSet_RedisCommand,"write",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.append",
        CryptoAppend_RedisCommand,"write",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.getset",
        CryptoGetSet_RedisCommand,"write",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.strlen",
        CryptoStrlen_RedisCommand,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.setnx",
        CryptoSetNX_RedisCommand,"write",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.hget",
        CryptoHGet_RedisCommand,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.hset",
        CryptoHSet_RedisCommand,"write",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.hmget",
        CryptoHMGet_RedisCommand,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.hmset",
        CryptoHMSet_RedisCommand,"write",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.hgetall",
        CryptoHGetAll_RedisCommand,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.hsetnx",
        CryptoHSetNX_RedisCommand,"write",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.hstrlen",
        CryptoHStrlen_RedisCommand,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"crypto.hvals",
        CryptoHVals_RedisCommand,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    const char * crypto = NULL;
    if ( argc > 0 )
    {
        size_t len;
        crypto = RedisModule_StringPtrLen(argv[0], &len);
    }
 
    if ( !crypto )
    {
        fputs ("Missing crypto lib\n", stderr);
        exit(1);
    }

    char *error;
    void *handle;
    handle = dlopen (crypto, RTLD_LAZY);
    if (!handle) {
        fputs (dlerror(), stderr);
        exit(1);
    }

    encrypt = dlsym(handle, "encrypt");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }
    
    decrypt = dlsym(handle, "decrypt");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }
    
    blocksize = dlsym(handle, "blocksize");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }

    //dlclose(handle);
    return REDISMODULE_OK;
}
