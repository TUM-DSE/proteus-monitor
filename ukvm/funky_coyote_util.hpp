typedef struct CoyoteBuffer {
  uint64_t mem_flags;
  size_t size;
  void* host_ptr;
  void* mem_ptr;
} CoyoteBuffer;

typedef struct CoyoteArg {
  CoyoteBuffer* buffer;
  size_t size;
  void* src;
} CoyoteArg;

uint8_t readByte(ifstream &fb)
{
    char temp;
    fb.read(&temp, 1);
    return (uint8_t)temp;
}