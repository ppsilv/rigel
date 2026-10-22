#include <stdint.h>
#include <ringbuffer.h>



uint8_t sys_haskey(void){
    return !ring_buf_is_empty();
}

uint8_t sys_getkey(void){
    return ring_buf_get();
}

uint8_t sys_getkbdkey(){
	uint16_t ch=0;
	while( ring_buf_is_empty() );
	ch = ring_buf_get(); 
	return ch;
}
