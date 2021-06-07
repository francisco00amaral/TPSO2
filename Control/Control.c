#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <fcntl.h>
#include <math.h>
#include "../AirPlane/Airplane.h"
#include "Control.h"

BOOL compare(TCHAR* arg1, TCHAR* arg2) {
	if (_strcmpi(arg1, arg2) == 0) {
		return true;
	}
	return false;
}

int createKeyAirplane() {
	HKEY key;
	TCHAR key_name[30] = TEXT("SOFTWARE\\Airplanes");
	DWORD last;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, key_name, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &last) != ERROR_SUCCESS) {
		_tprintf(TEXT("Error creating registry key.); \n"));
		return -1;
	}


	TCHAR pair_name[30] = TEXT("Max airplanes");
	// TCHAR pair_value[30] = TEXT("20");
	int pair_value = 5;

	/*
	LSTATUS RegSetValueExA(
	  HKEY       hKey,
	  LPCSTR     lpValueName,
	  DWORD      Reserved,
	  DWORD      dwType,
	  const BYTE *lpData,
	  DWORD      cbData
	);
	*/

	if (RegSetValueEx(key, pair_name, 0, REG_DWORD, (const BYTE*)&pair_value, sizeof(pair_value)) != ERROR_SUCCESS) {
		_tprintf(TEXT("Error accessing key.\n"));
		return -1;
	}

	int updated;
	int tam = sizeof(int);
	RegQueryValueEx(key, pair_name, 0, NULL, (LPBYTE)&updated, &tam);

	RegCloseKey(key);

	return updated;
}

int createKeyAirport() {
	HKEY key;
	TCHAR key_name[30] = TEXT("SOFTWARE\\Airport");
	DWORD last;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, key_name, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &last) != ERROR_SUCCESS) {
		_tprintf(TEXT("Error creating registry key.); \n"));
		return -1;
	}

	TCHAR pair_name[30] = TEXT("Max airports");
	int pair_value = 20;

	/*
	LSTATUS RegSetValueExA(
	  HKEY       hKey,
	  LPCSTR     lpValueName,
	  DWORD      Reserved,
	  DWORD      dwType,
	  const BYTE *lpData,
	  DWORD      cbData
	);
	*/

	if (RegSetValueEx(key, pair_name, 0, REG_DWORD, (const BYTE*)&pair_value, sizeof(pair_value)) != ERROR_SUCCESS) {
		_tprintf(TEXT("Error accessing key.\n"));
		return -1;
	}

	int updated;
	int tam = sizeof(int);
	RegQueryValueEx(key, pair_name, 0, NULL, (LPBYTE)&updated, &tam);

	RegCloseKey(key);

	return updated;
}



DWORD WINAPI waitingPassInfoThread(LPVOID params) {
	AerialSpace* data = (AerialSpace*)params;


	//Está à espera de info até dizerem para terminar
	while(data->endThreadReceiveInfo == false) {
	}
	return 0;
}


DWORD WINAPI compareTime(LPVOID params) {
	AerialSpace* dados = (AerialSpace*)params;
	
	while (!dados->terminar) {
		HANDLE hTimer = NULL;
		LARGE_INTEGER liDueTime;

		liDueTime.QuadPart = -35000000LL;

		// Create an unnamed waitable timer.
		hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

		// Set a timer to wait for 3(+/-) seconds.
		SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);

		// Wait for the timer.
		WaitForSingleObject(hTimer, INFINITE);
		// get local time
		time_t tm;
		struct tm* timeinfo;
		time(&tm);
		timeinfo = localtime(&tm);

		int minutesSeconds = (timeinfo->tm_min * 60);
		int hoursSeconds = (timeinfo->tm_hour * 60 * 60);
		int sum = timeinfo->tm_sec + minutesSeconds + hoursSeconds; // (da - me o total de segundos);
		// _tprintf(TEXT("Dentro da thread do tempo a soma atual foi de %d"), sum);
		
		for (int i = 0; i < dados->nAirPlanes; i++) {
			if (sum - dados->airPlanes[i].seconds > 3){
				_tprintf(TEXT("Perdeu-se conexao com o aviao com o id %d, a eliminar da lista de aviões!"),dados->airPlanes[i].id);
				// apagar do array
				deleteAirplane(&(*dados),dados->airPlanes[i]);
			}
		}
	}

	return 0;
}


