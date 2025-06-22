#!/usr/bin/env python3
"""
HE16 Hall Effect Keyboard Configuration Client
Interactive Python client for configuring the HE16 keyboard via CDC interface
"""

import serial
import struct
import time
import sys
from typing import Optional, Tuple, Dict, Any
from dataclasses import dataclass
from enum import IntEnum

# Protocol definitions
PROTOCOL_MAGIC = 0x4845  # "HE" for Hall Effect
PROTOCOL_VERSION = 0x01

class CommandCode(IntEnum):
    CMD_PING = 0x01
    CMD_GET_CONFIG = 0x02
    CMD_SET_CONFIG = 0x03
    CMD_GET_KEYMAP = 0x04
    CMD_SET_KEYMAP = 0x05
    CMD_SAVE_CONFIG = 0x06
    CMD_LOAD_CONFIG = 0x07
    CMD_RESET_CONFIG = 0x08
    CMD_GET_INFO = 0x09

class ResponseCode(IntEnum):
    RESP_OK = 0x00
    RESP_ERROR = 0x01
    RESP_INVALID_CMD = 0x02
    RESP_INVALID_PARAM = 0x03
    RESP_BUFFER_OVERFLOW = 0x04

@dataclass
class DeviceConfig:
    reverse_magnet_pole: int
    trigger_offset: int
    reset_threshold: int
    rapid_trigger_offset: int
    screaming_velocity_trigger: int
    tap_timeout: int

@dataclass
class DeviceInfo:
    device_name: str
    firmware_version: Tuple[int, int, int]
    matrix_rows: int
    matrix_cols: int
    layers_count: int

