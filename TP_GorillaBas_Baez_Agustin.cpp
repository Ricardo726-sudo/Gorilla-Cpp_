/*
Autor: Agustin Baez
Carrera: Ing. informatica
Año: 2026.
Programa: creacion de juego basado en "Gorilla.bash"
*/




#include <windows.h> // Permite crear ventanas, botones, campos de texto, timers y dibujar usando GDI.
#include <cmath>     // Permite usar sin, cos, sqrt y round para la trayectoria parabolica.
#include <ctime>     // Permite usar time(), util para generar valores aleatorios distintos.
#include <cstdlib>   // Permite usar srand() y rand() para variar alturas y estrellas.
#include <string>    // Permite usar string para nombres y mensajes del juego.
#include <vector>    // Permite usar vector para guardar la lista de edificios .

using namespace std;

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
const int PUNTOS_PARA_GANAR = 3; // Mejor de tres

const double PI = 3.14159265358979323846;

// Gravedad ajustada para que el juego sea divertido, no para simular fisica real.
// Con un valor mas bajo hay arcos mas largos y es posible acertar a distancia.

//Cambio manual 1- Gravedad
const double GRAVEDAD = 28.0; //antes 32.0

// Identificadores de controles.
// Windows los usa para reconocer que boton o campo genero un evento.
const int ID_ANGULO = 101;
const int ID_VELOCIDAD = 102;
const int ID_DISPARAR = 103;
const int ID_REINICIAR = 104;
const int ID_TIMER = 200;

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

// -----------------------------
// Prototipado de funciones
// -----------------------------


double gradosARadianes(double grados);
wstring convertirAWstring(const string& texto);
void dibujarTexto(HDC hdc, int x, int y, const string& texto);
double leerNumero(HWND campo, double valorPorDefecto);
double leerNumeroLimitado(HWND campo, double valorPorDefecto, double minimo, double maximo);
int obtenerTechoEnX(int x);

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
    mensajeEstado = "Turno del Jugador 1";

    EnableWindow(botonDisparar, TRUE);
    SetWindowTextA(botonReiniciar, "Reiniciar ronda");

    if (ventanaPrincipal != NULL) {
        InvalidateRect(ventanaPrincipal, NULL, TRUE);
    }
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
    EnableWindow(botonDisparar, FALSE);
    SetWindowTextA(botonReiniciar, "Nueva partida");
    InvalidateRect(ventanaPrincipal, NULL, TRUE);
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

        mensajeEstado = "Gana la ronda el Jugador 1. Presione Reiniciar ronda.";
    } else {
        jugador2.victorias++;

        if (jugador2.victorias >= PUNTOS_PARA_GANAR) {
            terminarPartida(jugador2.nombre);
            return;
        }

        mensajeEstado = "Gana la ronda el Jugador 2. Presione Reiniciar ronda.";
    }

    EnableWindow(botonDisparar, FALSE);
    InvalidateRect(ventanaPrincipal, NULL, TRUE);
}

// -----------------------------
// Disparo y fisica
// -----------------------------

void cambiarTurno() {
    turnoActual = (turnoActual == 1) ? 2 : 1;
    mensajeEstado = (turnoActual == 1) ? "Turno del Jugador 1" : "Turno del Jugador 2";
    EnableWindow(botonDisparar, TRUE);
}

void disparar() {
    if (rondaTerminada || partidaTerminada || proyectil.activo) {
        return;
    }

    double angulo = leerNumeroLimitado(campoAngulo, 45.0, 0.0, 90.0);
    // CAMBIO MANUAL 1 - VELOCIDAD INICIAL AJUSTADA
    double velocidad = leerNumeroLimitado(campoVelocidad, 110.0, 10.0, 220.0); //antes 90.0, 10.0, 220.0

   

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

    EnableWindow(botonDisparar, FALSE);

    SetTimer(ventanaPrincipal, ID_TIMER, 30, NULL);
}

void actualizarProyectil() {
    if (!proyectil.activo) {
        return;
    }
// CAMBIO MANUAL 1 - ESCALA DEL MOVIMIENTO
    proyectil.tiempo += 0.10; //antes 0.12

  
// CAMBIO MANUAL 1 - EFECTO DEL VIENTO
double viento = 0.06; //antes 0.09

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
                 "Mejor de 3  |  Jugador 1: " + to_string(jugador1.victorias) +
                 "  Jugador 2: " + to_string(jugador2.victorias));
    dibujarTexto(hdc, 20, ALTO_MUNDO + 78, "Angulo:");
    dibujarTexto(hdc, 190, ALTO_MUNDO + 78, "Velocidad:");
    
    






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

            iniciarNuevaPartida();
            return 0;
        }

        case WM_COMMAND: {
            int idControl = LOWORD(wParam);

            if (idControl == ID_DISPARAR) {
                disparar();
            } else if (idControl == ID_REINICIAR) {
                if (partidaTerminada) {
                    iniciarNuevaPartida();
                } else {
                    iniciarNuevaRonda();
                }
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
            dibujarEscenario(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, mensaje, wParam, lParam);
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOW;

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

    return 0;
}
