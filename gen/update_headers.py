import os
import argparse
import datetime
from typing import Optional

# --- Configuration ---
# This is the template for the license and file header comment.
# You can customize it to your needs.
LICENSE_TEMPLATE = """
/******************************************************************************
 * Elephant Gambit Chess Engine - a Chess AI
 * Copyright(C) 2025  Alexander Loodin Ek
 * 
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < http://www.gnu.org/licenses/>. 
 *****************************************************************************/

/**
 * @file {filename}
 * @brief {brief_description} 
 * 
 */"""

# A unique string to check if the license already exists in a file.
# The script will not modify files containing this string.
LICENSE_CHECK_STRING = "@copyright Copyright (c) 2025" 

# --- Functions ---

def get_header_files(directory):
    """Finds all .h and .hpp files in a directory and its subdirectories."""
    header_files = []
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.h', '.hpp')):
                header_files.append(os.path.join(root, file))
    return header_files

def has_license(file_path):
    """
    Checks if a file already contains the license comment by searching for
    LICENSE_CHECK_STRING in the first 20 lines.
    """
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for i, line in enumerate(f):
                if i >= 20:  # Only check the top of the file
                    break
                if LICENSE_CHECK_STRING in line:
                    return True
    except Exception as e:
        print(f"Could not read file {file_path}: {e}")
        return True # Skip file on error to be safe
    return False

# --- Brief generation ---

def _fallback_brief_from_filename(file_path: str) -> str:
    base = os.path.basename(file_path)
    name_without_ext = os.path.splitext(base)[0]
    return name_without_ext.replace('_', ' ').replace('-', ' ').capitalize()


def generate_brief_from_filename(file_path: str, use_openai: bool = False, model: str = "gpt-5-mini", api_key: Optional[str] = None) -> str:
    """Generate a concise @brief for the given file.
    - If use_openai is True and an API key is available, call OpenAI to summarize.
    - Otherwise, fall back to a filename-based brief.
    """
    if not use_openai:
        return _fallback_brief_from_filename(file_path)

    # Resolve API key
    resolved_key = api_key or os.getenv("OPENAI_API_KEY")
    if not resolved_key:
        return _fallback_brief_from_filename(file_path)

    # Read a reasonable slice of the file to keep the prompt small
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read(20000)  # cap to ~20KB
    except Exception:
        content = ""

    # If content is empty, still fall back
    if not content.strip():
        return _fallback_brief_from_filename(file_path)

    # Try using the latest OpenAI Python SDK, with fallback to legacy import
    try:
        try:
            from openai import OpenAI  # type: ignore
            client = OpenAI(api_key=resolved_key)
            sys_prompt = (
                "You are a senior C++ engineer. Given a C/C++ header or source file, "
                "produce a single concise doxygen @brief line that describes the file's purpose. "
                "Keep it impersonal, <= 18 words, no trailing period, no code fences."
            )
            user_prompt = (
                f"Filename: {os.path.basename(file_path)}\n\n"
                f"File contents (truncated):\n{content}"
            )
            resp = client.chat.completions.create(
                model=model,
                messages=[
                    {"role": "system", "content": sys_prompt},
                    {"role": "user", "content": user_prompt},
                ],
                temperature=0.2,
                max_tokens=60,
            )
            brief = (resp.choices[0].message.content or "").strip()
            if brief:
                # Normalize whitespace and strip trailing period if present
                brief = ' '.join(brief.split())
                if brief.endswith('.'):
                    brief = brief[:-1]
                return brief
        except ImportError:
            import openai  # type: ignore
            openai.api_key = resolved_key
            sys_prompt = (
                "You are a senior C++ engineer. Given a C/C++ header or source file, "
                "produce a single concise doxygen @brief line that describes the file's purpose. "
                "Keep it impersonal, <= 18 words, no trailing period, no code fences."
            )
            user_prompt = (
                f"Filename: {os.path.basename(file_path)}\n\n"
                f"File contents (truncated):\n{content}"
            )
            resp = openai.ChatCompletion.create(
                model=model,
                messages=[
                    {"role": "system", "content": sys_prompt},
                    {"role": "user", "content": user_prompt},
                ],
                temperature=0.2,
                max_tokens=60,
            )
            brief = (resp["choices"][0]["message"]["content"] or "").strip()
            if brief:
                brief = ' '.join(brief.split())
                if brief.endswith('.'):
                    brief = brief[:-1]
                return brief
    except Exception as e:
        print(f"OpenAI brief generation failed for {file_path}: {e}")

    return _fallback_brief_from_filename(file_path)


def add_license_to_file(file_path, use_openai: bool = False, model: str = "gpt-5-mini", api_key: Optional[str] = None):
    """Adds the license header to the top of a file."""
    filename = os.path.basename(file_path)
    brief = generate_brief_from_filename(file_path, use_openai=use_openai, model=model, api_key=api_key)
    date = datetime.date.today().isoformat()
    
    # Create the full comment block from the template
    license_comment = LICENSE_TEMPLATE.format(
        filename=filename,
        brief_description=brief,
        date=date
    )
    
    try:
        with open(file_path, 'r+', encoding='utf-8', errors='ignore') as f:
            original_content = f.read()
            f.seek(0, 0)
            f.write(license_comment + '\n\n' + original_content)
        print(f"Updated file: {file_path}")
    except Exception as e:
        print(f"Could not write to file {file_path}: {e}")

# --- Main Execution ---

def main():
    """Main function to parse arguments and process files."""
    parser = argparse.ArgumentParser(
        description=(
            "Adds a license header to C/C++ header files if one doesn't exist.\n"
            "Optionally uses OpenAI to generate a concise @brief from file contents."
        ),
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("directory", help="The root directory to search for header files.")
    parser.add_argument("--use-openai", action="store_true", help="Use OpenAI to generate @brief from file contents.")
    parser.add_argument("--api-key", default=None, help="OpenAI API key (fallbacks to OPENAI_API_KEY env var).")
    parser.add_argument("--model", default="gpt-5-mini", help="OpenAI model to use (default: gpt-5-mini).")
    args = parser.parse_args()

    if not os.path.isdir(args.directory):
        print(f"Error: Directory not found at '{args.directory}'")
        return

    header_files = get_header_files(args.directory)
    
    if not header_files:
        print("No header files found.")
        return

    print(f"Found {len(header_files)} header files. Checking and updating if necessary...")
    
    updated_count = 0
    for file_path in header_files:
        if not has_license(file_path):
            add_license_to_file(
                file_path,
                use_openai=args.use_openai,
                model=args.model,
                api_key=args.api_key,
            )
            updated_count += 1

    print(f"\nProcessing complete. Updated {updated_count} files.")

if __name__ == "__main__":
    main()
