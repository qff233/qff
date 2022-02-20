function(ragelmaker src_rl outputlist outputdir)
    get_filename_component(src_file ${src_rl} NAME_WE)

    set(rl_out ${outputdir}/${src_file}.rl.cpp)
    set(${outputlist} ${${outputlist}} ${rl_out} PARENT_SCOPE)

    add_custom_command(
        OUTPUT ${rl_out}
        COMMAND cd ${outputdir}
        COMMAND ragel ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl} -o ${rl_out} -l -C -G2  --error-format=msvc
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl}
        )
    set_source_files_properties(${rl_out} PROPERTIES GENERATED TRUE)
endfunction(ragelmaker)