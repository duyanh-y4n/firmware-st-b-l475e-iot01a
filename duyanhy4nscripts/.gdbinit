target remote :2338
mon reset
mon halt
load build/debug/target.elf
symbol-file build/debug/target.elf

define x_intern_buf
    x/30b internImgBuff
end

define x_extern_buf
    x/30b externImgBuff
end

define x_extern_buf_RGB
    x/30b externImgBuff_RGB
end

define x_intern_buf_RGB
    x/30b internImgBuff_RGB
end

define _dump_jpeg
    dump binary memory jpegdump internImgBuff (internImgBuff+156000)
end

b main
#b user_main.cpp:set_jpeg_config
# buser_main.cpp:147
c

set pagination off
tui en
info b