DWORD WINAPI ThreadConsumidor(LPVOID params){
	AerialSpace* dados = (AerialSpace*)params;
	Ping pong;

	while (!dados->terminar) {
		//aqui entramos na logica da aula teorica
		//esperamos por uma posicao para lermos
		WaitForSingleObject(dados->hSemLeitura, INFINITE);

		//esperamos que o mutex esteja livre
		WaitForSingleObject(dados->hMutex, INFINITE);

		//vamos copiar da proxima posicao de leitura do buffer circular para a nossa variavel pong
		CopyMemory(&pong, &dados->memPar->buffer[dados->memPar->posL], sizeof(Ping));

		dados->memPar->posL++; //incrementamos a posicao de leitura para o proximo consumidor ler na posicao seguinte
		//se apos o incremento a posicao de leitura chegar ao fim, tenho de voltar ao inicio
		if (dados->memPar->posL == TAM_BUFFER)
			dados->memPar->posL = 0;

		findAirplane(&(*dados),pong);

		//libertamos o mutex
		ReleaseMutex(dados->hMutex);
		//libertamos o semaforo. temos de libertar uma posicao de escrita
		ReleaseSemaphore(dados->hSemEscrita, 1, NULL);
		// _tprintf(TEXT("Consumi %d na segundo %d.\n"),pong.id, pong.seconds);
	}

	return 0;
}

void deleteAirplane(AerialSpace* data,AirPlane aviao) {
	for (int i = 0; i < data->nAirPlanes; i++) {
		if (data->airPlanes[i].id == aviao.id) {
			data->airPlanes[i] = data->airPlanes[data->nAirPlanes + 1];
			(data->nAirPlanes)--;
		}
	}
}

void findAirplane(AerialSpace *data,Ping pong) {
	for (int i = 0; i < data->nAirPlanes; i++) {
		if (data->airPlanes[i].id == pong.id) {
			data->airPlanes[i].seconds = pong.totalseconds;
		}
	}
}

DWORD WINAPI refreshData(LPVOID params) {
	AerialSpace* data = (AerialSpace*)params;

	while (data->endThreadReceiveInfo == false) {
		WaitForSingleObject(data->hEvent3, INFINITE);

		Map map;
		if (data->endThreadReceiveInfo == true) {
			break;
		}
		map.tam = 0;
		for (int i = 0; i < data->nAirPlanes; i++) {
			if (data->airPlanes[i].flying == true) {
				map.tam = map.tam + 1;
			}
		}

		int j = 0;
		for (int i = 0; i < data->nAirPlanes; i++) {
			if (data->airPlanes[i].flying == true) {
				map.posBusy[j].coordenates.x = data->airPlanes[i].coordenates.x;
				map.posBusy[j].coordenates.y = data->airPlanes[i].coordenates.y;
				map.posBusy[j].id = data->airPlanes[i].id;
				j++;
			}
		}
		
		CopyMemory(data->mapMemory, &map, sizeof(Map));
		printPos(map);
		ResetEvent(data->hEvent3);
	}
}

void printPos(Map data) {
	for (int i = 0; i < data.tam; i++) {
		_tprintf(TEXT("Id: %d, Pos x: %d, Pos y: %d\n"), data.posBusy[i].id, data.posBusy[i].coordenates.x, data.posBusy[i].coordenates.y);
	}

}


