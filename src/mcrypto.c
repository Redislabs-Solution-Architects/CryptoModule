
#include <mcrypt.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int keysize = 16; /* 128 bits */
static const char * IV = "qwdf*32NaR1)92<k";
static const char * key = "GH^738ahE12NHa*gw";


int encrypt(void* buffer, int buffer_len)
{
  MCRYPT td = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
  int blocksize = mcrypt_enc_get_block_size(td);
  if( buffer_len % blocksize != 0 ){return 1;}

  mcrypt_generic_init(td, key, keysize, IV);
  mcrypt_generic(td, buffer, buffer_len);
  mcrypt_generic_deinit (td);
  mcrypt_module_close(td);

  return 0;
}

int decrypt(void* buffer, int buffer_len)
{
  MCRYPT td = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
  int blocksize = mcrypt_enc_get_block_size(td);
  if( buffer_len % blocksize != 0 ){return 1;}

  mcrypt_generic_init(td, key, keysize, IV);
  mdecrypt_generic(td, buffer, buffer_len);
  mcrypt_generic_deinit (td);
  mcrypt_module_close(td);

  return 0;
}

int blocksize()
{
  MCRYPT td = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
  return mcrypt_enc_get_block_size(td);
}
