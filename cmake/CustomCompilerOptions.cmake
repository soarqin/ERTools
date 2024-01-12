macro(fix_compile_flags)
    add_compile_options($<$<OR:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>:/utf-8>)
endmacro()

macro(fix_release_flags)
    add_compile_options(
        $<$<AND:$<C_COMPILER_ID:MSVC>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:/GL>
    )
    add_link_options(
        $<$<AND:$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:-s>
        $<$<AND:$<C_COMPILER_ID:MSVC>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:/LTCG>
    )
endmacro()

# pass an argument 'ON' to this macro to enable static runtime by default
macro(add_static_runtime_option)
    option(USE_STATIC_CRT "Use static C/C++ runtime" ${ARGV0})
    if(USE_STATIC_CRT)
        add_link_options(
            $<$<AND:$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:-static\;-static-libgcc\;-static-libstdc++>
        )
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endmacro()