DWORD WINAPI waitingAirplaneInfoThread(LPVOID params) {
	AerialSpace* data = (AerialSpace*)params;
	AirPlane es;
	while (data->endThreadReceiveInfo == false) {
		WaitForSingleObject(data->hEvent, INFINITE);
		if (data->endThreadReceiveInfo == true) {
			break;
		}
		// esperar que o mutex esteja livre
		WaitForSingleObject(data->hMutex, INFINITE);

		CopyMemory(&es, data->airPlaneMemory, sizeof(AirPlane));
		
		if (es.startingLife == true) {
			int aux = 0; 
			if (data->maxAirPlanes == data->nAirPlanes)
				aux--;
			if (aux == 0) {
				for (int i = 0; i < data->nAirports; i++) {
					if (_strcmpi(data->airports[i].name, es.InitialAirport) == 0) {
						aux++;
						es.coordenates.x = data->airports[i].coordenates.x;
						es.coordenates.y = data->airports[i].coordenates.y;
						break;
					}
				}
			}
			if (aux <= 0) // nao existe o aeroporto ou nao ha espaço para mais avioes
				es.answer = false;
			else {
				es.answer = true;
				addAirPlane(&(*data), es);
			}
		}

		else if (es.refreshData == true) {
			for (int i = 0; i < data->nAirPlanes; i++) {
				if (data->airPlanes[i].id == es.id) {
					data->airPlanes[i].coordenates.x = es.coordenates.x;
					data->airPlanes[i].coordenates.y = es.coordenates.y;
				}
			}
			SetEvent(data->hEvent3);
		}
		else if(es.requestDestiny == true) {

			int aux = 0;
			for(int i = 0; i < data->nAirports; i++) {
				if(_strcmpi(data->airports[i].name, es.airportDestiny.name) == 0){
					aux++;
					es.airportDestiny.coordenates.x = data->airports[i].coordenates.x;
					es.airportDestiny.coordenates.y = data->airports[i].coordenates.y;
					_tprintf(TEXT("\n%d %d\n"), es.airportDestiny.coordenates.x, es.airportDestiny.coordenates.y);
				}
			}
			if (aux <= 0) // nao existe o aeroporto
				es.answer = false;
			else{
				es.answer = true;
			}
		}
		else if (es.requestFlying == true) {
						
			for (int i = 0; i < data->nAirPlanes; i++) {
				if (es.id == data->airPlanes[i].id)
					data->airPlanes[i].flying = true;
			}

			SetEvent(data->hEvent3);
		}

		ZeroMemory(data->airPlaneMemory, sizeof(AirPlane));
		CopyMemory(data->airPlaneMemory, &es, sizeof(AirPlane));
		SetEvent(data->hEvent2);
		ResetEvent(data->hEvent);

			
		ReleaseMutex(data->hMutex);
	}
}

void addAirPlane(AerialSpace* data, AirPlane ap) {
	data->airPlanes[data->nAirPlanes].capacity = ap.capacity;
	data->airPlanes[data->nAirPlanes].coordenates.x = ap.coordenates.x;
	data->airPlanes[data->nAirPlanes].coordenates.y = ap.coordenates.y;
	data->airPlanes[data->nAirPlanes].id = ap.id;
	data->airPlanes[data->nAirPlanes].flying = ap.flying;
	data->airPlanes[data->nAirPlanes].speed = ap.speed;
	data->airPlanes[data->nAirPlanes].tm = ap.tm;
	_tcscpy_s(data->airPlanes[data->nAirPlanes].InitialAirport, 100, ap.InitialAirport);
	data->nAirPlanes++;
}

