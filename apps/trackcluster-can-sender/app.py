"""
app.py -- TrackCluster CAN Live Sender (UNIFIED: RealDash + Center Cluster).

One app, two device profiles selectable in the UI:
  - RealDash      -> sends 0x3EF / 0x3F0 / 0x3F1  (Link G4X RealDash stream)
  - Center Cluster-> sends 0x3E8 / 0x3E9 / 0x3EA / 0x3EB / 0x3EE (ESP32-P4 cluster)

The Python side is catalog-agnostic: it transmits whatever (id, 8 bytes) the UI
computes. The two device signal maps live in ui/index.html and are byte-for-byte
copies of the two original per-device apps -- CAN IDs, byte layouts, scaling and
endianness are unchanged. This app only merges the launcher/backend; it does not
alter any channel or encoding.

Back end features (shared by both profiles):
  - Auto-detects and hot-plug auto-connects the moment a supported adapter appears
    (connect-only: never transmits until you press Send / enable continuous).
  - Unplug detection disconnects cleanly.
  - Selectable bitrate (default 1 Mbit/s -- both devices run 1 Mbit/s).

Supported adapters:
  gs_usb/candleLight (CANable, CANtact, Innomaker; bundled libusb), slcan,
  seeedstudio (Seeed USB-CAN Analyzer), and pcan/kvaser/vector when that vendor's
  driver is installed on the PC.
"""

import os
import sys
import threading
import time
import traceback

import can
import serial.tools.list_ports

DEFAULT_BITRATE = 1_000_000
POLL_SECONDS = 1.2

GS_USB_IDS = {
    (0x1D50, 0x606F),  # candleLight / CANable / Innomaker USB2CAN
    (0x1209, 0x2323),  # CANtact-class candleLight
    (0x1CD2, 0x606F),  # some candleLight rebadges
}
SLCAN_IDS = {
    (0x0483, 0x5740),  # STM32 VCP -- CANable slcan and clones
    (0x16D0, 0x117E),  # CANtact slcan
}
SEEED_IDS = {
    (0x1A86, 0x7523),  # CH340 -- Seeed USB-CAN Analyzer
}
VENDOR_INTERFACES = ("pcan", "kvaser", "vector")


def _libusb_backend():
    """Return a libusb backend, preferring the bundled one from libusb-package so
    gs_usb enumeration works in a portable/frozen build with no system libusb."""
    try:
        import usb.backend.libusb1 as libusb1
        try:
            import libusb_package
            return libusb1.get_backend(find_library=libusb_package.find_library)
        except Exception:
            return libusb1.get_backend()
    except Exception:
        return None


