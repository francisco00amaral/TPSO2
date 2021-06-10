#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>


typedef struct {
	TCHAR airportDestiny[100];
	TCHAR airportOrigin[100];
	TCHAR name[100];
	BOOL flying;
	int timeMaxWaiting; //Tempo máximo que o passageiro vai esperar
} Passanger;


typedef struct {
	Passanger pass;
} DataPassenger;