/*
Autor: Agustin Baez
Carrera: Ing. informatica
Año: 2026.
Programa: creacion de juego basado en "Gorilla.bash"
*/




#include <winsock2.h> // Permite conectar dos computadoras por TCP/IP.
#include <ws2tcpip.h> // Permite convertir direcciones IP escritas por el usuario.
#include <windows.h> // Permite crear ventanas, botones, campos de texto, timers y dibujar usando GDI.
#include <cmath>     // Permite usar sin, cos, sqrt y round para la trayectoria parabolica.
#include <ctime>     // Permite usar time(), util para generar valores aleatorios distintos.
#include <cstdlib>   // Permite usar srand() y rand() para variar alturas y estrellas.
#include <iomanip>   // Permite enviar textos con espacios dentro de los mensajes de red.
#include <sstream>   // Permite armar y leer mensajes de red.
#include <string>    // Permite usar string para nombres y mensajes del juego.
#include <vector>    // Permite usar vector para guardar la lista de edificios .

using namespace std;

#pragma comment(lib, "ws2_32.lib")

// -----------------------------
// Constantes generales del juego
// -----------------------------

const int ANCHO_VENTANA = 1000;
const int ALTO_VENTANA = 700;
const int ALTO_PANEL = 120;
const int ANCHO_MUNDO = ANCHO_VENTANA;
const int ALTO_MUNDO = ALTO_VENTANA - ALTO_PANEL;
// CAMBIO MANUAL 2 -edificios 
const int CANTIDAD_EDIFICIOS = 21;//antes 18
//Cambio manual 3: Regla del juego
const int PUNTOS_PARA_GANAR = 4; // Mejor de 4 

const double PI = 3.14159265358979323846;

// Gravedad ajustada para que el juego sea divertido, no para simular fisica real.
// Con un valor mas bajo hay arcos mas largos y es posible acertar a distancia.

//Cambio manual 1- Gravedad
const double GRAVEDAD = 9.8; //se cambio a 9.8

// Identificadores de controles.
// Windows los usa para reconocer que boton o campo genero un evento.
const int ID_ANGULO = 101;
const int ID_VELOCIDAD = 102;
const int ID_DISPARAR = 103;
const int ID_REINICIAR = 104;
const int ID_IP = 105;
const int ID_HOST = 106;
const int ID_UNIRSE = 107;
const int ID_SOLITARIO = 108;
const int ID_MULTIJUGADOR = 109;
const int ID_VOLVER_MENU = 110;
const int ID_PUERTO = 111;
const int ID_NOMBRE = 112;
const int ID_DESCONECTAR = 113;
const int ID_TIMER = 200;
const UINT WM_RED_MENSAJE = WM_APP + 1;
const int PUERTO_RED_DEFECTO = 8080;

// -----------------------------
// Estructuras de datos
// -----------------------------

struct Edificio {
    int x;
    int ancho;
    int alto;
    COLORREF color;
};

struct Jugador {
    string nombre;
    int x;
    int y;
    int victorias;
    COLORREF color;
};

struct Proyectil {
    double x;
    double y;
    double xInicial;
    double yInicial;
    double vx;
    double vy;
    double tiempo;
    bool activo;
};

struct Crater {
    int x;
    int y;
    int radio;
};

// -----------------------------
// Variables globales
// -----------------------------

HWND ventanaPrincipal;
HWND campoAngulo;
HWND campoVelocidad;
HWND botonDisparar;
HWND botonReiniciar;
HWND campoIP;
HWND botonHost;
HWND botonUnirse;
HWND botonSolitario;
HWND botonMultijugador;
HWND botonVolverMenu;
HWND campoPuerto;
HWND campoNombre;
HWND botonDesconectar;

vector<Edificio> edificios;
vector<Crater> crateres;
Jugador jugador1;
Jugador jugador2;
Proyectil proyectil;


int turnoActual = 1;
bool rondaTerminada = false;
bool partidaTerminada = false;
string mensajeEstado = "Turno del Jugador 1";
string ganadorPartida = "";
string mensajeMenuMultijugador = "";

enum ModoRed {
    RED_LOCAL,
    RED_HOST,
    RED_CLIENTE
};

enum PantallaActual {
    PANTALLA_MENU,
    PANTALLA_MENU_MULTIJUGADOR,
    PANTALLA_JUEGO
};

ModoRed modoRed = RED_LOCAL;
PantallaActual pantallaActual = PANTALLA_MENU;
SOCKET socketRed = INVALID_SOCKET;
SOCKET socketEscucha = INVALID_SOCKET;
HANDLE hiloRed = NULL;
bool redConectada = false;
bool esperandoConexion = false;
bool winsockIniciado = false;
bool procesandoDisparoRemoto = false;
int puertoRed = PUERTO_RED_DEFECTO;
CRITICAL_SECTION seccionEnvioRed;

// -----------------------------
// Prototipado de funciones
// -----------------------------


double gradosARadianes(double grados);
wstring convertirAWstring(const string& texto);
void dibujarTexto(HDC hdc, int x, int y, const string& texto);
double leerNumero(HWND campo, double valorPorDefecto);
double leerNumeroLimitado(HWND campo, double valorPorDefecto, double minimo, double maximo);
int obtenerTechoEnX(int x);
void enviarEstadoRed();
void enviarMensajeRed(const string& mensaje);
void procesarMensajeRed(const string& mensaje);
string crearMensajeEstado();
void aplicarMensajeEstado(const string& mensaje);
void iniciarHostRed();
void iniciarClienteRed();
void cerrarRed();
bool esTurnoLocal();
int leerPuertoRed();
string obtenerIPsLocales();
void mostrarMenuInicio();
void mostrarMenuMultijugador();
void mostrarPantallaJuego();
void iniciarModoSolitario();
void iniciarModoHost();
void iniciarModoCliente();
void desconectarYVolverAlMenu();
DWORD WINAPI hiloHostRed(LPVOID parametro);
DWORD WINAPI hiloClienteRed(LPVOID parametro);
DWORD WINAPI hiloRecepcionRed(LPVOID parametro);

void generarEdificios();
void suavizarEdificiosCercanos(int indiceJugador, bool miraHaciaDerecha);
void colocarJugadores();
void iniciarNuevaRonda();
void iniciarNuevaPartida();

bool puntoDentroCrater(int x, int y);
void crearCrater(int x, int y);
bool impactoContraJugador(const Jugador& jugador, int x, int y);
bool impactoContraEdificio(int x, int y);
void terminarPartida(const string& ganador);
void terminarRonda(int ganador);

void cambiarTurno();
void disparar();
void actualizarProyectil();

void dibujarCieloNocturno(HDC hdc);
void dibujarJugador(HDC hdc, const Jugador& jugador);
void dibujarEdificios(HDC hdc);
void dibujarCrateres(HDC hdc);
void dibujarProyectil(HDC hdc);
void dibujarPanel(HDC hdc);
void dibujarGameOver(HDC hdc);
void dibujarEscenario(HDC hdc);
void dibujarMenu(HDC hdc);

LRESULT CALLBACK ProcedimientoVentana(HWND hwnd, UINT mensaje, WPARAM wParam, LPARAM lParam);

// -----------------------------
// Funciones auxiliares
// -----------------------------

