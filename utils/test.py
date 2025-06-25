import serial
import time
import os
import threading

# Change COM port and baud rate accordingly
ser = serial.Serial('COM13', 115200, timeout=1)

# Terminal bar chart settings
MAX_ADC = 4095
BAR_WIDTH = 50  # Max characters per bar

# Calculate total number of channels based on keyboard_keys array dimensions
# ADC_CHANNEL_COUNT * AMUX_CHANNEL_COUNT (assuming 16 total based on original script)
TOTAL_CHANNELS = 16
CHANNELS = list(range(0, TOTAL_CHANNELS))

# Initialize min/max tracking
min_vals = [MAX_ADC] * TOTAL_CHANNELS
max_vals = [0] * TOTAL_CHANNELS

def clear_terminal():
    os.system('cls' if os.name == 'nt' else 'clear')

def draw_bars(values):
    clear_terminal()
    print("Live ADC Feed from keyboard_keys array (~1 kHz)")
    print("-" * 60)
    for i, val in enumerate(values):
        if i not in CHANNELS:
            continue
        bar_len = int((val / MAX_ADC) * BAR_WIDTH)
        bar = '#' * bar_len
        print(f"CH{i:02}: [{bar:<{BAR_WIDTH}}] {val:4}")
        print(f"   Min: {min_vals[i]:4}  Max: {max_vals[i]:4}  Diff: {max_vals[i] - min_vals[i]:4}")
    print("-" * 60)
    print("Press Ctrl+C to stop streaming")

def send_stream_command():
    """Send the stream command to start data flow"""
    try:
        # Clear any existing data in buffer
        ser.reset_input_buffer()
        
        # Send stream command
        ser.write(b'stream\r\n')
        print("Sent 'stream' command to device...")
        
        # Wait for acknowledgment
        time.sleep(0.5)
        
        # Read and discard any initial response messages
        timeout_count = 0
        while timeout_count < 10:  # Max 1 second wait
            try:
                line = ser.readline().decode(errors='ignore').strip()
                if not line:
                    timeout_count += 1
                    time.sleep(0.1)
                    continue
                    
                print(f"Device response: {line}")
                
                # Look for streaming confirmation message
                if "streaming" in line.lower() or "ctrl+c" in line.lower():
                    print("Streaming started successfully!")
                    return True
                    
            except Exception as e:
                print(f"Error reading response: {e}")
                timeout_count += 1
                time.sleep(0.1)
        
        print("Stream command sent, proceeding with data reading...")
        return True
        
    except Exception as e:
        print(f"Error sending stream command: {e}")
        return False

def stop_streaming():
    """Send Ctrl+C to stop streaming"""
    try:
        ser.write(b'\x03')  # Send Ctrl+C (ASCII 3)
        print("\nStopping stream...")
        time.sleep(0.5)
        
        # Read any remaining data
        while ser.in_waiting > 0:
            line = ser.readline().decode(errors='ignore').strip()
            if line:
                print(f"Device: {line}")
                
    except Exception as e:
        print(f"Error stopping stream: {e}")

def main():
    try:
        print("Connecting to HE16 Configuration Interface...")
        
        # Wait for device to be ready
        time.sleep(1)
        
        # Send stream command
        if not send_stream_command():
            print("Failed to start streaming. Exiting...")
            return
        
        print("Starting ADC visualization...")
        print("Press Ctrl+C to exit")
        
        # Main data reading loop
        consecutive_errors = 0
        max_consecutive_errors = 10
        
        while True:
            try:
                line = ser.readline().decode(errors='ignore').strip()
                
                if not line:
                    consecutive_errors += 1
                    if consecutive_errors >= max_consecutive_errors:
                        print("No data received for too long, attempting to restart stream...")
                        send_stream_command()
                        consecutive_errors = 0
                    continue
                
                # Reset error counter on successful read
                consecutive_errors = 0
                
                # Skip non-CSV lines (device messages)
                # if not line.replace(',', '').replace(' ', '').isdigit():
                #     # This might be a device message, print it
                #     if len(line) > 0 and not line.startswith('Ready>'):
                #         print(f"Device: {line}")
                #     continue
                
                # Parse CSV data
                values = list(map(int, line.split(',')))
                
                # Ensure we have the expected number of values
                if len(values) == TOTAL_CHANNELS:
                    # Update min/max for each channel
                    for i, val in enumerate(values):
                        if val < min_vals[i]:
                            min_vals[i] = val
                        if val > max_vals[i]:
                            max_vals[i] = val
                    
                    draw_bars(values)
                elif len(values) > 0:
                    print(f"Received {len(values)} values, expected {TOTAL_CHANNELS}")
                    
            except ValueError as e:
                # Skip lines that can't be parsed as CSV
                continue
            except Exception as e:
                print(f"Error processing data: {e}")
                consecutive_errors += 1
                if consecutive_errors >= max_consecutive_errors:
                    print("Too many errors, exiting...")
                    break
                    
    except KeyboardInterrupt:
        print("\nUser interrupted. Stopping...")
    except Exception as e:
        print(f"Unexpected error: {e}")
        
main()