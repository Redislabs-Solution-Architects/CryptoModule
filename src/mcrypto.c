#include <mcrypt.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char * IV = "qwdf*32NaR1)92<k";
static char * key = "GH^738ahE12NHa*gw";
static int keysize;

MCRYPT encryptD = NULL;
MCRYPT decryptD = NULL;

int init(int argc, const char ** argv)
{ 
   char * algo = "rijndael-128";
   char * mode = "cbc";

   for ( int i = 0; i < argc; i++ )
   {
   	char * eq = strchr(argv[i], '=');
        if ( eq == NULL )
	   continue;

	*eq = '\0';
        
	char * name = argv[i];
        char * value = eq + 1;
	if ( strlen(name) == 0 || strlen(value) == 0 )
	   continue;

        if ( strcmp(name, "algo") == 0 )
	   algo = value;
        if ( strcmp(name, "mode") == 0 )
	   mode = value;
        if ( strcmp(name, "key") == 0 )
           key = value;
        if ( strcmp(name, "iv") == 0 )
           IV = value;
   }

   encryptD = mcrypt_module_open(algo, NULL, mode, NULL);
   if ( encryptD == MCRYPT_FAILED)
        return -1;

   decryptD = mcrypt_module_open(algo, NULL, mode, NULL);
   if ( decryptD == MCRYPT_FAILED)
        return -1;

   keysize = mcrypt_enc_get_key_size(encryptD);
   
   if ( mcrypt_enc_get_iv_size(encryptD) != strlen(IV) )	
   {
	printf("Wrong IV size, should be %d\n", mcrypt_enc_get_iv_size(encryptD));
	return -1;
   }

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
