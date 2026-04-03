#!/usr/bin/env python3
"""
Cliente Sensor — envía datos simulados vía UDP
Uso: python3 sensor_client.py <SERVER_IP> <SENSOR_ID> <TIPO>
Tipos: temperatura, humedad, calidad_aire, lluvia
"""

import socket
import json
import time
import random
import sys

def generar_dato(sensor_id, tipo):
    if tipo == 'temperatura':
        valor = round(random.uniform(15.0, 35.0), 2)
        unidad = '°C'
    elif tipo == 'humedad':
        valor = round(random.uniform(40.0, 95.0), 2)
        unidad = '%'
    elif tipo == 'calidad_aire':
        valor = round(random.uniform(0, 300), 1)
        unidad = 'AQI'
    elif tipo == 'lluvia':
        valor = round(random.uniform(0, 50.0), 2)
        unidad = 'mm/h'
    else:
        valor = round(random.uniform(0, 100), 2)
        unidad = 'u'

    return {
        'sensor_id': sensor_id,
        'tipo': tipo,
        'valor': valor,
        'unidad': unidad
    }

def main():
    if len(sys.argv) < 4:
        print("Uso: python3 sensor_client.py <SERVER_IP> <SENSOR_ID> <TIPO>")
        print("Tipos disponibles: temperatura, humedad, calidad_aire, lluvia")
        sys.exit(1)

    server_ip = sys.argv[1]
    sensor_id = sys.argv[2]
    tipo = sys.argv[3]
    port = 9000

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"[SENSOR {sensor_id}] Enviando datos de {tipo} a {server_ip}:{port}")

    while True:
        dato = generar_dato(sensor_id, tipo)
        mensaje = json.dumps(dato).encode()
        sock.sendto(mensaje, (server_ip, port))
        print(f"[SENSOR] Enviado: {dato}")
        time.sleep(3)

if __name__ == '__main__':
    main()
