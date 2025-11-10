#!/bin/bash

# ESP-NOW Manual Control Script
# Use this to manually suspend/resume ESP-NOW traffic for OTA deployments

# Auto-detect USB serial port
detect_port() {
    if [ -e "/dev/ttyACM0" ]; then
        echo "/dev/ttyACM0"
    elif [ -e "/dev/ttyUSB0" ]; then
        echo "/dev/ttyUSB0"  
    elif ls /dev/cu.usbmodem* >/dev/null 2>&1; then
        ls /dev/cu.usbmodem* | head -1
    else
        echo ""
    fi
}

# Send command to node
send_command() {
    local command="$1"
    local port="$2"
    
    if [ -z "$port" ]; then
        port=$(detect_port)
    fi
    
    if [ -z "$port" ]; then
        echo "ERROR: No USB serial port found"
        echo "Connect a node via USB cable and try again"
        exit 1
    fi
    
    if [ ! -e "$port" ]; then
        echo "ERROR: Serial port $port not found"
        exit 1
    fi
    
    echo "Sending '$command' to $port..."
    echo "$command" > "$port"
    sleep 1
    
    # Try to read response
    timeout 2s cat "$port" 2>/dev/null | head -1 || echo "Command sent (no response read)"
}

case "$1" in
    "suspend"|"SUSPEND")
        echo "=== Suspending ESP-NOW traffic on all nodes ==="
        send_command "SUSPEND_ESPNOW" "$2"
        echo "✓ All nodes should now suspend ESP-NOW traffic"
        ;;
    "resume"|"RESUME") 
        echo "=== Resuming ESP-NOW traffic on all nodes ==="
        send_command "RESUME_ESPNOW" "$2"
        echo "✓ All nodes should now resume ESP-NOW traffic"
        ;;
    "status"|"STATUS")
        echo "=== Checking ESP-NOW status ==="
        send_command "STATUS_ESPNOW" "$2"
        ;;
    *)
        echo "ESP-NOW Manual Control Script"
        echo ""
        echo "Usage: $0 <command> [port]"
        echo ""
        echo "Commands:"
        echo "  suspend  - Suspend ESP-NOW traffic on all nodes (before OTA)"
        echo "  resume   - Resume ESP-NOW traffic on all nodes (after OTA)"  
        echo "  status   - Check current ESP-NOW status"
        echo ""
        echo "Port (optional): Serial port path (auto-detected if omitted)"
        echo ""
        echo "Examples:"
        echo "  $0 suspend                    # Auto-detect port and suspend"
        echo "  $0 resume /dev/ttyACM0        # Resume using specific port"
        echo "  $0 status                     # Check status"
        echo ""
        echo "Workflow for manual OTA deployment:"
        echo "1. $0 suspend                 # Stop ESP-NOW traffic"
        echo "2. Use Arduino IDE or espota.py to upload firmware"
        echo "3. $0 resume                  # Restore ESP-NOW traffic"
        ;;
esac