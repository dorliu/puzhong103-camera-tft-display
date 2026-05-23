import argparse
import time

import serial


def main():
    parser = argparse.ArgumentParser(description="Read text from the STM32 serial port.")
    parser.add_argument("--port", required=True, help="Serial port, for example COM5")
    parser.add_argument("--baud", type=int, default=921600)
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
        ser.setRTS(False)
        ser.setDTR(False)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print(f"Reading {args.port} at {args.baud}. Press Ctrl+C to stop.")
        while True:
            data = ser.read(256)
            if data:
                print(data.decode(errors="replace"), end="")
            time.sleep(0.02)


if __name__ == "__main__":
    main()
