#!/bin/bash

# Simple USB Upload Script
echo "=== M5StickC Plus2 USB Upload ==="

# Compile once
echo "Compiling firmware..."
if ! arduino-cli compile --fqbn esp32:esp32:m5stack_stickc_plus2 --build-path "./build" .; then
    echo "❌ Compilation failed!"
    exit 1
fi
echo "✅ Firmware compiled!"

NODE_COUNT=0
while true; do
    NODE_COUNT=$((NODE_COUNT + 1))
    echo ""
    echo "=== Node #$NODE_COUNT ==="
    echo "1. Connect your M5StickC Plus2 via USB"
    echo "2. Press Enter to upload (or type 'q' to quit)"
    read -r input
    
    if [ "$input" = "q" ] || [ "$input" = "Q" ]; then
        echo "Done!"
        break
    fi
    
    # Find USB port
    USB_PORT=""
    for port in /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial*; do
        if [ -e "$port" ]; then
            # Kill any processes using this port
            lsof -t "$port" 2>/dev/null | xargs -r kill 2>/dev/null || true
            sleep 1
            
            if [ -c "$port" ] && [ -w "$port" ]; then
                USB_PORT="$port"
                echo "Found device: $USB_PORT"
                break
            fi
        fi
    done
    
    if [ -z "$USB_PORT" ]; then
        echo "❌ No USB device found!"
        continue
    fi
    
    # Upload
    echo "Uploading..."
    if arduino-cli upload --fqbn esp32:esp32:m5stack_stickc_plus2 --port "$USB_PORT" --input-dir "./build"; then
        echo "✅ Node #$NODE_COUNT uploaded successfully!"
        echo "Please disconnect and connect next node."
    else
        echo "❌ Upload failed!"
        NODE_COUNT=$((NODE_COUNT - 1))  # Don't count failed uploads
    fi
done

echo "Upload session complete! Updated $NODE_COUNT nodes."