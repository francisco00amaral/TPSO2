#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "../AirPlane/Airplane.h"

typedef struct {
	TCHAR airportDestiny[100];
	TCHAR airportOrigin[100];
	TCHAR name[100];
	DWORD airplane;
	BOOL flying;
	int timeMaxWaiting; //Tempo máximo que o passageiro vai esperar
	BOOL pedidoSaida;
	BOOL pedidoEntrada;
	BOOL answer;
} Passanger;


typedef struct {
	Passanger pass;
	HANDLE pipeControlToPass;
	HANDLE pipePassToControl;
} DataPassenger;

DWORD WINAPI verificaTimeout(LPVOID params);
DWORD WINAPI threadMenu(LPVOID params);
BOOL compare(TCHAR* arg1, TCHAR* arg2);