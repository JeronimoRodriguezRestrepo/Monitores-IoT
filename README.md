# Sistema Distribuido de Monitoreo de Sensores IoT

Sistema de monitoreo IoT distribuido desplegado en AWS EC2, inspirado en SIATA.

## Arquitectura

```
[Sensores IoT] --UDP--> [Servidor C] <--TCP-- [Clientes Operadores]
                              |
                         [Auth Server]
                              |
                         [Dashboard HTTP]
                              |
                         [Route 53 DNS]
```

## Componentes

- **server.c** — Servidor central en C con sockets Berkeley (TCP + UDP)
- **auth_server.py** — Servicio externo de autenticación
- **sensor_client.py** — Cliente sensor en Python (UDP)
- **query_client.py** — Cliente operador en Python (TCP)
- **QueryClient.java** — Cliente operador en Java (TCP)
- **dashboard.html** — Interfaz web de monitoreo

## Puertos

| Servicio | Protocolo | Puerto |
|---|---|---|
| Sensores IoT | UDP | 9000 |
| Clientes TCP | TCP | 9001 |
| Autenticación | TCP | 9002 |
| Dashboard web | HTTP | 8080 |

## Despliegue en AWS

### 1 — Clonar el repositorio

```bash
git clone https://github.com/JeronimoRodriguezRestrepo/Monitores-IoT.git
cd Monitores-IoT
```

### 2 — Compilar el servidor

```bash
gcc -o server/server server/server.c -lpthread
```

### 3 — Configurar DNS con Route 53

1. Crear hosted zone privada `iot-monitor.local` en AWS Route 53
2. Crear registro tipo A: `server.iot-monitor.local` apuntando a la IP privada de la EC2
3. Configurar DNS en la instancia:

```bash
sudo ln -sf /run/systemd/resolve/resolv.conf /etc/resolv.conf
sudo systemctl restart systemd-resolved
sudo systemctl restart systemd-networkd
```

### 4 — Abrir puertos en Security Group

| Puerto | Protocolo |
|---|---|
| 9000 | UDP |
| 9001 | TCP |
| 9002 | TCP |
| 8080 | TCP |

### 5 — Iniciar servicios

```bash
# Terminal 1 - Servidor principal
./server/server 9001 server/logs.txt

# Terminal 2 - Servicio de autenticación
python3 server/auth_server.py

# Terminal 3 al 7 - Sensores (desde terminal de aws
python3 monitores-iot/clients/sensor_client.py server.iot-monitor.local sensor-01 temperatura
python3 monitores-iot/clients/sensor_client.py server.iot-monitor.local sensor-02 humedad
python3 monitores-iot/clients/sensor_client.py server.iot-monitor.local sensor-03 presion
python3 monitores-iot/clients/sensor_client.py server.iot-monitor.local sensor-04 vibracion
python3 monitores-iot/clients/sensor_client.py server.iot-monitor.local sensor-05 energia

# Cliente Python
python3 monitoes-iot/clients/query_client.py server.iot-monitor.local

# Cliente Java
cd monitores-iot
javac clients/QueryClient.java -d clients/
java -cp clients/ QueryClient server.iot-monitor.local
```

### 6 — Ver dashboard

```
http://<IP_PUBLICA_EC2>:8080
```

## Usuarios del sistema

| Usuario | Password | Rol |
|---|---|---|
| admin | admin123 | administrador |
| operador | oper456 | operador |
| sensor | sensor789 | sensor |
| monitor | monitor000 | operador |

## Configuración DNS

- Servicio: AWS Route 53
- Hosted zone: `iot-monitor.local` (privada)
- Registro: `server.iot-monitor.local` tipo A

## Como correr sensores desde computador local

1. Clonar repositorio
  ```bash
git clone https://github.com/JeronimoRodriguezRestrepo/Monitores-IoT.git
cd Monitores-IoT
```

2. Tener corriendo el server en la terminal AWS

3. Correr los sensores en el powershell
   ```bash
    cd monitores-iot
    cd clients
    #Cada uno se abre desde una terminal diferente
    python sensor_client.py <<IP_PUBLICA>> sensor-local-01 temperatura
    python sensor_client.py <<IP_PUBLICA>> sensor-local-02 humedad
    python sensor_client.py <<IP_PUBLICA>> sensor-local-03 presion
    python sensor_client.py <<IP_PUBLICA>> sensor-local-04 vibracion
    python sensor_client.py <<IP_PUBLICA>> sensor-local-05 energia
   

```
