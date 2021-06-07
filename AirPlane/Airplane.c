#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <windows.h>
#include <fcntl.h>
#include "../SO2_TP_DLL_2021/SO2_TP_DLL_2021.h"
#include "Airplane.h"

// fazer logica

BOOL compare(TCHAR* arg1, TCHAR* arg2) {
	if (_strcmpi(arg1, arg2) == 0) {
		return true;
	}

	return false;
}

DWORD WINAPI ThreadProdutor(LPVOID params){
	Data* dados = (Data*)params;
	Ping pong; // estrutura que vai guardar o id do avião, e o seu timestamp
	
	 //esperamos que o mutex esteja livre
	while (!dados->terminar) {
		pong.id = dados->airplane.id;
		time(&pong.tm);
		pong.timeinfo = localtime(&pong.tm);
		pong.seconds = pong.timeinfo->tm_sec;
		pong.minutes = pong.timeinfo->tm_min;
		pong.hours = pong.timeinfo->tm_hour;
		pong.totalseconds = (pong.seconds + (pong.minutes * 60) + (pong.hours * 60 * 60));

		//esperamos por uma posicao para escrevermos
		WaitForSingleObject(dados->hSemEscrita, INFINITE);

		WaitForSingleObject(dados->hMutexBuffer, INFINITE);
		//vamos copiar a variavel cel para a memoria partilhada (para a posição de escrita)
		CopyMemory(&dados->memPar->buffer[dados->memPar->posE], &pong, sizeof(Ping));

		dados->memPar->posE++; //incrementamos a posicao de escrita para o proximo produtor escrever na posicao seguinte

		//se apos o incremento a posicao de escrita chegar ao fim, tenho de voltar ao inicio
		if (dados->memPar->posE == TAM_BUFFER)
			dados->memPar->posE = 0;
		
		//libertamos o mutex

		ReleaseMutex(dados->hMutexBuffer);
		//libertamos o semaforo. temos de libertar uma posicao de leitura
		ReleaseSemaphore(dados->hSemLeitura, 1, NULL);

		// _tprintf(TEXT("Produzi Aviao %d no segundo %d "),pong.id,pong.totalseconds);
	}

	return 0;
}

void listAllCommands() {
	_tprintf(TEXT("Comandos disponíveis:\n"));
	_tprintf(TEXT("Definir um novo destino: ----- "));
	_tprintf(TEXT("destiny [nome]\n\n"));
	_tprintf(TEXT("Embarcar todos os passageiros que estiverem no aeroporto atual: ----- "));
	_tprintf(TEXT("board\n\n"));
	_tprintf(TEXT("Começar a voar: WARNING - Enquanto está em viagem não tem acesso a mais nenhum comando.----- "));
	_tprintf(TEXT("fly\n\n"));
	_tprintf(TEXT("Sair: ----- "));
	_tprintf(TEXT("exit\n\n"));
}

void resetRequests(AirPlane* ap) {
	ap->boarding = false;
	ap->exit = false;
	ap->refreshData = false;
	ap->startingLife = false;
	ap->requestDestiny = false;
	ap->requestFlying = false;
}

void startFlying(Data* data) {
	while(1) {
		Sleep(1000);
		int num;
		int aux1 = 0;
		for (int i = 0; i < data->airplane.speed; i++) {
			// : int move(int cur_x, int cur_y, int final_dest_x, int final_dest_y, int *next_x, int * next_y);

			num = move(data->airplane.coordenates.x,
				data->airplane.coordenates.y,
				data->airplane.airportDestiny.coordenates.x,
				data->airplane.airportDestiny.coordenates.y,
				&data->airplane.coordenates.x,
				&data->airplane.coordenates.y);
			if (num == 0) {
				aux1 = 1;
				break;
			}
		}

		if (aux1 == 1) {
			_tprintf(TEXT("O avião chegou ao destino\n"));
			break;
		}
		_tprintf(TEXT("%d %d"), data->airplane.coordenates.x, data->airplane.coordenates.y);
		int aux;
		do {
			aux = 1;
			for (int i = 0; i < data->memoryMap->tam; i++) {
				if (data->airplane.coordenates.x == data->memoryMap->posBusy[i].coordenates.x &&
					data->airplane.coordenates.y == data->memoryMap->posBusy[i].coordenates.y) {
					data->airplane.coordenates.x++;
					aux = 0;
				}
			}
		} while (aux == 0);
		resetRequests(&data->airplane);
		data->airplane.refreshData = true;
		WaitForSingleObject(data->hMutexMemory, INFINITE);
		CopyMemory(data->memory, &data->airplane, sizeof(AirPlane));
		SetEvent(data->hEvent);
		ReleaseMutex(data->hMutexMemory);
	}
}