double gradosARadianes(double grados) {
    return grados * PI / 180.0;
}

wstring convertirAWstring(const string& texto) {
    return wstring(texto.begin(), texto.end());
}

void dibujarTexto(HDC hdc, int x, int y, const string& texto) {
    wstring textoW = convertirAWstring(texto);
    TextOutW(hdc, x, y, textoW.c_str(), static_cast<int>(textoW.length()));
}

double leerNumero(HWND campo, double valorPorDefecto) {
    char buffer[64];
    GetWindowTextA(campo, buffer, 64);

    try {
        return stod(buffer);
    } catch (...) {
        return valorPorDefecto;
    }
}

double leerNumeroLimitado(HWND campo, double valorPorDefecto, double minimo, double maximo) {
    double valor = leerNumero(campo, valorPorDefecto);

    if (valor < minimo) {
        valor = minimo;
    }

    if (valor > maximo) {
        valor = maximo;
    }

    string texto = to_string(static_cast<int>(valor));
    SetWindowTextA(campo, texto.c_str());

    return valor;
}

int obtenerTechoEnX(int x) {
    for (const Edificio& edificio : edificios) {
        bool dentroDelEdificio = x >= edificio.x && x <= edificio.x + edificio.ancho;

        if (dentroDelEdificio) {
            return ALTO_MUNDO - edificio.alto;
        }
    }

    return ALTO_MUNDO;
}

bool esTurnoLocal() {
    if (modoRed == RED_LOCAL) {
        return true;
    }

    return (modoRed == RED_HOST && turnoActual == 1) ||
           (modoRed == RED_CLIENTE && turnoActual == 2);
}

int leerPuertoRed() {
    char buffer[32];
    GetWindowTextA(campoPuerto, buffer, 32);

    int puerto = PUERTO_RED_DEFECTO;

    try {
        puerto = stoi(buffer);
    } catch (...) {
        puerto = PUERTO_RED_DEFECTO;
    }

    if (puerto < 1024 || puerto > 65535) {
        puerto = PUERTO_RED_DEFECTO;
    }

    SetWindowTextA(campoPuerto, to_string(puerto).c_str());
    return puerto;
}

string obtenerIPsLocales() {
    char nombreEquipo[256];
    string resultado = "IP local del servidor: ";
    bool encontroIP = false;

    if (gethostname(nombreEquipo, sizeof(nombreEquipo)) == 0) {
        hostent* host = gethostbyname(nombreEquipo);

        if (host != NULL) {
            for (int i = 0; host->h_addr_list[i] != NULL; i++) {
                in_addr direccion;
                memcpy(&direccion, host->h_addr_list[i], sizeof(in_addr));
                string ip = inet_ntoa(direccion);

                if (ip.rfind("127.", 0) == 0) {
                    continue;
                }

                if (encontroIP) {
                    resultado += " / ";
                }

                resultado += ip;
                encontroIP = true;
            }
        }
    }

    if (!encontroIP) {
        resultado += "no detectada";
    }

    return resultado;
}

void actualizarControlesPorTurno() {
    if (pantallaActual != PANTALLA_JUEGO) {
        EnableWindow(botonDisparar, FALSE);
        return;
    }

    bool redPermiteJugar = (modoRed == RED_LOCAL) || redConectada;
    bool puedeDisparar = !rondaTerminada &&
                         !partidaTerminada &&
                         !proyectil.activo &&
                         redPermiteJugar &&
                         esTurnoLocal();

    EnableWindow(botonDisparar, puedeDisparar ? TRUE : FALSE);
}

void mostrarMenuInicio() {
    pantallaActual = PANTALLA_MENU;
    modoRed = RED_LOCAL;
    mensajeMenuMultijugador = "";
    esperandoConexion = false;
    redConectada = false;

    ShowWindow(campoAngulo, SW_HIDE);
    ShowWindow(campoVelocidad, SW_HIDE);
    ShowWindow(botonDisparar, SW_HIDE);
    ShowWindow(botonReiniciar, SW_HIDE);
    ShowWindow(campoIP, SW_HIDE);
    ShowWindow(campoPuerto, SW_HIDE);
    ShowWindow(campoNombre, SW_HIDE);
    ShowWindow(botonHost, SW_HIDE);
    ShowWindow(botonUnirse, SW_HIDE);
    ShowWindow(botonVolverMenu, SW_HIDE);
    ShowWindow(botonDesconectar, SW_HIDE);

    ShowWindow(botonSolitario, SW_SHOW);
    ShowWindow(botonMultijugador, SW_SHOW);
    EnableWindow(botonSolitario, TRUE);
    EnableWindow(botonMultijugador, TRUE);
    EnableWindow(campoNombre, TRUE);
    EnableWindow(campoIP, TRUE);
    EnableWindow(botonHost, TRUE);
    EnableWindow(botonUnirse, TRUE);

    InvalidateRect(ventanaPrincipal, NULL, TRUE);
}

void mostrarMenuMultijugador() {
    pantallaActual = PANTALLA_MENU_MULTIJUGADOR;

    ShowWindow(botonSolitario, SW_HIDE);
    ShowWindow(botonMultijugador, SW_HIDE);
    ShowWindow(campoAngulo, SW_HIDE);
    ShowWindow(campoVelocidad, SW_HIDE);
    ShowWindow(botonDisparar, SW_HIDE);
    ShowWindow(botonReiniciar, SW_HIDE);

    MoveWindow(campoNombre, 395, 295, 210, 28, TRUE);
    MoveWindow(campoIP, 395, 365, 210, 28, TRUE);
    MoveWindow(botonHost, 330, 440, 160, 38, TRUE);
    MoveWindow(botonUnirse, 520, 440, 150, 38, TRUE);
    MoveWindow(botonVolverMenu, 430, 505, 140, 34, TRUE);

    ShowWindow(campoIP, SW_SHOW);
    ShowWindow(campoPuerto, SW_HIDE);
    ShowWindow(campoNombre, SW_SHOW);
    ShowWindow(botonHost, SW_SHOW);
    ShowWindow(botonUnirse, SW_SHOW);
    ShowWindow(botonVolverMenu, SW_SHOW);
    ShowWindow(botonDesconectar, SW_HIDE);
    bool esperando = esperandoConexion || redConectada;
    EnableWindow(botonHost, esperando ? FALSE : TRUE);
    EnableWindow(botonUnirse, esperando ? FALSE : TRUE);
    EnableWindow(botonVolverMenu, TRUE);
    EnableWindow(campoNombre, esperando ? FALSE : TRUE);
    EnableWindow(campoIP, esperando ? FALSE : TRUE);

    InvalidateRect(ventanaPrincipal, NULL, TRUE);
}

