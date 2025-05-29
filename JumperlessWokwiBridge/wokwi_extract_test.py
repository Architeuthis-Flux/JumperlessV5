#!/usr/bin/env python3
# Test script to extract sketch.ino from a Wokwi project

import requests
from bs4 import BeautifulSoup
import json
import re
import sys

def extract_sketch_from_wokwi(wokwi_url):
    """Extract sketch.ino content from a Wokwi project"""
    print(f"Fetching Wokwi project: {wokwi_url}")
    
    # Convert Wokwi project URL to full page URL if needed
    if "/projects/" in wokwi_url:
        project_id = wokwi_url.split("/projects/")[1].split("/")[0]
        page_url = f"https://wokwi.com/projects/{project_id}"
    else:
        page_url = wokwi_url
    
    # Fetch the project page
    print(f"Requesting: {page_url}")
    response = requests.get(page_url, timeout=10)
    if response.status_code != 200:
        print(f"Failed to fetch Wokwi project page: {response.status_code}")
        return None
    
    # Save HTML content for debugging
    with open("wokwi_response.html", "w", encoding="utf-8") as f:
        f.write(response.text)
    print("Saved HTML response to wokwi_response.html for debugging")
    
    # Parse HTML
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # Find the JSON data containing the project information
    decoded_data = None
    sketch_content = None
    
    # Approach 0: Direct extraction of visible code if present
    try:
        # Look for the sketch code in the DOM - it might be in pre or code tags
        code_elements = soup.find_all(['pre', 'code'])
        for code_elem in code_elements:
            code_text = code_elem.get_text(strip=True)
            if code_text and 'void setup()' in code_text and 'void loop()' in code_text:
                sketch_content = code_text
                print("Found sketch.ino directly in page content")
                break
    except Exception as e:
        print(f"Failed to extract code from page content: {e}")
    
    # Approach 1: Try to find embedded project data using regex
    if not sketch_content:
        try:
            # Look for a pattern like e:["$","$L16",null,{"project":...
            matches = re.findall(r'e:\[.*?"project":\{.*?"files":\[(.*?)\]', response.text, re.DOTALL)
            if matches:
                for match in matches:
                    try:
                        # Extract files array and parse
                        files_json = '[' + match + ']'
                        files_data = json.loads(files_json)
                        
                        # Look for sketch.ino
                        for file_data in files_data:
                            if isinstance(file_data, dict) and file_data.get('name') == 'sketch.ino':
                                sketch_content = file_data.get('content', '')
                                if sketch_content:
                                    print("Found sketch.ino using embedded JSON approach")
                                    break
                    except Exception as e:
                        print(f"Failed to parse embedded JSON: {e}")
                        continue
                    
                    if sketch_content:
                        break
        except Exception as e:
            print(f"Failed in regex approach 1: {e}")
    
    # Approach 2: Look for raw code section
    if not sketch_content:
        try:
            # Wokwi sometimes has the raw code in a code block comment in plain text
            code_blocks = re.findall(r'```\s*(?:arduino|cpp)?\s*(void\s+setup\(\)[\s\S]*?void\s+loop\(\)[\s\S]*?)```', response.text)
            if code_blocks:
                sketch_content = code_blocks[0].strip()
                print("Found sketch.ino in code block")
        except Exception as e:
            print(f"Failed to extract code block: {e}")
    
    # Approach 3: Try to find project data in script tags
    if not sketch_content:
        script_tags = soup.find_all('script')
        for script in script_tags:
            if script.string and 'project' in script.string and 'files' in script.string:
                try:
                    # Look for project data pattern
                    match = re.search(r'project":\s*{.*?"files":\s*\[(.*?)\]', script.string, re.DOTALL)
                    if match:
                        files_json = '[' + match.group(1) + ']'
                        # Now parse the extracted JSON array
                        files_data = json.loads(files_json)
                        
                        # Look for sketch.ino
                        for file_data in files_data:
                            if isinstance(file_data, dict) and file_data.get('name') == 'sketch.ino':
                                sketch_content = file_data.get('content', '')
                                if sketch_content:
                                    print("Found sketch.ino using script tag approach")
                                    break
                except Exception as e:
                    print(f"Failed to parse script tag: {e}")
                    continue
                
                if sketch_content:
                    break
    
    # Approach 4: Direct regex on the HTML content
    if not sketch_content:
        try:
            # Pattern to match sketch.ino content with minimal context
            sketch_pattern = r'"name"\s*:\s*"sketch.ino"\s*,\s*"content"\s*:\s*"(.*?)(?:"\s*[,}])'
            sketch_matches = re.findall(sketch_pattern, response.text, re.DOTALL)
            
            if sketch_matches:
                # Use the first match
                raw_content = sketch_matches[0]
                # Unescape the content
                sketch_content = raw_content.replace('\\n', '\n').replace('\\\"', '\"').replace('\\\\', '\\')
                print(f"Found sketch.ino using direct regex (match length: {len(raw_content)})")
        except Exception as e:
            print(f"Failed to extract content using direct regex: {e}")
    
    # Approach 5: Manual extraction from content in the provided project
    if not sketch_content and "void setup()" in response.text and "void loop()" in response.text:
        try:
            # Try to extract the Arduino code directly from the page
            start_idx = response.text.find("void setup()")
            if start_idx > 0:
                # Look for a reasonable end of the code - either the end of void loop or a closing brace
                end_idx = response.text.find("}", response.text.find("void loop()", start_idx))
                if end_idx > start_idx:
                    # Extract the code and clean it up
                    raw_code = response.text[start_idx:end_idx+1]
                    # Remove HTML tags if present
                    clean_code = re.sub(r'<[^>]+>', '', raw_code)
                    sketch_content = clean_code.strip()
                    print("Extracted sketch directly from page content")
        except Exception as e:
            print(f"Failed to extract sketch manually: {e}")
    
    # Return the result
    if sketch_content:
        # Clean up the content - remove excess whitespace and normalize line endings
        sketch_content = sketch_content.strip()
        sketch_content = re.sub(r'\r\n', '\n', sketch_content)
        sketch_content = re.sub(r'\n{3,}', '\n\n', sketch_content)
        return sketch_content
    else:
        print("No sketch.ino found in the project")
        return None

def main():
    if len(sys.argv) > 1:
        url = sys.argv[1]
    else:
        url = input("Enter Wokwi project URL: ")
    
    sketch = extract_sketch_from_wokwi(url)
    if sketch:
        print("\n--- Sketch Content ---")
        print(sketch)
        print("---------------------")
        print(f"\nExtracted {len(sketch)} characters")
    
if __name__ == "__main__":
    main() 