#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <time.h>
#include "../Passenger/Passenger.h"
#include "../AirPlane/Airplane.h"
#define DIM 10

typedef struct {
	Passanger passenger;
	HANDLE pipeControlToPass;
	HANDLE pipePassToControl;
} InfoPassenger;


typedef struct {
	AirPlane* airPlanes;
	int nAirPlanes;
	int maxAirPlanes; // max de avioes
	Airport* airports;
	int nAirports;
	int maxAirports;
	//InfoPassenger 
	//Variaveis de controlo:
	BOOL endThreadReceiveInfo; //1 caso seja para terminar a thread
	HANDLE hMutex;
	HANDLE hEvent;
	HANDLE hEvent2;
	HANDLE hEvent3;
	Map* mapMemory;
	AirPlane* airPlaneMemory;
	HANDLE aerialMapObjMap;
	HANDLE airPlaneObjMap;
	// cenas para o buffer circular
	int terminar;
	BufferCircular* memPar;
	HANDLE hSemEscrita;
	HANDLE hSemLeitura;
	HANDLE hMutexBuffer;
	HANDLE hFileMap;
} AerialSpace;


BOOL compare(TCHAR* arg1, TCHAR* arg2);
int createKeyAirport();
int createKeyAirplane();
void listAirPlanes(AirPlane ap[], int nAirplanes);
void listAirports(Airport ap[], int nAirports);
void listAllCommands();
void addAirPlane(AerialSpace* data, AirPlane ap);
void printPos(Map data);
DWORD WINAPI refreshData(LPVOID params);
DWORD WINAPI waitingPassInfoThread(LPVOID data); //Thread que fica à espera da info dos passageiros
DWORD WINAPI waitingAirplaneInfoThread(LPVOID params); //Thread que fica à espera da info  dos aviões
DWORD WINAPI compareTime(LPVOID params); // thread para comparar os tempos do 3 segundos do keepalive
DWORD WINAPI ThreadConsumidor(LPVOID params); // thread do buffer circular
DWORD WINAPI menuPrincipal(LPVOID params); // menu
BOOL createAirport(AerialSpace* data, TCHAR* nome, Coordenates coordenates);
void findAirplane(AerialSpace* data);
void deleteAirplane(AerialSpace* data, AirPlane aviao);
LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TrataEventosCriarAeroporto(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TrataEventosVerAvioes(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TrataEventosVerAeroportos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
