import os
import sys


input_folder_path = '../../src'

output_txt_filename = 'compiled_code.txt'

allowed_extensions = [
    '.cpp',
    '.h',
    '.py',
    '.ts',
    '.html',
    # '.scss',
    # '.css',
    '.js',
    '.tex',
    '.rst',
]
allowed_extensions = [ext.lower() for ext in allowed_extensions]

excluded_folders_exact = [
    'build',
    'lib',
    '.git',
    '.vscode',
    'node_modules',
    'dist',
    # Add any other specific folder names to ignore by exact match
]

# 5. Folders containing "env" (case-insensitive) will ALSO be excluded automatically.

# 6. Customize the separator written between files in the output .txt
separator_template = "\n\n---=== File: {filepath} ===---\n\n"
# --- End of Configuration ---


# --- Script Logic ---

def compile_code_to_txt(root_folder, output_txt, extensions, exclusions_exact, separator_fmt):
    """
    Recursively finds files with specified extensions in root_folder,
    excluding specified subfolders (by exact match or containing 'env'),
    and compiles their relative path and content into a single text file.
    """
    # --- Input Validation ---
    if not os.path.isdir(root_folder):
        print(f"Error: Input folder not found: '{root_folder}'")
        return

    # Convert exact exclusions to a set for faster lookups
    exclusion_set_exact = set(exclusions_exact)
    # Convert extensions to a tuple for faster 'endswith' check
    extension_tuple = tuple(extensions)

    found_files_count = 0
    processed_files_count = 0
    skipped_encoding_errors = 0

    print(f"Starting scan in: '{root_folder}'")
    print(f"Including extensions: {', '.join(extensions)}")
    print(f"Excluding folders by exact match: {', '.join(exclusions_exact)}")
    print(f"Excluding any folder containing 'env' (case-insensitive).") # Added clarification
    print(f"Outputting to: '{output_txt}'")

    try:
        with open(output_txt, 'w', encoding='utf-8') as outfile:
            print(f"\nCompiling into '{output_txt}'...")

            for current_path, dirs, files in os.walk(root_folder, topdown=True):

                # --- Prune Excluded Directories ---
                # Modify 'dirs' in-place to prevent os.walk from descending into them
                # Exclude if name is in exclusion_set_exact OR if name contains "env" (case-insensitive)
                dirs[:] = [d for d in dirs if d not in exclusion_set_exact and "env" not in d.lower()]
                files.sort()

                for filename in files:
                    if filename.lower().endswith(extension_tuple):
                        found_files_count += 1
                        file_path = os.path.join(current_path, filename)
                        relative_path = os.path.relpath(file_path, root_folder)
                        relative_path = relative_path.replace(os.sep, '/')

                        print(f"  Adding: {relative_path}")

                        try:
                            content = ""
                            try:
                                with open(file_path, 'r', encoding='utf-8') as infile:
                                    content = infile.read()
                            except UnicodeDecodeError:
                                try:
                                    with open(file_path, 'r', encoding='latin-1') as infile:
                                        content = infile.read()
                                except Exception as enc_err:
                                     print(f"    - Error: Could not decode file '{relative_path}'. Skipping. Error: {enc_err}")
                                     skipped_encoding_errors += 1
                                     continue

                            outfile.write(separator_fmt.format(filepath=relative_path))
                            outfile.write(content)
                            processed_files_count += 1

                        except Exception as e:
                            print(f"    - Error processing file '{relative_path}': {e}. Skipping this file.")

        print("\n--- Compilation Summary ---")
        print(f"Scan complete.")
        print(f"Total files found matching extensions: {found_files_count}")
        print(f"Files successfully processed and added to TXT: {processed_files_count}")
        if skipped_encoding_errors > 0:
             print(f"Files skipped due to encoding errors: {skipped_encoding_errors}")
        print(f"Output saved to '{output_txt}'")
        if found_files_count == 0:
            print("Warning: No files matching the specified extensions were found in the target folder (after exclusions).")

    except Exception as e:
        print(f"\nFatal Error writing to output file '{output_txt}': {e}")
        sys.exit(1)

# --- Run the function ---
if __name__ == "__main__":
    if input_folder_path == 'your_angular_project_folder' or not input_folder_path:
         print("Error: Please update the 'input_folder_path' variable in the script!")
         print("       Set it to the directory you want to scan.")
    else:
        # Renamed variable for clarity
        compile_code_to_txt(input_folder_path, output_txt_filename, allowed_extensions, excluded_folders_exact, separator_template)