void mostrarPantallaJuego() {
    pantallaActual = PANTALLA_JUEGO;

    ShowWindow(botonSolitario, SW_HIDE);
    ShowWindow(botonMultijugador, SW_HIDE);
    ShowWindow(botonVolverMenu, SW_HIDE);

    MoveWindow(botonDesconectar, 820, ALTO_MUNDO + 70, 145, 34, TRUE);

    ShowWindow(campoAngulo, SW_SHOW);
    ShowWindow(campoVelocidad, SW_SHOW);
    ShowWindow(botonDisparar, SW_SHOW);
    ShowWindow(botonReiniciar, SW_SHOW);

    bool esMultijugador = modoRed != RED_LOCAL;
    ShowWindow(campoIP, SW_HIDE);
    ShowWindow(campoPuerto, SW_HIDE);
    ShowWindow(campoNombre, SW_HIDE);
    ShowWindow(botonHost, SW_HIDE);
    ShowWindow(botonUnirse, SW_HIDE);
    ShowWindow(botonDesconectar, esMultijugador ? SW_SHOW : SW_HIDE);
    EnableWindow(botonHost, FALSE);
    EnableWindow(botonUnirse, FALSE);
    EnableWindow(botonDesconectar, esMultijugador ? TRUE : FALSE);

    actualizarControlesPorTurno();
    InvalidateRect(ventanaPrincipal, NULL, TRUE);
}

void iniciarModoSolitario() {
    cerrarRed();
    modoRed = RED_LOCAL;
    jugador1.nombre = "Jugador 1";
    jugador2.nombre = "Jugador 2";
    iniciarNuevaPartida();
    mostrarPantallaJuego();
}

void iniciarModoHost() {
    char bufferNombre[64];
    GetWindowTextA(campoNombre, bufferNombre, 64);
    string nombre = bufferNombre;

    if (nombre.empty()) {
        nombre = "Jugador 1";
    }

    modoRed = RED_HOST;
    jugador1.nombre = nombre;
    jugador2.nombre = "Jugador 2";
    mensajeMenuMultijugador = "Creando servidor...";
    mostrarMenuMultijugador();
    iniciarHostRed();
}

void iniciarModoCliente() {
    char bufferNombre[64];
    GetWindowTextA(campoNombre, bufferNombre, 64);
    string nombre = bufferNombre;

    if (nombre.empty()) {
        nombre = "Jugador 2";
    }

    modoRed = RED_CLIENTE;
    jugador1.nombre = "Jugador 1";
    jugador2.nombre = nombre;
    mensajeMenuMultijugador = "Conectando al servidor...";
    mostrarMenuMultijugador();
    iniciarClienteRed();
}

void desconectarYVolverAlMenu() {
    cerrarRed();
    modoRed = RED_LOCAL;
    mensajeEstado = "Conexion cerrada.";
    mostrarMenuInicio();
}

bool iniciarWinsockSiHaceFalta() {
    if (winsockIniciado) {
        return true;
    }

    WSADATA datos;
    if (WSAStartup(MAKEWORD(2, 2), &datos) != 0) {
        MessageBoxA(ventanaPrincipal, "No se pudo iniciar Winsock.", "Error de red", MB_OK | MB_ICONERROR);
        return false;
    }

    winsockIniciado = true;
    return true;
}

void enviarMensajeRed(const string& mensaje) {
    if (!redConectada || socketRed == INVALID_SOCKET) {
        return;
    }

    string linea = mensaje + "\n";
    const char* datos = linea.c_str();
    int enviados = 0;
    int total = static_cast<int>(linea.size());

    EnterCriticalSection(&seccionEnvioRed);

    while (enviados < total) {
        int resultado = send(socketRed, datos + enviados, total - enviados, 0);

        if (resultado == SOCKET_ERROR || resultado == 0) {
            break;
        }

        enviados += resultado;
    }

    LeaveCriticalSection(&seccionEnvioRed);
}

string crearMensajeEstado() {
    ostringstream salida;

    salida << "STATE "
           << turnoActual << ' '
           << rondaTerminada << ' '
           << partidaTerminada << ' '
           << quoted(mensajeEstado) << ' '
           << quoted(ganadorPartida) << ' '
           << quoted(jugador1.nombre) << ' '
           << quoted(jugador2.nombre) << ' '
           << jugador1.victorias << ' '
           << jugador2.victorias << ' '
           << proyectil.activo << ' '
           << proyectil.x << ' '
           << proyectil.y << ' '
           << proyectil.xInicial << ' '
           << proyectil.yInicial << ' '
           << proyectil.vx << ' '
           << proyectil.vy << ' '
           << proyectil.tiempo << ' '
           << edificios.size();

    for (const Edificio& edificio : edificios) {
        salida << ' ' << edificio.x
               << ' ' << edificio.ancho
               << ' ' << edificio.alto
               << ' ' << static_cast<unsigned long>(edificio.color);
    }

    salida << ' ' << crateres.size();

    for (const Crater& crater : crateres) {
        salida << ' ' << crater.x
               << ' ' << crater.y
               << ' ' << crater.radio;
    }

    return salida.str();
}

void aplicarMensajeEstado(const string& mensaje) {
    istringstream entrada(mensaje);
    string tipo;
    size_t cantidadEdificios = 0;
    size_t cantidadCrateres = 0;
    unsigned long color = 0;

    entrada >> tipo;

    if (tipo != "STATE") {
        return;
    }

    entrada >> turnoActual
            >> rondaTerminada
            >> partidaTerminada
            >> quoted(mensajeEstado)
            >> quoted(ganadorPartida)
            >> quoted(jugador1.nombre)
            >> quoted(jugador2.nombre)
            >> jugador1.victorias
            >> jugador2.victorias
            >> proyectil.activo
            >> proyectil.x
            >> proyectil.y
            >> proyectil.xInicial
            >> proyectil.yInicial
            >> proyectil.vx
            >> proyectil.vy
            >> proyectil.tiempo
            >> cantidadEdificios;

    if (entrada.fail()) {
        return;
    }

    edificios.clear();

    for (size_t i = 0; i < cantidadEdificios; i++) {
        Edificio edificio;
        entrada >> edificio.x >> edificio.ancho >> edificio.alto >> color;
        edificio.color = static_cast<COLORREF>(color);
        edificios.push_back(edificio);
    }

    entrada >> cantidadCrateres;
    crateres.clear();

    for (size_t i = 0; i < cantidadCrateres; i++) {
        Crater crater;
        entrada >> crater.x >> crater.y >> crater.radio;
        crateres.push_back(crater);
    }

    colocarJugadores();
    actualizarControlesPorTurno();
    InvalidateRect(ventanaPrincipal, NULL, TRUE);
}

void enviarEstadoRed() {
    if (modoRed == RED_HOST && redConectada) {
        enviarMensajeRed(crearMensajeEstado());
    }
}

void procesarMensajeRed(const string& mensaje) {
    if (mensaje.rfind("STATE ", 0) == 0) {
        if (modoRed == RED_CLIENTE && pantallaActual != PANTALLA_JUEGO) {
            mostrarPantallaJuego();
        }

        aplicarMensajeEstado(mensaje);
        return;
    }

    if (mensaje.rfind("NAME|", 0) == 0 && modoRed == RED_HOST) {
        string nombre = mensaje.substr(5);

        if (!nombre.empty()) {
            jugador2.nombre = nombre;
            iniciarNuevaPartida();
            mostrarPantallaJuego();
            mensajeEstado = nombre + " se conecto. Turno de " + jugador1.nombre + ".";
            enviarEstadoRed();
            InvalidateRect(ventanaPrincipal, NULL, TRUE);
        }

        return;
    }

    if (mensaje.rfind("SHOOT ", 0) == 0 && modoRed == RED_HOST && turnoActual == 2 && !proyectil.activo) {
        istringstream entrada(mensaje);
        string tipo;
        double angulo;
        double velocidad;

        entrada >> tipo >> angulo >> velocidad;

        if (!entrada.fail()) {
            SetWindowTextA(campoAngulo, to_string(static_cast<int>(angulo)).c_str());
            SetWindowTextA(campoVelocidad, to_string(static_cast<int>(velocidad)).c_str());
            procesandoDisparoRemoto = true;
            disparar();
            procesandoDisparoRemoto = false;
        }

        return;
    }

    if (mensaje == "RESTART" && modoRed == RED_HOST) {
        if (partidaTerminada) {
            iniciarNuevaPartida();
        } else {
            iniciarNuevaRonda();
        }

        enviarEstadoRed();
    }
}

