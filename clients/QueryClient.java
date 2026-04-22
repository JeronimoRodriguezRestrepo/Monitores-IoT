import java.io.*;
import java.net.*;

public class QueryClient {

    static String resolverHostname(String hostname) throws UnknownHostException {
        InetAddress addr = InetAddress.getByName(hostname);
        System.out.println("[DNS] " + hostname + " → " + addr.getHostAddress());
        return addr.getHostAddress();
    }

    static String enviarComando(PrintWriter out, BufferedReader in, String comando) throws IOException {
        out.println(comando);
        return in.readLine();
    }

    public static void main(String[] args) throws Exception {
        String hostname = args.length > 0 ? args[0] : "server.iot-monitor.local";
        int port = 9001;

        String ip = resolverHostname(hostname);

        Socket socket = new Socket(ip, port);
        PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
        BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        BufferedReader consola = new BufferedReader(new InputStreamReader(System.in));

        System.out.println("Conectado a " + hostname + ":" + port);

        while (true) {
            System.out.println("\nComandos:");
            System.out.println("  1) LIST  — sensores activos");
            System.out.println("  2) GET   — datos de un sensor");
            System.out.println("  3) STATS — estadísticas");
            System.out.println("  4) Salir");
            System.out.print("\nOpción: ");

            String opcion = consola.readLine().trim();

            if (opcion.equals("1")) {
                String resp = enviarComando(out, in, "LIST|");
                System.out.println("Respuesta: " + resp);

            } else if (opcion.equals("2")) {
                System.out.print("ID del sensor: ");
                String sid = consola.readLine().trim();
                String resp = enviarComando(out, in, "GET|" + sid);
                System.out.println("Respuesta: " + resp);

            } else if (opcion.equals("3")) {
                String resp = enviarComando(out, in, "STATS|");
                System.out.println("Respuesta: " + resp);

            } else if (opcion.equals("4")) {
                break;
            }
        }

        socket.close();
    }
}
