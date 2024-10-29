add_library(uthash INTERFACE
    utarray.h uthash.h utlist.h utringbuffer.h utstack.h utstring.h
)
target_include_directories(uthash INTERFACE .)
add_library(uthash::headers ALIAS uthash)