BOOL createAirport(AerialSpace* data, TCHAR* nome, Coordenates coordenates) {
	int aux = 0;
	Airport ap;
	if (data->maxAirports == data->nAirports)
		return false;
	if (coordenates.y > 1000 || coordenates.y < 0 || coordenates.x > 1000 || coordenates.x < 0) {
		_tprintf(TEXT("Coordenate cannot be higher than 1000 or lower than 0"));
		return false;
	}

	for (int i = 0; i < data->nAirports; i++) {
		if (_tcscmp(data->airports[i].name, nome) == 0) {
			aux++;
			break;
		}
		int dis = (int) sqrt(((coordenates.x - data->airports[i].coordenates.x) * (coordenates.x - data->airports[i].coordenates.x)) +
			((coordenates.y - data->airports[i].coordenates.y) * (coordenates.y - data->airports[i].coordenates.y)));
		if (dis <= 10) {
			aux++;
			break;
		}
	}

	if (aux > 0)
		return false;

	_tcscpy_s(ap.name, 100, nome);
	ap.coordenates.x = coordenates.x;
	ap.coordenates.y = coordenates.y;

	data->airports[data->nAirports] = ap;
	data->nAirports++;

	return true;
}

void listAirports(Airport ap[], int nAirports) {
	for (int i = 0; i < nAirports; i++) {
		_tprintf(TEXT("Nome: %s\n"), ap[i].name);
		_tprintf(TEXT("Coordenadas: %d, %d\n\n"), ap[i].coordenates.x, ap[i].coordenates.y);
	}
}

void listAirPlanes(AirPlane ap[], int nAirplanes) {
	for (int i = 0; i < nAirplanes; i++) {
		_tprintf(TEXT("Aviao com o id de: %d\n"), ap[i].id);
		_tprintf(TEXT("Esta no aeroporto: %s\n"), ap[i].InitialAirport);
		_tprintf(TEXT("Coordenadas: %d, %d\n\n"), ap[i].coordenates.x, ap[i].coordenates.y);
	}
}

void listAllCommands() {
	_tprintf(TEXT("Comandos disponíveis:\n"));
	_tprintf(TEXT("Criar aeroporto: ----- "));
	_tprintf(TEXT("createAirport [nome] [coordenada x] [coordenada y]\n\n"));
	_tprintf(TEXT("Listar tudo: ----- "));
	_tprintf(TEXT("list\n\n"));
	_tprintf(TEXT("Sair: ----- "));
	_tprintf(TEXT("exit\n\n"));
}


