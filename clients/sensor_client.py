#!/usr/bin/env python3
"""
Cliente Sensor — envía datos simulados vía UDP
Uso: python3 sensor_client.py <SERVER_HOSTNAME> <SENSOR_ID> <TIPO>
Tipos: temperatura, humedad, presion, vibracion, energia
"""

import socket
import json
import time
import random
import sys

def resolver_hostname(hostname):
    try:
        ip = socket.gethostbyname(hostname)
        print(f"[DNS] {hostname} → {ip}")
        return ip
    except socket.gaierror:
        print(f"[ERROR] No se pudo resolver {hostname}")
        sys.exit(1)

def generar_dato(sensor_id, tipo):
    if tipo == 'temperatura':
        valor = round(random.uniform(15.0, 40.0), 2)
        unidad = 'C'
    elif tipo == 'humedad':
        valor = round(random.uniform(40.0, 95.0), 2)
        unidad = '%'
    elif tipo == 'presion':
        valor = round(random.uniform(980.0, 1060.0), 2)
        unidad = 'hPa'
    elif tipo == 'vibracion':
        valor = round(random.uniform(0.0, 10.0), 2)
        unidad = 'mm/s'
    elif tipo == 'energia':
        valor = round(random.uniform(100.0, 500.0), 2)
        unidad = 'W'
    else:
        valor = round(random.uniform(0, 100), 2)
        unidad = 'u'

    return sensor_id, tipo, valor, unidad

def main():
    if len(sys.argv) < 4:
        print("Uso: python3 sensor_client.py <SERVER_HOSTNAME> <SENSOR_ID> <TIPO>")
        print("Tipos: temperatura, humedad, presion, vibracion, energia")
        sys.exit(1)

    hostname = sys.argv[1]
    sensor_id = sys.argv[2]
    tipo = sys.argv[3]
    port = 9000

    # Resolver DNS
    server_ip = resolver_hostname(hostname)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Registrar sensor primero via TCP
    try:
        tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp_sock.connect((server_ip, 9001))
        msg = f"REGISTER|{sensor_id}|{tipo}\n"
        tcp_sock.sendall(msg.encode())
        resp = tcp_sock.recv(1024).decode().strip()
        print(f"[REGISTER] {resp}")
        tcp_sock.close()
    except Exception as e:
        print(f"[ERROR] No se pudo registrar: {e}")
        sys.exit(1)

    print(f"[SENSOR {sensor_id}] Enviando datos de {tipo} a {hostname}:{port}")

    while True:
        sensor_id_r, tipo_r, valor, unidad = generar_dato(sensor_id, tipo)
        # Protocolo: DATA|sensor_id|tipo|valor|unidad
        mensaje = f"DATA|{sensor_id_r}|{tipo_r}|{valor}|{unidad}"
        sock.sendto(mensaje.encode(), (server_ip, port))
        print(f"[SENSOR] Enviado: {mensaje}")
        time.sleep(3)

if __name__ == '__main__':
    main()