DWORD WINAPI hiloRecepcionRed(LPVOID parametro) {
    SOCKET socketConexion = reinterpret_cast<SOCKET>(parametro);
    string acumulado;
    char buffer[512];

    while (true) {
        int recibidos = recv(socketConexion, buffer, sizeof(buffer), 0);

        if (recibidos <= 0) {
            break;
        }

        acumulado.append(buffer, recibidos);

        size_t posicionSalto = string::npos;
        while ((posicionSalto = acumulado.find('\n')) != string::npos) {
            string linea = acumulado.substr(0, posicionSalto);
            acumulado.erase(0, posicionSalto + 1);

            string* mensaje = new string(linea);
            PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(mensaje));
        }
    }

    string* mensaje = new string("DISCONNECT");
    PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(mensaje));
    return 0;
}

DWORD WINAPI hiloHostRed(LPVOID parametro) {
    SOCKET escucha = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (escucha == INVALID_SOCKET) {
        PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(new string("NET_ERROR")));
        return 0;
    }

    sockaddr_in direccion = {};
    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = INADDR_ANY;
    direccion.sin_port = htons(static_cast<u_short>(puertoRed));

    BOOL reutilizar = TRUE;
    setsockopt(escucha, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reutilizar), sizeof(reutilizar));

    if (bind(escucha, reinterpret_cast<sockaddr*>(&direccion), sizeof(direccion)) == SOCKET_ERROR ||
        listen(escucha, 1) == SOCKET_ERROR) {
        closesocket(escucha);
        PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(new string("NET_ERROR")));
        return 0;
    }

    socketEscucha = escucha;
    SOCKET cliente = accept(escucha, NULL, NULL);
    closesocket(escucha);
    socketEscucha = INVALID_SOCKET;

    if (cliente == INVALID_SOCKET) {
        PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(new string("NET_ERROR")));
        return 0;
    }

    socketRed = cliente;
    PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(new string("CONNECTED")));
    hiloRecepcionRed(reinterpret_cast<LPVOID>(cliente));
    return 0;
}

DWORD WINAPI hiloClienteRed(LPVOID parametro) {
    string direccionHost = *reinterpret_cast<string*>(parametro);
    delete reinterpret_cast<string*>(parametro);

    SOCKET conexion = socket(AF_INET, SOCK_STREAM, 0);

    if (conexion == INVALID_SOCKET) {
        PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(new string("NET_ERROR")));
        return 0;
    }

    sockaddr_in servidor = {};
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(static_cast<u_short>(puertoRed));
    servidor.sin_addr.s_addr = inet_addr(direccionHost.c_str());

    if (servidor.sin_addr.s_addr == INADDR_NONE) {
        closesocket(conexion);
        PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(new string("NET_ERROR")));
        return 0;
    }

    if (connect(conexion, reinterpret_cast<sockaddr*>(&servidor), sizeof(servidor)) == SOCKET_ERROR) {
        closesocket(conexion);
        PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(new string("NET_ERROR")));
        return 0;
    }

    socketRed = conexion;
    PostMessage(ventanaPrincipal, WM_RED_MENSAJE, 0, reinterpret_cast<LPARAM>(new string("CONNECTED")));
    hiloRecepcionRed(reinterpret_cast<LPVOID>(conexion));
    return 0;
}

void iniciarHostRed() {
    if (!iniciarWinsockSiHaceFalta() || esperandoConexion || redConectada) {
        return;
    }

    puertoRed = leerPuertoRed();
    modoRed = RED_HOST;
    esperandoConexion = true;
    mensajeMenuMultijugador = obtenerIPsLocales() + "  |  Puerto: " + to_string(puertoRed) +
                               ". Esperando al otro jugador...";
    EnableWindow(botonHost, FALSE);
    EnableWindow(botonUnirse, FALSE);
    EnableWindow(campoNombre, FALSE);
    EnableWindow(campoIP, FALSE);
    actualizarControlesPorTurno();
    InvalidateRect(ventanaPrincipal, NULL, TRUE);

    hiloRed = CreateThread(NULL, 0, hiloHostRed, NULL, 0, NULL);
}

void iniciarClienteRed() {
    if (!iniciarWinsockSiHaceFalta() || esperandoConexion || redConectada) {
        return;
    }

    char bufferIP[64];
    GetWindowTextA(campoIP, bufferIP, 64);

    puertoRed = leerPuertoRed();
    modoRed = RED_CLIENTE;
    esperandoConexion = true;
    mensajeMenuMultijugador = "Conectando al servidor...";
    EnableWindow(botonHost, FALSE);
    EnableWindow(botonUnirse, FALSE);
    EnableWindow(campoNombre, FALSE);
    EnableWindow(campoIP, FALSE);
    actualizarControlesPorTurno();
    InvalidateRect(ventanaPrincipal, NULL, TRUE);

    hiloRed = CreateThread(NULL, 0, hiloClienteRed, new string(bufferIP), 0, NULL);
}

void cerrarRed() {
    redConectada = false;
    esperandoConexion = false;

    if (socketEscucha != INVALID_SOCKET) {
        closesocket(socketEscucha);
        socketEscucha = INVALID_SOCKET;
    }

    if (socketRed != INVALID_SOCKET) {
        closesocket(socketRed);
        socketRed = INVALID_SOCKET;
    }

    if (winsockIniciado) {
        WSACleanup();
        winsockIniciado = false;
    }
}

// -----------------------------
// Generacion del escenario
// -----------------------------

void generarEdificios() {
    edificios.clear();
    
    int anchoEdificio = (ANCHO_MUNDO / CANTIDAD_EDIFICIOS) + 5; // CAMBIO MANUAL 2 - edificios más separados visualmente

    int alturasBase[CANTIDAD_EDIFICIOS] = {
        95, 250, 275, 245, 150, 165, 220, 305, 335,
        210, 300, 355, 230, 265, 345, 285, 240, 185
    };

    COLORREF colores[CANTIDAD_EDIFICIOS] = {
        RGB(0, 120, 145), RGB(145, 145, 155), RGB(155, 25, 30),
        RGB(170, 35, 35), RGB(130, 45, 35), RGB(165, 165, 175),
        RGB(140, 45, 35), RGB(140, 140, 150), RGB(0, 125, 145),
        RGB(160, 160, 170), RGB(145, 145, 155), RGB(0, 120, 145),
        RGB(160, 160, 170), RGB(140, 45, 35), RGB(0, 125, 145),
        RGB(145, 145, 155), RGB(165, 165, 175), RGB(140, 45, 35)
    };

    for (int i = 0; i < CANTIDAD_EDIFICIOS; i++) {
        Edificio edificio;
        edificio.x = i * anchoEdificio;
        edificio.ancho = anchoEdificio + 1;
        // CAMBIO MANUAL 2 - alturas más estables
        edificio.alto = alturasBase[i] + (rand() % 15 - 7);//antes 31-15
       
        edificio.color = RGB(90, 90, 120); // CAMBIO MANUAL 5 - edificios con estilo uniforme
        edificios.push_back(edificio);
    }
}

