#!/usr/bin/env python3
"""
Cliente Operador — consulta el servidor vía TCP
Uso: python3 query_client.py <SERVER_HOSTNAME>
"""

import socket
import sys

def resolver_hostname(hostname):
    try:
        ip = socket.gethostbyname(hostname)
        print(f"[DNS] {hostname} → {ip}")
        return ip
    except socket.gaierror:
        print(f"[ERROR] No se pudo resolver {hostname}")
        sys.exit(1)

def enviar_comando(sock, comando):
    sock.sendall((comando + "\n").encode())
    respuesta = sock.recv(4096).decode().strip()
    return respuesta

def main():
    hostname = sys.argv[1] if len(sys.argv) > 1 else 'server.iot-monitor.local'
    port = 9001

    server_ip = resolver_hostname(hostname)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((server_ip, port))
    print(f"Conectado a {hostname}:{port}\n")

    while True:
        print("\nComandos:")
        print("  1) LIST  — sensores activos")
        print("  2) GET   — datos de un sensor")
        print("  3) STATS — estadísticas")
        print("  4) Salir")
        opcion = input("\nOpción: ").strip()

        if opcion == '1':
            resp = enviar_comando(sock, "LIST|")
            print(f"Respuesta: {resp}")

        elif opcion == '2':
            sid = input("ID del sensor: ").strip()
            resp = enviar_comando(sock, f"GET|{sid}")
            print(f"Respuesta: {resp}")

        elif opcion == '3':
            resp = enviar_comando(sock, "STATS|")
            print(f"Respuesta: {resp}")

        elif opcion == '4':
            break

    sock.close()

if __name__ == '__main__':
    main()
