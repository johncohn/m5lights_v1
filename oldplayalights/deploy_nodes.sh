#!/bin/bash

# NeoPixel Node Deployment Script - Enhanced with Robust Retry Logic
# 
# Features:
# - Up to 3 retry attempts per node
# - Detailed logging to ota.log file
# - Comprehensive success/failure tracking
# - Enhanced error detection and reporting
# - Beautiful summary table with attempt counts
# - Complete deployment history logging
#
# Compiles once, then uploads to multiple nodes via OTA with automatic retries

echo "Starting Enhanced NeoPixel Node Deployment..."

# Configuration
FQBN="esp32:esp32:m5stack_stickc_plus2"
PASSWORD="neopixel123"
BUILD_DIR="./build"
NETWORK_BASE="192.168.0"
START_IP=100
END_IP=250
OTA_PORT=3232
VERSION_FILE="version.txt"
MAX_RETRIES=3
ESPOTA_PATH="/Users/johncohn/Library/Arduino15/packages/esp32/hardware/esp32/3.2.1/tools/espota.py"

# Arrays for tracking nodes
declare -a NODES=()
declare -a LAST_SUCCESSFUL_NODES=()
declare -a LAST_FAILED_NODES=()
declare -a RETRY_NODES=()

# Function to get and increment version number
get_and_increment_version() {
    if [ ! -f "$VERSION_FILE" ]; then
        echo "1.1.0" > "$VERSION_FILE"
        echo "Created $VERSION_FILE with initial version 1.1.0"
    fi
    
    local current_version=$(cat "$VERSION_FILE")
    echo "Current version: $current_version"
    
    # Parse version (assumes X.Y.Z format)
    IFS='.' read -r major minor patch <<< "$current_version"
    
    # Increment patch version
    patch=$((patch + 1))
    
    local new_version="${major}.${minor}.${patch}"
    echo "$new_version" > "$VERSION_FILE"
    echo "New version: $new_version"
    
    # Create version header file
    cat > "version.h" << EOF
#ifndef VERSION_H
#define VERSION_H

#define FIRMWARE_VERSION "$new_version"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

#endif
EOF
    
    echo "Created version.h with version $new_version"
}

# Function to check if recompilation is needed
needs_recompile() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "No build cache found - full compile needed"
        return 0
    fi
    
    local build_time=$(stat -f %m "$BUILD_DIR" 2>/dev/null || stat -c %Y "$BUILD_DIR" 2>/dev/null || echo 0)
    local newest_source=0
    
    for file in *.ino *.cpp *.h; do
        if [ -f "$file" ]; then
            local file_time=$(stat -f %m "$file" 2>/dev/null || stat -c %Y "$file" 2>/dev/null || echo 0)
            if [ $file_time -gt $newest_source ]; then
                newest_source=$file_time
            fi
        fi
    done
    
    if [ $newest_source -gt $build_time ]; then
        echo "Source files changed - recompile needed"
        return 0
    else
        echo "No changes detected - using cached build"
        return 1
    fi
}

# Function to compile the project with force option
compile_project() {
    local force_compile=${1:-false}
    
    if $force_compile || needs_recompile; then
        echo "Updating version and compiling project..."
        get_and_increment_version
        
        mkdir -p "$BUILD_DIR"
        
        if arduino-cli compile --fqbn $FQBN --build-path "$BUILD_DIR" .; then
            echo "Compilation successful with new version!"
            return 0
        else
            echo "Compilation failed!"
            exit 1
        fi
    else
        echo "Using cached build - skipping compilation and version increment!"
        return 0
    fi
}

