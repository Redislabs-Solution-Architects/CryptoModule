#include <mcrypt.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int keysize = 16; /* 128 bits */
static char * IV = "qwdf*32NaR1)92<k";
static char * key = "GH^738ahE12NHa*gw";

MCRYPT encryptD = NULL;
MCRYPT decryptD = NULL;

int init(int argc, const char ** argv)
{
   encryptD = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
   if ( encryptD == MCRYPT_FAILED)
        return -1;

   decryptD = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
   if ( decryptD == MCRYPT_FAILED)
        return -1;

  return 0;
}

int encrypt(void* buffer, int buffer_len)
{
  int blocksize = mcrypt_enc_get_block_size(encryptD);
  if( buffer_len % blocksize != 0 ){return 1;}

  mcrypt_generic_init(encryptD, key, keysize, IV);
  int ret = mcrypt_generic(encryptD, buffer, buffer_len);
  mcrypt_generic_deinit (encryptD);

  return ret;
}

int decrypt(void* buffer, int buffer_len)
{
  int blocksize = mcrypt_enc_get_block_size(decryptD);
  if( buffer_len % blocksize != 0 ){return 1;}

  mcrypt_generic_init(decryptD, key, keysize, IV);
  int ret = mdecrypt_generic(decryptD, buffer, buffer_len);
  mcrypt_generic_deinit (decryptD);
  return ret;
}

int blocksize()
{
  return mcrypt_enc_get_block_size(encryptD);
}
