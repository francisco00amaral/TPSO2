#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <time.h>
#define TAM_BUFFER 10

typedef struct {
	int x, y; //Coordenadas
} Coordenates;

typedef struct {
	DWORD id;
	time_t tm;
	struct tm* timeinfo;
	int seconds;
	int minutes;
	int hours;
	int totalseconds;
} Ping;

typedef struct {
	int posE;
	int posL;
	Ping buffer[TAM_BUFFER];
}BufferCircular;

typedef struct {
	Coordenates coordenates;
	int id;
} AerialAirplane;

typedef struct {
	AerialAirplane posBusy[5];
	int tam;
} Map;

typedef struct {
	Coordenates coordenates;
	TCHAR name[100];
} Airport;

typedef struct {
	DWORD id; //id do processo em si DWORD
	Coordenates coordenates;
	Airport airportDestiny;
	TCHAR InitialAirport[100];
	BOOL flying; //Se está a voar ou não 
	int capacity; //Numero máximo de passajeiros do avião
	int speed;  //Velocidade máxima do avião (posições por segundo)
	//Variaveis de pedidos
	BOOL refreshData; //Pedido para atualizar dados
	BOOL startingLife;  //Pedido no inicio do programa
	BOOL boarding;		//Pedido para iniciar viagem
	BOOL requestFlying;
	BOOL finishFly;    //Dizer que chegou ao destino
	BOOL requestDestiny;
	BOOL exit;			//Caso saia do programa
	//Variavel de resposta
	BOOL answer;
	int seconds;
} AirPlane;

typedef struct {
	AirPlane airplane;
	//HANDLES
	HANDLE hMapFileCom;
	HANDLE hMapFileMap;
	HANDLE hMutexMemory; //Mutexe para controlar os copyMemories
	AirPlane* memory;
	Map* memoryMap;
	HANDLE hEvent;
	HANDLE hEvent2;
	// DADOS PARA O BUFFER CIRCULAR
	BufferCircular* memPar;
	HANDLE hSemEscrita;
	HANDLE hSemLeitura;
	HANDLE hMutexBuffer;
	HANDLE hFileMap; // handle para o filemap do buffer circular
	int terminar;
	// int finish;
} Data;

void resetRequests(AirPlane* ap);
void configureAirplane(AirPlane* airplane); // Função que inicia e completa os dados de avião pela linha de comandos
void menu(Data* data);
void listAllCommands();