#include <stdint.h>
#include <ringbuffer.h>



uint8_t sys_haskey(void){
    return !ring_buf_is_empty();
}