from flask import Flask, render_template, jsonify
import serial
import serial.tools.list_ports
import threading
import re
import time
from collections import deque

app = Flask(__name__)

# ============================================================
# CONFIGURATION
# ============================================================

BAUD_RATE = 115200

# Keep None for automatic COM-port detection.
# If automatic detection does not work, use:
# SERIAL_PORT = "COM5"
SERIAL_PORT = None

MAX_GRAPH_POINTS = 120


# ============================================================
# LIVE SYSTEM STATE
# ============================================================

state = {
    "connected": False,
    "port": None,
    "last_update": None,

    # Component status
    "wifi": "Unknown",
    "mpu": "Unknown",
    "gps": "Unknown",
    "telegram": "Unknown",

    # Accident
    "accident": "Normal",
    "impact": 0.0,
    "accident_count": 0,
    "warning_seconds": None,

    # MPU6050
    "ax": 0.0,
    "ay": 0.0,
    "az": 0.0,

    # GPS
    "latitude": None,
    "longitude": None,
    "altitude": None,
    "speed": None,
    "satellites": None,

    # Events
    "last_event": "Waiting for ESP32...",
    "logs": deque(maxlen=100),

    # Graph
    "graph": deque(maxlen=MAX_GRAPH_POINTS),
}

lock = threading.Lock()


# ============================================================
# FIND ESP32 SERIAL PORT
# ============================================================

def find_port():

    # If manually specified, use it.
    if SERIAL_PORT:
        return SERIAL_PORT

    ports = list(serial.tools.list_ports.comports())

    keywords = [
        "CP210",
        "CH340",
        "CH341",
        "USB-SERIAL",
        "USB UART",
        "ESP32",
        "SILICON LABS",
        "FTDI"
    ]

    # Try to identify ESP32 USB-UART adapter.
    for port in ports:

        description = (
            f"{port.description} "
            f"{port.manufacturer or ''}"
        ).upper()

        if any(keyword in description for keyword in keywords):
            return port.device

    # If only one serial port exists,
    # use that port.
    if len(ports) == 1:
        return ports[0].device

    return None


# ============================================================
# ADD EVENT TO EVENT LOG
# ============================================================

def add_log(message):

    with lock:

        state["logs"].appendleft({
            "time": time.strftime("%H:%M:%S"),
            "message": message
        })

        state["last_event"] = message


# ============================================================
# PARSE EXISTING ESP32 SERIAL OUTPUT
# ============================================================