void suavizarEdificiosCercanos(int indiceJugador, bool miraHaciaDerecha) {
    int alturaBase = edificios[indiceJugador].alto;
    int direccion = miraHaciaDerecha ? 1 : -1;

    for (int distancia = 1; distancia <= 2; distancia++) {
        int indiceVecino = indiceJugador + direccion * distancia;

        if (indiceVecino < 0 || indiceVecino >= CANTIDAD_EDIFICIOS) {
            continue;
        }
        // CAMBIO MANUAL 2 - terreno menos agresivo
        int alturaMaximaPermitida = alturaBase + 10 + distancia * 12;

        if (edificios[indiceVecino].alto > alturaMaximaPermitida) {
            edificios[indiceVecino].alto = alturaMaximaPermitida;
        }
    }
}

void colocarJugadores() {
    jugador1.x = edificios[3].x + edificios[3].ancho / 2;
    jugador1.y = obtenerTechoEnX(jugador1.x) - 22;

    int indiceDerecha = CANTIDAD_EDIFICIOS - 4;
    jugador2.x = edificios[indiceDerecha].x + edificios[indiceDerecha].ancho / 2;
    jugador2.y = obtenerTechoEnX(jugador2.x) - 22;
}

void iniciarNuevaRonda() {
    generarEdificios();
    crateres.clear();

    suavizarEdificiosCercanos(3, true);
    suavizarEdificiosCercanos(CANTIDAD_EDIFICIOS - 4, false);

    colocarJugadores();

    proyectil.activo = false;
    proyectil.tiempo = 0.0;
    turnoActual = 1;
    rondaTerminada = false;
    mensajeEstado = "Turno de " + jugador1.nombre;

    actualizarControlesPorTurno();
    SetWindowTextA(botonReiniciar, "Reiniciar ronda");

    if (ventanaPrincipal != NULL) {
        InvalidateRect(ventanaPrincipal, NULL, TRUE);
    }

    enviarEstadoRed();
}

void iniciarNuevaPartida() {
    jugador1.victorias = 0;
    jugador2.victorias = 0;
    partidaTerminada = false;
    ganadorPartida = "";
    iniciarNuevaRonda();
}

// -----------------------------
// Deteccion de impactos
// -----------------------------

bool puntoDentroCrater(int x, int y) {
    for (const Crater& crater : crateres) {
        int dx = x - crater.x;
        int dy = y - crater.y;

        if (dx * dx + dy * dy <= crater.radio * crater.radio) {
            return true;
        }
    }

    return false;
}

void crearCrater(int x, int y) {
    Crater crater;
    crater.x = x;
    crater.y = y;
    crater.radio = 18;
    crateres.push_back(crater);
}

bool impactoContraJugador(const Jugador& jugador, int x, int y) {
    RECT cajaJugador;
    cajaJugador.left = jugador.x - 31;
    cajaJugador.right = jugador.x + 31;
    cajaJugador.top = jugador.y - 42;
    cajaJugador.bottom = jugador.y + 22;

    return x >= cajaJugador.left && x <= cajaJugador.right &&
           y >= cajaJugador.top && y <= cajaJugador.bottom;
}

bool impactoContraEdificio(int x, int y) {
    if (puntoDentroCrater(x, y)) {
        return false;
    }

    for (const Edificio& edificio : edificios) {
        bool dentroHorizontal = x >= edificio.x && x <= edificio.x + edificio.ancho;
        bool dentroVertical = y >= ALTO_MUNDO - edificio.alto && y <= ALTO_MUNDO;

        if (dentroHorizontal && dentroVertical) {
            return true;
        }
    }

    return false; 
}

void terminarPartida(const string& ganador) {
    partidaTerminada = true;
    rondaTerminada = true;
    proyectil.activo = false;
    ganadorPartida = ganador;
    mensajeEstado = "Game Over: gano " + ganador + " al mejor de tres.";

    KillTimer(ventanaPrincipal, ID_TIMER);
    actualizarControlesPorTurno();
    SetWindowTextA(botonReiniciar, "Nueva partida");
    InvalidateRect(ventanaPrincipal, NULL, TRUE);
    enviarEstadoRed();
}

void terminarRonda(int ganador) {
    rondaTerminada = true;
    proyectil.activo = false;
    KillTimer(ventanaPrincipal, ID_TIMER);

    if (ganador == 1) {
        jugador1.victorias++;

        if (jugador1.victorias >= PUNTOS_PARA_GANAR) {
            terminarPartida(jugador1.nombre);
            return;
        }

        mensajeEstado = "Gana la ronda " + jugador1.nombre + ". Presione Reiniciar ronda.";
    } else {
        jugador2.victorias++;

        if (jugador2.victorias >= PUNTOS_PARA_GANAR) {
            terminarPartida(jugador2.nombre);
            return;
        }

        mensajeEstado = "Gana la ronda " + jugador2.nombre + ". Presione Reiniciar ronda.";
    }

    actualizarControlesPorTurno();
    InvalidateRect(ventanaPrincipal, NULL, TRUE);
    enviarEstadoRed();
}

// -----------------------------
// Disparo y fisica
// -----------------------------

void cambiarTurno() {
    turnoActual = (turnoActual == 1) ? 2 : 1;
    mensajeEstado = (turnoActual == 1) ? "Turno de " + jugador1.nombre : "Turno de " + jugador2.nombre;
    actualizarControlesPorTurno();
    enviarEstadoRed();
}

void disparar() {
    if (rondaTerminada || partidaTerminada || proyectil.activo) {
        return;
    }

    double angulo = leerNumeroLimitado(campoAngulo, 45.0, 0.0, 90.0);
    // CAMBIO MANUAL 1 - VELOCIDAD INICIAL AJUSTADA
    double velocidad = leerNumeroLimitado(campoVelocidad, 110.0, 10.0, 220.0); //antes 90.0, 10.0, 220.0

    if (!procesandoDisparoRemoto && !esTurnoLocal()) {
        return;
    }

    if (!procesandoDisparoRemoto && modoRed == RED_CLIENTE) {
        ostringstream salida;
        salida << "SHOOT " << angulo << ' ' << velocidad;
        enviarMensajeRed(salida.str());
        mensajeEstado = "Disparo enviado al host...";
        actualizarControlesPorTurno();
        InvalidateRect(ventanaPrincipal, NULL, TRUE);
        return;
    }

    Jugador tirador = (turnoActual == 1) ? jugador1 : jugador2;
    Jugador objetivo = (turnoActual == 1) ? jugador2 : jugador1;

    double radianes = gradosARadianes(angulo);
    int direccion = (objetivo.x > tirador.x) ? 1 : -1;

    int salidaX = tirador.x + direccion * 28;
    int salidaY = tirador.y - 58;

    proyectil.xInicial = salidaX;
    proyectil.yInicial = salidaY;
    proyectil.x = proyectil.xInicial;
    proyectil.y = proyectil.yInicial;
    proyectil.vx = cos(radianes) * velocidad * direccion;
    proyectil.vy = sin(radianes) * velocidad;
    proyectil.tiempo = 0.0;
    proyectil.activo = true;


   // CAMBIO MANUAL 4 - mostrar datos del disparo
mensajeEstado = "Angulo: " + to_string((int)angulo) + 
                " | Velocidad: " + to_string((int)velocidad);

    actualizarControlesPorTurno();
    enviarEstadoRed();

    SetTimer(ventanaPrincipal, ID_TIMER, 30, NULL);
}