class Api:
    """Exposed to the UI as window.pywebview.api.<method>(...). Bus access is
    serialized with a lock; a background poller does detection + auto-connect."""

    def __init__(self):
        self._bus = None
        self._backend = None
        self._identifier = None
        self._bitrate = DEFAULT_BITRATE
        self._auto = True
        self._adapters = []
        self._message = "Starting…"
        self._lock = threading.Lock()
        self._usb_backend = _libusb_backend()
        self._poller = None
        self._stop = threading.Event()

    # ---- settings from the UI ---------------------------------------------
    def set_bitrate(self, bps):
        try:
            self._bitrate = int(bps)
        except Exception:
            self._bitrate = DEFAULT_BITRATE
        return {"ok": True, "bitrate": self._bitrate}

    def set_auto(self, enabled):
        self._auto = bool(enabled)
        return {"ok": True, "auto": self._auto}

    # ---- adapter discovery -------------------------------------------------
    def scan_adapters(self):
        adapters = []
        if self._usb_backend is not None:
            try:
                import usb.core
                idx = 0
                for dev in usb.core.find(find_all=True, backend=self._usb_backend):
                    if (dev.idVendor, dev.idProduct) in GS_USB_IDS:
                        try:
                            product = dev.product or "candleLight/gs_usb"
                        except Exception:
                            product = "candleLight/gs_usb"
                        adapters.append({
                            "backend": "gs_usb", "identifier": str(idx),
                            "label": f"{product} (candleLight/gs_usb)",
                            "auto_ok": True})
                        idx += 1
            except Exception as e:
                print("gs_usb scan skipped:", e)

        try:
            for port in serial.tools.list_ports.comports():
                vidpid = (port.vid, port.pid)
                if vidpid in SEEED_IDS:
                    adapters.append({
                        "backend": "seeedstudio", "identifier": port.device,
                        "label": f"{port.device} — Seeed USB-CAN Analyzer",
                        "auto_ok": True})
                elif vidpid in SLCAN_IDS:
                    adapters.append({
                        "backend": "slcan", "identifier": port.device,
                        "label": f"{port.device} — {port.description} (slcan)",
                        "auto_ok": True})
                else:
                    adapters.append({
                        "backend": "slcan", "identifier": port.device,
                        "label": f"{port.device} — {port.description} "
                                 f"(pick if in slcan mode)",
                        "auto_ok": False})
        except Exception as e:
            print("serial scan skipped:", e)

        for iface in VENDOR_INTERFACES:
            try:
                for cfg in can.detect_available_configs(iface):
                    chan = cfg.get("channel")
                    if chan is None:
                        continue  # no usable channel -> don't offer a dead adapter
                    adapters.append({
                        "backend": iface, "identifier": str(chan),
                        "label": f"{iface} — channel {chan}",
                        "auto_ok": True})
            except Exception:
                pass
        return adapters

    # ---- connection --------------------------------------------------------
    def _open_bus(self, backend, identifier, bitrate):
        if backend == "gs_usb":
            return can.Bus(interface="gs_usb", channel=int(identifier), bitrate=bitrate)
        if backend == "slcan":
            return can.Bus(interface="slcan", channel=identifier, bitrate=bitrate)
        if backend == "seeedstudio":
            return can.Bus(interface="seeedstudio", channel=identifier, bitrate=bitrate)
        if backend in VENDOR_INTERFACES:
            try:
                chan = int(identifier)
            except (TypeError, ValueError):
                chan = identifier
            return can.Bus(interface=backend, channel=chan, bitrate=bitrate)
        raise ValueError(f"Unknown backend: {backend}")

    def connect(self, backend, identifier, bitrate=None):
        try:
            br = int(bitrate) if bitrate else self._bitrate
            self._bitrate = br
            with self._lock:
                if self._bus is not None:
                    try:
                        self._bus.shutdown()
                    except Exception:
                        pass
                    self._bus = None
                self._bus = self._open_bus(backend, identifier, br)
                self._backend = backend
                self._identifier = identifier
            self._message = f"Connected — {backend} {identifier} @ {br} bps"
            return {"ok": True, "backend": backend, "identifier": identifier, "bitrate": br}
        except Exception as e:
            # _open_bus failed: make sure we don't leave stale backend/identifier
            # (get_state() must report a consistent "disconnected" view).
            with self._lock:
                self._bus = None
                self._backend = None
                self._identifier = None
            self._message = f"Connect failed: {e}"
            traceback.print_exc()
            return {"ok": False, "error": str(e)}

    def disconnect(self):
        self._auto = False
        with self._lock:
            if self._bus is not None:
                try:
                    self._bus.shutdown()
                except Exception:
                    pass
            self._bus = None
            self._backend = None
            self._identifier = None
        self._message = "Disconnected (auto-connect off — re-enable to resume)"
        return {"ok": True}

    def status(self):
        return {"connected": self._bus is not None, "backend": self._backend,
                "identifier": self._identifier}

    def get_state(self):
        return {
            "connected": self._bus is not None,
            "backend": self._backend,
            "identifier": self._identifier,
            "bitrate": self._bitrate,
            "auto": self._auto,
            "adapters": list(self._adapters),
            "message": self._message,
        }

    # ---- sending -----------------------------------------------------------
    def send_frame(self, can_id_hex, data_bytes):
        try:
            arb = int(can_id_hex, 16)
            if not (0 <= arb <= 0x7FF):
                return {"ok": False,
                        "error": f"CAN ID {can_id_hex} outside 11-bit range (0x000-0x7FF)"}
            data = bytes(data_bytes)
            if len(data) != 8:
                return {"ok": False,
                        "error": f"Payload must be 8 bytes, got {len(data)}"}
            msg = can.Message(
                arbitration_id=arb,
                data=data,
                is_extended_id=False,
            )
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

    # ---- background auto-detect / hot-plug --------------------------------
    def start_poller(self):
        if self._poller is None:
            self._message = "Watching for adapters…"
            self._poller = threading.Thread(target=self._poll_loop, daemon=True)
            self._poller.start()

    def _current_present(self, adapters):
        for a in adapters:
            if a["backend"] == self._backend and a["identifier"] == self._identifier:
                return True
        return False

    def _poll_loop(self):
        while not self._stop.is_set():
            try:
                adapters = self.scan_adapters()
                self._adapters = adapters
                if self._bus is not None:
                    if not self._current_present(adapters):
                        self._message = "Adapter unplugged — disconnected"
                        with self._lock:
                            try:
                                if self._bus:
                                    self._bus.shutdown()
                            except Exception:
                                pass
                            self._bus = None
                            self._backend = None
                            self._identifier = None
                elif self._auto:
                    target = next((a for a in adapters if a.get("auto_ok")), None)
                    if target:
                        self._message = (f"Adapter detected — auto-connecting to "
                                         f"{target['label']}")
                        self.connect(target["backend"], target["identifier"],
                                     self._bitrate)
                    else:
                        self._message = "Watching for adapters…"
            except Exception as e:
                self._message = f"Poller error: {e}"
            self._stop.wait(POLL_SECONDS)


def resource_path(relative_path):
    base_path = getattr(sys, "_MEIPASS", os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base_path, relative_path)


def main():
    import webview
    api = Api()
    api.start_poller()
    window = webview.create_window(
        "TrackCluster — CAN Live Sender",
        resource_path("ui/index.html"),
        js_api=api,
        width=1180,
        height=880,
        min_size=(720, 600),
    )

    # Optional deep-link: launch straight into a device profile with
    # TC_DEVICE=cluster|realdash (used for automated screenshots/tests; harmless
    # when unset -- the UI then uses its saved/last profile).
    dev = os.environ.get("TC_DEVICE")
    if dev in ("cluster", "realdash"):
        def _on_loaded():
            try:
                window.evaluate_js(
                    "document.getElementById('deviceSelect').value=%r;"
                    "applyProfile(%r);" % (dev, dev)
                )
            except Exception:
                pass
        window.events.loaded += _on_loaded

    webview.start()


if __name__ == "__main__":
    main()