class HE16Client:
    def __init__(self, port: str, baudrate: int = 115200):
        """Initialize the HE16 keyboard client"""
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        self.device_info = None
        
    def connect(self) -> bool:
        """Connect to the keyboard device"""
        try:
            self.serial = serial.Serial(self.port, self.baudrate, timeout=2.0)
            time.sleep(0.1)  # Give device time to initialize
            
            # Test connection with ping
            if self.ping():
                print(f"✓ Connected to HE16 keyboard on {self.port}")
                
                # Get device info
                self.device_info = self.get_device_info()
                if self.device_info:
                    print(f"✓ Device: {self.device_info.device_name}")
                    print(f"✓ Firmware: {'.'.join(map(str, self.device_info.firmware_version))}")
                    print(f"✓ Matrix: {self.device_info.matrix_rows}x{self.device_info.matrix_cols}")
                    print(f"✓ Layers: {self.device_info.layers_count}")
                
                return True
            else:
                print("✗ Failed to ping device")
                return False
                
        except Exception as e:
            print(f"✗ Connection failed: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the device"""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("✓ Disconnected")
    
    def _calculate_checksum(self, data: bytes) -> int:
        """Calculate XOR checksum"""
        checksum = 0
        for byte in data:
            checksum ^= byte
        return checksum
    
    def _send_packet(self, cmd: CommandCode, payload: bytes = b'') -> bool:
        """Send a packet to the device"""
        if not self.serial or not self.serial.is_open:
            return False
        
        # Calculate checksum (header + payload, excluding checksum field)
        # Build header without checksum first
        header_no_checksum = struct.pack('<HBBH', 
                                       PROTOCOL_MAGIC, 
                                       PROTOCOL_VERSION, 
                                       cmd.value, 
                                       len(payload))
        
        checksum_data = header_no_checksum + payload
        checksum = self._calculate_checksum(checksum_data)
        
        # Build complete header with checksum
        header = struct.pack('<HBBHB', 
                           PROTOCOL_MAGIC, 
                           PROTOCOL_VERSION, 
                           cmd.value, 
                           len(payload),
                           checksum)
        
        # Send packet
        self.serial.write(header + payload)
        self.serial.flush()
        return True
    
    def _receive_response(self) -> Optional[Tuple[ResponseCode, bytes]]:
        """Receive a response from the device"""
        if not self.serial or not self.serial.is_open:
            return None
        
        try:
            # Read response header (sizeof(response_header_t) = 7 bytes)
            header_data = self.serial.read(7)
            if len(header_data) != 7:
                return None
            
            magic, version, resp_code, length, checksum = struct.unpack('<HBBHB', header_data)
            
            if magic != PROTOCOL_MAGIC or version != PROTOCOL_VERSION:
                return None
            
            # Read payload if present
            payload = b''
            if length > 0:
                payload = self.serial.read(length)
                if len(payload) != length:
                    return None
            
            # Verify checksum (header without checksum + payload)
            checksum_data = header_data[:-1] + payload
            if self._calculate_checksum(checksum_data) != checksum:
                return None
            
            return ResponseCode(resp_code), payload
            
        except Exception as e:
            print(f"Error receiving response: {e}")
            return None
    
    def ping(self) -> bool:
        """Ping the device"""
        if not self._send_packet(CommandCode.CMD_PING):
            return False
        
        response = self._receive_response()
        return response is not None and response[0] == ResponseCode.RESP_OK
    
    def get_config(self) -> Optional[DeviceConfig]:
        """Get current device configuration"""
        if not self._send_packet(CommandCode.CMD_GET_CONFIG):
            return None
        
        response = self._receive_response()
        if not response or response[0] != ResponseCode.RESP_OK:
            return None
        
        if len(response[1]) != 8:  # sizeof(config_packet_t) = 8 bytes
            return None
        
        data = struct.unpack('<BBBBBBH', response[1])
        return DeviceConfig(
            reverse_magnet_pole=data[0],
            trigger_offset=data[1],
            reset_threshold=data[2],
            rapid_trigger_offset=data[3],
            screaming_velocity_trigger=data[4],
            tap_timeout=data[5]
        )
    
    def set_config(self, config: DeviceConfig) -> bool:
        """Set device configuration"""
        payload = struct.pack('<BBBBBBH',
                            config.reverse_magnet_pole,
                            config.trigger_offset,
                            config.reset_threshold,
                            config.rapid_trigger_offset,
                            config.screaming_velocity_trigger,
                            config.tap_timeout)
        
        if not self._send_packet(CommandCode.CMD_SET_CONFIG, payload):
            return False
        
        response = self._receive_response()
        return response is not None and response[0] == ResponseCode.RESP_OK
    
    def get_keymap(self, layer: int) -> Optional[list]:
        """Get keymap for a specific layer"""
        if not self.device_info:
            return None
        
        payload = struct.pack('<B', layer)
        
        if not self._send_packet(CommandCode.CMD_GET_KEYMAP, payload):
            return None
        
        response = self._receive_response()
        if not response or response[0] != ResponseCode.RESP_OK:
            return None
        
        # Unpack keymap data
        expected_size = self.device_info.matrix_rows * self.device_info.matrix_cols * 2
        if len(response[1]) != expected_size:
            return None
        
        keymap = []
        for i in range(0, len(response[1]), 2):
            keymap.append(struct.unpack('<H', response[1][i:i+2])[0])
        
        return keymap
    
    def set_key(self, layer: int, row: int, col: int, value: int) -> bool:
        """Set a single key in the keymap"""
        payload = struct.pack('<BBBH', layer, row, col, value)
        
        if not self._send_packet(CommandCode.CMD_SET_KEYMAP, payload):
            return False
        
        response = self._receive_response()
        return response is not None and response[0] == ResponseCode.RESP_OK
    
    def save_config(self) -> bool:
        """Save configuration to persistent storage"""
        if not self._send_packet(CommandCode.CMD_SAVE_CONFIG):
            return False
        
        response = self._receive_response()
        return response is not None and response[0] == ResponseCode.RESP_OK
    
    def load_config(self) -> bool:
        """Load configuration from persistent storage"""
        if not self._send_packet(CommandCode.CMD_LOAD_CONFIG):
            return False
        
        response = self._receive_response()
        return response is not None and response[0] == ResponseCode.RESP_OK
    
    def reset_config(self) -> bool:
        """Reset configuration to defaults"""
        if not self._send_packet(CommandCode.CMD_RESET_CONFIG):
            return False
        
        response = self._receive_response()
        return response is not None and response[0] == ResponseCode.RESP_OK
    
    def get_device_info(self) -> Optional[DeviceInfo]:
        """Get device information"""
        if not self._send_packet(CommandCode.CMD_GET_INFO):
            return None
        
        response = self._receive_response()
        if not response or response[0] != ResponseCode.RESP_OK:
            return None
        
        if len(response[1]) < 38:  # minimum size for device_info_t
            return None
        
        # Unpack device info
        data = response[1]
        device_name = data[:32].decode('utf-8').rstrip('\0')
        fw_version = struct.unpack('<BBB', data[32:35])
        matrix_rows = data[35]
        matrix_cols = data[36]
        layers_count = data[37]
        
        return DeviceInfo(
            device_name=device_name,
            firmware_version=fw_version,
            matrix_rows=matrix_rows,
            matrix_cols=matrix_cols,
            layers_count=layers_count
        )

def print_menu():
    """Print the interactive menu"""
    print("\n" + "="*50)
    print("HE16 Keyboard Configuration Client")
    print("="*50)
    print("1. Ping device")
    print("2. Get configuration")
    print("3. Set configuration")
    print("4. Get keymap")
    print("5. Set key")
    print("6. Save configuration")
    print("7. Load configuration")
    print("8. Reset configuration")
    print("9. Get device info")
    print("0. Exit")
    print("-"*50)

def main():
    """Main interactive loop"""
    if len(sys.argv) != 2:
        print("Usage: python he16_client.py <serial_port>")
        print("Example: python he16_client.py /dev/ttyACM0")
        print("Example: python he16_client.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    client = HE16Client(port)
    
    if not client.connect():
        sys.exit(1)
    
    try:
        while True:
            print_menu()
            choice = input("Enter your choice: ").strip()
            
            if choice == '0':
                break
            elif choice == '1':
                # Ping
                if client.ping():
                    print("✓ Ping successful")
                else:
                    print("✗ Ping failed")
            
            elif choice == '2':
                # Get configuration
                config = client.get_config()
                if config:
                    print("Current Configuration:")
                    print(f"  Reverse magnet pole: {config.reverse_magnet_pole}")
                    print(f"  Trigger offset: {config.trigger_offset}")
                    print(f"  Reset threshold: {config.reset_threshold}")
                    print(f"  Rapid trigger offset: {config.rapid_trigger_offset}")
                    print(f"  Screaming velocity trigger: {config.screaming_velocity_trigger}")
                    print(f"  Tap timeout: {config.tap_timeout}")
                else:
                    print("✗ Failed to get configuration")
            
            elif choice == '3':
                # Set configuration
                try:
                    print("Enter new configuration values:")
                    reverse_magnet_pole = int(input("Reverse magnet pole (0/1): "))
                    trigger_offset = int(input("Trigger offset: "))
                    reset_threshold = int(input("Reset threshold: "))
                    rapid_trigger_offset = int(input("Rapid trigger offset: "))
                    screaming_velocity_trigger = int(input("Screaming velocity trigger: "))
                    tap_timeout = int(input("Tap timeout: "))
                    
                    config = DeviceConfig(
                        reverse_magnet_pole=reverse_magnet_pole,
                        trigger_offset=trigger_offset,
                        reset_threshold=reset_threshold,
                        rapid_trigger_offset=rapid_trigger_offset,
                        screaming_velocity_trigger=screaming_velocity_trigger,
                        tap_timeout=tap_timeout
                    )
                    
                    if client.set_config(config):
                        print("✓ Configuration updated")
                    else:
                        print("✗ Failed to set configuration")
                        
                except ValueError:
                    print("✗ Invalid input")
            
            elif choice == '4':
                # Get keymap
                try:
                    layer = int(input("Enter layer number: "))
                    keymap = client.get_keymap(layer)
                    if keymap and client.device_info:
                        print(f"Keymap for layer {layer}:")
                        for row in range(client.device_info.matrix_rows):
                            for col in range(client.device_info.matrix_cols):
                                idx = row * client.device_info.matrix_cols + col
                                print(f"  [{row},{col}]: 0x{keymap[idx]:04X}")
                    else:
                        print("✗ Failed to get keymap")
                except ValueError:
                    print("✗ Invalid layer number")
            
            elif choice == '5':
                # Set key
                try:
                    layer = int(input("Enter layer: "))
                    row = int(input("Enter row: "))
                    col = int(input("Enter column: "))
                    value = int(input("Enter key value (hex, e.g., 0x1C for 'a'): "), 16)
                    
                    if client.set_key(layer, row, col, value):
                        print("✓ Key updated")
                    else:
                        print("✗ Failed to set key")
                        
                except ValueError:
                    print("✗ Invalid input")
            
            elif choice == '6':
                # Save configuration
                if client.save_config():
                    print("✓ Configuration saved")
                else:
                    print("✗ Failed to save configuration")
            
            elif choice == '7':
                # Load configuration
                if client.load_config():
                    print("✓ Configuration loaded")
                else:
                    print("✗ Failed to load configuration")
            
            elif choice == '8':
                # Reset configuration
                confirm = input("Are you sure you want to reset to defaults? (y/N): ")
                if confirm.lower() == 'y':
                    if client.reset_config():
                        print("✓ Configuration reset to defaults")
                    else:
                        print("✗ Failed to reset configuration")
            
            elif choice == '9':
                # Get device info
                info = client.get_device_info()
                if info:
                    print("Device Information:")
                    print(f"  Name: {info.device_name}")
                    print(f"  Firmware: {'.'.join(map(str, info.firmware_version))}")
                    print(f"  Matrix: {info.matrix_rows}x{info.matrix_cols}")
                    print(f"  Layers: {info.layers_count}")
                else:
                    print("✗ Failed to get device info")
            
            else:
                print("Invalid choice")
            
            input("\nPress Enter to continue...")
    
    except KeyboardInterrupt:
        print("\nExiting...")
    
    finally:
        client.disconnect()

if __name__ == "__main__":
    main()11111212123132457686cdeac