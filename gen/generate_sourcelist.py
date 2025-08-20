import os
import argparse

def get_files_in_directory(directory, extensions):
    """Gets all files with given extensions in a directory, grouped by subdirectory."""
    file_list = []
    for root, _, files in os.walk(directory):
        for file in sorted(files):
            if any(file.endswith(ext) for ext in extensions):
                rel_path = os.path.relpath(os.path.join(root, file), directory)
                file_list.append(rel_path.replace('\\', '/'))
    return file_list

def generate_cmake_file_list(files, variable_name, base_dir_name):
    """Generates a CMake list of files with comments for subdirectories."""
    if not files:
        return ""
    
    # Group files by their subdirectory for better readability
    grouped_files = {}
    for f in files:
        f_norm = f.replace('\\', '/')
        parts = f_norm.split('/')
        key = '/'.join(parts[:-1]) if len(parts) > 1 else '.'
        if key not in grouped_files:
            grouped_files[key] = []
        grouped_files[key].append(f_norm)

    cmake_list = ""
    
    # Custom sort order: root files first, then subdirectories alphabetically
    sorted_keys = sorted(grouped_files.keys(), key=lambda x: (x != '.', x))

    first_group = True
    for key in sorted_keys:
        if not first_group:
            cmake_list += "\n"
        first_group = False

        # Add a comment for the subdirectory
        if key == '.':
            cmake_list += "## /...\n"
        else:
            cmake_list += f"## /{key}/...\n"
        
        # Add files for the current group
        for f in sorted(grouped_files[key]):
            cmake_list += f'    ${{{variable_name}}}/{f}\n'
            
    return cmake_list.strip()

def generate_sourcelist(root_dir, output_file, identifier):
    """Generates the sourcelist.cmake file."""
    
    inc_dir = os.path.join(root_dir, 'inc')
    src_dir = os.path.join(root_dir, 'src')

    has_inc_dir = os.path.isdir(inc_dir)
    has_src_dir = os.path.isdir(src_dir)

    if not has_inc_dir and not has_src_dir:
        print(f"Error: Neither 'inc' nor 'src' directory found in '{root_dir}'")
        return

    header_files = get_files_in_directory(inc_dir, ['.h', '.hpp']) if has_inc_dir else []
    source_files = get_files_in_directory(src_dir, ['.cpp', '.h']) if has_src_dir else []

    # Manually add elephant_gambit_config.h as it is a generated file
    if 'elephant_gambit_config.h.in' in header_files:
        header_files.remove('elephant_gambit_config.h.in')
        header_files.insert(1, 'elephant_gambit_config.h')


    cmake_template = """set({identifier}_SRC_DIR ${{CMAKE_CURRENT_LIST_DIR}}/src)
set({identifier}_INC_DIR ${{CMAKE_CURRENT_LIST_DIR}}/inc)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
endif()

configure_file(${{{identifier}_INC_DIR}}/elephant_gambit_config.h.in ${{{identifier}_INC_DIR}}/elephant_gambit_config.h)

set({identifier}_SOURCE_INCLUDE ${{{identifier}_SOURCE_INCLUDE}}
{include_files}
)

set({identifier}_SOURCE ${{{identifier}_SOURCE}}
{source_files}
)

set({identifier}_SOURCE_ALL ${{{identifier}_SOURCE}} ${{{identifier}_SOURCE_INCLUDE}})
"""

    include_files_str = generate_cmake_file_list(header_files, f'{identifier.upper()}_INC_DIR', 'inc')
    source_files_str = generate_cmake_file_list(source_files, f'{identifier.upper()}_SRC_DIR', 'src')

    output_content = cmake_template.format(
        identifier=identifier.upper(),
        include_files=include_files_str,
        source_files=source_files_str
    )

    with open(output_file, 'w') as f:
        f.write(output_content)
    
    print(f"Successfully generated '{output_file}'")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate sourcelist.cmake from a directory structure.")
    parser.add_argument("root_dir", help="The root directory containing 'inc' and 'src' folders.")
    parser.add_argument("-o", "--output", default="sourcelist.cmake", help="The name of the output file.")
    parser.add_argument("-i", "--identifier", default="ENGINE", help="The identifier for CMake variables (e.g., ENGINE, TEST).")
    
    args = parser.parse_args()
    
    generate_sourcelist(args.root_dir, args.output, args.identifier)
