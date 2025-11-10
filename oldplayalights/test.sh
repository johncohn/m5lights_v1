#!/bin/bash

# Test script to check syntax
echo "Testing syntax..."

# Simple function to test
test_function() {
    echo "Function works"
    local test_array=("item1" "item2")
    
    for item in "${test_array[@]}"; do
        echo "Processing: $item"
    done
    
    case "test" in
        "test")
            echo "Case works"
            ;;
        *)
            echo "Default case"
            ;;
    esac
}

# Test condition
if [ -f "test.txt" ]; then
    echo "File exists"
else
    echo "File doesn't exist"
fi

# Call function
test_function

echo "Syntax test complete"