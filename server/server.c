#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_SENSORS 100
#define MAX_OPERATORS 50
#define MAX_MSG 2048
#define ALERT_TEMP 35.0
#define ALERT_HUMIDITY 90.0
#define ALERT_ENERGY 400.0
#define ALERT_VIBRATION 8.0
#define ALERT_PRESSURE 1050.0

// ─────────────────────────────────────────
// ESTRUCTURAS
// ─────────────────────────────────────────
typedef struct {
    char sensor_id[64];
    char tipo[32];
    double valor;
    char unidad[16];
    char ip[INET_ADDRSTRLEN];
    int port;
    time_t ultimo_reporte;
    int activo;
} Sensor;

typedef struct {
    int socket;
    char ip[INET_ADDRSTRLEN];
    int port;
    char rol[16];
    int activo;
} Operador;

// ─────────────────────────────────────────
// GLOBALES
// ─────────────────────────────────────────
Sensor sensores[MAX_SENSORS];
Operador operadores[MAX_OPERATORS];
int num_sensores = 0;
int num_operadores = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *log_file = NULL;
int tcp_port = 9001;
int udp_port = 9000;

// ─────────────────────────────────────────
// LOGGING
// ─────────────────────────────────────────
void log_evento(const char *ip, int port, const char *mensaje, const char *respuesta) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    char linea[4096];
    snprintf(linea, sizeof(linea),
        "[%s] IP:%s PORT:%d MSG:%s RESP:%s\n",
        timestamp, ip, port, mensaje, respuesta);

    printf("%s", linea);
    if (log_file) {
        fprintf(log_file, "%s", linea);
        fflush(log_file);
    }
}

