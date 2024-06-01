import os
import re

# Define the regex pattern for the old comment format
old_comment_pattern = re.compile(r'REM\{///////////////////////////////////////////////////\}\nREM\{(.*?)\}\nREM\{///////////////////////////////////////////////////\}', re.MULTILINE | re.DOTALL)

# New comment template
new_comment_template = """///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////  {comment}  ///////////////////////////
///////////////////////////////////////////////////////////////////////////////
"""

# Function to replace old comments with new comments in a file
def replace_comments(file_path):
    with open(file_path, 'r') as file:
        content = file.read()
    
    # Find all matches of the old comment pattern
    matches = old_comment_pattern.findall(content)
    
    # Replace each old comment with the new comment format
    for match in matches:
        old_comment = f'REM{{///////////////////////////////////////////////////}}\nREM{{{match}}}\nREM{{///////////////////////////////////////////////////}}'
        new_comment = new_comment_template.format(comment=match.strip())
        content = content.replace(old_comment, new_comment)
    
    with open(file_path, 'w') as file:
        file.write(content)
    
    print(f'Comments replaced in {file_path}')

# Function to recursively find and process all .txt files
def process_files(root_directory):
    for root, _, files in os.walk(root_directory):
        for file in files:
            if file.endswith('.txt'):
                file_path = os.path.join(root, file)
                replace_comments(file_path)

if __name__ == "__main__":
    # Define the root directory to search for .txt files
    root_directory = r'C:\Users\yassi\Desktop\destination\portage'
    
    # Process all .txt files
    process_files(root_directory)
    print(f'Processing complete. Comments have been replaced.')
