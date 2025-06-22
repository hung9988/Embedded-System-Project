import serial
import time

# Change this to match your STM32 virtual COM port
PORT = "COM13"  # On Linux/macOS, use something like "/dev/ttyACM0"
BAUDRATE = 9600

def show_config():
    try:
        with serial.Serial(PORT, BAUDRATE, timeout=1) as ser:
            # Give the board time to initialize if needed
            time.sleep(0.5)

            # Send 'show' command followed by newline
            print("Sending command to show configuration...")
            ser.write(b"show\n")
            ser.flush()

            # Read all available lines
            print("Reading config:")
            while True:
                line = ser.readline()
                if not line:
                    break
                print(line.decode("utf-8").rstrip())

    except serial.SerialException as e:
        print(f"Error: {e}")
        print("Make sure the device is connected and the correct port is used.")

if __name__ == "__main__":
    show_config()
