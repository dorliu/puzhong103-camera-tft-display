import argparse
import time
from pathlib import Path

import cv2
import numpy as np
import serial


HEAD = bytes((0xA5, 0x5A))
PACKET_TYPE_LINE = 0x01
FRAME_WIDTH = 160
FRAME_HEIGHT = 120


def build_line_packet(line_index: int, row_words: np.ndarray) -> bytes:
    payload = row_words.astype(">u2", copy=False).tobytes()
    length = len(payload)
    meta = bytes((PACKET_TYPE_LINE, line_index, (length >> 8) & 0xFF, length & 0xFF))
    checksum = (sum(meta) + sum(payload)) & 0xFF
    return HEAD + meta + payload + bytes((checksum,))


def crop_to_aspect(frame: np.ndarray, target_width: int, target_height: int) -> np.ndarray:
    height, width = frame.shape[:2]
    target_aspect = target_width / target_height
    source_aspect = width / height

    if source_aspect > target_aspect:
        crop_width = int(height * target_aspect)
        x0 = (width - crop_width) // 2
        return frame[:, x0 : x0 + crop_width]

    crop_height = int(width / target_aspect)
    y0 = (height - crop_height) // 2
    return frame[y0 : y0 + crop_height, :]


def frame_to_rgb565(frame: np.ndarray) -> np.ndarray:
    frame = crop_to_aspect(frame, FRAME_WIDTH, FRAME_HEIGHT)
    frame = cv2.resize(frame, (FRAME_WIDTH, FRAME_HEIGHT), interpolation=cv2.INTER_AREA)
    frame = cv2.flip(frame, 1)

    blue = frame[:, :, 0].astype(np.uint16)
    green = frame[:, :, 1].astype(np.uint16)
    red = frame[:, :, 2].astype(np.uint16)

    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def send_frame(ser: serial.Serial, rgb565: np.ndarray, line_delay: float) -> None:
    for line_index in range(FRAME_HEIGHT):
        ser.write(build_line_packet(line_index, rgb565[line_index]))
        ser.flush()
        if line_delay > 0:
            time.sleep(line_delay)


def main() -> None:
    parser = argparse.ArgumentParser(description="PC camera to STM32 TFT streamer")
    parser.add_argument("--port", default="COM4", help="serial port, default COM4")
    parser.add_argument("--baud", type=int, default=230400, help="serial baud rate")
    parser.add_argument("--camera", default="0", help="camera index or an IP camera URL")
    parser.add_argument("--line-delay", type=float, default=0.003, help="delay after each LCD line packet")
    parser.add_argument("--save-dir", default=str(Path(__file__).resolve().parent / "photos"), help="photo save directory")
    args = parser.parse_args()

    print(f"Opening {args.port} @ {args.baud} ...")
    ser = serial.Serial(
        port=args.port,
        baudrate=args.baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.05,
        rtscts=False,
        dsrdtr=False,
    )
    ser.setRTS(False)
    ser.setDTR(False)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    time.sleep(1.5)
    camera_source = int(args.camera) if args.camera.isdigit() else args.camera
    if isinstance(camera_source, int):
        cap = cv2.VideoCapture(camera_source, cv2.CAP_DSHOW)
    else:
        cap = cv2.VideoCapture(camera_source)

    if not cap.isOpened():
        ser.close()
        raise SystemExit("Camera open failed")

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)
    save_dir = Path(args.save_dir)
    save_dir.mkdir(parents=True, exist_ok=True)

    ser.write(b"CAM=ON\r\n")
    time.sleep(0.2)
    if ser.in_waiting:
        print(ser.read(ser.in_waiting).decode(errors="ignore").strip())

    frame_counter = 0
    time_base = time.time()
    print("Streaming... press p to save a photo, q in preview window to stop.")

    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("Frame capture failed")
                time.sleep(0.1)
                continue

            rgb565 = frame_to_rgb565(frame)
            send_frame(ser, rgb565, args.line_delay)

            preview = cv2.resize(frame, (FRAME_WIDTH * 4, FRAME_HEIGHT * 4), interpolation=cv2.INTER_NEAREST)
            cv2.imshow("camera_to_tft", preview)
            frame_counter += 1

            now = time.time()
            if now - time_base >= 1.0:
                fps = frame_counter / (now - time_base)
                print(f"Approx FPS: {fps:.2f}")
                frame_counter = 0
                time_base = now

            key = cv2.waitKey(1) & 0xFF
            if key == ord("p"):
                filename = save_dir / time.strftime("photo_%Y%m%d_%H%M%S.jpg")
                cv2.imwrite(str(filename), frame)
                print(f"Saved photo: {filename}")
            elif key == ord("q"):
                break
    finally:
        try:
            ser.write(b"CAM=OFF\r\n")
            time.sleep(0.1)
        except serial.SerialException:
            pass
        cap.release()
        ser.close()
        cv2.destroyAllWindows()
        print("Camera stream stopped.")


if __name__ == "__main__":
    main()
