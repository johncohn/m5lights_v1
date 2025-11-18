#!/bin/bash

# USB Upload Automation for M5StickC Plus2 nodes
# Hook up a node via USB and run this script

echo "=== M5StickC Plus2 USB Upload Automation ==="
echo "Hook up your M5StickC Plus2 via USB and press Enter to upload..."
read -p "Ready? Press Enter to continue..."

# Function to find M5StickC Plus2 USB port
find_usb_port() {
    echo "Scanning for USB devices..."
    
    # Look for common USB serial patterns
    for port in /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial*; do
        if [ -e "$port" ]; then
            echo "Found potential device: $port"
            
            # Check if port is available (not busy)
            if lsof "$port" >/dev/null 2>&1; then
                echo "  Port $port is busy - killing processes..."
                # Kill any processes using this port
                lsof -t "$port" | xargs -r kill 2>/dev/null || true
                sleep 2
            fi
            
            # Test if it's accessible
            if [ -c "$port" ] && [ -w "$port" ]; then
                echo "  Using port: $port"
                USB_PORT="$port"
                return 0
            fi
        fi
    done
    
    echo "No USB device found!"
    echo "Make sure your M5StickC Plus2 is connected via USB"
    echo "Available ports:"
    ls -la /dev/cu.* | grep -E "(usbserial|SLAB|wchusbserial)" || echo "  None found"
    return 1
}

# Function to compile firmware once
compile_firmware() {
    echo "Compiling firmware (one-time)..."
    if ! arduino-cli compile --fqbn esp32:esp32:m5stack_stickc_plus2 --build-path "./build" .; then
        echo "❌ Compilation failed!"
        return 1
    fi
    echo "✅ Firmware compiled successfully!"
    return 0
}

# Function to upload firmware (no compilation)
upload_firmware() {
    echo "Uploading to $USB_PORT..."
    if arduino-cli upload --fqbn esp32:esp32:m5stack_stickc_plus2 --port "$USB_PORT" --input-dir "./build"; then
        echo "✅ Upload successful!"
        echo "You can disconnect this node and connect the next one."
        return 0
    else
        echo "❌ Upload failed!"
        return 1
    fi
}

# Compile once at the start
echo ""
if ! compile_firmware; then
    echo "Failed to compile firmware. Exiting."
    exit 1
fi

# Main loop - just upload to each device
NODE_COUNT=0
while true; do
    echo ""
    echo "=== Ready for next node ==="
    read -p "Connect your node via USB and press Enter when ready (or 'q' to quit): " choice
    if [ "$choice" = "q" ] || [ "$choice" = "Q" ]; then
        break
    fi
    
    if find_usb_port; then
        NODE_COUNT=$((NODE_COUNT + 1))
        echo "Uploading to Node #$NODE_COUNT..."
        if upload_firmware; then
            echo ""
            echo "✅ Node #$NODE_COUNT updated successfully!"
            echo "Please disconnect this node before connecting the next one."
        else
            echo ""
            read -p "Upload failed. Try again? (y/n): " retry
            if [ "$retry" != "y" ] && [ "$retry" != "Y" ]; then
                break
            fi
        fi
    else
        echo ""
        read -p "No device found. Try again? (y/n): " retry
        if [ "$retry" != "y" ] && [ "$retry" != "Y" ]; then
            break
        fi
    fi
done

echo "Upload session complete!"