# Especificación del Protocolo de Aplicación IoT

## Descripción general

Protocolo basado en texto con campos separados por `|`.

Formato general:
```
COMANDO|campo1|campo2|...\n
```

## Comandos

### REGISTER — Registro de sensor
```
Cliente → Servidor: REGISTER|sensor_id|tipo
Servidor → Cliente: OK|sensor registrado
                    ERROR|mensaje
```

### DATA — Envío de medición (UDP)
```
Cliente → Servidor: DATA|sensor_id|tipo|valor|unidad
Servidor → Cliente: OK|dato recibido
                    ERROR|mensaje
```

### LOGIN — Autenticación de usuario
```
Cliente → Servidor: LOGIN|usuario|password
Servidor → Auth:    AUTH|usuario|password
Auth → Servidor:    OK|rol
Servidor → Cliente: OK|bienvenido usuario|rol:rol
                    ERROR|credenciales invalidas
```

### LIST — Listar sensores activos
```
Cliente → Servidor: LIST|
Servidor → Cliente: OK|sensor_id:tipo:valorunidad;...
```

### GET — Obtener datos de un sensor
```
Cliente → Servidor: GET|sensor_id
Servidor → Cliente: OK|sensor_id|tipo|valor|unidad
                    ERROR|sensor no encontrado
```

### STATS — Estadísticas del sistema
```
Cliente → Servidor: STATS|
Servidor → Cliente: OK|sensores:N|operadores:N
```

### ALERT — Notificación de alerta
```
Servidor → Operadores: ALERT|descripcion
```

## Umbrales de alerta

| Sensor | Umbral |
|---|---|
| temperatura | > 35 °C |
| humedad | > 90 % |
| presion | > 1050 hPa |
| vibracion | > 8 mm/s |
| energia | > 400 W |

## Tipos de socket

| Componente | Socket | Justificación |
|---|---|---|
| Sensores → Servidor | UDP | Alta frecuencia, latencia mínima |
| Operadores → Servidor | TCP | Confiabilidad en consultas |
| Auth → Servidor | TCP | Confiabilidad en autenticación |
| Dashboard | HTTP/TCP | Estándar web |

## Manejo de errores

- Si el DNS falla, el sistema registra el error y continúa sin finalizar
- Si el auth falla, el servidor responde ERROR sin caerse
- Si un cliente se desconecta, el hilo termina limpiamente
