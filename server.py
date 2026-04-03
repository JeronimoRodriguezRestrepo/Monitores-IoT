#!/usr/bin/env python3
"""
Servidor Central - Sistema de Monitoreo IoT
- Puerto 9000: UDP (recibe datos de sensores)
- Puerto 9001: TCP (atiende consultas de clientes)
- Puerto 8080: HTTP (dashboard web)
"""

import socket
import threading
import json
import time
import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler

# Base de datos en memoria
sensor_data = {}
lock = threading.Lock()

# ─────────────────────────────────────────
# 1. SERVIDOR UDP — recibe datos de sensores
# ─────────────────────────────────────────
def udp_server(host='0.0.0.0', port=9000):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((host, port))
    print(f"[UDP] Servidor escuchando en {host}:{port}")

    while True:
        try:
            data, addr = sock.recvfrom(1024)
            mensaje = json.loads(data.decode())
            sensor_id = mensaje.get('sensor_id', 'desconocido')
            timestamp = datetime.datetime.now().isoformat()

            with lock:
                if sensor_id not in sensor_data:
                    sensor_data[sensor_id] = []
                sensor_data[sensor_id].append({
                    'timestamp': timestamp,
                    'datos': mensaje,
                    'ip': addr[0]
                })
                # Guardar solo los últimos 100 registros por sensor
                sensor_data[sensor_id] = sensor_data[sensor_id][-100:]

            print(f"[UDP] Sensor {sensor_id} desde {addr}: {mensaje}")
        except Exception as e:
            print(f"[UDP] Error: {e}")

# ─────────────────────────────────────────
# 2. SERVIDOR TCP — atiende consultas
# ─────────────────────────────────────────
def handle_tcp_client(conn, addr):
    print(f"[TCP] Conexión desde {addr}")
    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break

            mensaje = json.loads(data.decode())
            comando = mensaje.get('comando', '')

            if comando == 'LIST':
                # Listar todos los sensores activos
                with lock:
                    sensores = list(sensor_data.keys())
                respuesta = {'status': 'ok', 'sensores': sensores}

            elif comando == 'GET':
                # Obtener últimos datos de un sensor
                sensor_id = mensaje.get('sensor_id')
                with lock:
                    datos = sensor_data.get(sensor_id, [])
                respuesta = {'status': 'ok', 'sensor_id': sensor_id, 'datos': datos[-10:]}

            elif comando == 'STATS':
                # Estadísticas globales
                with lock:
                    total = sum(len(v) for v in sensor_data.values())
                respuesta = {
                    'status': 'ok',
                    'total_sensores': len(sensor_data),
                    'total_lecturas': total
                }

            else:
                respuesta = {'status': 'error', 'mensaje': 'Comando desconocido'}

            conn.sendall(json.dumps(respuesta).encode())

    except Exception as e:
        print(f"[TCP] Error con {addr}: {e}")
    finally:
        conn.close()
        print(f"[TCP] Conexión cerrada: {addr}")

def tcp_server(host='0.0.0.0', port=9001):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((host, port))
    sock.listen(5)
    print(f"[TCP] Servidor escuchando en {host}:{port}")

    while True:
        conn, addr = sock.accept()
        t = threading.Thread(target=handle_tcp_client, args=(conn, addr))
        t.daemon = True
        t.start()

# ─────────────────────────────────────────
# 3. SERVIDOR HTTP — dashboard web
# ─────────────────────────────────────────
class DashboardHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass  # silenciar logs de HTTP

    def do_GET(self):
        if self.path == '/':
            self.serve_dashboard()
        elif self.path == '/api/data':
            self.serve_api()
        else:
            self.send_error(404)

    def serve_api(self):
        with lock:
            datos = {sid: registros[-5:] for sid, registros in sensor_data.items()}
        body = json.dumps(datos).encode()
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(body)

    def serve_dashboard(self):
        html = """<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <title>IoT Monitor - SIATA</title>
    <meta http-equiv="refresh" content="5">
    <style>
        body { font-family: Arial, sans-serif; background: #0f172a; color: #e2e8f0; padding: 20px; }
        h1 { color: #38bdf8; }
        .sensor { background: #1e293b; border-radius: 8px; padding: 15px; margin: 10px 0; border-left: 4px solid #38bdf8; }
        .lectura { font-size: 12px; color: #94a3b8; margin: 3px 0; }
        .badge { background: #0ea5e9; padding: 2px 8px; border-radius: 12px; font-size: 11px; }
    </style>
</head>
<body>
    <h1>Sistema de Monitoreo IoT</h1>
    <p>Actualización automática cada 5 segundos</p>
    <div id="content">Cargando...</div>
    <script>
        fetch('/api/data')
            .then(r => r.json())
            .then(data => {
                const keys = Object.keys(data);
                if (keys.length === 0) {
                    document.getElementById('content').innerHTML = '<p>Sin datos aún. Inicia los clientes sensores.</p>';
                    return;
                }
                let html = '';
                keys.forEach(sid => {
                    const registros = data[sid];
                    const ultimo = registros[registros.length - 1];
                    html += `<div class="sensor">
                        <strong> Sensor: <span class="badge">${sid}</span></strong>
                        <br><small>IP: ${ultimo.ip} | Última lectura: ${ultimo.timestamp}</small>`;
                    registros.reverse().forEach(r => {
                        html += `<div class="lectura"> ${JSON.stringify(r.datos)}</div>`;
                    });
                    html += '</div>';
                });
                document.getElementById('content').innerHTML = html;
            });
    </script>
</body>
</html>"""
        body = html.encode()
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(body)

def http_server(host='0.0.0.0', port=8080):
    server = HTTPServer((host, port), DashboardHandler)
    print(f"[HTTP] Dashboard en http://{host}:{port}")
    server.serve_forever()

# ─────────────────────────────────────────
# MAIN — lanzar todos los servidores
# ─────────────────────────────────────────
if __name__ == '__main__':
    threads = [
        threading.Thread(target=udp_server, daemon=True),
        threading.Thread(target=tcp_server, daemon=True),
        threading.Thread(target=http_server, daemon=True),
    ]
    for t in threads:
        t.start()

    print("\n Servidor IoT iniciado. Ctrl+C para detener.\n")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nServidor detenido.")
