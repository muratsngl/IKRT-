#!/bin/bash
# Git add script for IKRT project
# Adds all files from previously committed directories

echo "Adding all files from committed directories to git staging area..."

# Add files from all previously committed directories
git add .vscode/
git add assets/
git add include/
git add src/
git add vendor/

# Add root level files
git add CMakeLists.txt
git add build.sh
git add README.md
git add *.md
git add *.txt
git add *.ini
git add *.py
git add *.json
git add *.sh

echo "Files added to staging area. Current status:"
git status --short

echo "Ready to commit with: git commit -m 'your message'"