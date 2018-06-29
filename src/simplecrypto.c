
static const char xorArray[] = "AU7*$+jdLS{|VB2!";
static int xorArraySize = 16;

int init(int argc, const char ** argv)
{
  return 0;
}

int encrypt(void* buffer, int buffer_len)
{
  char * ptr = buffer;
  int i = 0;
  for (;i < buffer_len; i++)
  {
	ptr[i] = ptr[i] ^ xorArray[i % xorArraySize];
  }
  return 0;
}

int decrypt(void* buffer, int buffer_len)
{
  char * ptr = buffer;
  int i = 0;
  for (;i < buffer_len; i++)
  {
        ptr[i] = ptr[i] ^ xorArray[i % xorArraySize];
  }

  return 0;
}

int blocksize()
{
    return 0;
}