int _tmain(int argc, LPTSTR argv[]) {

#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
#endif

	//Verificação de existir apenas uma instancia
	HANDLE semaphore;
	if (semaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, TEXT("Semaphore")) != NULL) {
		_tprintf(TEXT("Já existe um programa a correr"));
		return -1;
	}

	//Semaforo para controlar que só existe um control
	semaphore = CreateSemaphore(NULL, 1, 1, TEXT("Semaphore"));
	if(semaphore == NULL){
		_tprintf(TEXT("Creating semaphore failed.\n"));
		return -1;
	}
	

	//Criação das variaveis
	AerialSpace aerialSpace;
	aerialSpace.maxAirPlanes = createKeyAirplane();
	aerialSpace.maxAirports = createKeyAirport();

	aerialSpace.airports = malloc(aerialSpace.maxAirports * sizeof(Airport)); // aloco memoria para max aeroportos;
	if (aerialSpace.airports == NULL) {
		_tprintf(TEXT("Memory allocation failed.\n"));
		// fechar handles dps
		return -1;
	}

	aerialSpace.airPlanes = malloc(aerialSpace.maxAirPlanes * sizeof(AirPlane));
	if (aerialSpace.airPlanes == NULL) {
		_tprintf(TEXT("Memory allocation failed.\n"));
		// fechar handles dps
		return -1;
	}

	aerialSpace.nAirPlanes = 0;
	aerialSpace.nAirports = 0;


	TCHAR smName[] = TEXT("Shared Memory Control/Airplane");
	TCHAR smName2[] = TEXT("Shared Memory Map");

	// SEMAFORO PARA GARANTIR QUE SO HA X AVIOES
	HANDLE maxAirplaneSemaphore;
	//0 porque nao ha nada para ser lido e depois podemos ir até um maximo de max airplanes definidos na estrutura para serem lidas
	maxAirplaneSemaphore = CreateSemaphore(NULL, aerialSpace.maxAirPlanes, aerialSpace.maxAirPlanes, TEXT("Max airplanes"));

	if(semaphore == NULL){
		_tprintf(TEXT("Creating semaphore failed.\n"));
		return -1;
	}

	//Criação dos eventos

	aerialSpace.hEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("Evento"));
	if (aerialSpace.hEvent == NULL) {
		_tprintf(TEXT("Erro a criar o evento"));
		return -1;
	}

	aerialSpace.hEvent2 = CreateEvent(NULL, TRUE, FALSE, TEXT("Evento2"));
	if (aerialSpace.hEvent2 == NULL) {
		_tprintf(TEXT("Erro a criar o evento"));
		return -1;
	}

	aerialSpace.hEvent3 = CreateEvent(NULL, TRUE, FALSE, TEXT("Evento3"));
	if (aerialSpace.hEvent3 == NULL) {
		_tprintf(TEXT("Erro a criar o evento"));
		return -1;
	}


	aerialSpace.hMutex = CreateMutex(NULL, FALSE, TEXT("SO2_MUTEX"));
	if(aerialSpace.hMutex == NULL){
		_tprintf(TEXT("Erro a criar o mutex"));
		return -1;
	}

	aerialSpace.aerialMapObjMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Map), smName2);

	if (aerialSpace.aerialMapObjMap == NULL) {
		_tprintf(TEXT("Erro a criar o mapeamento do ficheiro\n"));
		return -1;
	}

	aerialSpace.mapMemory = (Map*)MapViewOfFile(aerialSpace.aerialMapObjMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (aerialSpace.mapMemory == NULL) {
		_tprintf(TEXT("Erro a mapear o ficheiro\n"));
		CloseHandle(aerialSpace.aerialMapObjMap);
		return -1;
	}

	aerialSpace.airPlaneObjMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(AirPlane), smName);

	if (aerialSpace.airPlaneObjMap == NULL) {
		_tprintf(TEXT("Erro a criar o mapeamento do ficheiro\n"));
		return -1;
	}

	aerialSpace.airPlaneMemory = (AirPlane*) MapViewOfFile(aerialSpace.airPlaneObjMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (aerialSpace.airPlaneMemory == NULL) {
		_tprintf(TEXT("Erro a mapear o ficheiro\n"));
		CloseHandle(aerialSpace.airPlaneObjMap);
		return -1;
	}


	_tprintf(TEXT("Max airplanes: %d\n"), aerialSpace.maxAirPlanes);
	aerialSpace.endThreadReceiveInfo = false;
	//Criação de threads
	HANDLE hThread;
	DWORD idThread;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)waitingPassInfoThread, &aerialSpace, 0, &idThread);
	if (hThread == NULL) {
		_tprintf(TEXT("Erro a criar a thread com id: %d"), idThread);
		return -1;
	}

	HANDLE hThread2;
	DWORD idThread2;
	hThread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)waitingAirplaneInfoThread, &aerialSpace, 0, &idThread2);
	if (hThread2 == NULL) {
		_tprintf(TEXT("Erro a criar a thread com id: %d"), idThread2);
		return -1;
	}

	HANDLE hThread3;
	DWORD idThread3;
	hThread3 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)refreshData, &aerialSpace, 0, &idThread3);
	if (hThread3 == NULL) {
		_tprintf(TEXT("Erro a criar a thread com id: %d"), idThread3);
		return -1;
	}

	HANDLE hThread5;
	DWORD idThread5;
	hThread5 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)compareTime, &aerialSpace, 0, &idThread5);
	if (hThread5 == NULL) {
		_tprintf(TEXT("Erro a criar a thread com id: %d"), idThread5);
		return -1;
	}

	aerialSpace.hSemEscrita = CreateSemaphore(NULL, TAM_BUFFER, TAM_BUFFER, TEXT("SO2_SEMAFORO_ESCRITA"));

	//criar semaforo que conta as leituras
	//0 porque nao ha nada para ser lido e depois podemos ir até um maximo de 10 posicoes para serem lidas
	aerialSpace.hSemLeitura = CreateSemaphore(NULL, 0, TAM_BUFFER, TEXT("SO2_SEMAFORO_LEITURA"));
	//criar mutex para os produtores
	aerialSpace.hMutexBuffer = CreateMutex(NULL, FALSE, TEXT("SO2_MUTEX_CONSUMIDOR"));

	if (aerialSpace.hSemEscrita == NULL || aerialSpace.hSemLeitura == NULL || aerialSpace.hMutexBuffer == NULL) {
		_tprintf(TEXT("Erro no CreateSemaphore ou no CreateMutex\n"));
		return -1;
	}

	//o openfilemapping vai abrir um filemapping com o nome que passamos no lpName
	//se devolver um HANDLE ja existe e nao fazemos a inicializacao
	//se devolver NULL nao existe e vamos fazer a inicializacao
	aerialSpace.hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(BufferCircular), TEXT("SO2_MEM_BUFFER"));
	//mapeamos o bloco de memoria para o espaco de enderaçamento do nosso processo
	aerialSpace.memPar = (BufferCircular*)MapViewOfFile(aerialSpace.hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (aerialSpace.memPar == NULL) {
		_tprintf(TEXT("Erro no MapViewOfFile\n"));
		return -1;
	}


	aerialSpace.memPar->posE = 0;
	aerialSpace.memPar->posL = 0;
	aerialSpace.terminar = 0;

	// lancamos a thread
	HANDLE hThread4;
	DWORD idThread4;
	hThread4 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadConsumidor, &aerialSpace, 0, &idThread4);
	if (hThread4 == NULL) {
		_tprintf(TEXT("Error a criar a thread com o id %d"), idThread4);
	}

	_tprintf(TEXT("[help] para mostrar todos os comandos disponíveis.\n\n"));

	while (1) {
		TCHAR command[256];
		_fgetts(command, 256, stdin);
		command[_tcslen(command) - 1] = '\0';
		//Mete tudo para minusculas para comparar
		for (int i = 0; i < _tcslen(command); i++) {
			command[i] = tolower(command[i]);

		}
		//Menu
		
		if (compare(command, TEXT("createairport"))) {
			TCHAR name[200];
			Coordenates coo;
			_tprintf(TEXT("Nome do aeroporto: "));
			_fgetts(name, 200, stdin);
			name[_tcslen(name) - 1] = '\0';
			_tprintf(TEXT("Coordenadas (x y): "));
			_tscanf_s(TEXT("%d %d"), &coo.x, &coo.y);
			createAirport(&aerialSpace, name, coo);
		}
		else if (compare(command, TEXT("list"))){
			listAirports(aerialSpace.airports, aerialSpace.nAirports);
			listAirPlanes(aerialSpace.airPlanes, aerialSpace.nAirPlanes);
		}
		else if (compare(command, TEXT("help")))
			listAllCommands();
		else if (compare(command, TEXT("exit")))
			break;
		else
			_tprintf(TEXT("Comando inválido\n"));
	}

	//Esperar terminar tudo
	free(aerialSpace.airports);
	free(aerialSpace.airPlanes);
	ReleaseSemaphore(semaphore, 1, NULL);
	aerialSpace.endThreadReceiveInfo = true;
	SetEvent(aerialSpace.hEvent);
	SetEvent(aerialSpace.hEvent3);
	WaitForSingleObject(hThread, INFINITE);
	WaitForSingleObject(hThread2, INFINITE);
	WaitForSingleObject(hThread3, INFINITE);
	WaitForSingleObject(hThread4, INFINITE);
	WaitForSingleObject(hThread5, INFINITE);
	CloseHandle(semaphore);
	UnmapViewOfFile(aerialSpace.mapMemory);
	UnmapViewOfFile(aerialSpace.memPar);
	CloseHandle(aerialSpace.aerialMapObjMap);
	CloseHandle(hThread);
	CloseHandle(hThread2);
	CloseHandle(hThread3);
	return 0;
}