def parse_status_line(line):

    text = line.strip()

    if not text:
        return

    upper = text.upper()

    with lock:

        state["connected"] = True
        state["last_update"] = time.time()

        # ----------------------------------------------------
        # X AXIS
        # ----------------------------------------------------

        match = re.search(
            r'X\s*[:=]\s*(-?\d+(?:\.\d+)?)\s*G',
            text,
            re.IGNORECASE
        )

        if match:
            state["ax"] = float(match.group(1))


        # ----------------------------------------------------
        # Y AXIS
        # ----------------------------------------------------

        match = re.search(
            r'Y\s*[:=]\s*(-?\d+(?:\.\d+)?)\s*G',
            text,
            re.IGNORECASE
        )

        if match:
            state["ay"] = float(match.group(1))


        # ----------------------------------------------------
        # Z AXIS
        # ----------------------------------------------------

        match = re.search(
            r'Z\s*[:=]\s*(-?\d+(?:\.\d+)?)\s*G',
            text,
            re.IGNORECASE
        )

        if match:
            state["az"] = float(match.group(1))


        # ----------------------------------------------------
        # TOTAL G / IMPACT
        # ----------------------------------------------------

        match = re.search(
            r'(?:TOTAL\s*G|IMPACT)'
            r'\s*[:=]\s*'
            r'(\d+(?:\.\d+)?)\s*G?',
            text,
            re.IGNORECASE
        )

        if match:
            state["impact"] = float(match.group(1))


        # ----------------------------------------------------
        # LATITUDE
        # ----------------------------------------------------

        match = re.search(
            r'(?:LAT(?:ITUDE)?)'
            r'\s*[:=]\s*'
            r'(-?\d+(?:\.\d+)?)',
            text,
            re.IGNORECASE
        )

        if match:
            state["latitude"] = float(match.group(1))


        # ----------------------------------------------------
        # LONGITUDE
        # ----------------------------------------------------

        match = re.search(
            r'(?:LON|LONG|LONGITUDE)'
            r'\s*[:=]\s*'
            r'(-?\d+(?:\.\d+)?)',
            text,
            re.IGNORECASE
        )

        if match:
            state["longitude"] = float(match.group(1))


        # ----------------------------------------------------
        # ALTITUDE
        # ----------------------------------------------------

        match = re.search(
            r'(?:ALT(?:ITUDE)?)'
            r'\s*[:=]\s*'
            r'(-?\d+(?:\.\d+)?)',
            text,
            re.IGNORECASE
        )

        if match:
            state["altitude"] = float(match.group(1))


        # ----------------------------------------------------
        # SPEED
        # ----------------------------------------------------

        match = re.search(
            r'(?:SPEED)'
            r'\s*[:=]\s*'
            r'(-?\d+(?:\.\d+)?)',
            text,
            re.IGNORECASE
        )

        if match:
            state["speed"] = float(match.group(1))


        # ----------------------------------------------------
        # SATELLITE COUNT
        # ----------------------------------------------------

        match = re.search(
            r'(?:SAT(?:ELLITES)?|SATELLITE COUNT)'
            r'\s*[:=]\s*(\d+)',
            text,
            re.IGNORECASE
        )

        if match:
            state["satellites"] = int(match.group(1))


        # ----------------------------------------------------
        # ACCIDENT COUNT
        # ----------------------------------------------------

        match = re.search(
            r'(?:ACCIDENT\s*COUNT|ACCIDENTS)'
            r'\s*[:=]\s*(\d+)',
            text,
            re.IGNORECASE
        )

        if match:
            state["accident_count"] = int(match.group(1))


        # ----------------------------------------------------
        # WARNING COUNTDOWN
        # ----------------------------------------------------

        match = re.search(
            r'(\d+)\s*'
            r'(?:SECONDS?|SEC)'
            r'\s*(?:REMAINING|LEFT)',
            text,
            re.IGNORECASE
        )

        if not match:

            match = re.search(
                r'(?:COUNTDOWN|WARNING)'
                r'.*?(\d+)\s*'
                r'(?:S|SEC|SECONDS?)',
                text,
                re.IGNORECASE
            )

        if match:
            state["warning_seconds"] = int(match.group(1))


        # ----------------------------------------------------
        # WIFI STATUS
        # ----------------------------------------------------

        if "WIFI" in upper:

            if any(word in upper for word in [
                "CONNECTED",
                "OK",
                "ONLINE"
            ]):
                state["wifi"] = "Connected"

            elif any(word in upper for word in [
                "FAILED",
                "DISCONNECTED",
                "OFFLINE"
            ]):
                state["wifi"] = "Disconnected"


        # ----------------------------------------------------
        # MPU6050 STATUS
        # ----------------------------------------------------

        if "MPU" in upper or "MPU6050" in upper:

            if any(word in upper for word in [
                "OK",
                "READY",
                "CONNECTED"
            ]):
                state["mpu"] = "OK"

            elif any(word in upper for word in [
                "FAIL",
                "ERROR",
                "NOT FOUND"
            ]):
                state["mpu"] = "Error"


        # ----------------------------------------------------
        # GPS STATUS
        # ----------------------------------------------------

        if "GPS" in upper:

            if any(word in upper for word in [
                "FIX",
                "LOCK",
                "READY"
            ]):
                state["gps"] = "Fix"

            elif any(word in upper for word in [
                "NO FIX",
                "WAITING"
            ]):
                state["gps"] = "Waiting"


        # ----------------------------------------------------
        # TELEGRAM STATUS
        # ----------------------------------------------------

        if "TELEGRAM" in upper:

            if any(word in upper for word in [
                "WORKING",
                "SENT",
                "SUCCESS",
                "OK"
            ]):
                state["telegram"] = "Working"

            elif any(word in upper for word in [
                "FAIL",
                "ERROR",
                "NOT CONFIGURED"
            ]):
                state["telegram"] = "Error"


        # ----------------------------------------------------
        # ACCIDENT STATE
        # ----------------------------------------------------

        if any(word in upper for word in [
            "ACCIDENT CONFIRMED",
            "ACCIDENT DETECTED"
        ]):

            state["accident"] = "Accident Confirmed"
            state["warning_seconds"] = None


        elif any(word in upper for word in [
            "ACCIDENT CANCELLED",
            "CANCELLED"
        ]):

            state["accident"] = "Cancelled"
            state["warning_seconds"] = None


        elif any(word in upper for word in [
            "ACCIDENT WARNING",
            "WARNING",
            "POSSIBLE ACCIDENT"
        ]):

            state["accident"] = "Warning"


        elif "NORMAL" in upper:

            state["accident"] = "Normal"


        # ----------------------------------------------------
        # GRAPH DATA
        # ----------------------------------------------------

        state["graph"].append({
            "t": time.strftime("%H:%M:%S"),
            "g": round(state["impact"], 3)
        })


    # --------------------------------------------------------
    # EVENT LOG
    # --------------------------------------------------------

    event_words = [
        "ACCIDENT",
        "WARNING",
        "CANCEL",
        "GPS",
        "TELEGRAM",
        "WIFI",
        "MPU6050",
        "SYSTEM READY",
        "START"
    ]

    if any(word in upper for word in event_words):

        add_log(text)