// ─────────────────────────────────────────
// ALERTAS — notificar a operadores
// ─────────────────────────────────────────
void notificar_operadores(const char *alerta) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_operadores; i++) {
        if (operadores[i].activo) {
            send(operadores[i].socket, alerta, strlen(alerta), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

int verificar_alerta(const char *tipo, double valor, char *msg_alerta) {
    if (strcmp(tipo, "temperatura") == 0 && valor > ALERT_TEMP) {
        snprintf(msg_alerta, MAX_MSG, "ALERT|temperatura alta: %.2f C\n", valor);
        return 1;
    }
    if (strcmp(tipo, "humedad") == 0 && valor > ALERT_HUMIDITY) {
        snprintf(msg_alerta, MAX_MSG, "ALERT|humedad alta: %.2f%%\n", valor);
        return 1;
    }
    if (strcmp(tipo, "energia") == 0 && valor > ALERT_ENERGY) {
        snprintf(msg_alerta, MAX_MSG, "ALERT|consumo energetico alto: %.2f W\n", valor);
        return 1;
    }
    if (strcmp(tipo, "vibracion") == 0 && valor > ALERT_VIBRATION) {
        snprintf(msg_alerta, MAX_MSG, "ALERT|vibracion alta: %.2f mm/s\n", valor);
        return 1;
    }
    if (strcmp(tipo, "presion") == 0 && valor > ALERT_PRESSURE) {
        snprintf(msg_alerta, MAX_MSG, "ALERT|presion alta: %.2f hPa\n", valor);
        return 1;
    }
    return 0;
}

// ─────────────────────────────────────────
// PROTOCOLO — parsear y procesar mensajes
// ─────────────────────────────────────────

// Formato: COMANDO|campo1|campo2|...
void procesar_mensaje(const char *msg, char *respuesta, const char *ip, int port) {
    char buffer[MAX_MSG];
    strncpy(buffer, msg, MAX_MSG - 1);

    char *cmd = strtok(buffer, "|");
    if (!cmd) {
        snprintf(respuesta, MAX_MSG, "ERROR|comando invalido");
        return;
    }

    // REGISTER|sensor_id|tipo
    if (strcmp(cmd, "REGISTER") == 0) {
        char *sensor_id = strtok(NULL, "|");
        char *tipo = strtok(NULL, "|");
        if (!sensor_id || !tipo) {
            snprintf(respuesta, MAX_MSG, "ERROR|faltan campos");
            return;
        }
        pthread_mutex_lock(&mutex);
        // Buscar si ya existe
        for (int i = 0; i < num_sensores; i++) {
            if (strcmp(sensores[i].sensor_id, sensor_id) == 0) {
                sensores[i].activo = 1;
                sensores[i].ultimo_reporte = time(NULL);
                pthread_mutex_unlock(&mutex);
                snprintf(respuesta, MAX_MSG, "OK|sensor ya registrado");
                return;
            }
        }
        if (num_sensores < MAX_SENSORS) {
            strncpy(sensores[num_sensores].sensor_id, sensor_id, 63);
            strncpy(sensores[num_sensores].tipo, tipo, 31);
            strncpy(sensores[num_sensores].ip, ip, INET_ADDRSTRLEN - 1);
            sensores[num_sensores].port = port;
            sensores[num_sensores].activo = 1;
            sensores[num_sensores].ultimo_reporte = time(NULL);
            num_sensores++;
            snprintf(respuesta, MAX_MSG, "OK|sensor registrado");
        } else {
            snprintf(respuesta, MAX_MSG, "ERROR|capacidad maxima alcanzada");
        }
        pthread_mutex_unlock(&mutex);
    }

    // DATA|sensor_id|tipo|valor|unidad
    else if (strcmp(cmd, "DATA") == 0) {
        char *sensor_id = strtok(NULL, "|");
        char *tipo      = strtok(NULL, "|");
        char *valor_str = strtok(NULL, "|");
        char *unidad    = strtok(NULL, "|");
        if (!sensor_id || !tipo || !valor_str || !unidad) {
            snprintf(respuesta, MAX_MSG, "ERROR|faltan campos");
            return;
        }
        double valor = atof(valor_str);
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_sensores; i++) {
            if (strcmp(sensores[i].sensor_id, sensor_id) == 0) {
                sensores[i].valor = valor;
                strncpy(sensores[i].unidad, unidad, 15);
                sensores[i].ultimo_reporte = time(NULL);
                break;
            }
        }
        pthread_mutex_unlock(&mutex);

        // Verificar alerta
        char msg_alerta[MAX_MSG];
        if (verificar_alerta(tipo, valor, msg_alerta)) {
            notificar_operadores(msg_alerta);
            log_evento(ip, port, msg, msg_alerta);
        }
        snprintf(respuesta, MAX_MSG, "OK|dato recibido");
    }

    // LIST|
    else if (strcmp(cmd, "LIST") == 0) {
        char lista[MAX_MSG] = "OK|";
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_sensores; i++) {
            if (sensores[i].activo) {
                char entry[256];
                snprintf(entry, sizeof(entry), "%s:%s:%.2f%s;",
                    sensores[i].sensor_id,
                    sensores[i].tipo,
                    sensores[i].valor,
                    sensores[i].unidad);
                strncat(lista, entry, MAX_MSG - strlen(lista) - 1);
            }
        }
        pthread_mutex_unlock(&mutex);
        strncpy(respuesta, lista, MAX_MSG - 1);
    }

    // STATS|
    else if (strcmp(cmd, "STATS") == 0) {
        pthread_mutex_lock(&mutex);
        int activos = 0;
        for (int i = 0; i < num_sensores; i++)
            if (sensores[i].activo) activos++;
        snprintf(respuesta, MAX_MSG, "OK|sensores:%d|operadores:%d",
            activos, num_operadores);
        pthread_mutex_unlock(&mutex);
    }

    // GET|sensor_id
    else if (strcmp(cmd, "GET") == 0) {
        char *sensor_id = strtok(NULL, "|");
        if (!sensor_id) {
            snprintf(respuesta, MAX_MSG, "ERROR|falta sensor_id");
            return;
        }
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_sensores; i++) {
            if (strcmp(sensores[i].sensor_id, sensor_id) == 0) {
                snprintf(respuesta, MAX_MSG, "OK|%s|%s|%.2f|%s",
                    sensores[i].sensor_id,
                    sensores[i].tipo,
                    sensores[i].valor,
                    sensores[i].unidad);
                pthread_mutex_unlock(&mutex);
                return;
            }
        }
        pthread_mutex_unlock(&mutex);
        snprintf(respuesta, MAX_MSG, "ERROR|sensor no encontrado");
    }

    else {
        snprintf(respuesta, MAX_MSG, "ERROR|comando desconocido");
    }
}

