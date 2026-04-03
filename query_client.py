#!/usr/bin/env python3
"""
Cliente de Consulta — se conecta al servidor vía TCP
Uso: python3 query_client.py <SERVER_IP>
"""

import socket
import json
import sys

def enviar_comando(sock, comando_dict):
    sock.sendall(json.dumps(comando_dict).encode())
    respuesta = sock.recv(4096)
    return json.loads(respuesta.decode())

def main():
    server_ip = sys.argv[1] if len(sys.argv) > 1 else '127.0.0.1'
    port = 9001

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((server_ip, port))
    print(f"✅ Conectado al servidor {server_ip}:{port}\n")

    while True:
        print("\nComandos disponibles:")
        print("  1) LIST  — listar sensores activos")
        print("  2) GET   — obtener datos de un sensor")
        print("  3) STATS — estadísticas globales")
        print("  4) Salir")
        opcion = input("\nOpción: ").strip()

        if opcion == '1':
            resp = enviar_comando(sock, {'comando': 'LIST'})
            print(f"Sensores activos: {resp.get('sensores', [])}")

        elif opcion == '2':
            sid = input("ID del sensor: ").strip()
            resp = enviar_comando(sock, {'comando': 'GET', 'sensor_id': sid})
            datos = resp.get('datos', [])
            print(f"\nÚltimas lecturas de {sid}:")
            for d in datos:
                print(f"  {d['timestamp']} → {d['datos']}")

        elif opcion == '3':
            resp = enviar_comando(sock, {'comando': 'STATS'})
            print(f"Total sensores: {resp.get('total_sensores')}")
            print(f"Total lecturas: {resp.get('total_lecturas')}")

        elif opcion == '4':
            break

    sock.close()

if __name__ == '__main__':
    main()

