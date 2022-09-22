#!/bin/bash

# Only use the first C++ example.
rm -rf android java-maven java-tutorial make-variables tutorial
cp -R cpp-tutorial/stage1/* .
source ../build-cmd
