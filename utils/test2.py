import serial
import time
import os

# Change COM port and baud rate accordingly
ser = serial.Serial('/dev/tty.usbmodem337B336E30341', 115200, timeout=1)

# Terminal bar chart settings
MAX_ADC = 4095
BAR_WIDTH = 50  # Max characters per bar

# Indices to monitor
CHANNELS =list(range(0,16))

# Initialize min/max tracking
min_vals = [MAX_ADC] * 16
max_vals = [0] * 16

def clear_terminal():
    os.system('cls' if os.name == 'nt' else 'clear')

def draw_bars(values):
    clear_terminal()
    print("Live ADC Feed (1 kHz)")
    print("-" * 60)
    for i, val in enumerate(values):
        if i not in CHANNELS:
            continue
        bar_len = int((val / MAX_ADC) * BAR_WIDTH)
        bar = '#' * bar_len
        print(f"CH{i:02}: [{bar:<{BAR_WIDTH}}] {val:4}")
        print(f"   Min: {min_vals[i]:4}  Max: {max_vals[i]:4}  Diff: {max_vals[i] - min_vals[i]:4}")
    print("-" * 60)

try:
    while True:
        line = ser.readline().decode(errors='ignore').strip()
        if not line:
            continue
        try:
            values = list(map(int, line.split(',')))
            if len(values) == 16:
                # Update min/max for each channel
                for i, val in enumerate(values):
                    if val < min_vals[i]:
                        min_vals[i] = val
                    if val > max_vals[i]:
                        max_vals[i] = val
                draw_bars(values)
        except ValueError:
            continue
        # time.sleep(0.001)  # ~1 kHz refresh
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
