#!/usr/bin/env python3
"""
Servicio de Autenticación Externo
Puerto 9002
Protocolo: AUTH|usuario|password → OK|rol o ERROR|mensaje
"""

import socket
import threading

# Base de usuarios (en un sistema real sería una BD)
USUARIOS = {
    'admin':    {'password': 'admin123',   'rol': 'administrador'},
    'operador': {'password': 'oper456',    'rol': 'operador'},
    'sensor':   {'password': 'sensor789',  'rol': 'sensor'},
    'monitor':  {'password': 'monitor000', 'rol': 'operador'},
}

def handle_client(conn, addr):
    print(f"[AUTH] Conexión desde {addr}")
    try:
        data = conn.recv(1024).decode().strip()
        partes = data.split('|')

        if len(partes) < 3 or partes[0] != 'AUTH':
            conn.sendall(b'ERROR|formato invalido\n')
            return

        usuario = partes[1]
        password = partes[2]

        if usuario in USUARIOS and USUARIOS[usuario]['password'] == password:
            rol = USUARIOS[usuario]['rol']
            respuesta = f'OK|{rol}\n'
            print(f"[AUTH] Usuario '{usuario}' autenticado como {rol}")
        else:
            respuesta = 'ERROR|usuario o password incorrectos\n'
            print(f"[AUTH] Fallo autenticación para '{usuario}'")

        conn.sendall(respuesta.encode())
    except Exception as e:
        print(f"[AUTH] Error: {e}")
    finally:
        conn.close()

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', 9002))
    server.listen(5)
    print("[AUTH] Servicio de autenticación en puerto 9002")

    while True:
        conn, addr = server.accept()
        t = threading.Thread(target=handle_client, args=(conn, addr))
        t.daemon = True
        t.start()

if __name__ == '__main__':
    main()
