System for Creating Snapshots and Isolating Dangerous Files

Description

This program is designed to create snapshots of file systems, analyze files for malicious content and quarantine suspicious files. 
It recursively scans directories, captures metadata of files, checks for missing permissions and isolates potentially dangerous files in a separate directory.

Features

    Snapshot Creation: Recursively scans directories and creates snapshots of files, capturing metadata such as name, size, permissions and timestamps.
    Malicious Content Analysis: Checks files for malicious content using the provided verify_for_malicious.sh script.
    Quarantine Functionality: Moves suspicious files to a designated quarantine directory for further investigation.
    Parallel Processing: Utilizes parallel processing to improve efficiency in handling large file systems.
