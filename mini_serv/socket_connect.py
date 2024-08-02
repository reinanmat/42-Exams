import sys
import socket

def connect_loop(times, host, port, message):
    for _ in range(times):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((host, port))
            if len(message) != 0:
                s.sendall(message.encode())
            s.close()
        except:
            print("Error: connect")
            break
    

def main():
    try:
        # DEFAULT VALUES
        host = "localhost"
        port = 8000
        times = 10
        message = ""

        if not (len(sys.argv) == 5 or len(sys.argv) == 7 or len(sys.argv) == 9):
            raise
        i = 1
        while (i < len(sys.argv)):
            arg = sys.argv[i]
            if host == "localhost" and arg == "-h" or arg == "--host":
                host = sys.argv[i + 1]
            elif port == 8000 and arg == "-p" or arg == "--port":
                port = int(sys.argv[i + 1])
            elif times == 10 and arg == "-c" or arg == "--connections":
                times = int(sys.argv[i + 1])
            elif message == "" and arg == "-m" or arg == "--message":
                message = sys.argv[i + 1]
            else:
                raise
            i += 2
    except:
        print("Error: Invalid parameters")
        sys.exit(1)

    print(f"try connecting in {host}:{port} {times} times", (f'and send message "{message}"' if len(message) != 0 else ""))
    connect_loop(times, host, port, message + "\n")


if __name__ == "__main__":
    main()
