import serial
import time
import keyboard  # pip install keyboard

# === Serial Settings ===
SERIAL_PORT = 'COM4'
BAUD_RATE = 9600

# === Authorized RFID IDs ===
authorized_cards = ["0008089233"]


def read_rfid():
    """Reads RFID card from keyboard-type reader."""
    print("Please tap your RFID card...")
    rfid_data = ""
    while True:
        event = keyboard.read_event()
        if event.event_type == keyboard.KEY_DOWN:
            key = event.name
            if key == 'enter':
                print(f"RFID scanned: {rfid_data}")
                return rfid_data.strip()
            elif len(key) == 1:
                rfid_data += key


def main():
    try:
        arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        print("Connected to Arduino.")

        while True:
            card = read_rfid()
            if card in authorized_cards:
                print("✅ Card verified. Proceed with circular motion.")
                arduino.write(b'A')  # authorized
            else:
                print("❌ Card denied.")
                arduino.write(b'D')  # denied

            time.sleep(0.5)

    except KeyboardInterrupt:
        print("Program terminated.")
    finally:
        if 'arduino' in locals() and arduino.is_open:
            arduino.close()


if _name_ == "_main_":
    main()