int _tmain(int argc, LPTSTR argv[]) {

#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
	(void) _setmode(_fileno(stderr), _O_WTEXT);
#endif

	// HANDLE hThread;
	 //Verificação de se o controlador não estiver aberto fecha o programa.

	HANDLE maxAirplaneSemaphore;

	maxAirplaneSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, TEXT("Max airplanes"));
	if(maxAirplaneSemaphore == NULL){
		_tprintf(TEXT("Control is not running"));
		return -1;
	}

	WaitForSingleObject(maxAirplaneSemaphore, INFINITE);

	Data data;
	TCHAR smName[] = TEXT("Shared Memory Control/Airplane");
	TCHAR smName2[] = TEXT("Shared Memory Map");

	// 1º argumento é a velocidade, 2º argumento capacidade e o 3º o nome do aeroporto onde começa
	data.airplane.speed = _ttoi(argv[1]);
	data.airplane.capacity = _ttoi(argv[2]);
	time_t t = time(NULL);
	time(&data.airplane.tm);
	_tcscpy_s(data.airplane.InitialAirport, 100, argv[3]);
	data.airplane.flying = false;
	data.airplane.exit = false;
	data.airplane.boarding = false;
	data.airplane.startingLife = true;
	data.airplane.answer = false;
	data.airplane.id = GetCurrentProcessId();

	_tprintf(TEXT("New airplane created!\n Starting airport: %s \nCapacity: %d \nSpeed: %d Process ID: %lu\n\n"), data.airplane.InitialAirport, data.airplane.capacity, data.airplane.speed,data.airplane.id);

	// menu(&data);


	data.hMutexMemory = CreateMutex(NULL, FALSE, TEXT("MUTEX_MEMORIA"));
	if (data.hMutexMemory == NULL) {
		_tprintf(TEXT("Erro a criar o mutex"));
		return -1;
	}

	data.hMapFileCom = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, smName);

	if (data.hMapFileCom == NULL) {
		_tprintf(TEXT("Erro a abrir o file mapping object\n"));
		return -1;
	}

	data.memory = (AirPlane*)MapViewOfFile(data.hMapFileCom, FILE_MAP_ALL_ACCESS, 0, 0, 0);


	if (data.memory == NULL) {
		_tprintf(TEXT("Erro a abrir o map view of file\n"));
		CloseHandle(data.hMapFileCom);
		return -1;
	}

	data.hMapFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, smName2);

	if (data.hMapFileMap == NULL) {
		_tprintf(TEXT("Erro a abrir o file mapping object\n"));
		return -1;
	}

	data.memoryMap = (Map*)MapViewOfFile(data.hMapFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);


	if (data.memoryMap == NULL) {
		_tprintf(TEXT("Erro a abrir o map view of file\n"));
		CloseHandle(data.hMapFileMap);
		return -1;
	}


	//Criação do evento

	data.hEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("Evento"));
	if (data.hEvent == NULL) {
		_tprintf(TEXT("Erro a criar o evento"));
		return -1;
	}


	data.hEvent2 = CreateEvent(NULL, TRUE, FALSE, TEXT("Evento2"));
	if (data.hEvent2 == NULL) {
		_tprintf(TEXT("Erro a criar o evento"));
		return -1;
	}


	//limpa memoria antes de fazer a copia
	WaitForSingleObject(data.hMutexMemory, INFINITE);
	ZeroMemory(data.memory, sizeof(AirPlane));

	CopyMemory(data.memory, &data.airplane, sizeof(AirPlane));
	SetEvent(data.hEvent);
	WaitForSingleObject(data.hEvent2, INFINITE);
	CopyMemory(&data.airplane, data.memory, sizeof(AirPlane));
	ResetEvent(data.hEvent2);
	ReleaseMutex(data.hMutexMemory);
	if (data.airplane.answer == false) {
		_tprintf(TEXT("Não ha esse aeroporto"));
		return -1;
	}

	// lancamos a thread do produtor que vai estar sempre a mandar os dados de 3 em 3 segundos.
	HANDLE hThread;
	data.hSemEscrita = CreateSemaphore(NULL, TAM_BUFFER, TAM_BUFFER, TEXT("SO2_SEMAFORO_ESCRITA"));
	//criar semaforo que conta as leituras
	//0 porque nao ha nada para ser lido e depois podemos ir até um maximo de 10 posicoes para serem lidas
	data.hSemLeitura = CreateSemaphore(NULL, 0, TAM_BUFFER, TEXT("SO2_SEMAFORO_LEITURA"));

	data.hMutexBuffer = CreateMutex(NULL, FALSE, TEXT("SO2_MUTEX_PRODUTOR"));

	if (data.hSemEscrita == NULL || data.hSemLeitura == NULL || data.hMutexBuffer == NULL) {
		_tprintf(TEXT("Erro no CreateMutex ou no CreateSemaphore\n"));
		return -1;
	}

	//o openfilemapping vai abrir um filemapping com o nome que passamos no lpName
	//se devolver um HANDLE ja existe e nao fazemos a inicializacao
	//se devolver NULL nao existe e vamos fazer a inicializacao

	data.hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("SO2_MEM_BUFFER"));
	if (data.hFileMap == NULL) {
		//criamos o bloco de memoria partilhada
		data.hFileMap = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			sizeof(BufferCircular), //tamanho da memoria partilhada
			TEXT("SO2_MEM_BUFFER"));//nome do filemapping. nome que vai ser usado para partilha entre processos

		if (data.hFileMap == NULL) {
			_tprintf(TEXT("Erro no CreateFileMapping\n"));
			return -1;
		}
	}

	data.memPar = (BufferCircular*)MapViewOfFile(data.hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (data.memPar == NULL) {
		_tprintf(TEXT("Erro no MapViewOfFile\n"));
		return -1;
	}

	data.memPar->posE = 0;
	data.memPar->posL = 0;
	data.terminar = 0;


	hThread = CreateThread(NULL, 0,ThreadProdutor, &data, 0, NULL);
	if (hThread == NULL) {
		_tprintf(TEXT("Erro a criar a thread com id: "));
		return -1;
	}

	while (1) {
		TCHAR command[256];
		_fgetts(command, 256, stdin);

		command[_tcslen(command) - 1] = '\0';
		fseek(stdin, 0, SEEK_END); // limpar o buffer

		//Mete tudo para minusculas para comparar
		for (unsigned int i = 0; i < _tcslen(command); i++) {
			command[i] = tolower(command[i]);
		}

		if (compare(command, TEXT("destiny"))) {
			TCHAR name[200];
			_tprintf(TEXT("Nome do próximo destino: "));
			_fgetts(name, 200, stdin);
			name[_tcslen(name) - 1] = '\0';
			// depois é mudar na estrutura o airportdestiny para este definido aqui acho eu
			_tcscpy_s(data.airplane.airportDestiny.name, 100, name);
			if (compare(name, data.airplane.InitialAirport)) {
				_tprintf(TEXT("Ja esta no aeroporto que decidiu definir como destino...\n"));
			}
			else {
				resetRequests(&data.airplane);
				data.airplane.requestDestiny = true;
				WaitForSingleObject(data.hMutexMemory, INFINITE);
				ZeroMemory(data.memory, sizeof(AirPlane));

				CopyMemory(data.memory, &data.airplane, sizeof(AirPlane));
				SetEvent(data.hEvent);
				WaitForSingleObject(data.hEvent2, INFINITE);
				CopyMemory(&data.airplane, data.memory, sizeof(AirPlane));
				if (data.airplane.answer == true) {
					_tprintf(TEXT("Proximo destino definido: %s, com as coordenadas de x: %d  y: %d\n"),
						data.airplane.airportDestiny.name,
						data.airplane.airportDestiny.coordenates.x,
						data.airplane.airportDestiny.coordenates.y);
				}
				else {
					_tprintf(TEXT("Proximo destino inexistente\n"));
				}
				ResetEvent(data.hEvent2);
				ReleaseMutex(data.hMutexMemory);
			}
		}
		else if (compare(command, TEXT("board"))) {
			// logica de meter os passageiros dentro do aviao -- ainda nao e para esta meta.
		}

		else if (compare(command, TEXT("fly"))) {
			// logica de começar a voar  - onde esta sempre a usar a dll
			data.airplane.flying = true;
			resetRequests(&data.airplane);
			data.airplane.requestFlying = true;
			WaitForSingleObject(data.hMutexMemory, INFINITE);
			ZeroMemory(data.memory, sizeof(AirPlane));

			CopyMemory(data.memory, &data.airplane, sizeof(AirPlane));
			SetEvent(data.hEvent);
			WaitForSingleObject(data.hEvent2, INFINITE);
			CopyMemory(&data.airplane, data.memory, sizeof(AirPlane));
			ResetEvent(data.hEvent2);
			ReleaseMutex(data.hMutexMemory);
			if (data.airplane.answer == true)
				startFlying(&data);
			else
				_tprintf(TEXT("Aeroporto destino nao existe\n"));
		}else if (compare(command, TEXT("help"))){
			listAllCommands();
		}
		else if (compare(command, TEXT("end"))) {
			// sai-me do ciclo e termina o programa e tem de fazer a logica.
			break;
		}
		// strcompares
	}

	

	ReleaseSemaphore(maxAirplaneSemaphore, 1, NULL);

	ResetEvent(data.hEvent);


	// Fechar as cenas todas

	UnmapViewOfFile(data.memory);
	UnmapViewOfFile(data.memPar);

	CloseHandle(data.hMapFileCom);
	CloseHandle(data.hMapFileMap);

	return 0;
}