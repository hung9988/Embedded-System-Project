import serial
import serial.tools.list_ports
import time

def list_com_ports():
    """Lists available COM ports."""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No COM ports found.")
        return []
    
    print("Available COM ports:")
    for i, port in enumerate(ports):
        print(f"  {i}: {port.device} - {port.description}")
    return ports

def main():
    """Main function to read from a COM port."""
    available_ports = list_com_ports()
    if not available_ports:
        return

    # Get user selection
    while True:
        try:
            port_index_str = input(f"Enter the number of the COM port to use (0-{len(available_ports)-1}): ")
            port_index = int(port_index_str)
            if 0 <= port_index < len(available_ports):
                selected_port_info = available_ports[port_index]
                port_name = selected_port_info.device
                break
            else:
                print("Invalid selection. Please try again.")
        except ValueError:
            print("Invalid input. Please enter a number.")

    # Get baud rate
    while True:
        try:
            baud_rate_str = input("Enter the baud rate (e.g., 9600, 115200): ")
            baud_rate = int(baud_rate_str)
            break
        except ValueError:
            print("Invalid input. Please enter a valid integer for the baud rate.")

    ser = None
    try:
        print(f"Opening {port_name} at {baud_rate} bps...")
        ser = serial.Serial(port_name, baud_rate, timeout=1)
        print("Listening for data... Press Ctrl+C to stop.")
        
        while True:
            # Read data from the serial port
            data = ser.readline()
            if data:
                try:
                    # Try to decode as UTF-8, but fall back to a raw representation
                    print(data.decode('utf-8').strip())
                except UnicodeDecodeError:
                    print(f"Received (raw): {data!r}")

            time.sleep(0.01) # Small delay to prevent high CPU usage

    except serial.SerialException as e:
        print(f"Error: {e}")
    except KeyboardInterrupt:
        print("\nStopping listener.")
    finally:
        if ser and ser.is_open:
            ser.close()
            print(f"COM port {port_name} closed.")

if __name__ == "__main__":
    main() 