// ─────────────────────────────────────────
// HILO TCP — manejar operador
// ─────────────────────────────────────────
typedef struct {
    int socket;
    struct sockaddr_in addr;
} ClienteArgs;

void *handle_tcp_client(void *arg) {
    ClienteArgs *ca = (ClienteArgs *)arg;
    int sock = ca->socket;
    char ip[INET_ADDRSTRLEN];
    int port = ntohs(ca->addr.sin_port);
    inet_ntop(AF_INET, &ca->addr.sin_addr, ip, INET_ADDRSTRLEN);
    free(ca);

    // Registrar operador
    pthread_mutex_lock(&mutex);
    int op_idx = -1;
    for (int i = 0; i < MAX_OPERATORS; i++) {
        if (!operadores[i].activo) {
            operadores[i].socket = sock;
            strncpy(operadores[i].ip, ip, INET_ADDRSTRLEN - 1);
            operadores[i].port = port;
            operadores[i].activo = 1;
            if (i >= num_operadores) num_operadores = i + 1;
            op_idx = i;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    char msg[MAX_MSG];
    char respuesta[MAX_MSG];

    while (1) {
        memset(msg, 0, MAX_MSG);
        int n = recv(sock, msg, MAX_MSG - 1, 0);
        if (n <= 0) break;

        // Quitar salto de linea
        msg[strcspn(msg, "\n")] = 0;

        procesar_mensaje(msg, respuesta, ip, port);
        strncat(respuesta, "\n", MAX_MSG - strlen(respuesta) - 1);
        send(sock, respuesta, strlen(respuesta), 0);
        log_evento(ip, port, msg, respuesta);
    }

    // Desregistrar operador
    if (op_idx >= 0) {
        pthread_mutex_lock(&mutex);
        operadores[op_idx].activo = 0;
        pthread_mutex_unlock(&mutex);
    }
    close(sock);
    return NULL;
}

// ─────────────────────────────────────────
// HILO UDP — recibir datos de sensores
// ─────────────────────────────────────────
void *udp_server(void *arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(udp_port);

    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    printf("[UDP] Escuchando en puerto %d\n", udp_port);

    char msg[MAX_MSG];
    char respuesta[MAX_MSG];
    struct sockaddr_in cliente;
    socklen_t len = sizeof(cliente);

    while (1) {
        memset(msg, 0, MAX_MSG);
        int n = recvfrom(sock, msg, MAX_MSG - 1, 0,
                         (struct sockaddr *)&cliente, &len);
        if (n <= 0) continue;

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliente.sin_addr, ip, INET_ADDRSTRLEN);
        int port = ntohs(cliente.sin_port);

        msg[strcspn(msg, "\n")] = 0;
        procesar_mensaje(msg, respuesta, ip, port);
        log_evento(ip, port, msg, respuesta);

        sendto(sock, respuesta, strlen(respuesta), 0,
               (struct sockaddr *)&cliente, len);
    }
    return NULL;
}

// ─────────────────────────────────────────
// SERVIDOR HTTP — dashboard básico
// ─────────────────────────────────────────
void *http_server(void *arg) {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);
    bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sock, 5);
    printf("[HTTP] Dashboard en puerto 8080\n");

    while (1) {
        int client = accept(server_sock, NULL, NULL);
        if (client < 0) continue;

        char req[2048];
        recv(client, req, sizeof(req) - 1, 0);

        // Construir JSON de sensores
        char json[MAX_MSG * 4] = "[";
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_sensores; i++) {
            if (sensores[i].activo) {
                char entry[512];
                snprintf(entry, sizeof(entry),
                    "{\"id\":\"%s\",\"tipo\":\"%s\",\"valor\":%.2f,\"unidad\":\"%s\",\"ip\":\"%s\"}%s",
                    sensores[i].sensor_id,
                    sensores[i].tipo,
                    sensores[i].valor,
                    sensores[i].unidad,
                    sensores[i].ip,
                    (i < num_sensores - 1) ? "," : "");
                strncat(json, entry, sizeof(json) - strlen(json) - 1);
            }
        }
        pthread_mutex_unlock(&mutex);
        strncat(json, "]", sizeof(json) - strlen(json) - 1);

        // Detectar si es /api/data
        if (strstr(req, "GET /api/data")) {
            char resp[MAX_MSG * 5];
            snprintf(resp, sizeof(resp),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n\r\n%s", json);
            send(client, resp, strlen(resp), 0);
        } else {
            // HTML dashboard
            char html[8192];
            snprintf(html, sizeof(html),
                "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                "<title>IoT Monitor</title>"
                "<meta http-equiv='refresh' content='5'>"
                "<style>body{font-family:Arial;background:#0f172a;color:#e2e8f0;padding:20px}"
                "h1{color:#38bdf8}.sensor{background:#1e293b;border-radius:8px;padding:15px;"
                "margin:10px 0;border-left:4px solid #38bdf8}"
                ".badge{background:#0ea5e9;padding:2px 8px;border-radius:12px;font-size:11px}"
                "</style></head><body>"
                "<h1>Sistema de Monitoreo IoT</h1>"
                "<p>Actualizacion automatica cada 5 segundos</p>"
                "<div id='content'>Cargando...</div>"
                "<script>"
                "fetch('/api/data').then(r=>r.json()).then(data=>{"
                "let h='';data.forEach(s=>{"
                "h+=`<div class='sensor'><strong>Sensor: <span class='badge'>${s.id}</span></strong>"
                "<br>Tipo: ${s.tipo} | Valor: <b>${s.valor} ${s.unidad}</b> | IP: ${s.ip}</div>`;"
                "});document.getElementById('content').innerHTML=h||'Sin sensores activos';"
                "});</script></body></html>");

            char resp[9000];
            snprintf(resp, sizeof(resp),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n\r\n%s", html);
            send(client, resp, strlen(resp), 0);
        }
        close(client);
    }
    return NULL;
}