void actualizarProyectil() {
    if (!proyectil.activo) {
        return;
    }
// CAMBIO MANUAL 1 - ESCALA DEL MOVIMIENTO
    proyectil.tiempo += 0.10; //antes 0.12

  
// CAMBIO MANUAL 1 - EFECTO DEL VIENTO
double viento = 0.99; //antes 0.09  a ahora a 0.99 para que el viento tenga un efecto más sutil y permita tiros más predecibles, pero aún así influya en la trayectoria.

proyectil.x = proyectil.xInicial +
              (proyectil.vx + viento * proyectil.tiempo) * proyectil.tiempo;
    
    proyectil.y = proyectil.yInicial - proyectil.vy * proyectil.tiempo +
                  (GRAVEDAD * proyectil.tiempo * proyectil.tiempo) / 2.0;

    int x = static_cast<int>(round(proyectil.x));
    int y = static_cast<int>(round(proyectil.y));

    Jugador tirador = (turnoActual == 1) ? jugador1 : jugador2;
    Jugador objetivo = (turnoActual == 1) ? jugador2 : jugador1;

    double distanciaAlTirador = sqrt(
        (x - tirador.x) * (x - tirador.x) +
        (y - tirador.y) * (y - tirador.y)
    );

    if (x < 0 || x > ANCHO_MUNDO || y > ALTO_MUNDO) {
        mensajeEstado = "Disparo fallido"; //Cambio manual 5
        proyectil.activo = false;
        KillTimer(ventanaPrincipal, ID_TIMER);
        cambiarTurno();
    } else if (impactoContraJugador(objetivo, x, y)) {
      
    mensajeEstado = "Impacto directo!";//cambio manual 5
        terminarRonda(turnoActual);
    } else if (proyectil.tiempo > 0.8 &&
               distanciaAlTirador > 55 &&
               impactoContraJugador(tirador, x, y)) {
        int ganador = (turnoActual == 1) ? 2 : 1;
        terminarRonda(ganador);
    } else if (impactoContraEdificio(x, y)) {
        
    mensajeEstado = "Impacto en edificio"; //Cambio manual 5
        crearCrater(x, y);
        proyectil.activo = false;
        KillTimer(ventanaPrincipal, ID_TIMER);
        cambiarTurno();
    }

    InvalidateRect(ventanaPrincipal, NULL, TRUE);
    enviarEstadoRed();
}

// -----------------------------
// Dibujo del juego
// -----------------------------

void dibujarCieloNocturno(HDC hdc) {
    

    HBRUSH cielo = CreateSolidBrush(RGB(0, 0, 25)); // CAMBIO MANUAL 5 - cielo más oscuro
    RECT areaCielo = {0, 0, ANCHO_MUNDO, ALTO_MUNDO};
    FillRect(hdc, &areaCielo, cielo);
    DeleteObject(cielo);

    HBRUSH estrella = CreateSolidBrush(RGB(245, 245, 210));

    for (int i = 0; i < 75; i++) {
        int x = (i * 137 + 41) % ANCHO_MUNDO;
        int y = (i * 73 + 29) % 250;
        RECT punto = {x, y, x + 2, y + 2};
        FillRect(hdc, &punto, estrella);
    }

    DeleteObject(estrella);

    HBRUSH luna = CreateSolidBrush(RGB(245, 235, 170));
    HBRUSH sombra = CreateSolidBrush(RGB(8, 10, 45));
    HBRUSH anterior = (HBRUSH)SelectObject(hdc, luna);
    Ellipse(hdc, 770, 45, 825, 100);
    SelectObject(hdc, sombra);
    Ellipse(hdc, 790, 40, 840, 95);
    SelectObject(hdc, anterior);
    DeleteObject(luna);
    DeleteObject(sombra);
}

void dibujarJugador(HDC hdc, const Jugador& jugador) {
    HBRUSH brochaCuerpo = CreateSolidBrush(jugador.color);
    HBRUSH brochaCara = CreateSolidBrush(RGB(235, 190, 140));
    HBRUSH brochaNegra = CreateSolidBrush(RGB(20, 20, 20));
    HBRUSH anterior = (HBRUSH)SelectObject(hdc, brochaCuerpo);

    Ellipse(hdc, jugador.x - 18, jugador.y - 22, jugador.x + 18, jugador.y + 12);
    Ellipse(hdc, jugador.x - 13, jugador.y - 42, jugador.x + 13, jugador.y - 16);
    Ellipse(hdc, jugador.x - 21, jugador.y - 35, jugador.x - 9, jugador.y - 22);
    Ellipse(hdc, jugador.x + 9, jugador.y - 35, jugador.x + 21, jugador.y - 22);
    Ellipse(hdc, jugador.x - 31, jugador.y - 18, jugador.x - 13, jugador.y + 17);
    Ellipse(hdc, jugador.x + 13, jugador.y - 18, jugador.x + 31, jugador.y + 17);
    Ellipse(hdc, jugador.x - 17, jugador.y + 4, jugador.x - 2, jugador.y + 22);
    Ellipse(hdc, jugador.x + 2, jugador.y + 4, jugador.x + 17, jugador.y + 22);

    SelectObject(hdc, brochaCara);
    Ellipse(hdc, jugador.x - 9, jugador.y - 34, jugador.x + 9, jugador.y - 19);
    Ellipse(hdc, jugador.x - 10, jugador.y - 25, jugador.x + 10, jugador.y - 12);

    SelectObject(hdc, brochaNegra);
    Ellipse(hdc, jugador.x - 6, jugador.y - 29, jugador.x - 2, jugador.y - 25);
    Ellipse(hdc, jugador.x + 2, jugador.y - 29, jugador.x + 6, jugador.y - 25);
    Ellipse(hdc, jugador.x - 3, jugador.y - 22, jugador.x + 3, jugador.y - 17);

    SelectObject(hdc, anterior);
    DeleteObject(brochaCuerpo);
    DeleteObject(brochaCara);
    DeleteObject(brochaNegra);
}

void dibujarEdificios(HDC hdc) {
    for (const Edificio& edificio : edificios) {
        HBRUSH brochaEdificio = CreateSolidBrush(edificio.color);
        HBRUSH anterior = (HBRUSH)SelectObject(hdc, brochaEdificio);

        Rectangle(hdc, edificio.x, ALTO_MUNDO - edificio.alto,
                  edificio.x + edificio.ancho, ALTO_MUNDO);

        SelectObject(hdc, anterior);
        DeleteObject(brochaEdificio);

        HBRUSH brochaVentana = CreateSolidBrush(RGB(255, 225, 95));

        for (int y = ALTO_MUNDO - edificio.alto + 18; y < ALTO_MUNDO - 20; y += 28) {
            for (int x = edificio.x + 12; x < edificio.x + edificio.ancho - 12; x += 26) {
                RECT ventana = {x, y, x + 8, y + 12};
                FillRect(hdc, &ventana, brochaVentana);
            }
        }

        DeleteObject(brochaVentana);
    }
}

