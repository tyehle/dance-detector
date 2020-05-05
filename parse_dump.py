import numpy as np
import matplotlib.pyplot as plt
import sys
import math


def bytes_to_float(bs):
    return list(np.array(bs, dtype=np.uint8).view(dtype=np.float32))


def parse_accel_packet(data_bytes, start):
    x, y, z = bytes_to_float(data_bytes[start+2:start+14])
    return (x, y, z)


def parse_all_packets(data_bytes):
    packet_lengths = {
        ord('A'): 15,
        ord('G'): 15,
        ord('M'): 15,
        ord('Q'): 19,
        ord('B'): 5,
        ord('C'): 6,
        ord('L'): 15,
    }

    pos = 0
    packets = []
    while pos < len(data_bytes):
        if data_bytes[pos] != 33:
            # Not the start of a packet, keep looking
            pos += 1
        elif data_bytes[pos+1] not in packet_lengths:
            print(f"Unknown packet type: {chr(data_bytes[pos+1])}")
            pos += 1
        else:
            # This is the start of a packet
            if chr(data_bytes[pos+1]) == 'A':
                packets.append(parse_accel_packet(data_bytes, pos))
            
            pos += packet_lengths[data_bytes[pos+1]]

    return packets


def plot_packets(packets):
    plt.plot([x for x,y,z in packets], label="x")
    plt.plot([y for x,y,z in packets], label="y")
    plt.plot([z for x,y,z in packets], label="z")
    plt.ylabel("accel / Gs")
    plt.xlabel("samples")
    plt.legend()
    plt.show()


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "dump.txt"
    
    data_bytes = []
    with open(path, "r") as h:
        for line in h.readlines():
            data_bytes.extend(int(s, 16) for s in line.split())

    print(len(data_bytes))
    accel_data = parse_all_packets(data_bytes)
    print(f"Found {len(accel_data)} packets")

    plot_packets(accel_data)

    # plt.plot([math.sqrt(x*x + y*y + z*z) for x,y,z in accel_data])
    # plt.show()


if __name__ == "__main__":
    main()
