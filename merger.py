import os
import re
import shutil

# Template for the upper part of the SYNOR 5000 code
synor_5000_header = """///////////////////////////////////////////////////////////////////////////////
// Test Program   : 104585721-AC
// Creation date  : 07/10/2022
// Assembly       : 104212188-AB
// Wiring Diagram : 103612566-AD
// Wiring List    : 103612565-AC
// Drawing        : N/A
// Specification  : N/A
///////////////////////////////////////////////////////////////////////////////
// Revision Detail: Rev AA 07/10/2022
// Revision Detail: - Creation in GEMS
// Revision Detail: Rev AB 14/02/2023
// Revision Detail: - Passage du CKT DIAG de AC à AD pour inversion des fils TX et RX en LH
// Revision Detail: Rev AC 02/05/2024
// Revision Detail: - Transfert from SYNOR 1200 to SYNOR 5000
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////  TO BE MODIFIED  //////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Varibles forGeMS part number and revision: TO BE MODIFIED
var $DMS_PN = "104585721"                              // GEMS test program P/N
var $DMS_REV = "AC"                                    // GEMS test program revision
var $DMS_TOOL_PN = "104212188"                         // GEMS tested tool P/N
var $DMS_TOOL_DESCRIPTION = "CHASSIS, WITH HARNESS, CALIPER UP TO REV. AB"  // GEMS Tested tool description
var $DMS_DIAGRAM_PN = "103612566"                      // GEMS tested tool diagram P/N
var $DMS_DIAGRAM_REV = "AD"                            // GEMS tested tool diagram revision
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////  DO NOT MODIFY  ///////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Check Tester calibration
var $BufferTesterCabibration = $TESTER_DATE.desc
HV R 14 ($BufferTesterCabibration)

calc $BufferTesterDate = TextToDate($TESTER_DATE);
calc $BufferTestStartDate = TextToDate($TEST_START_DATE);

if $BufferTestStartDate >= $BufferTesterDate then
    var $BufferTesterCabibration = "Tester out of calibration."
    HV R 14 ($BufferTesterCabibration)
end else
    var $BufferTesterCabibration = "Tester is calibrated."
    HV V 14 ($BufferTesterCabibration)
end

// Path creation for connection dialog box
var $BoxTestConn = "C:\\Test Programs\\" + $DMS_PN
var $BoxTestConn = $BoxTestConn.text + "-"
var $BoxTestConn = $BoxTestConn.text + $DMS_REV
var $BoxTestConn = $BoxTestConn.text + "\\"
var $BoxTestConn = $BoxTestConn.text + "TestConnections.htm"

//Dialog box for operator informations
DIAL (StdDialogBox)

// Dialog box for test connections
NOTE ($BoxTestConn)

// Formatting test serila number foar file test result
var $BufferSerialNumber = "SN-" + $SerialNumber.text

// Formatting test tittle line 1 for WinReport
var $bufferTittle = "TEST OF " + $DMS_TOOL_DESCRIPTION

// Formatting test tittle line 2 for WinReport
var $bufferTittle1 = "FOLLOWING THE DIAGRAM " + $DMS_DIAGRAM_PN
var $bufferTittle1 = $bufferTittle1 + "-"
var $bufferTittle1 = $bufferTittle1 + $DMS_DIAGRAM_REV
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////  START OF THE TEST  ///////////////////////////
///////////////////////////////////////////////////////////////////////////////
"""

# Function to extract the relevant test part from a SYNOR 1200 program
def extract_test_section(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()
    
    start_marker = "TARE :"
    extracting = False
    test_section = []
    
    for line in lines:
        if start_marker in line:
            extracting = True
        if extracting:
            test_section.append(line)
    
    return test_section

# Function to merge the SYNOR 5000 header with the test section from SYNOR 1200
def merge_programs(source_file, target_file):
    test_section = extract_test_section(source_file)
    
    with open(target_file, 'w') as target:
        target.write(synor_5000_header)
        target.writelines(test_section)
    
    print(f'Merged program written to {target_file}')

# Function to get the most recent file based on its revision identifier
def get_most_recent_file(files):
    def parse_revision(filename):
        match = re.search(r'\d{8}[A-Z]{2}', filename)
        if match:
            return match.group()
        return None
    
    files_with_revisions = [(file, parse_revision(file)) for file in files]
    files_with_revisions = [item for item in files_with_revisions if item[1]]
    
    if not files_with_revisions:
        raise ValueError("No valid revision identifiers found in filenames.")
    
    most_recent_file = max(files_with_revisions, key=lambda item: item[1])[0]
    
    return most_recent_file

# Function to recursively find all .tes files and process the most recent one in each directory
def process_files(root_directory, output_directory):
    for root, _, files in os.walk(root_directory):
        tes_files = [file for file in files if file.endswith('.tes')]
        if tes_files:
            try:
                most_recent_file = get_most_recent_file(tes_files)
                source_file = os.path.join(root, most_recent_file)
                # Create a corresponding target file path in the output directory
                relative_path = os.path.relpath(root, root_directory)
                target_dir = os.path.join(output_directory, relative_path)
                os.makedirs(target_dir, exist_ok=True)
                target_file = os.path.join(target_dir, os.path.splitext(most_recent_file)[0] + '_merged.txt')
                
                merge_programs(source_file, target_file)
            except ValueError as e:
                print(f"Skipping directory {root} due to error: {e}")

if __name__ == "__main__":
    # Define the root directory to search for .tes files and the output directory
    root_directory = r'E:\Programs to be transfered'
    output_directory = r'C:\Users\yassi\Desktop\destination\portage'
    
    # Process all .tes files
    process_files(root_directory, output_directory)
    print(f'Processing complete. Merged files are saved in {output_directory}')
