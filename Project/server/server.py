import os
import socket
import argparse
import itertools
import sys

# --- Argument parsing ---
parser = argparse.ArgumentParser(description="TCP data receiver for accelerometer data")
parser.add_argument("--port", "-p", type=int, default=7123, help="Port to listen on")
parser.add_argument("--label", "-l", type=str, default=None, help="Optional label for output filenames")
args = parser.parse_args()

output_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
os.makedirs(output_dir, exist_ok=True)

last_num = itertools.count(1)

def get_unique_output_path():
    """Generate a unique CSV filename (like 00001.csv or label.00001.csv)."""
    while True:
        i = next(last_num)
        filename = f"{args.label + '.' if args.label else ''}{i:05d}.csv"
        out_path = os.path.join(output_dir, filename)
        if not os.path.exists(out_path):
            return out_path

def print_local_ips(port):
    """List available IPv4 addresses so the user can connect the Particle device."""
    print("Available network interfaces and addresses:")
    try:
        hostname = socket.gethostname()
        local_ip = socket.gethostbyname(hostname)
        print(f"  Host: {local_ip}")
    except Exception:
        print("  Could not determine local IP address.")
    print(f"\nServer listening on port {port}\n")

def handle_client(conn, addr):
    """Handle one TCP connection from Particle device."""
    print(f"Data connection started from {addr[0]}")
    out_path = get_unique_output_path()

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("timestamp,accX,accY,accZ\n")
        while True:
            data = conn.recv(4096)
            if not data:
                break
            try:
                text = data.decode("utf-8", errors="ignore")
                f.write(text)
            except Exception as e:
                print(f"Error decoding data: {e}")
                break

    print(f"Transmission complete. Data saved to {out_path}")
    conn.close()

def main():
    """Main TCP server loop."""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", args.port))
    server.listen(1)

    print_local_ips(args.port)

    try:
        while True:
            conn, addr = server.accept()
            handle_client(conn, addr)
    except KeyboardInterrupt:
        print("\nServer stopped by user (CTRL+C).")
    finally:
        try:
            server.close()
        except Exception:
            pass
        sys.exit(0)

if __name__ == "__main__":
    main()