# ============================================================
# SERIAL READER THREAD
# ============================================================

def serial_worker():

    while True:

        port = find_port()

        if not port:

            with lock:
                state["connected"] = False
                state["port"] = None

            time.sleep(2)

            continue


        try:

            with serial.Serial(
                port,
                BAUD_RATE,
                timeout=1
            ) as ser:

                with lock:
                    state["connected"] = True
                    state["port"] = port

                add_log(
                    f"Serial connected: {port}"
                )

                # ESP32 may restart when serial is opened.
                time.sleep(2)


                while True:

                    raw = ser.readline()

                    if not raw:
                        continue

                    line = raw.decode(
                        "utf-8",
                        errors="ignore"
                    ).strip()

                    if line:
                        parse_status_line(line)


        except (
            serial.SerialException,
            OSError
        ):

            with lock:

                state["connected"] = False
                state["port"] = None

            add_log(
                "Serial disconnected. "
                "Waiting for ESP32..."
            )

            time.sleep(2)


        except Exception as error:

            add_log(
                f"Parser error: {error}"
            )

            time.sleep(1)


# ============================================================
# DASHBOARD PAGE
# ============================================================

@app.route("/")
def index():

    return render_template(
        "index.html"
    )


# ============================================================
# API USED BY JAVASCRIPT DASHBOARD
# ============================================================

@app.route("/api/status")
def api_status():

    with lock:

        data = dict(state)

        data["logs"] = list(
            state["logs"]
        )

        data["graph"] = list(
            state["graph"]
        )


    # If no Serial data has arrived for a while,
    # consider the ESP32 disconnected.
    if data["last_update"] is not None:

        if time.time() - data["last_update"] > 8:

            data["connected"] = False


    return jsonify(data)


# ============================================================
# START APPLICATION
# ============================================================

if __name__ == "__main__":

    worker = threading.Thread(
        target=serial_worker,
        daemon=True
    )

    worker.start()


    print()
    print("=" * 50)
    print("        VIGIDRIVE DASHBOARD")
    print("=" * 50)
    print()
    print("Embedded C program: UNCHANGED")
    print("Serial bridge: ACTIVE")
    print()
    print("Open this address in your browser:")
    print()
    print("http://127.0.0.1:5000")
    print()
    print("=" * 50)


    app.run(
        host="127.0.0.1",
        port=5000,
        debug=False
    )