void dibujarCrateres(HDC hdc) {
    HBRUSH brochaCrater = CreateSolidBrush(RGB(8, 10, 45));
    HBRUSH anterior = (HBRUSH)SelectObject(hdc, brochaCrater);

    for (const Crater& crater : crateres) {
        Ellipse(hdc,
                crater.x - crater.radio,
                crater.y - crater.radio,
                crater.x + crater.radio,
                crater.y + crater.radio);
    }

    SelectObject(hdc, anterior);
    DeleteObject(brochaCrater);
}

void dibujarProyectil(HDC hdc) {
    if (!proyectil.activo) {
        return;
    }

    HBRUSH brochaProyectil = CreateSolidBrush(RGB(255, 80, 80)); // CAMBIO MANUAL 5 - proyectil más visible
    HBRUSH anterior = (HBRUSH)SelectObject(hdc, brochaProyectil);

    Ellipse(hdc,
            static_cast<int>(proyectil.x) - 5,
            static_cast<int>(proyectil.y) - 5,
            static_cast<int>(proyectil.x) + 5,
            static_cast<int>(proyectil.y) + 5);

    SelectObject(hdc, anterior);
    DeleteObject(brochaProyectil);
}

void dibujarPanel(HDC hdc) {
    HBRUSH panel = CreateSolidBrush(RGB(230, 230, 235));
    RECT areaPanel = {0, ALTO_MUNDO, ANCHO_VENTANA, ALTO_VENTANA};
    FillRect(hdc, &areaPanel, panel);
    DeleteObject(panel);

    dibujarTexto(hdc, 20, ALTO_MUNDO + 12, mensajeEstado);
    dibujarTexto(hdc, 20, ALTO_MUNDO + 38,
                 "Mejor de 3  |  " + jugador1.nombre + ": " + to_string(jugador1.victorias) +
                 "  " + jugador2.nombre + ": " + to_string(jugador2.victorias));
    dibujarTexto(hdc, 20, ALTO_MUNDO + 78, "Angulo:");
    dibujarTexto(hdc, 190, ALTO_MUNDO + 78, "Velocidad:");

    string estadoRed = "Modo: local";

    if (modoRed == RED_HOST) {
        estadoRed = redConectada ? "Modo: host conectado" : "Modo: host esperando";
    } else if (modoRed == RED_CLIENTE) {
        estadoRed = redConectada ? "Modo: cliente conectado" : "Modo: cliente conectando";
    }

    dibujarTexto(hdc, 725, ALTO_MUNDO + 12, estadoRed);
    
    






}

void dibujarGameOver(HDC hdc) {
    if (!partidaTerminada) {
        return;
    }

    RECT fondo = {250, 145, 750, 340};
    HBRUSH brochaFondo = CreateSolidBrush(RGB(15, 15, 30));
    FillRect(hdc, &fondo, brochaFondo);
    DeleteObject(brochaFondo);

    HPEN borde = CreatePen(PS_SOLID, 3, RGB(255, 225, 95));
    HGDIOBJ bordeAnterior = SelectObject(hdc, borde);
    HGDIOBJ brochaAnterior = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, fondo.left, fondo.top, fondo.right, fondo.bottom);
    SelectObject(hdc, bordeAnterior);
    SelectObject(hdc, brochaAnterior);
    DeleteObject(borde);

    SetTextColor(hdc, RGB(255, 225, 95));
    SetBkMode(hdc, TRANSPARENT);
    dibujarTexto(hdc, 405, 185, "GAME OVER");
    dibujarTexto(hdc, 335, 225, "Ganador: " + ganadorPartida);
    dibujarTexto(hdc, 315, 265, "Resultado final: " + to_string(jugador1.victorias) +
                 " - " + to_string(jugador2.victorias));
    dibujarTexto(hdc, 300, 300, "Presione Nueva partida para jugar otra vez.");
    SetTextColor(hdc, RGB(0, 0, 0));
}

void dibujarEscenario(HDC hdc) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));

    dibujarCieloNocturno(hdc);
    dibujarEdificios(hdc);
    dibujarCrateres(hdc);
    dibujarJugador(hdc, jugador1);
    dibujarJugador(hdc, jugador2);
    dibujarProyectil(hdc);
    dibujarPanel(hdc);
    dibujarGameOver(hdc);
}

void dibujarMenu(HDC hdc) {
    HBRUSH fondo = CreateSolidBrush(RGB(0, 0, 25));
    RECT area = {0, 0, ANCHO_VENTANA, ALTO_VENTANA};
    FillRect(hdc, &area, fondo);
    DeleteObject(fondo);

    HBRUSH estrella = CreateSolidBrush(RGB(245, 245, 210));

    for (int i = 0; i < 90; i++) {
        int x = (i * 113 + 57) % ANCHO_VENTANA;
        int y = (i * 67 + 31) % ALTO_VENTANA;
        RECT punto = {x, y, x + 2, y + 2};
        FillRect(hdc, &punto, estrella);
    }

    DeleteObject(estrella);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 225, 95));

    HFONT fuenteTitulo = CreateFontA(
        58, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial"
    );
    HFONT fuenteTexto = CreateFontA(
        22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial"
    );

    HGDIOBJ fuenteAnterior = SelectObject(hdc, fuenteTitulo);
    dibujarTexto(hdc, 315, 145, "Gorillas C++");

    SelectObject(hdc, fuenteTexto);
    SetTextColor(hdc, RGB(235, 235, 240));

    if (pantallaActual == PANTALLA_MENU) {
        dibujarTexto(hdc, 330, 230, "Elija un modo de juego");
    } else {
        dibujarTexto(hdc, 290, 210, "Multijugador en red local");
        dibujarTexto(hdc, 240, 250, "Una PC crea el servidor y la otra se une usando la IP que aparece.");
        dibujarTexto(hdc, 285, 297, "Nombre:");
        dibujarTexto(hdc, 285, 367, "IP servidor:");

        if (!mensajeMenuMultijugador.empty()) {
            dibujarTexto(hdc, 130, 560, mensajeMenuMultijugador);
        } else {
            dibujarTexto(hdc, 210, 560, "Al crear servidor se mostrara la IP local para la otra computadora.");
        }
    }

    SelectObject(hdc, fuenteAnterior);
    DeleteObject(fuenteTitulo);
    DeleteObject(fuenteTexto);
}

