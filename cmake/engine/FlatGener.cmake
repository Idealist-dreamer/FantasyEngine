function(FlatGenerateFun)
  set(options)
  set(one_value_args "TARGET" "INCLUDE_PREFIX" "OUTPUT_DIR" "SCHEMA_ROOT" "CUSTOM_INCLUDE")
  set(multi_value_args "SCHEMAS" "INCLUDE" "FLAGS")
  cmake_parse_arguments(FLAT "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(TARGET flatbuffers::flatc)
    set(FLATC flatbuffers::flatc)
  else()
    find_program(FLATC flatc REQUIRED)
  endif()

  if("${FLAT_SCHEMA_ROOT}" STREQUAL "")
      get_filename_component(FLAT_SCHEMA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
  endif()

  if ("${FLAT_OUTPUT_DIR}" STREQUAL "")
      set(FLAT_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/flatbuffers_generated")
  endif()

  set(include_params "")
  foreach (include_dir ${FLAT_INCLUDE})
    list(APPEND include_params -I "${include_dir}")
  endforeach()

  set(custom_include_arg "")
  if(NOT "${FLAT_CUSTOM_INCLUDE}" STREQUAL "")
      set(custom_include_arg "--cpp-include" "${FLAT_CUSTOM_INCLUDE}")
  endif()

  set(generated_custom_commands "")
  set(all_generated_header_files "")

  foreach(schema ${FLAT_SCHEMAS})
    get_filename_component(abs_schema ${schema} ABSOLUTE)
    get_filename_component(filename ${schema} NAME_WE)
    get_filename_component(schema_dir ${abs_schema} DIRECTORY)
    
    file(RELATIVE_PATH schema_rel_dir "${FLAT_SCHEMA_ROOT}" "${schema_dir}")
    set(current_output_dir "${FLAT_OUTPUT_DIR}/${schema_rel_dir}")
    file(MAKE_DIRECTORY "${current_output_dir}")

    set(generated_include "${current_output_dir}/${filename}.flat.h")

    add_custom_command(
      OUTPUT "${generated_include}"
      COMMAND ${FLATC}
      -o "${current_output_dir}"
      ${include_params}
      -c "${abs_schema}"
      ${FLAT_FLAGS}
      ${custom_include_arg}      
      --cpp-ptr-type "stl::unique_ptr"
      --cpp-str-type "stl::string"
      --cpp-str-flex-ctor
      --cpp-std "c++20"         
      --filename-suffix ".flat"
      DEPENDS "${abs_schema}"
      COMMENT "FlatBuffers: Building ${schema}..."
      VERBATIM
    )
    
    list(APPEND all_generated_header_files "${generated_include}")
    list(APPEND generated_custom_commands "${generated_include}")
  endforeach()

  set(gen_target_name "GENERATE_${FLAT_TARGET}")
  add_custom_target(${gen_target_name} DEPENDS ${generated_custom_commands})

  add_library(${FLAT_TARGET} INTERFACE)
  target_sources(${FLAT_TARGET} INTERFACE ${all_generated_header_files})
  add_dependencies(${FLAT_TARGET} ${gen_target_name})
  target_include_directories(${FLAT_TARGET} INTERFACE "${FLAT_OUTPUT_DIR}")
endfunction()