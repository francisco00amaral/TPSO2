#pragma once
#include <stdlib.h>
#include <stdio.h>
// #include <windows.h>


typedef struct {
	TCHAR AirportDestiny[100];
	TCHAR name[100];
	BOOL Flying;
	int TimeMaxWaiting; //Tempo máximo que o passageiro vai esperar
} Passanger;