# Function to upload to a single node with retry logic
upload_to_node() {
    local ip=$1
    local node_num=$2
    local attempt=${3:-1}
    
    echo "Uploading to Node $node_num ($ip) - Attempt $attempt/$MAX_RETRIES..."
    
    # Log attempt start
    echo "$(date '+%Y-%m-%d %H:%M:%S') - Node $ip - Attempt $attempt - STARTED" >> ota.log
    
    # Find the compiled binary
    local binary_file="$BUILD_DIR/playalights_claude_v58.ino.bin"
    if [ ! -f "$binary_file" ]; then
        echo "   Node $node_num (Attempt $attempt): Binary file not found: $binary_file"
        echo "FAILED: Node $node_num ($ip) - Attempt $attempt failed (no binary)"
        echo "$(date '+%Y-%m-%d %H:%M:%S') - Node $ip - Attempt $attempt - FAILED (no binary)" >> ota.log
        return 1
    fi
    
    # Check if espota.py exists
    if [ ! -f "$ESPOTA_PATH" ]; then
        echo "   Node $node_num (Attempt $attempt): espota.py not found at: $ESPOTA_PATH"
        echo "FAILED: Node $node_num ($ip) - Attempt $attempt failed (no espota.py)"
        echo "$(date '+%Y-%m-%d %H:%M:%S') - Node $ip - Attempt $attempt - FAILED (no espota.py)" >> ota.log
        return 1
    fi
    
    # Use espota.py for OTA upload
    local upload_output
    local exit_code
    upload_output=$(python3 "$ESPOTA_PATH" -i "$ip" -p "$OTA_PORT" -a "$PASSWORD" -f "$binary_file" -r -d -t 60 2>&1)
    exit_code=$?
    
    # Show output with node prefix
    echo "$upload_output" | while IFS= read -r line; do
        echo "   Node $node_num (Attempt $attempt): $line"
    done
    
    # Much stricter success detection - require explicit success indicators
    if echo "$upload_output" | grep -q "Result: OK" && echo "$upload_output" | grep -q "Success" && echo "$upload_output" | grep -q "100%"; then
        echo "SUCCESS: Node $node_num ($ip) - Attempt $attempt succeeded"
        echo "$(date '+%Y-%m-%d %H:%M:%S') - Node $ip - Attempt $attempt - SUCCESS" >> ota.log
        return 0
    elif echo "$upload_output" | grep -q "ERROR\|FAILED\|Connection refused\|Timeout\|No route to host\|timed out\|Authentication Failed\|Host.*Not Found"; then
        local error_reason=$(echo "$upload_output" | grep -o "ERROR\|FAILED\|Connection refused\|Timeout\|No route to host\|timed out\|Authentication Failed\|Host.*Not Found" | head -1)
        echo "FAILED: Node $node_num ($ip) - Attempt $attempt failed ($error_reason)"
        echo "$(date '+%Y-%m-%d %H:%M:%S') - Node $ip - Attempt $attempt - FAILED ($error_reason)" >> ota.log
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "FAILED: Node $node_num ($ip) - Attempt $attempt failed (exit code: $exit_code)"
        echo "$(date '+%Y-%m-%d %H:%M:%S') - Node $ip - Attempt $attempt - FAILED (exit code: $exit_code)" >> ota.log
        return 1
    else
        # If no explicit success indicators, treat as failed
        echo "FAILED: Node $node_num ($ip) - Attempt $attempt failed (incomplete upload)"
        echo "$(date '+%Y-%m-%d %H:%M:%S') - Node $ip - Attempt $attempt - FAILED (incomplete upload)" >> ota.log
        return 1
    fi
}

