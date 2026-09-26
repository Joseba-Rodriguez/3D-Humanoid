#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
simulador_humanoide.py
Simulador virtual (sin hardware) del humanoide 3D: ojos, cuello y mano.

Reproduce en Python la misma logica que los .ino de este repositorio
(mano-automatica.ino, cuello-automatico.ino, ojos-automatic.ino,
humanoide-automatico.ino):
  - Las mismas poses, angulos y rangos de cada servo.
  - Los mismos modos automaticos (poses aleatorias, parpadeo, piedra-papel-
    tijera, movimientos lentos de cuello) con temporizaciones equivalentes.
  - Un "monitor serie" (consola) que imprime los mismos mensajes que
    Serial.println() en el Arduino real.
  - Control manual con deslizadores (sliders) para cada servo, igual que si
    escribieras servo.write(angulo) a mano.
  - Un dibujo esquematico que se anima en tiempo real con los valores.

No requiere ningun hardware ni librerias externas: solo Python 3 + tkinter
(incluido en la instalacion estandar de Python en Windows).

Ejecutar:
    python simulador_humanoide.py
"""

import math
import random
import time
import tkinter as tk
from tkinter import ttk


# ============================================================
# Utilidades comunes
# ============================================================

def now_ms():
    return time.time() * 1000.0


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def lerp(a, b, t):
    return a + (b - a) * t


# ============================================================
# ANGULOS DE REFERENCIA (idénticos a los .ino)
# ============================================================

# ---- Mano ----
DEDO_CERRADO, DEDO_ABIERTO, DEDO_MEDIO = 0, 180, 90
PULGAR_FLEX_CERRADO, PULGAR_FLEX_ABIERTO, PULGAR_FLEX_MEDIO = 0, 180, 90
PULGAR_VERT_PLANO, PULGAR_VERT_ARRIBA, PULGAR_VERT_MEDIO = 0, 180, 90
MUNECA_IZQUIERDA, MUNECA_CENTRO, MUNECA_DERECHA = 0, 90, 180
CODO_ABAJO, CODO_MEDIO, CODO_ARRIBA = 0, 90, 180

HAND_FIELDS = ("pulgar_flexion", "indice", "corazon", "anular_menique",
               "pulgar_vertical", "muneca", "codo")

POSES_MANO = [
    ("Mano relajada", PULGAR_FLEX_MEDIO, DEDO_MEDIO, DEDO_MEDIO, DEDO_MEDIO, PULGAR_VERT_MEDIO, MUNECA_CENTRO, CODO_MEDIO),
    ("Puno cerrado", PULGAR_FLEX_CERRADO, DEDO_CERRADO, DEDO_CERRADO, DEDO_CERRADO, PULGAR_VERT_PLANO, MUNECA_CENTRO, CODO_ARRIBA),
    ("Mano abierta", PULGAR_FLEX_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, PULGAR_VERT_ARRIBA, MUNECA_CENTRO, CODO_ABAJO),
    ("Senal de paz", PULGAR_FLEX_CERRADO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_CERRADO, PULGAR_VERT_PLANO, MUNECA_DERECHA, CODO_MEDIO),
    ("Pulgar arriba", PULGAR_FLEX_ABIERTO, DEDO_CERRADO, DEDO_CERRADO, DEDO_CERRADO, PULGAR_VERT_ARRIBA, MUNECA_CENTRO, CODO_ARRIBA),
    ("Senalar", PULGAR_FLEX_CERRADO, DEDO_ABIERTO, DEDO_CERRADO, DEDO_CERRADO, PULGAR_VERT_PLANO, MUNECA_IZQUIERDA, CODO_ABAJO),
    ("OK", PULGAR_FLEX_MEDIO, DEDO_MEDIO, DEDO_ABIERTO, DEDO_ABIERTO, PULGAR_VERT_MEDIO, MUNECA_CENTRO, CODO_MEDIO),
    ("Garra", PULGAR_FLEX_MEDIO, DEDO_MEDIO, DEDO_MEDIO, DEDO_MEDIO, PULGAR_VERT_MEDIO, MUNECA_DERECHA, CODO_ARRIBA),
]

JUGADAS_RPS = [
    ("Piedra", PULGAR_FLEX_CERRADO, DEDO_CERRADO, DEDO_CERRADO, DEDO_CERRADO, PULGAR_VERT_PLANO, MUNECA_CENTRO, CODO_ARRIBA),
    ("Papel", PULGAR_FLEX_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_ABIERTO, PULGAR_VERT_ARRIBA, MUNECA_CENTRO, CODO_MEDIO),
    ("Tijera", PULGAR_FLEX_CERRADO, DEDO_ABIERTO, DEDO_ABIERTO, DEDO_CERRADO, PULGAR_VERT_PLANO, MUNECA_CENTRO, CODO_MEDIO),
]

# ---- Cuello ----
NEUTRO = 90
AMPLITUD_INCLINACION = 12
AMPLITUD_ROTACION = 15
INCLINACION_IZQ_ACTIVA = NEUTRO + AMPLITUD_INCLINACION
INCLINACION_IZQ_RELAJADA = NEUTRO - (AMPLITUD_INCLINACION // 2)
INCLINACION_DER_ACTIVA = NEUTRO + AMPLITUD_INCLINACION
INCLINACION_DER_RELAJADA = NEUTRO - (AMPLITUD_INCLINACION // 2)
ROTACION_IZQUIERDA = NEUTRO - AMPLITUD_ROTACION
ROTACION_DERECHA = NEUTRO + AMPLITUD_ROTACION

NECK_FIELDS = ("cuello_incl_izq", "cuello_incl_der", "cuello_rotacion")

POSES_CUELLO = [
    ("Centro", NEUTRO, NEUTRO, NEUTRO),
    ("Inclinar poco a la izquierda", INCLINACION_IZQ_ACTIVA, INCLINACION_DER_RELAJADA, NEUTRO),
    ("Inclinar poco a la derecha", INCLINACION_IZQ_RELAJADA, INCLINACION_DER_ACTIVA, NEUTRO),
    ("Girar poco a la izquierda", NEUTRO, NEUTRO, ROTACION_IZQUIERDA),
    ("Girar poco a la derecha", NEUTRO, NEUTRO, ROTACION_DERECHA),
]

# ---- Ojos ----
PARPADO_SUP_IZQ_ABIERTO, PARPADO_SUP_IZQ_CERRADO = 90, 130
PARPADO_SUP_DER_ABIERTO, PARPADO_SUP_DER_CERRADO = 90, 50
PARPADO_INF_IZQ_ABIERTO, PARPADO_INF_IZQ_CERRADO = 90, 50
PARPADO_INF_DER_ABIERTO, PARPADO_INF_DER_CERRADO = 90, 130

OJOS_VERTICAL_CENTRO, OJOS_VERTICAL_ARRIBA, OJOS_VERTICAL_ABAJO = 90, 70, 110
OJOS_HORIZONTAL_CENTRO, OJOS_HORIZONTAL_IZQUIERDA, OJOS_HORIZONTAL_DERECHA = 90, 60, 120

EYE_FIELDS = ("parp_sup_izq", "parp_sup_der", "parp_inf_izq", "parp_inf_der",
              "ojos_vertical", "ojos_horizontal")

EYELID_RANGES = {
    "parp_sup_izq": (PARPADO_SUP_IZQ_ABIERTO, PARPADO_SUP_IZQ_CERRADO),
    "parp_sup_der": (PARPADO_SUP_DER_ABIERTO, PARPADO_SUP_DER_CERRADO),
    "parp_inf_izq": (PARPADO_INF_IZQ_ABIERTO, PARPADO_INF_IZQ_CERRADO),
    "parp_inf_der": (PARPADO_INF_DER_ABIERTO, PARPADO_INF_DER_CERRADO),
}


# ============================================================
# CONTROLADORES AUTOMATICOS (replican el loop() de cada .ino)
# ============================================================

class AutoOjos:
    """Replica ojos-automatic.ino: parpadeo con maquina de estados + mirada aleatoria."""

    PARPADEO_MIN_MS, PARPADEO_MAX_MS = 2000, 6000
    PARPADEO_DURACION_MS = 120
    PROB_DOBLE_PARPADEO = 20  # %
    MIRADA_MIN_MS, MIRADA_MAX_MS = 1500, 4500
    PASOS_NORMAL, RETARDO_NORMAL = 20, 15
    PASOS_SNAP, RETARDO_SNAP = 3, 4
    PASOS_ESCANEO, RETARDO_ESCANEO = 40, 12

    def __init__(self, app):
        self.app = app
        self.active = False
        self.estado_parpadeo = "esperando"  # esperando | cerrado
        self.proximo_parpadeo = 0
        self.reapertura = 0
        self.doble_pendiente = False

        self.gaze_moviendo = False
        self.gaze_pasos_totales = self.PASOS_NORMAL
        self.gaze_retardo = self.RETARDO_NORMAL
        self.gaze_paso_actual = 0
        self.gaze_ultimo_paso = 0
        self.gaze_origen = (OJOS_VERTICAL_CENTRO, OJOS_HORIZONTAL_CENTRO)
        self.gaze_destino = self.gaze_origen
        self.proxima_mirada = 0
        # secuencia especial en curso (escaneo/sorpresa/sospecha), lista de pasos a ejecutar
        self.secuencia = []

    def start(self):
        self.active = True
        t = now_ms()
        self.proximo_parpadeo = t + random.uniform(self.PARPADEO_MIN_MS, self.PARPADEO_MAX_MS)
        self.proxima_mirada = t + random.uniform(self.MIRADA_MIN_MS, self.MIRADA_MAX_MS)
        self.app.log("Ojos: modo automatico iniciado")

    def stop(self):
        self.active = False
        self.secuencia = []
        self.gaze_moviendo = False

    def _abrir(self):
        v = self.app.vars
        v["parp_sup_izq"].set(PARPADO_SUP_IZQ_ABIERTO)
        v["parp_sup_der"].set(PARPADO_SUP_DER_ABIERTO)
        v["parp_inf_izq"].set(PARPADO_INF_IZQ_ABIERTO)
        v["parp_inf_der"].set(PARPADO_INF_DER_ABIERTO)

    def _cerrar(self):
        v = self.app.vars
        v["parp_sup_izq"].set(PARPADO_SUP_IZQ_CERRADO)
        v["parp_sup_der"].set(PARPADO_SUP_DER_CERRADO)
        v["parp_inf_izq"].set(PARPADO_INF_IZQ_CERRADO)
        v["parp_inf_der"].set(PARPADO_INF_DER_CERRADO)

    def _entornar(self):
        v = self.app.vars
        v["parp_sup_izq"].set((PARPADO_SUP_IZQ_ABIERTO + PARPADO_SUP_IZQ_CERRADO) // 2)
        v["parp_sup_der"].set((PARPADO_SUP_DER_ABIERTO + PARPADO_SUP_DER_CERRADO) // 2)
        v["parp_inf_izq"].set((PARPADO_INF_IZQ_ABIERTO + PARPADO_INF_IZQ_CERRADO) // 2)
        v["parp_inf_der"].set((PARPADO_INF_DER_ABIERTO + PARPADO_INF_DER_CERRADO) // 2)

    def _iniciar_mirada(self, destino, pasos, retardo):
        self.gaze_origen = (self.app.vars["ojos_vertical"].get(), self.app.vars["ojos_horizontal"].get())
        self.gaze_destino = destino
        self.gaze_pasos_totales = pasos
        self.gaze_retardo = retardo
        self.gaze_paso_actual = 0
        self.gaze_moviendo = True
        self.gaze_ultimo_paso = now_ms()

    def _mirada_aleatoria(self):
        azar = random.randint(0, 99)
        v, h = (random.randint(OJOS_VERTICAL_ARRIBA, OJOS_VERTICAL_ABAJO),
                 random.randint(OJOS_HORIZONTAL_IZQUIERDA, OJOS_HORIZONTAL_DERECHA))
        if azar < 40:
            self.app.log("Ojos: movimiento normal")
            self._iniciar_mirada((v, h), self.PASOS_NORMAL, self.RETARDO_NORMAL)
        elif azar < 65:
            self.app.log("Ojos: snap robotico")
            self._iniciar_mirada((v, h), self.PASOS_SNAP, self.RETARDO_SNAP)
        elif azar < 82:
            self.app.log("Ojos: barrido tipo radar")
            self.secuencia = [
                (OJOS_VERTICAL_CENTRO, OJOS_HORIZONTAL_IZQUIERDA, self.PASOS_ESCANEO, self.RETARDO_ESCANEO),
                (OJOS_VERTICAL_CENTRO, OJOS_HORIZONTAL_DERECHA, self.PASOS_ESCANEO * 2, self.RETARDO_ESCANEO),
                (OJOS_VERTICAL_CENTRO, OJOS_HORIZONTAL_CENTRO, self.PASOS_ESCANEO, self.RETARDO_ESCANEO),
            ]
            self._siguiente_paso_secuencia()
        elif azar < 93:
            self.app.log("Ojos: mirada de sospecha")
            hlado = OJOS_HORIZONTAL_IZQUIERDA if random.randint(0, 1) == 0 else OJOS_HORIZONTAL_DERECHA
            self._iniciar_mirada((OJOS_VERTICAL_CENTRO, hlado), self.PASOS_NORMAL, self.RETARDO_NORMAL)
            self.secuencia = [("sospecha_entornar", None, None, None)]
        else:
            self.app.log("Ojos: sorpresa!")
            self._abrir()
            self._iniciar_mirada((OJOS_VERTICAL_ARRIBA, OJOS_HORIZONTAL_CENTRO), self.PASOS_SNAP, self.RETARDO_SNAP)
            self.secuencia = [("sorpresa_parpadeo", None, None, None)] * 3

    def _siguiente_paso_secuencia(self):
        if not self.secuencia:
            return
        paso = self.secuencia.pop(0)
        if paso[0] == "sospecha_entornar":
            self._entornar()
            self.app.after(900, self._sospecha_fin)
            return
        if paso[0] == "sorpresa_parpadeo":
            self._cerrar()
            self.app.after(self.PARPADEO_DURACION_MS, self._sorpresa_paso_abrir)
            return
        v, h, pasos, retardo = paso
        self._iniciar_mirada((v, h), pasos, retardo)

    def _sospecha_fin(self):
        self._abrir()

    def _sorpresa_paso_abrir(self):
        self._abrir()
        self.app.after(90, self._siguiente_paso_secuencia)

    def update(self, t):
        if not self.active:
            return
        # --- parpadeo ---
        if self.estado_parpadeo == "esperando":
            if t >= self.proximo_parpadeo:
                self._cerrar()
                self.app.log("Ojos: parpadeo")
                self.estado_parpadeo = "cerrado"
                self.reapertura = t + self.PARPADEO_DURACION_MS
        else:
            if t >= self.reapertura:
                self._abrir()
                self.estado_parpadeo = "esperando"
                if not self.doble_pendiente and random.randint(0, 99) < self.PROB_DOBLE_PARPADEO:
                    self.doble_pendiente = True
                    self.proximo_parpadeo = t + 150
                else:
                    self.doble_pendiente = False
                    self.proximo_parpadeo = t + random.uniform(self.PARPADEO_MIN_MS, self.PARPADEO_MAX_MS)

        # --- mirada ---
        if not self.gaze_moviendo:
            if t >= self.proxima_mirada:
                self._mirada_aleatoria()
                self.proxima_mirada = t + random.uniform(self.MIRADA_MIN_MS, self.MIRADA_MAX_MS)
            return

        if t - self.gaze_ultimo_paso >= self.gaze_retardo:
            self.gaze_ultimo_paso = t
            self.gaze_paso_actual += 1
            frac = clamp(self.gaze_paso_actual / self.gaze_pasos_totales, 0, 1)
            vo, ho = self.gaze_origen
            vd, hd = self.gaze_destino
            self.app.vars["ojos_vertical"].set(round(lerp(vo, vd, frac)))
            self.app.vars["ojos_horizontal"].set(round(lerp(ho, hd, frac)))
            if self.gaze_paso_actual >= self.gaze_pasos_totales:
                self.gaze_moviendo = False
                if self.secuencia:
                    self._siguiente_paso_secuencia()


class AutoCuello:
    """Replica cuello-automatico.ino: movimientos lentos y pequenos, sin repetir pose."""

    MIN_MS, MAX_MS = 5000, 10000
    PASOS, RETARDO = 120, 40

    def __init__(self, app):
        self.app = app
        self.active = False
        self.proximo_movimiento = 0
        self.indice_anterior = 0
        self.moviendo = False
        self.paso_actual = 0
        self.ultimo_paso = 0
        self.origen = POSES_CUELLO[0][1:]
        self.destino = self.origen

    def start(self):
        self.active = True
        self.proximo_movimiento = now_ms() + random.uniform(self.MIN_MS, self.MAX_MS)
        self.app.log("Cuello: modo automatico iniciado")

    def stop(self):
        self.active = False
        self.moviendo = False

    def _elegir_pose(self):
        i = self.indice_anterior
        while True:
            i = random.randint(0, len(POSES_CUELLO) - 1)
            if i != self.indice_anterior or len(POSES_CUELLO) == 1:
                return i

    def update(self, t):
        if not self.active:
            return
        if not self.moviendo:
            if t >= self.proximo_movimiento:
                idx = self._elegir_pose()
                self.indice_anterior = idx
                nombre = POSES_CUELLO[idx][0]
                self.app.log(f"Nuevo movimiento de cuello -> {nombre}")
                v = self.app.vars
                self.origen = (v["cuello_incl_izq"].get(), v["cuello_incl_der"].get(), v["cuello_rotacion"].get())
                self.destino = POSES_CUELLO[idx][1:]
                self.paso_actual = 0
                self.moviendo = True
                self.ultimo_paso = t
                self.proximo_movimiento = t + random.uniform(self.MIN_MS, self.MAX_MS)
            return

        if t - self.ultimo_paso >= self.RETARDO:
            self.ultimo_paso = t
            self.paso_actual += 1
            frac = clamp(self.paso_actual / self.PASOS, 0, 1)
            v = self.app.vars
            for i, campo in enumerate(NECK_FIELDS):
                v[campo].set(round(lerp(self.origen[i], self.destino[i], frac)))
            if self.paso_actual >= self.PASOS:
                self.moviendo = False


class AutoMano:
    """Replica mano-automatica.ino: modo 0 (poses aleatorias) y modo 1 (piedra/papel/tijera)."""

    POSE_MIN_MS, POSE_MAX_MS = 3000, 7000
    RPS_MIN_MS, RPS_MAX_MS = 4000, 8000
    PASOS, RETARDO = 30, 20
    SACUDIDAS_CODO = 3
    SACUDIDA_PASOS, SACUDIDA_RETARDO = 10, 18

    def __init__(self, app):
        self.app = app
        self.active = False
        self.modo = 0
        self.indice_pose_anterior = 0
        self.indice_jugada_anterior = 0
        self.proximo_evento = 0

        self.moviendo = False
        self.paso_actual = 0
        self.ultimo_paso = 0
        self.origen = POSES_MANO[0][1:]
        self.destino = self.origen

        self.sacudiendo = False
        self.sac_origen = CODO_MEDIO
        self.sac_destino = CODO_ARRIBA
        self.sac_paso = 0
        self.sac_ultimo = 0
        self.sac_contador = 0
        self.sac_subida = True
        self.jugada_pendiente_idx = None

    def start(self, modo):
        self.active = True
        self.modo = modo
        t = now_ms()
        self.proximo_evento = t + random.uniform(self.POSE_MIN_MS, self.POSE_MAX_MS if modo == 0 else self.RPS_MAX_MS)
        nombre_modo = "Piedra, papel o tijera" if modo == 1 else "Poses aleatorias"
        self.app.log(f"Mano: modo cambiado a: {nombre_modo}")

    def stop(self):
        self.active = False
        self.moviendo = False
        self.sacudiendo = False

    def set_modo(self, modo):
        self.modo = modo
        t = now_ms()
        if modo == 0:
            self.proximo_evento = t + random.uniform(self.POSE_MIN_MS, self.POSE_MAX_MS)
        else:
            self.proximo_evento = t + random.uniform(self.RPS_MIN_MS, self.RPS_MAX_MS)
        nombre_modo = "Piedra, papel o tijera" if modo == 1 else "Poses aleatorias"
        self.app.log(f"Mano: modo cambiado a: {nombre_modo}")

    def _elegir(self, lista, anterior):
        while True:
            i = random.randint(0, len(lista) - 1)
            if i != anterior or len(lista) == 1:
                return i

    def _iniciar_movimiento(self, destino_pose):
        v = self.app.vars
        self.origen = tuple(v[c].get() for c in HAND_FIELDS)
        self.destino = destino_pose
        self.paso_actual = 0
        self.moviendo = True
        self.ultimo_paso = now_ms()

    def update(self, t):
        if not self.active:
            return

        if self.sacudiendo:
            if t - self.sac_ultimo >= self.SACUDIDA_RETARDO:
                self.sac_ultimo = t
                self.sac_paso += 1
                frac = clamp(self.sac_paso / self.SACUDIDA_PASOS, 0, 1)
                self.app.vars["codo"].set(round(lerp(self.sac_origen, self.sac_destino, frac)))
                if self.sac_paso >= self.SACUDIDA_PASOS:
                    self.sac_paso = 0
                    if self.sac_subida:
                        self.sac_origen, self.sac_destino = CODO_ARRIBA, CODO_MEDIO
                        self.sac_subida = False
                    else:
                        self.sac_contador += 1
                        if self.sac_contador >= self.SACUDIDAS_CODO:
                            self.sacudiendo = False
                            nombre, *angs = JUGADAS_RPS[self.jugada_pendiente_idx]
                            self.app.log(f"Jugando... -> {nombre}")
                            self._iniciar_movimiento(tuple(angs))
                        else:
                            self.sac_origen, self.sac_destino = CODO_MEDIO, CODO_ARRIBA
                            self.sac_subida = True
            return

        if self.moviendo:
            if t - self.ultimo_paso >= self.RETARDO:
                self.ultimo_paso = t
                self.paso_actual += 1
                frac = clamp(self.paso_actual / self.PASOS, 0, 1)
                v = self.app.vars
                for i, campo in enumerate(HAND_FIELDS):
                    v[campo].set(round(lerp(self.origen[i], self.destino[i], frac)))
                if self.paso_actual >= self.PASOS:
                    self.moviendo = False
            return

        if t >= self.proximo_evento:
            if self.modo == 0:
                idx = self._elegir(POSES_MANO, self.indice_pose_anterior)
                self.indice_pose_anterior = idx
                nombre, *angs = POSES_MANO[idx]
                self.app.log(f"Nueva pose seleccionada -> {nombre}")
                self._iniciar_movimiento(tuple(angs))
                self.proximo_evento = t + random.uniform(self.POSE_MIN_MS, self.POSE_MAX_MS)
            else:
                idx = self._elegir(JUGADAS_RPS, self.indice_jugada_anterior)
                self.indice_jugada_anterior = idx
                self.jugada_pendiente_idx = idx
                self.app.log("Preparando jugada...")
                self.sacudiendo = True
                self.sac_paso = 0
                self.sac_contador = 0
                self.sac_subida = True
                self.sac_origen, self.sac_destino = CODO_MEDIO, CODO_ARRIBA
                self.sac_ultimo = t
                self.proximo_evento = t + random.uniform(self.RPS_MIN_MS, self.RPS_MAX_MS)


# ============================================================
# APLICACION PRINCIPAL (tkinter)
# ============================================================

class HumanoidSimulatorApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Simulador virtual del Humanoide 3D (ojos + cuello + mano)")
        self.geometry("1180x760")
        self.minsize(1050, 680)

        self.vars = {}
        self._init_vars()

        self.auto_ojos = AutoOjos(self)
        self.auto_cuello = AutoCuello(self)
        self.auto_mano = AutoMano(self)

        self._build_ui()
        self._redraw()
        self.after(15, self._master_loop)

    # ---------------- variables ----------------
    def _init_vars(self):
        defaults = {
            # mano (pose "Mano relajada")
            "pulgar_flexion": PULGAR_FLEX_MEDIO, "indice": DEDO_MEDIO, "corazon": DEDO_MEDIO,
            "anular_menique": DEDO_MEDIO, "pulgar_vertical": PULGAR_VERT_MEDIO,
            "muneca": MUNECA_CENTRO, "codo": CODO_MEDIO,
            # cuello
            "cuello_incl_izq": NEUTRO, "cuello_incl_der": NEUTRO, "cuello_rotacion": NEUTRO,
            # ojos
            "parp_sup_izq": PARPADO_SUP_IZQ_ABIERTO, "parp_sup_der": PARPADO_SUP_DER_ABIERTO,
            "parp_inf_izq": PARPADO_INF_IZQ_ABIERTO, "parp_inf_der": PARPADO_INF_DER_ABIERTO,
            "ojos_vertical": OJOS_VERTICAL_CENTRO, "ojos_horizontal": OJOS_HORIZONTAL_CENTRO,
        }
        for k, val in defaults.items():
            v = tk.IntVar(value=val)
            v.trace_add("write", lambda *_: self._redraw())
            self.vars[k] = v

    # ---------------- UI ----------------
    def _build_ui(self):
        main = ttk.Frame(self)
        main.pack(fill="both", expand=True)

        left = ttk.Frame(main)
        left.pack(side="left", fill="both", expand=True, padx=6, pady=6)

        right = ttk.Frame(main, width=420)
        right.pack(side="right", fill="y", padx=6, pady=6)
        right.pack_propagate(False)

        # ----- Canvas (dibujo esquematico) -----
        self.canvas = tk.Canvas(left, bg="#101820", width=680, height=520, highlightthickness=0)
        self.canvas.pack(fill="both", expand=True)

        # ----- Consola tipo Monitor Serie -----
        console_frame = ttk.LabelFrame(left, text="Monitor Serie (log)")
        console_frame.pack(fill="both", expand=False, pady=(6, 0))
        self.console = tk.Text(console_frame, height=8, bg="#0b0b0b", fg="#33ff66",
                                font=("Consolas", 9), state="disabled")
        self.console.pack(fill="both", expand=True, side="left")
        scroll = ttk.Scrollbar(console_frame, command=self.console.yview)
        scroll.pack(side="right", fill="y")
        self.console.configure(yscrollcommand=scroll.set)

        # ----- Notebook de controles -----
        notebook = ttk.Notebook(right)
        notebook.pack(fill="both", expand=True)

        self.tab_mano = ttk.Frame(notebook)
        self.tab_cuello = ttk.Frame(notebook)
        self.tab_ojos = ttk.Frame(notebook)
        notebook.add(self.tab_mano, text="Mano")
        notebook.add(self.tab_cuello, text="Cuello")
        notebook.add(self.tab_ojos, text="Ojos")

        self._build_tab_mano(self.tab_mano)
        self._build_tab_cuello(self.tab_cuello)
        self._build_tab_ojos(self.tab_ojos)

        self.log("=== Simulador iniciado ===")
        self.log("Ajusta los sliders manualmente o activa el modo automatico de cada parte.")

    def _slider(self, parent, label, var, frm, to, row):
        ttk.Label(parent, text=label).grid(row=row, column=0, sticky="w", padx=4, pady=2)
        s = ttk.Scale(parent, from_=frm, to=to, orient="horizontal", variable=var,
                       command=lambda _v: var.set(round(float(var.get()))))
        s.grid(row=row, column=1, sticky="ew", padx=4, pady=2)
        lbl = ttk.Label(parent, textvariable=var, width=4)
        lbl.grid(row=row, column=2, padx=4)
        parent.columnconfigure(1, weight=1)
        return s

    def _build_tab_mano(self, parent):
        sliders = ttk.LabelFrame(parent, text="Servos de la mano (manual)")
        sliders.pack(fill="x", padx=4, pady=4)
        self._slider(sliders, "Pulgar flexion", self.vars["pulgar_flexion"], 0, 180, 0)
        self._slider(sliders, "Indice", self.vars["indice"], 0, 180, 1)
        self._slider(sliders, "Corazon", self.vars["corazon"], 0, 180, 2)
        self._slider(sliders, "Anular+Menique", self.vars["anular_menique"], 0, 180, 3)
        self._slider(sliders, "Pulgar vertical", self.vars["pulgar_vertical"], 0, 180, 4)
        self._slider(sliders, "Muneca", self.vars["muneca"], 0, 180, 5)
        self._slider(sliders, "Codo", self.vars["codo"], 0, 180, 6)
        self.mano_sliders = sliders

        poses_frame = ttk.LabelFrame(parent, text="Poses (movimiento suave, como en el .ino)")
        poses_frame.pack(fill="x", padx=4, pady=4)
        for i, pose in enumerate(POSES_MANO):
            b = ttk.Button(poses_frame, text=pose[0],
                            command=lambda p=pose: self._ir_a_pose_mano(p))
            b.grid(row=i // 2, column=i % 2, sticky="ew", padx=3, pady=2)
        poses_frame.columnconfigure(0, weight=1)
        poses_frame.columnconfigure(1, weight=1)

        rps_frame = ttk.LabelFrame(parent, text="Jugar Piedra / Papel / Tijera (manual)")
        rps_frame.pack(fill="x", padx=4, pady=4)
        for i, jugada in enumerate(JUGADAS_RPS):
            b = ttk.Button(rps_frame, text=jugada[0], command=lambda j=jugada: self._jugar_rps(j))
            b.grid(row=0, column=i, sticky="ew", padx=3, pady=2)
            rps_frame.columnconfigure(i, weight=1)

        auto_frame = ttk.LabelFrame(parent, text="Modo automatico de la mano")
        auto_frame.pack(fill="x", padx=4, pady=4)
        self.mano_modo_var = tk.IntVar(value=0)
        ttk.Radiobutton(auto_frame, text="0 - Poses aleatorias", variable=self.mano_modo_var,
                         value=0, command=self._cambiar_modo_mano).grid(row=0, column=0, sticky="w", padx=4)
        ttk.Radiobutton(auto_frame, text="1 - Piedra, papel o tijera", variable=self.mano_modo_var,
                         value=1, command=self._cambiar_modo_mano).grid(row=0, column=1, sticky="w", padx=4)
        self.mano_auto_btn = ttk.Button(auto_frame, text="Iniciar modo automatico",
                                          command=self._toggle_auto_mano)
        self.mano_auto_btn.grid(row=1, column=0, columnspan=2, sticky="ew", padx=4, pady=4)

    def _build_tab_cuello(self, parent):
        sliders = ttk.LabelFrame(parent, text="Servos del cuello (manual)")
        sliders.pack(fill="x", padx=4, pady=4)
        self._slider(sliders, "Inclinacion izquierda", self.vars["cuello_incl_izq"], 60, 120, 0)
        self._slider(sliders, "Inclinacion derecha", self.vars["cuello_incl_der"], 60, 120, 1)
        self._slider(sliders, "Rotacion", self.vars["cuello_rotacion"], 60, 120, 2)
        self.cuello_sliders = sliders

        poses_frame = ttk.LabelFrame(parent, text="Poses de cuello (movimiento lento y suave)")
        poses_frame.pack(fill="x", padx=4, pady=4)
        for i, pose in enumerate(POSES_CUELLO):
            b = ttk.Button(poses_frame, text=pose[0], command=lambda p=pose: self._ir_a_pose_cuello(p))
            b.grid(row=i, column=0, sticky="ew", padx=3, pady=2)
        poses_frame.columnconfigure(0, weight=1)

        auto_frame = ttk.LabelFrame(parent, text="Modo automatico del cuello")
        auto_frame.pack(fill="x", padx=4, pady=4)
        self.cuello_auto_btn = ttk.Button(auto_frame, text="Iniciar modo automatico",
                                            command=self._toggle_auto_cuello)
        self.cuello_auto_btn.pack(fill="x", padx=4, pady=4)

    def _build_tab_ojos(self, parent):
        sliders = ttk.LabelFrame(parent, text="Servos de los ojos (manual)")
        sliders.pack(fill="x", padx=4, pady=4)
        self._slider(sliders, "Parpado sup. izq.", self.vars["parp_sup_izq"], 40, 140, 0)
        self._slider(sliders, "Parpado sup. der.", self.vars["parp_sup_der"], 40, 140, 1)
        self._slider(sliders, "Parpado inf. izq.", self.vars["parp_inf_izq"], 40, 140, 2)
        self._slider(sliders, "Parpado inf. der.", self.vars["parp_inf_der"], 40, 140, 3)
        self._slider(sliders, "Mirada vertical", self.vars["ojos_vertical"], 60, 120, 4)
        self._slider(sliders, "Mirada horizontal", self.vars["ojos_horizontal"], 50, 130, 5)
        self.ojos_sliders = sliders

        acciones = ttk.LabelFrame(parent, text="Acciones manuales")
        acciones.pack(fill="x", padx=4, pady=4)
        ttk.Button(acciones, text="Parpadear", command=self._accion_parpadear).grid(row=0, column=0, sticky="ew", padx=3, pady=2)
        ttk.Button(acciones, text="Abrir ojos", command=self._accion_abrir).grid(row=0, column=1, sticky="ew", padx=3, pady=2)
        ttk.Button(acciones, text="Entornar (sospecha)", command=self._accion_entornar).grid(row=1, column=0, sticky="ew", padx=3, pady=2)
        ttk.Button(acciones, text="Mirar al centro", command=self._accion_centro).grid(row=1, column=1, sticky="ew", padx=3, pady=2)
        acciones.columnconfigure(0, weight=1)
        acciones.columnconfigure(1, weight=1)

        auto_frame = ttk.LabelFrame(parent, text="Modo automatico de los ojos")
        auto_frame.pack(fill="x", padx=4, pady=4)
        self.ojos_auto_btn = ttk.Button(auto_frame, text="Iniciar modo automatico",
                                          command=self._toggle_auto_ojos)
        self.ojos_auto_btn.pack(fill="x", padx=4, pady=4)

    # ---------------- acciones manuales ----------------
    def _set_sliders_state(self, frame, state):
        for child in frame.winfo_children():
            if isinstance(child, ttk.Scale):
                child.configure(state=state)

    def _animar_a(self, campos, origen, destino, pasos, retardo, on_done=None, paso=0):
        frac = clamp(paso / pasos, 0, 1)
        for i, campo in enumerate(campos):
            self.vars[campo].set(round(lerp(origen[i], destino[i], frac)))
        if paso >= pasos:
            if on_done:
                on_done()
            return
        self.after(retardo, lambda: self._animar_a(campos, origen, destino, pasos, retardo, on_done, paso + 1))

    def _ir_a_pose_mano(self, pose):
        if self.auto_mano.active:
            self.log("Detén el modo automatico de la mano antes de aplicar una pose manual.")
            return
        nombre, *angs = pose
        self.log(f"Pose manual de mano -> {nombre}")
        origen = tuple(self.vars[c].get() for c in HAND_FIELDS)
        self._animar_a(HAND_FIELDS, origen, tuple(angs), 30, 20)

    def _jugar_rps(self, jugada):
        if self.auto_mano.active:
            self.log("Detén el modo automatico de la mano antes de jugar manualmente.")
            return
        nombre, *angs = jugada
        self.log(f"Jugando manualmente -> {nombre}")
        origen = tuple(self.vars[c].get() for c in HAND_FIELDS)
        self._animar_a(HAND_FIELDS, origen, tuple(angs), 30, 20)

    def _ir_a_pose_cuello(self, pose):
        if self.auto_cuello.active:
            self.log("Detén el modo automatico del cuello antes de aplicar una pose manual.")
            return
        nombre, *angs = pose
        self.log(f"Pose manual de cuello -> {nombre}")
        origen = tuple(self.vars[c].get() for c in NECK_FIELDS)
        self._animar_a(NECK_FIELDS, origen, tuple(angs), 120, 40)

    def _accion_parpadear(self):
        if self.auto_ojos.active:
            self.log("Detén el modo automatico de los ojos antes de parpadear manualmente.")
            return
        self.log("Ojos: parpadeo manual")
        self.auto_ojos._cerrar()
        self.after(120, self.auto_ojos._abrir)

    def _accion_abrir(self):
        if not self.auto_ojos.active:
            self.auto_ojos._abrir()

    def _accion_entornar(self):
        if not self.auto_ojos.active:
            self.auto_ojos._entornar()

    def _accion_centro(self):
        if self.auto_ojos.active:
            return
        self.vars["ojos_vertical"].set(OJOS_VERTICAL_CENTRO)
        self.vars["ojos_horizontal"].set(OJOS_HORIZONTAL_CENTRO)

    # ---------------- toggles de modo automatico ----------------
    def _cambiar_modo_mano(self):
        if self.auto_mano.active:
            self.auto_mano.set_modo(self.mano_modo_var.get())

    def _toggle_auto_mano(self):
        if self.auto_mano.active:
            self.auto_mano.stop()
            self.mano_auto_btn.configure(text="Iniciar modo automatico")
            self._set_sliders_state(self.mano_sliders, "normal")
            self.log("Mano: modo automatico detenido")
        else:
            self.auto_mano.start(self.mano_modo_var.get())
            self.mano_auto_btn.configure(text="Detener modo automatico")
            self._set_sliders_state(self.mano_sliders, "disabled")

    def _toggle_auto_cuello(self):
        if self.auto_cuello.active:
            self.auto_cuello.stop()
            self.cuello_auto_btn.configure(text="Iniciar modo automatico")
            self._set_sliders_state(self.cuello_sliders, "normal")
            self.log("Cuello: modo automatico detenido")
        else:
            self.auto_cuello.start()
            self.cuello_auto_btn.configure(text="Detener modo automatico")
            self._set_sliders_state(self.cuello_sliders, "disabled")

    def _toggle_auto_ojos(self):
        if self.auto_ojos.active:
            self.auto_ojos.stop()
            self.ojos_auto_btn.configure(text="Iniciar modo automatico")
            self._set_sliders_state(self.ojos_sliders, "normal")
            self.log("Ojos: modo automatico detenido")
        else:
            self.auto_ojos.start()
            self.ojos_auto_btn.configure(text="Detener modo automatico")
            self._set_sliders_state(self.ojos_sliders, "disabled")

    # ---------------- log / consola ----------------
    def log(self, msg):
        self.console.configure(state="normal")
        self.console.insert("end", msg + "\n")
        self.console.see("end")
        self.console.configure(state="disabled")

    # ---------------- bucle maestro (equivalente a loop()) ----------------
    def _master_loop(self):
        t = now_ms()
        self.auto_ojos.update(t)
        self.auto_cuello.update(t)
        self.auto_mano.update(t)
        self.after(15, self._master_loop)

    # ---------------- dibujo ----------------
    def _redraw(self):
        c = self.canvas
        c.delete("all")
        w = int(c.winfo_width() or 680)
        h = int(c.winfo_height() or 520)

        self._draw_head_and_eyes(c, w, h)
        self._draw_arm_and_hand(c, w, h)

    def _draw_head_and_eyes(self, c, w, h):
        v = self.vars
        incl_izq = v["cuello_incl_izq"].get()
        incl_der = v["cuello_incl_der"].get()
        rot = v["cuello_rotacion"].get()

        # Offsets visuales derivados de los angulos del cuello
        tilt_offset = (incl_der - incl_izq) * 1.4      # inclinacion lateral -> desplaza la cabeza en X
        rot_offset = (rot - NEUTRO) * 1.6               # rotacion -> desplaza tambien en X (vista frontal)
        head_cx = w * 0.28 + tilt_offset + rot_offset
        head_cy = 110
        head_r = 62

        # Cuerpo / cuello
        body_top_x, body_top_y = w * 0.28, 230
        c.create_line(body_top_x, body_top_y, head_cx, head_cy + head_r - 6,
                       fill="#5577aa", width=10, capstyle="round")
        c.create_rectangle(w * 0.28 - 55, 230, w * 0.28 + 55, 340, fill="#26364a", outline="")

        # Cabeza
        c.create_oval(head_cx - head_r, head_cy - head_r, head_cx + head_r, head_cy + head_r,
                       fill="#f2c79c", outline="#8a5a35", width=2)

        # Ojos
        eye_dx = 26
        eye_r = 15
        for side, sign in (("izq", -1), ("der", 1)):
            ex = head_cx + sign * eye_dx
            ey = head_cy - 4
            c.create_oval(ex - eye_r, ey - eye_r, ex + eye_r, ey + eye_r, fill="white", outline="#333")

            # pupila desplazada segun mirada
            vfrac = (v["ojos_vertical"].get() - OJOS_VERTICAL_CENTRO) / (OJOS_VERTICAL_ABAJO - OJOS_VERTICAL_CENTRO)
            hfrac = (v["ojos_horizontal"].get() - OJOS_HORIZONTAL_CENTRO) / (OJOS_HORIZONTAL_DERECHA - OJOS_HORIZONTAL_CENTRO)
            px = ex + clamp(hfrac, -1, 1) * (eye_r - 6)
            py = ey + clamp(vfrac, -1, 1) * (eye_r - 6)
            c.create_oval(px - 6, py - 6, px + 6, py + 6, fill="#1a1a1a")

            # parpados (superior e inferior) como rectangulos que cubren segun % cerrado
            sup_key = f"parp_sup_{side}"
            inf_key = f"parp_inf_{side}"
            frm, to = EYELID_RANGES[sup_key]
            pct_sup = clamp((v[sup_key].get() - frm) / (to - frm) if to != frm else 0, 0, 1)
            frm, to = EYELID_RANGES[inf_key]
            pct_inf = clamp((v[inf_key].get() - frm) / (to - frm) if to != frm else 0, 0, 1)

            cover_sup = pct_sup * (eye_r * 2)
            c.create_rectangle(ex - eye_r - 1, ey - eye_r - 1, ex + eye_r + 1, ey - eye_r + cover_sup,
                                fill="#f2c79c", outline="")
            cover_inf = pct_inf * (eye_r * 2)
            c.create_rectangle(ex - eye_r - 1, ey + eye_r + 1 - cover_inf, ex + eye_r + 1, ey + eye_r + 1,
                                fill="#f2c79c", outline="")

        # Etiquetas de cuello
        c.create_text(w * 0.28, 355, fill="#cddcee",
                       text=f"Cuello  incl.izq={incl_izq}  incl.der={incl_der}  rot={rot}",
                       font=("Consolas", 9))

    def _draw_arm_and_hand(self, c, w, h):
        v = self.vars
        ox, oy = w * 0.68, 140   # hombro
        upper_len = 90
        c.create_line(ox, oy, ox, oy + upper_len, fill="#5577aa", width=14, capstyle="round")

        codo = v["codo"].get()  # 0 abajo(extendido) .. 180 arriba(flexionado)
        theta = math.radians(codo)  # 0 -> recto hacia abajo, 180 -> doblado hacia arriba
        forearm_len = 100
        ex, ey = ox, oy + upper_len
        fx = ex + forearm_len * math.sin(theta)
        fy = ey + forearm_len * math.cos(theta)
        c.create_line(ex, ey, fx, fy, fill="#6688bb", width=12, capstyle="round")
        c.create_oval(ex - 6, ey - 6, ex + 6, ey + 6, fill="#334", outline="")

        # Muneca: dibujada como un pequeno dial (arco) en la muneca
        muneca = v["muneca"].get()
        wrist_ang = math.radians((muneca - 90) * 0.6)
        palm_dir = theta + wrist_ang
        palm_len = 40
        px = fx + palm_len * math.sin(palm_dir)
        py = fy + palm_len * math.cos(palm_dir)
        c.create_line(fx, fy, px, py, fill="#f2c79c", width=16, capstyle="round")
        c.create_text(fx + 14, fy - 4, fill="#cddcee", text=f"Muneca:{muneca}", font=("Consolas", 8), anchor="w")

        # Dedos como barras cuya altura representa apertura (0 cerrado .. 180 abierto)
        fingers = [
            ("Pulgar", v["pulgar_flexion"].get(), v["pulgar_vertical"].get()),
            ("Indice", v["indice"].get(), None),
            ("Corazon", v["corazon"].get(), None),
            ("An+Men", v["anular_menique"].get(), None),
        ]
        base_x = px - 70
        base_y = py + 30
        bar_w = 30
        gap = 10
        for i, (nombre, angulo, extra) in enumerate(fingers):
            bx = base_x + i * (bar_w + gap)
            max_h = 70
            height = max_h * (angulo / 180.0)
            color = "#7fd47f" if angulo > 120 else ("#e0c060" if angulo > 60 else "#e07f7f")
            c.create_rectangle(bx, base_y - max_h, bx + bar_w, base_y, outline="#555")
            c.create_rectangle(bx, base_y - height, bx + bar_w, base_y, fill=color, outline="")
            c.create_text(bx + bar_w / 2, base_y + 12, text=nombre, fill="#cddcee", font=("Consolas", 7))
            c.create_text(bx + bar_w / 2, base_y - height - 8, text=str(angulo), fill="white", font=("Consolas", 7))
            if extra is not None:
                estado = "arriba" if extra > 90 else "plano"
                c.create_text(bx + bar_w / 2, base_y + 24, text=f"vert:{extra}({estado})",
                               fill="#9fb", font=("Consolas", 6))

        c.create_text(w * 0.68, base_y + 45, fill="#cddcee",
                       text=f"Codo={codo}  (0=abajo/extendido, 180=arriba/flexionado)",
                       font=("Consolas", 9))


def main():
    app = HumanoidSimulatorApp()
    app.mainloop()


if __name__ == "__main__":
    main()