LRESULT CALLBACK ProcedimientoVentana(HWND hwnd, UINT mensaje, WPARAM wParam, LPARAM lParam) {
    switch (mensaje) {
        case WM_CREATE: {
            ventanaPrincipal = hwnd;

            campoAngulo = CreateWindowA(
                "EDIT", "45",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                85, ALTO_MUNDO + 73, 80, 28,
                hwnd, (HMENU)ID_ANGULO, NULL, NULL
            );

            campoVelocidad = CreateWindowA(
                "EDIT", "90",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                285, ALTO_MUNDO + 73, 80, 28,
                hwnd, (HMENU)ID_VELOCIDAD, NULL, NULL
            );

            botonDisparar = CreateWindowA(
                "BUTTON", "Disparar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                405, ALTO_MUNDO + 70, 115, 34,
                hwnd, (HMENU)ID_DISPARAR, NULL, NULL
            );

            botonReiniciar = CreateWindowA(
                "BUTTON", "Reiniciar ronda",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                540, ALTO_MUNDO + 70, 145, 34,
                hwnd, (HMENU)ID_REINICIAR, NULL, NULL
            );

            campoIP = CreateWindowA(
                "EDIT", "127.0.0.1",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                775, ALTO_MUNDO + 37, 120, 26,
                hwnd, (HMENU)ID_IP, NULL, NULL
            );

            campoPuerto = CreateWindowA(
                "EDIT", "8080",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                910, ALTO_MUNDO + 37, 70, 26,
                hwnd, (HMENU)ID_PUERTO, NULL, NULL
            );

            campoNombre = CreateWindowA(
                "EDIT", "Jugador 2",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                350, 365, 210, 28,
                hwnd, (HMENU)ID_NOMBRE, NULL, NULL
            );

            botonHost = CreateWindowA(
                "BUTTON", "Crear servidor",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                710, ALTO_MUNDO + 70, 145, 34,
                hwnd, (HMENU)ID_HOST, NULL, NULL
            );

            botonUnirse = CreateWindowA(
                "BUTTON", "Unirse",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                805, ALTO_MUNDO + 70, 90, 34,
                hwnd, (HMENU)ID_UNIRSE, NULL, NULL
            );

            botonSolitario = CreateWindowA(
                "BUTTON", "Solitario",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                365, 315, 270, 48,
                hwnd, (HMENU)ID_SOLITARIO, NULL, NULL
            );

            botonMultijugador = CreateWindowA(
                "BUTTON", "Multijugador",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                365, 385, 270, 48,
                hwnd, (HMENU)ID_MULTIJUGADOR, NULL, NULL
            );

            botonVolverMenu = CreateWindowA(
                "BUTTON", "Volver",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                430, 420, 140, 34,
                hwnd, (HMENU)ID_VOLVER_MENU, NULL, NULL
            );

            botonDesconectar = CreateWindowA(
                "BUTTON", "Desconectar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                820, ALTO_MUNDO + 70, 145, 34,
                hwnd, (HMENU)ID_DESCONECTAR, NULL, NULL
            );

            mostrarMenuInicio();
            return 0;
        }

        case WM_COMMAND: {
            int idControl = LOWORD(wParam);

            if (idControl == ID_DISPARAR) {
                disparar();
            } else if (idControl == ID_REINICIAR) {
                if (modoRed == RED_CLIENTE) {
                    enviarMensajeRed("RESTART");
                } else {
                    if (partidaTerminada) {
                        iniciarNuevaPartida();
                    } else {
                        iniciarNuevaRonda();
                    }
                }
            } else if (idControl == ID_HOST) {
                iniciarModoHost();
            } else if (idControl == ID_UNIRSE) {
                iniciarModoCliente();
            } else if (idControl == ID_SOLITARIO) {
                iniciarModoSolitario();
            } else if (idControl == ID_MULTIJUGADOR) {
                mostrarMenuMultijugador();
            } else if (idControl == ID_VOLVER_MENU) {
                cerrarRed();
                mostrarMenuInicio();
            } else if (idControl == ID_DESCONECTAR) {
                desconectarYVolverAlMenu();
            }

            return 0;
        }

        case WM_RED_MENSAJE: {
            string* mensajeRed = reinterpret_cast<string*>(lParam);
            string texto = *mensajeRed;
            delete mensajeRed;

            if (texto == "CONNECTED") {
                redConectada = true;
                esperandoConexion = false;

                if (modoRed == RED_HOST) {
                    mensajeMenuMultijugador = "Jugador conectado. Esperando su nombre...";
                } else {
                    mensajeMenuMultijugador = "Conectado. Enviando nombre...";
                    enviarMensajeRed("NAME|" + jugador2.nombre);
                }

                actualizarControlesPorTurno();
                InvalidateRect(hwnd, NULL, TRUE);
            } else if (texto == "DISCONNECT") {
                redConectada = false;
                esperandoConexion = false;
                cerrarRed();
                mensajeEstado = "Conexion cerrada.";
                modoRed = RED_LOCAL;
                EnableWindow(botonHost, TRUE);
                EnableWindow(botonUnirse, TRUE);
                mostrarMenuInicio();
            } else if (texto == "NET_ERROR") {
                redConectada = false;
                esperandoConexion = false;
                cerrarRed();
                mensajeMenuMultijugador = "No se pudo conectar. Revise que la IP sea de la misma red y que el servidor este creado.";
                modoRed = RED_LOCAL;
                EnableWindow(botonHost, TRUE);
                EnableWindow(botonUnirse, TRUE);
                EnableWindow(campoNombre, TRUE);
                EnableWindow(campoIP, TRUE);
                mostrarMenuMultijugador();
            } else {
                procesarMensajeRed(texto);
            }

            return 0;
        }

        case WM_TIMER:
            if (wParam == ID_TIMER) {
                actualizarProyectil();
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            if (pantallaActual == PANTALLA_JUEGO) {
                dibujarEscenario(hdc);
            } else {
                dibujarMenu(hdc);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            cerrarRed();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, mensaje, wParam, lParam);
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOW;

    InitializeCriticalSection(&seccionEnvioRed);

    srand(static_cast<unsigned int>(time(NULL)));

    jugador1.nombre = "Jugador 1";
    jugador1.victorias = 0;
    jugador1.color = RGB(220, 60, 60);

    jugador2.nombre = "Jugador 2";
    jugador2.victorias = 0;
    jugador2.color = RGB(55, 95, 225);

    WNDCLASSA claseVentana = {};
    claseVentana.lpfnWndProc = ProcedimientoVentana;
    claseVentana.hInstance = hInstance;
    claseVentana.lpszClassName = "ClaseGorillasCpp";
    claseVentana.hCursor = LoadCursor(NULL, IDC_ARROW);
    claseVentana.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&claseVentana);

    DWORD estiloVentana = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT tamanoVentana = {0, 0, ANCHO_VENTANA, ALTO_VENTANA};
    AdjustWindowRect(&tamanoVentana, estiloVentana, FALSE);

    ventanaPrincipal = CreateWindowA(
        "ClaseGorillasCpp",
        "Gorillas C++ - Mejor de tres",
        estiloVentana,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        tamanoVentana.right - tamanoVentana.left,
        tamanoVentana.bottom - tamanoVentana.top,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (ventanaPrincipal == NULL) {
        return 0;
    }

    ShowWindow(ventanaPrincipal, nCmdShow);
    UpdateWindow(ventanaPrincipal);

    MSG mensaje = {};

    while (GetMessage(&mensaje, NULL, 0, 0)) {
        TranslateMessage(&mensaje);
        DispatchMessage(&mensaje);
    }

    DeleteCriticalSection(&seccionEnvioRed);
    return 0;
}
