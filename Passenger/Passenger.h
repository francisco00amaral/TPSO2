#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>



typedef struct {
	TCHAR AirportDestiny[100];
	TCHAR name[100];
	bool Flying;
	int TimeMaxWaiting; //Tempo máximo que o passageiro vai esperar
} Passanger;