# Function to discover nodes
discover_nodes() {
    echo "Discovering nodes on network ${NETWORK_BASE}.${START_IP}-${END_IP}..."
    
    local discovered_nodes=()
    local temp_files=()
    local pids=()
    
    for ((i=START_IP; i<=END_IP; i++)); do
        local ip="${NETWORK_BASE}.${i}"
        local temp_file=$(mktemp)
        temp_files+=("$temp_file")
        
        (
            if ping -c 1 -W 500 "$ip" >/dev/null 2>&1; then
                echo "$ip" > "$temp_file"
            fi
        ) &
        pids+=($!)
        
        if [ ${#pids[@]} -ge 20 ]; then
            for pid in "${pids[@]}"; do
                wait "$pid"
            done
            pids=()
            local progress=$(( (i - START_IP + 1) * 100 / (END_IP - START_IP + 1) ))
            echo "Progress: ${progress}% complete"
        fi
    done
    
    for pid in "${pids[@]}"; do
        wait "$pid"
    done
    
    for temp_file in "${temp_files[@]}"; do
        if [ -s "$temp_file" ]; then
            local ip=$(cat "$temp_file")
            discovered_nodes+=("$ip")
            echo "Found responsive device: $ip"
        fi
        rm -f "$temp_file"
    done
    
    NODES=("${discovered_nodes[@]}")
    
    echo ""
    if [ ${#NODES[@]} -eq 0 ]; then
        echo "No responsive devices found"
        return 1
    else
        echo "Found ${#NODES[@]} responsive devices"
        return 0
    fi
}

# Function to load only failed nodes from OTA.txt
load_failed_nodes() {
    local ota_file="OTA.txt"
    
    if [ ! -f "$ota_file" ]; then
        echo "No $ota_file found. Cannot retry failed nodes."
        echo "Run a full deployment first to generate the summary file."
        return 1
    fi
    
    echo "Loading failed nodes from $ota_file..."
    local failed_nodes=()
    
    # Read failed IPs from OTA.txt
    while IFS=', ' read -r ip attempts status || [[ -n "$ip" ]]; do
        # Skip comments and empty lines
        if [[ "$ip" =~ ^[[:space:]]*# ]] || [[ -z "${ip// }" ]]; then
            continue
        fi
        
        # Clean up whitespace and check if status is FAILED
        ip=$(echo "$ip" | xargs)
        status=$(echo "$status" | xargs)
        
        if [[ "$status" == "FAILED" ]]; then
            # Validate IP format
            if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
                failed_nodes+=("$ip")
                echo "   Found failed node: $ip (previous attempts: $attempts)"
            fi
        fi
    done < "$ota_file"
    
    if [ ${#failed_nodes[@]} -eq 0 ]; then
        echo "🎉 No failed nodes found in $ota_file - all nodes previously succeeded!"
        return 1
    fi
    
    NODES=("${failed_nodes[@]}")
    echo "Ready to retry ${#NODES[@]} failed nodes from $ota_file"
    return 0
}

# Function to load nodes from file
load_nodes_from_file() {
    local node_file="nodes.txt"
    
    if [ ! -f "$node_file" ]; then
        echo "No $node_file found. Creating template..."
        cat > "$node_file" << 'EOF'
# NeoPixel Node IP Addresses
# Add one IP address per line
# Lines starting with # are ignored
192.168.0.140
192.168.0.141
192.168.0.148
192.168.0.150
192.168.0.181
192.168.0.232
192.168.0.236
192.168.0.238
EOF
        echo "Created $node_file with default IPs. Edit this file and run script again."
        exit 0
    fi
    
    echo "Loading nodes from $node_file..."
    local file_nodes=()
    local working_nodes=()
    
    # Read IPs from file, skip comments and empty lines
    while IFS= read -r line || [[ -n "$line" ]]; do
        # Skip comments and empty lines
        if [[ "$line" =~ ^[[:space:]]*# ]] || [[ -z "${line// }" ]]; then
            continue
        fi
        
        # Clean up whitespace
        ip=$(echo "$line" | xargs)
        
        # Validate IP format
        if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
            file_nodes+=("$ip")
        else
            echo "Warning: Invalid IP format: $line"
        fi
    done < "$node_file"
    
    echo "Found ${#file_nodes[@]} IPs in $node_file"
    
    # Use all nodes from file, skipping ping test for faster deployment
    NODES=("${file_nodes[@]}")
    echo "Ready to deploy to ${#NODES[@]} nodes from $node_file (skipping connectivity test)"
}

# Function to add known nodes (legacy fallback)
add_known_nodes() {
    echo "Using legacy hardcoded node list..."
    local known_ips=("192.168.0.140" "192.168.0.141" "192.168.0.148" "192.168.0.150" "192.168.0.181" "192.168.0.232" "192.168.0.236" "192.168.0.238")
    local working_nodes=()
    
    for ip in "${known_ips[@]}"; do
        echo -n "Testing $ip... "
        if ping -c 1 -W 2000 "$ip" >/dev/null 2>&1; then
            working_nodes+=("$ip")
            echo "ONLINE"
        else
            echo "OFFLINE"
        fi
    done
    
    NODES=("${working_nodes[@]}")
    echo "Ready to deploy to ${#NODES[@]} nodes"
}

# Function to send serial command to a connected node
send_serial_command() {
    local command="$1"
    local port="$2"
    
    if [ -z "$port" ]; then
        # Try to auto-detect USB serial port
        if [ -e "/dev/ttyACM0" ]; then
            port="/dev/ttyACM0"
        elif [ -e "/dev/ttyUSB0" ]; then
            port="/dev/ttyUSB0"
        elif ls /dev/cu.usbmodem* >/dev/null 2>&1; then
            port=$(ls /dev/cu.usbmodem* | head -1)
        else
            echo "   WARNING: No USB serial port detected. Serial commands skipped."
            echo "   To use manual ESP-NOW control, connect a node via USB and retry."
            return 1
        fi
    fi
    
    if [ ! -e "$port" ]; then
        echo "   WARNING: Serial port $port not found. Serial commands skipped."
        return 1
    fi
    
    echo "   Sending '$command' to $port..."
    
    # Send command and wait for response
    timeout 5s bash -c "
        echo '$command' > '$port'
        sleep 1
        # Read response (if any)
        timeout 2s cat '$port' 2>/dev/null | head -1
    " || echo "   Serial command sent (no response read)"
    
    return 0
}

# Function to deploy with retry logic
deploy_with_retries() {
    echo "Deploying to nodes with retry logic (max $MAX_RETRIES attempts per node)..."
    local success_count=0
    local failed_count=0
    
    # Initialize detailed log file
    echo "# OTA Deployment Log - $(date '+%Y-%m-%d %H:%M:%S')" > ota.log
    echo "# Format: TIMESTAMP - IP_ADDRESS - ATTEMPT - STATUS" >> ota.log
    echo "" >> ota.log
    
    LAST_SUCCESSFUL_NODES=()
    LAST_FAILED_NODES=()
    RETRY_NODES=()
    
    # Step 1: Send ESP-NOW suspend command via USB serial
    echo ""
    echo "=== STEP 1: Suspending ESP-NOW traffic on all nodes ==="
    echo "Looking for USB-connected node to send global suspend command..."
    if send_serial_command "SUSPEND_ESPNOW"; then
        echo "✓ ESP-NOW suspend command sent successfully"
        echo "   All nodes should now suspend ESP-NOW traffic to prevent OTA interference"
        sleep 2  # Give time for command to propagate across mesh
    else
        echo "⚠ Could not send ESP-NOW suspend command via USB"
        echo "   OTA may experience interference from ESP-NOW traffic"
        echo "   Consider manually connecting a node via USB for better reliability"
    fi
    
    # Step 2: Deploy to all nodes via OTA
    echo ""
    echo "=== STEP 2: Deploying firmware via OTA ==="
    
    # First pass - try all nodes once
    for i in "${!NODES[@]}"; do
        local ip=${NODES[$i]}
        local node_num=$((i + 1))
        
        echo ""
        echo "=== First attempt: Node $node_num ($ip) ==="
        if upload_to_node $ip $node_num 1; then
            ((success_count++))
            LAST_SUCCESSFUL_NODES+=("$ip")
        else
            RETRY_NODES+=("$ip")
        fi
    done
    
    # Retry failed nodes
    local retry_attempt=2
    while [ ${#RETRY_NODES[@]} -gt 0 ] && [ $retry_attempt -le $MAX_RETRIES ]; do
        echo ""
        echo "=== Retry Pass $retry_attempt (${#RETRY_NODES[@]} nodes) ==="
        local current_retry_nodes=("${RETRY_NODES[@]}")
        RETRY_NODES=()
        
        for ip in "${current_retry_nodes[@]}"; do
            # Find original node number
            local node_num=0
            for i in "${!NODES[@]}"; do
                if [ "${NODES[$i]}" = "$ip" ]; then
                    node_num=$((i + 1))
                    break
                fi
            done
            
            echo ""
            echo "=== Retry attempt $retry_attempt: Node $node_num ($ip) ==="
            if upload_to_node $ip $node_num $retry_attempt; then
                ((success_count++))
                LAST_SUCCESSFUL_NODES+=("$ip")
            else
                RETRY_NODES+=("$ip")
            fi
        done
        
        ((retry_attempt++))
    done
    
    # Final failed nodes
    LAST_FAILED_NODES=("${RETRY_NODES[@]}")
    failed_count=${#LAST_FAILED_NODES[@]}
    
    echo ""
    echo "=================================="
    echo "Deployment Complete: $success_count/${#NODES[@]} nodes updated successfully, $failed_count failed after $MAX_RETRIES attempts"
    
    if [ ${#LAST_SUCCESSFUL_NODES[@]} -gt 0 ]; then
        echo ""
        echo "SUCCESSFUL UPLOADS:"
        for ip in "${LAST_SUCCESSFUL_NODES[@]}"; do
            echo "   ✓ $ip"
        done
    fi
    
    if [ ${#LAST_FAILED_NODES[@]} -gt 0 ]; then
        echo ""
        echo "FAILED UPLOADS (after $MAX_RETRIES attempts):"
        for ip in "${LAST_FAILED_NODES[@]}"; do
            echo "   ✗ $ip"
        done
        echo ""
        echo "Troubleshooting failed nodes:"
        echo "1. Check if nodes are powered on and connected to WiFi"
        echo "2. Try manually uploading to individual nodes"
        echo "3. Leader nodes may fail due to ESP-NOW traffic - try again when not leading"
        echo "4. Remove permanently offline IPs from nodes.txt"
    fi
    
    # Step 3: Send ESP-NOW resume command via USB serial
    echo ""
    echo "=== STEP 3: Resuming ESP-NOW traffic on all nodes ==="
    echo "Looking for USB-connected node to send global resume command..."
    if send_serial_command "RESUME_ESPNOW"; then
        echo "✓ ESP-NOW resume command sent successfully"
        echo "   All nodes should now resume normal ESP-NOW mesh operation"
    else
        echo "⚠ Could not send ESP-NOW resume command via USB"
        echo "   Nodes may remain suspended - manually send 'RESUME_ESPNOW' via serial"
        echo "   Or power cycle nodes to restore normal operation"
    fi
    
    # Generate comprehensive summary and append to log
    echo "" >> ota.log
    echo "# DEPLOYMENT SUMMARY - $(date '+%Y-%m-%d %H:%M:%S')" >> ota.log
    echo "# Total Nodes: ${#NODES[@]}" >> ota.log
    echo "# Successful: $success_count" >> ota.log
    echo "# Failed: $failed_count" >> ota.log
    echo "# Max Retries: $MAX_RETRIES" >> ota.log
    echo "" >> ota.log
    
    # Create clean summary file for retry automation
    echo "# OTA Deployment Summary - $(date '+%Y-%m-%d %H:%M:%S')" > OTA.txt
    echo "# Format: IP_ADDRESS, ATTEMPTS, STATUS" >> OTA.txt
    
    # Add detailed results for each node to both files
    for i in "${!NODES[@]}"; do
        local ip=${NODES[$i]}
        local node_num=$((i + 1))
        
        # Count attempts for this node
        local attempts=$(grep "Node $ip" ota.log | wc -l | xargs)
        
        # Determine final status
        if [[ " ${LAST_SUCCESSFUL_NODES[@]} " =~ " ${ip} " ]]; then
            echo "Node $ip - $attempts attempts - SUCCESS" >> ota.log
            echo "$ip, $attempts, SUCCESS" >> OTA.txt
        else
            echo "Node $ip - $attempts attempts - FAILED" >> ota.log
            echo "$ip, $attempts, FAILED" >> OTA.txt
        fi
    done
    
    # Display final summary
    echo ""
    echo "=================================================="
    echo "COMPREHENSIVE OTA DEPLOYMENT SUMMARY"
    echo "=================================================="
    echo "📊 Total Nodes Attempted: ${#NODES[@]}"
    echo "✅ Successfully Updated: $success_count"
    echo "❌ Failed After $MAX_RETRIES Attempts: $failed_count"
    echo "📝 Detailed log saved to: ota.log"
    echo "📋 Clean summary saved to: OTA.txt"
    if [ $failed_count -gt 0 ]; then
        echo "🔄 To retry failed nodes only: ./deploy_nodes.sh → option 3"
    fi
    echo ""
    
    if [ -f ota.log ]; then
        echo "📋 DETAILED RESULTS:"
        echo "┌─────────────────┬──────────┬────────────┐"
        echo "│ IP Address      │ Attempts │ Status     │"
        echo "├─────────────────┼──────────┼────────────┤"
        
        for i in "${!NODES[@]}"; do
            local ip=${NODES[$i]}
            local attempts=$(grep "Node $ip" ota.log | wc -l | xargs)
            
            if [[ " ${LAST_SUCCESSFUL_NODES[@]} " =~ " ${ip} " ]]; then
                printf "│ %-15s │ %-8s │ %-10s │\n" "$ip" "$attempts" "✅ SUCCESS"
            else
                printf "│ %-15s │ %-8s │ %-10s │\n" "$ip" "$attempts" "❌ FAILED"
            fi
        done
        
        echo "└─────────────────┴──────────┴────────────┘"
        echo ""
        echo "📄 Complete log contents:"
        echo "----------------------------------------"
        cat ota.log
        echo "----------------------------------------"
    fi
    
    echo ""
    echo "🎯 Deployment $([ $failed_count -eq 0 ] && echo "COMPLETED SUCCESSFULLY" || echo "COMPLETED WITH $failed_count FAILURES")"
    echo "=================================================="
}

# Function to clean cache
clean_cache() {
    echo "Cleaning build cache..."
    rm -rf "$BUILD_DIR" "./cache"
    echo "Cache cleaned!"
}

# Function to show cache status
show_cache_status() {
    echo "Build Cache Status:"
    if [ -d "$BUILD_DIR" ]; then
        local cache_size=$(du -sh "$BUILD_DIR" 2>/dev/null | cut -f1)
        echo "   Build cache: $cache_size"
    else
        echo "   No build cache found"
    fi
    
    if [ -f "$VERSION_FILE" ]; then
        local current_version=$(cat "$VERSION_FILE")
        echo "   Current version: $current_version"
    else
        echo "   No version file found"
    fi
}

# Main function
main() {
    echo "Network: ${NETWORK_BASE}.${START_IP}-${END_IP}"
    echo "Password: $PASSWORD"
    echo "Max retries per node: $MAX_RETRIES"
    echo ""
    
    show_cache_status
    echo ""
    
    echo "Choose action:"
    echo "1) Deploy (compile if needed + upload with retries) [DEFAULT]"
    echo "2) Force recompile and deploy"
    echo "3) Upload only (skip compile)"
    echo "4) Compile only (no upload, bump version)"
    echo "5) Clean cache"
    read -p "Enter choice (1-5) [1]: " action_choice
    
    action_choice=${action_choice:-1}
    
    # Handle compile-only mode first (doesn't need nodes)
    if [ "$action_choice" = "4" ]; then
        echo "Compile-only mode: compile with version increment (using cache if possible)..."
        compile_project true
        echo ""
        echo "=================================="
        echo "Compile-only complete!"
        if [ -f "$VERSION_FILE" ]; then
            local current_version=$(cat "$VERSION_FILE")
            echo "New version: $current_version"
        fi
        echo "Build ready for deployment when needed."
        echo "Use option 3 (Upload only) to deploy this build later."
        echo "=================================="
        exit 0
    fi
    
    # Handle clean cache mode (doesn't need nodes)
    if [ "$action_choice" = "5" ]; then
        clean_cache
        exit 0
    fi
    
    # For all other modes, we need to discover nodes first
    echo "Choose node discovery method:"
    echo "1) Auto-discover nodes on network"
    echo "2) Load from nodes.txt file [DEFAULT]"
    echo "3) Retry failed nodes from OTA.txt"
    echo "4) Manual IP entry"
    read -p "Enter choice (1-4) [2]: " discovery_choice
    
    discovery_choice=${discovery_choice:-2}
    
    case $discovery_choice in
        1)
            if ! discover_nodes; then
                echo "Auto-discovery failed. Using nodes.txt if available..."
                if [ -f "nodes.txt" ]; then
                    load_nodes_from_file
                else
                    add_known_nodes
                fi
            fi
            ;;
        2)
            load_nodes_from_file
            ;;
        3)
            if ! load_failed_nodes; then
                echo "Falling back to full nodes.txt file..."
                load_nodes_from_file
            fi
            ;;
        4)
            echo "Enter IP addresses (press Enter to finish):"
            NODES=()
            while true; do
                read -p "Node IP: " ip
                if [ -z "$ip" ]; then
                    break
                fi
                if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
                    NODES+=("$ip")
                    echo "Added: $ip"
                else
                    echo "Invalid IP format"
                fi
            done
            ;;
        *)
            echo "Invalid choice. Using nodes.txt if available..."
            if [ -f "nodes.txt" ]; then
                load_nodes_from_file
            else
                add_known_nodes
            fi
            ;;
    esac
    
    if [ ${#NODES[@]} -eq 0 ]; then
        echo "No nodes configured. Exiting."
        exit 1
    fi
    
    # Handle the remaining action choices that need nodes
    case $action_choice in
        1)
            compile_project
            ;;
        2)
            echo "Forcing full recompile with version increment..."
            clean_cache
            compile_project true
            ;;
        3)
            if [ ! -d "$BUILD_DIR" ]; then
                echo "No build found! Run compile first."
                exit 1
            fi
            echo "Skipping compile - using existing build"
            ;;
        *)
            echo "Invalid choice. Exiting."
            exit 1
            ;;
    esac
    
    deploy_with_retries
}

# Check dependencies
check_dependencies() {
    if ! command -v arduino-cli >/dev/null 2>&1; then
        echo "Missing arduino-cli. Install with: brew install arduino-cli"
        exit 1
    fi
    
    if ! command -v expect >/dev/null 2>&1; then
        echo "Warning: expect not found. Install with: brew install expect"
        echo "Upload timeouts may not work properly without expect."
    fi
}

check_dependencies
main