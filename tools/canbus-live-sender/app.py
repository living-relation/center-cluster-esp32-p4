"""
app.py -- TrackCluster CAN bench-test LIVE SENDER.

Companion to ../../docs/canbus-bench-test.html (which is a browser-only, copy
-the-hex-out tool). This app has the same UI, but actually transmits frames to
a connected CAN-USB adapter via python-can, so gauges react live on the bench.

Adapter support:
  - candleLight/gs_usb  -- CANable's default firmware, what CANgaroo already
                            talks to on this hardware. Needs the Zadig/WinUSB
                            driver bound once, same as CANgaroo requires.
  - slcan                -- shows up as a plain COM port; used if the adapter
                            is reflashed to slcan mode.

This file intentionally duplicates the signal map from docs/canbus-bench-test.html
rather than sharing code with it, so that file is never modified by this app.
"""

import os
import sys
import threading
import time
import traceback

import can
import serial.tools.list_ports
import usb.core
import webview

GS_USB_VID = 0x1D50
GS_USB_PID = 0x606F
BITRATE = 1_000_000


class Api:
    """Exposed to the UI as window.pywebview.api.<method>(...) -- each call
    returns a Promise in JS and runs in its own Python thread, so all bus
    access below is serialized with a lock."""

    def __init__(self):
        self._bus = None
        self._backend = None
        self._identifier = None
        self._lock = threading.Lock()

    # ---- adapter discovery -------------------------------------------------
    def scan_adapters(self):
        adapters = []

        # candleLight / gs_usb devices (CANable's default firmware).
        try:
            devices = list(
                usb.core.find(idVendor=GS_USB_VID, idProduct=GS_USB_PID, find_all=True)
            )
            for i, dev in enumerate(devices):
                try:
                    product = dev.product or f"gs_usb device #{i}"
                except Exception:
                    product = f"gs_usb device #{i}"
                adapters.append(
                    {
                        "backend": "gs_usb",
                        "identifier": str(i),
                        "label": f"{product} (candleLight/gs_usb)",
                    }
                )
        except Exception as e:
            # libusb backend missing or no permission -- not fatal, slcan
            # devices below are still listed.
            print("gs_usb scan skipped:", e)

        # slcan-mode adapters show up as a plain COM port.
        try:
            for port in serial.tools.list_ports.comports():
                adapters.append(
                    {
                        "backend": "slcan",
                        "identifier": port.device,
                        "label": f"{port.device} — {port.description} (pick if adapter is in slcan mode)",
                    }
                )
        except Exception as e:
            print("slcan scan skipped:", e)

        return adapters

    # ---- connection ----------------------------------------------------------
    def connect(self, backend, identifier):
        try:
            with self._lock:
                if self._bus is not None:
                    self._bus.shutdown()
                    self._bus = None

                if backend == "gs_usb":
                    self._bus = can.Bus(
                        interface="gs_usb", channel=int(identifier), bitrate=BITRATE
                    )
                elif backend == "slcan":
                    self._bus = can.Bus(
                        interface="slcan", channel=identifier, bitrate=BITRATE
                    )
                else:
                    return {"ok": False, "error": f"Unknown backend: {backend}"}

                self._backend = backend
                self._identifier = identifier
            return {"ok": True, "backend": backend, "identifier": identifier}
        except Exception as e:
            traceback.print_exc()
            return {"ok": False, "error": str(e)}

    def disconnect(self):
        with self._lock:
            if self._bus is not None:
                try:
                    self._bus.shutdown()
                except Exception:
                    pass
                self._bus = None
                self._backend = None
                self._identifier = None
        return {"ok": True}

    def status(self):
        return {
            "connected": self._bus is not None,
            "backend": self._backend,
            "identifier": self._identifier,
        }

    # ---- sending ---------------------------------------------------------
    def send_frame(self, can_id_hex, data_bytes):
        """data_bytes: list of 8 ints (0-255), already computed by the UI."""
        try:
            msg = can.Message(
                arbitration_id=int(can_id_hex, 16),
                data=bytes(data_bytes),
                is_extended_id=False,
            )
            # The None-check and the send happen under the same lock
            # acquisition, so a concurrent disconnect() can't set _bus=None
            # in between (which would otherwise raise AttributeError instead
            # of returning a clean "Not connected").
            with self._lock:
                if self._bus is None:
                    return {"ok": False, "error": "Not connected"}
                self._bus.send(msg, timeout=0.2)
            return {"ok": True, "sent_at": time.strftime("%H:%M:%S")}
        except can.CanError as e:
            return {"ok": False, "error": str(e)}
        except Exception as e:
            traceback.print_exc()
            return {"ok": False, "error": str(e)}


def resource_path(relative_path):
    """Resolve a bundled resource path whether running from source or as a
    PyInstaller --onefile executable. A frozen onefile build extracts its
    --add-data payload to a temp dir at sys._MEIPASS at runtime -- it is NOT
    next to the .exe or the current working directory, so a plain relative
    path like "ui/index.html" resolves to nothing there and the window opens
    blank."""
    base_path = getattr(sys, "_MEIPASS", os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base_path, relative_path)


def main():
    api = Api()
    webview.create_window(
        "TrackCluster Center — CAN Live Sender",
        resource_path("ui/index.html"),
        js_api=api,
        width=1180,
        height=860,
        min_size=(720, 600),
    )
    webview.start()


if __name__ == "__main__":
    main()
