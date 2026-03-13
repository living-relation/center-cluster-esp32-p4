import json
import sys
import os
import cantools

OUTPUT_DIR = "main/canbus/protocols"


def sanitize(name):
    return name.lower().replace(" ", "_")


def dbc_to_protocol(dbc_file):

    db = cantools.database.load_file(dbc_file)

    frames = []

    for message in db.messages:

        frame = {
            "id": hex(message.frame_id),
            "signals": []
        }

        for sig in message.signals:

            # convert bit start to byte offset
            byte_offset = sig.start // 8

            # convert bit length to bytes
            length_bytes = max(1, sig.length // 8)

            endian = "little" if sig.byte_order == "little_endian" else "big"

            signal = {
                "name": sanitize(sig.name),
                "offset": byte_offset,
                "len": length_bytes,
                "scale": sig.scale,
                "offset_val": sig.offset,
                "endian": endian
            }

            frame["signals"].append(signal)

        frames.append(frame)

    protocol_name = sanitize(os.path.basename(dbc_file).replace(".dbc", ""))

    return {
        "name": protocol_name,
        "bitrate": 1000000,
        "frames": frames
    }, protocol_name


def main():

    if len(sys.argv) < 2:
        print("Usage:")
        print("python dbc_to_dash_json.py file.dbc")
        return

    dbc_file = sys.argv[1]

    protocol, name = dbc_to_protocol(dbc_file)

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    output_file = os.path.join(OUTPUT_DIR, name + ".json")

    with open(output_file, "w") as f:
        json.dump(protocol, f, indent=2)

    print("Protocol created:", output_file)


if __name__ == "__main__":
    main()