// ─────────────────────────────────────────
// TCP SERVER — aceptar operadores
// ─────────────────────────────────────────
void *tcp_server(void *arg) {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(tcp_port);
    bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sock, 10);
    printf("[TCP] Escuchando en puerto %d\n", tcp_port);

    while (1) {
        ClienteArgs *ca = malloc(sizeof(ClienteArgs));
        socklen_t len = sizeof(ca->addr);
        ca->socket = accept(server_sock, (struct sockaddr *)&ca->addr, &len);
        if (ca->socket < 0) { free(ca); continue; }

        pthread_t t;
        pthread_create(&t, NULL, handle_tcp_client, ca);
        pthread_detach(t);
    }
    return NULL;
}

// ─────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: ./server <puerto> <archivoDeLogs>\n");
        return 1;
    }

    tcp_port = atoi(argv[1]);
    udp_port = tcp_port - 1;  // UDP en puerto anterior al TCP

    log_file = fopen(argv[2], "a");
    if (!log_file) {
        perror("No se pudo abrir archivo de logs");
        return 1;
    }

    printf("=== Servidor IoT iniciado ===\n");
    printf("TCP puerto: %d\n", tcp_port);
    printf("UDP puerto: %d\n", udp_port);
    printf("Logs en: %s\n\n", argv[2]);

    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, udp_server, NULL);
    pthread_create(&t2, NULL, tcp_server, NULL);
    pthread_create(&t3, NULL, http_server, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    fclose(log_file);
    return 0;
}
