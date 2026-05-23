import argparse
import sys
import time
from pathlib import Path

import serial
from serial.tools import list_ports


FRAME_W = 160
FRAME_H = 120
MAGIC = b"\x55\xaa"
SERIAL_CHUNK_SIZE = 1024


def crop_to_aspect(frame, target_w, target_h):
    height, width = frame.shape[:2]
    target_aspect = target_w / target_h
    source_aspect = width / height

    if source_aspect > target_aspect:
        crop_w = int(height * target_aspect)
        x0 = (width - crop_w) // 2
        return frame[:, x0:x0 + crop_w]

    crop_h = int(width / target_aspect)
    y0 = (height - crop_h) // 2
    return frame[y0:y0 + crop_h, :]


def prepare_frame(frame, mirror):
    frame = crop_to_aspect(frame, FRAME_W, FRAME_H)
    frame = cv2.resize(frame, (FRAME_W, FRAME_H), interpolation=cv2.INTER_AREA)
    if mirror:
        frame = cv2.flip(frame, 1)
    return frame


def frame_to_rgb565_bytes(frame):
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    r = (rgb[:, :, 0].astype("uint16") >> 3) << 11
    g = (rgb[:, :, 1].astype("uint16") >> 2) << 5
    b = rgb[:, :, 2].astype("uint16") >> 3
    rgb565 = r | g | b
    return rgb565.byteswap().tobytes()


def send_frame(port, frame):
    header = MAGIC + b"F" + FRAME_W.to_bytes(2, "big") + FRAME_H.to_bytes(2, "big")
    packet = header + frame_to_rgb565_bytes(frame)
    for offset in range(0, len(packet), SERIAL_CHUNK_SIZE):
        port.write(packet[offset:offset + SERIAL_CHUNK_SIZE])


def main():
    parser = argparse.ArgumentParser(description="Send PC/mobile camera video to Puzhong STM32F103 LCD.")
    parser.add_argument("--port", help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=921600, help="Serial baud rate")
    parser.add_argument("--camera", default="0", help="Camera index or an IP camera URL")
    parser.add_argument("--fps", type=float, default=2.0, help="Target sending fps")
    parser.add_argument("--no-mirror", action="store_true", help="Do not mirror the preview/video")
    default_save_dir = Path(__file__).resolve().parent / "photos"
    parser.add_argument("--save-dir", default=str(default_save_dir), help="Directory for captured photos")
    args = parser.parse_args()

    if not args.port:
        ports = list(list_ports.comports())
        if not ports:
            print("No serial ports found. Replug the USB serial cable and check Device Manager.")
            return 1
        print("Available serial ports:")
        for port in ports:
            print(f"  {port.device}: {port.description}")
        print("Run again with --port COMx, for example: python host_tools\\camera_to_puzhong103.py --port COM3")
        return 0

    global cv2
    import cv2

    camera_source = int(args.camera) if args.camera.isdigit() else args.camera
    cap = cv2.VideoCapture(camera_source, cv2.CAP_DSHOW) if isinstance(camera_source, int) else cv2.VideoCapture(camera_source)
    if not cap.isOpened():
        print("Cannot open camera. Check camera index or URL.", file=sys.stderr)
        return 1

    save_dir = Path(args.save_dir)
    save_dir.mkdir(parents=True, exist_ok=True)

    try:
        with serial.Serial(args.port, args.baud, timeout=0.05, write_timeout=2) as ser:
            print(f"Serial opened: {args.port} @ {args.baud}")
            print("Press p to take a photo, q to quit.")
            next_send = 0.0
            last_frame = None
            while True:
                ok, frame = cap.read()
                if not ok:
                    print("Camera frame read failed.", file=sys.stderr)
                    break

                display_frame = prepare_frame(frame, mirror=not args.no_mirror)
                last_frame = display_frame
                preview = cv2.resize(display_frame, (FRAME_W * 3, FRAME_H * 3), interpolation=cv2.INTER_NEAREST)
                cv2.imshow("Camera to Puzhong103", preview)
                key = cv2.waitKey(1) & 0xFF

                if key == ord("q"):
                    break
                if key == ord("p") and last_frame is not None:
                    filename = save_dir / time.strftime("photo_%Y%m%d_%H%M%S.jpg")
                    cv2.imwrite(str(filename), last_frame)
                    ser.write(MAGIC + b"S")
                    print(f"Saved {filename}")

                now = time.monotonic()
                if now >= next_send:
                    try:
                        send_frame(ser, display_frame)
                    except serial.SerialTimeoutException:
                        print("Serial write timeout. Check that STM32 is flashed with this project and baud rates match.", file=sys.stderr)
                        break
                    next_send = now + 1.0 / max(args.fps, 0.2)

                reply = ser.read(64)
                if reply:
                    print(reply.decode(errors="ignore").strip())
    except KeyboardInterrupt:
        print("\nStopped by Ctrl+C.")

    cap.release()
    cv2.destroyAllWindows()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
