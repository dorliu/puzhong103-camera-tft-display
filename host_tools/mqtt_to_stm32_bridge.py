import sys
import time
import datetime

import paho.mqtt.client as mqtt
import serial


BROKER_HOST = "broker.emqx.io"
BROKER_PORT = 1883
MQTT_USERNAME = ""
MQTT_PASSWORD = ""
MQTT_CLIENT_ID = "group08"
MQTT_CMD_TOPIC = "topic911/cmd"
MQTT_STATUS_TOPIC = "topic911/status"

STM32_COM_PORT = "COM4"
STM32_BAUDRATE = 115200

SEND_TIME_SYNC = True
TIME_SYNC_INTERVAL_SEC = 30
TIME_SYNC_RETRY_SEC = 1
POLL_STATUS = True
STATUS_POLL_INTERVAL_SEC = 1.0

ALLOWED_COMMANDS = {
    "LED1=ON",
    "LED1=OFF",
    "LED2=ON",
    "LED2=OFF",
    "ALL=ON",
    "ALL=OFF",
    "STATUS?",
}


class BridgeApp:
    def __init__(self):
        self.serial_port = None
        self.mqtt_client = None
        self._last_time_sync_at = 0.0
        self._time_synced = False
        self._last_status_poll_at = 0.0

    def open_serial(self):
        self.serial_port = serial.Serial(
            port=STM32_COM_PORT,
            baudrate=STM32_BAUDRATE,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.2,
            rtscts=False,
            dsrdtr=False,
        )
        self.serial_port.setRTS(False)
        self.serial_port.setDTR(False)
        self.serial_port.reset_input_buffer()
        self.serial_port.reset_output_buffer()
        time.sleep(1.5)
        print("STM32 serial opened:", STM32_COM_PORT, STM32_BAUDRATE)

    def send_time_sync(self):
        if self.serial_port is None:
            return

        now = datetime.datetime.utcnow() + datetime.timedelta(hours=8)
        payload = now.strftime("TIME=%Y-%m-%d %H:%M:%S")
        line = payload + "\r\n"
        self.serial_port.write(line.encode("ascii", errors="ignore"))
        self.serial_port.flush()
        print("Time sync sent:", payload)
        self.publish_status(payload)

    def poll_status(self):
        if self.serial_port is None:
            return
        line = "STATUS?\r\n"
        self.serial_port.write(line.encode("ascii", errors="ignore"))
        self.serial_port.flush()

    def on_connect(self, client, userdata, flags, reason_code, properties=None):
        if reason_code == 0:
            print("MQTT connected:", BROKER_HOST, BROKER_PORT)
            client.subscribe(MQTT_CMD_TOPIC, qos=1)
            print("Subscribed topic:", MQTT_CMD_TOPIC)
        else:
            print("MQTT connect failed, code:", reason_code)

    def on_message(self, client, userdata, msg):
        payload = msg.payload.decode("utf-8", errors="ignore").strip()
        print("MQTT recv:", payload)

        if payload not in ALLOWED_COMMANDS:
            print("Ignored unsupported command:", payload)
            return

        if self.serial_port is None:
            print("Serial not ready")
            return

        line = payload + "\r\n"
        self.serial_port.write(line.encode("ascii"))
        self.serial_port.flush()
        print("Forwarded to STM32:", payload)

    def setup_mqtt(self):
        self.mqtt_client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id=MQTT_CLIENT_ID,
        )
        if MQTT_USERNAME:
            self.mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message
        print("Connecting MQTT broker:", BROKER_HOST, BROKER_PORT)
        self.mqtt_client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
        self.mqtt_client.loop_start()

    def publish_status(self, line):
        if self.mqtt_client is not None:
            self.mqtt_client.publish(MQTT_STATUS_TOPIC, line, qos=0, retain=False)

    def run(self):
        self.open_serial()
        self.setup_mqtt()

        print("Bridge running. Publish one of these commands to", MQTT_CMD_TOPIC)
        for cmd in sorted(ALLOWED_COMMANDS):
            print("  -", cmd)

        try:
            while True:
                if self.serial_port is not None:
                    raw = self.serial_port.readline()
                    if raw:
                        text = raw.decode("utf-8", errors="ignore").strip()
                        if text:
                            print("STM32:", text)
                            self.publish_status(text)
                            if text == "ACK:TIME":
                                self._time_synced = True

                if SEND_TIME_SYNC and (self.serial_port is not None):
                    now_ts = time.time()
                    if not self._time_synced:
                        if (now_ts - self._last_time_sync_at) >= TIME_SYNC_RETRY_SEC:
                            self.send_time_sync()
                            self._last_time_sync_at = now_ts
                    else:
                        if (now_ts - self._last_time_sync_at) >= TIME_SYNC_INTERVAL_SEC:
                            self.send_time_sync()
                            self._last_time_sync_at = now_ts

                if POLL_STATUS and (self.serial_port is not None):
                    now_ts = time.time()
                    if (now_ts - self._last_status_poll_at) >= STATUS_POLL_INTERVAL_SEC:
                        self.poll_status()
                        self._last_status_poll_at = now_ts
                time.sleep(0.05)
        except KeyboardInterrupt:
            print("Bridge stopped by user")
        finally:
            if self.mqtt_client is not None:
                self.mqtt_client.loop_stop()
                self.mqtt_client.disconnect()
            if self.serial_port is not None and self.serial_port.is_open:
                self.serial_port.close()


if __name__ == "__main__":
    try:
        app = BridgeApp()
        app.run()
    except serial.SerialException as exc:
        print("Serial error:", exc)
        sys.exit(1)
    except TimeoutError:
        print("MQTT connect timeout: check BROKER_HOST/BROKER_PORT or network access")
        sys.exit(1)
    except Exception as exc:
        print("Fatal error:", exc)
        sys.exit(1)
