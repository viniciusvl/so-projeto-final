
global far_jump

far_jump:    
    jmp 0x08:flush_cs  
    flush_cs:           
    ret