#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <Windowsx.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include "resource.h"
#include "../AirPlane/Airplane.h"
#include "Control.h"
#define PIPE_CONTROL_TO_PASS TEXT("\\\\.\\pipe\\ControlToPass")
#define PIPE_PASS_TO_CONTROL TEXT("\\\\.\\pipe\\PassToControl")
HINSTANCE hInstance;
//BITMAP
// uma vez que temos de usar estas vars tanto na main como na funcao de tratamento de eventos
// nao ha uma maneira de fugir ao uso de vars globais, dai estarem aqui
HBITMAP hBmp; // handle para o bitmap
HBITMAP hBmpAeroporto;
HDC bmpDC; // hdc do bitmap
HDC bmpDCAeroporto;
BITMAP bmp; // informação sobre o bitmap
BITMAP bmpAeroporto;
int xBitmap; // posicao onde o bitmap vai ser desenhado
int yBitmap;
int xBitmapAeroporto; // posicao onde o bitmap vai ser desenhado
int yBitmapAeroporto;
int x[10];
int y[10];
int total;
int xAvioes[10];
int yAvioes[10];
int totalAvioes;

int limDir; // limite direito
int limDirAeroporto; // limite direito

HWND hWndGlobal; // handle para a janela
HANDLE hMutexBitMap;

HDC memDC = NULL; // copia do device context que esta em memoria, tem de ser inicializado a null
HBITMAP hBitmapDB; // copia as caracteristicas da janela original para a janela que vai estar em memoria

BOOL compare(TCHAR* arg1, TCHAR* arg2) {
	if (_tcscmp(arg1, arg2) == 0) {
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


DWORD WINAPI passRequestsThread(LPVOID params) {
	InfoPassenger* info = (InfoPassenger*)params;
	Passanger pass;
	DWORD n;
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("Evento4"));
	if (hEvent == NULL) {
		return -1;
	}

	HANDLE hEvent2 = CreateEvent(NULL, TRUE, FALSE, TEXT("Evento5"));
	if (hEvent2 == NULL) {
		return -1;
	}

	while (1) {
		BOOL ret = ReadFile(info->pipePassToControl, &pass, sizeof(Passanger), &n, NULL);

		if (ret && n > 0) {
			if (pass.pedidoEntrada == true) {
				info->passenger.pedidoEntrada = true;
				_tcscpy_s(info->passenger.airportOrigin, 100, pass.airportOrigin);
				_tcscpy_s(info->passenger.airportDestiny, 100, pass.airportDestiny);
				SetEvent(hEvent);
				WaitForSingleObject(hEvent2, INFINITE);
				if (info->passenger.answer == false)
					break;
				ResetEvent(hEvent2);

			}
			if (pass.pedidoSaida == true) {
				info->passenger.pedidoSaida = true;
				SetEvent(hEvent);
				break;
			}
		}
		pass.pedidoEntrada = false;
		pass.pedidoSaida = false;
	}

	CloseHandle(info->pipeControlToPass);
	CloseHandle(info->pipePassToControl);
}

DWORD WINAPI passRefreshInfo(LPVOID params) {
	AerialSpace* data = (AerialSpace*)params;
	DWORD n;
	while (data->endThreadReceiveInfo == false) {
		WaitForSingleObject(data->hEvent4, INFINITE);
		WaitForSingleObject(data->hMutex2, INFINITE);

		for (int i = 0; i < data->nPassangers; i++) {
			int aux = 0;
			if (data->infoPass[i].passenger.pedidoEntrada == true) {
				for (int j = 0; j < data->nAirports; j++) {
					if (_tcscmp(data->airports[j].name, data->infoPass[i].passenger.airportOrigin) == 0)
						aux++;
					if (_tcscmp(data->airports[j].name, data->infoPass[i].passenger.airportDestiny) == 0)
						aux++;
				}
				if (aux == 2) {
					data->infoPass[i].passenger.answer = true;
				}
				else {
					data->infoPass[i].passenger.answer = false;
					data->infoPass[i] = data->infoPass[data->nPassangers - 1];
					data->nPassangers--;
				}
				if (!WriteFile(data->infoPass[i].pipeControlToPass, &data->infoPass[i].passenger, sizeof(Passanger), &n, NULL))
					exit(-1);
				SetEvent(data->hEvent5);
			}
			if (data->infoPass[i].passenger.pedidoSaida == true) {
				aux = 3;
				data->infoPass[i] = data->infoPass[data->nPassangers - 1];
				data->nPassangers--;
			}
			if (aux == 3)
				break;
		}

		ResetEvent(data->hEvent4);
		ReleaseMutex(data->hMutex2);
	}
}


DWORD WINAPI waitingPassInfoThread(LPVOID params) {
	AerialSpace* data = (AerialSpace*)params;


	//Está à espera de info até dizerem para terminar
	while(data->endThreadReceiveInfo == false) {
		//Criação dos pipes
		data->infoPass[data->nPassangers].pipeControlToPass = CreateNamedPipe(PIPE_CONTROL_TO_PASS,
			PIPE_ACCESS_OUTBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
			PIPE_UNLIMITED_INSTANCES, sizeof(Passanger), sizeof(Passanger), 1000, NULL);
		if (data->infoPass[data->nPassangers].pipeControlToPass == INVALID_HANDLE_VALUE)
			exit(-1);

		data->infoPass[data->nPassangers].pipePassToControl = CreateNamedPipe(PIPE_PASS_TO_CONTROL,
			PIPE_ACCESS_INBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
			PIPE_UNLIMITED_INSTANCES, sizeof(Passanger), sizeof(Passanger), 1000, NULL);
		if (data->infoPass[data->nPassangers].pipePassToControl == INVALID_HANDLE_VALUE)
			exit(-1);

		if (!ConnectNamedPipe(data->infoPass[data->nPassangers].pipeControlToPass, NULL) && GetLastError() != 535)
			exit(-1);

		if (!ConnectNamedPipe(data->infoPass[data->nPassangers].pipePassToControl, NULL) && GetLastError() != 535)
			exit(-1);
		HANDLE threadInfoPass = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)passRequestsThread, &data->infoPass[data->nPassangers], 0, NULL);
		if (threadInfoPass == NULL) {
			exit(-1);
		}
		data->nPassangers++;
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
			data->airPlanes[i] = data->airPlanes[data->nAirPlanes - 1];
			data->nAirPlanes--;
			break;
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
					if (_tcscmp(data->airports[i].name, es.InitialAirport) == 0) {
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

		else if(es.finishFly == true) {
			for(int i = 0; i < data->nAirPlanes; i++) {
				if(data->airPlanes[i].id == es.id) {
					for(int j = 0; j < data->nAirports; j++) {
						if(data->airports[j].coordenates.x == data->airPlanes[i].coordenates.x && 
						data->airports[j].coordenates.y == data->airPlanes[i].coordenates.y) {
							_tcscpy_s(data->airPlanes[i].InitialAirport, 100, data->airports[j].name);
							break;
						}
					}
					break;
				}
			}

			for (int i = 0; i < data->mapMemory->tam; i++) {
				if (es.id == data->mapMemory->posBusy[i].id) {
					data->mapMemory->posBusy[i] = data->mapMemory->posBusy[data->mapMemory->tam - 1];
					data->mapMemory->tam--;
					break;
					
				}
			}
		}
		else if(es.requestDestiny == true) {

			int aux = 0;
			for(int i = 0; i < data->nAirports; i++) {
				if(_tcscmp(data->airports[i].name, es.airportDestiny.name) == 0){
					aux++;
					es.airportDestiny.coordenates.x = data->airports[i].coordenates.x;
					es.airportDestiny.coordenates.y = data->airports[i].coordenates.y;
					_tprintf(TEXT("\n%d %d\n"), es.airportDestiny.coordenates.x, es.airportDestiny.coordenates.y);
					break;
				}
			}
			if (aux <= 0) // nao existe o aeroporto
				es.answer = false;
			else {
				es.answer = true;
			}
		}
		else if (es.boarding == true) {
			int aux = 0;
			for (int i = 0; i < data->nPassangers; i++) {
				if (_tcscmp(data->infoPass[i].passenger.airportDestiny, es.airportDestiny.name) == 0 &&
					_tcscmp(data->infoPass[i].passenger.airportOrigin, es.InitialAirport) == 0) {
					if (aux < es.capacity) {
						aux++;
						data->infoPass[i].passenger.airplane = es.id;
					}
				}
			}
			es.answer = true;
		}
		else if (es.requestFlying == true) {
						
			for (int i = 0; i < data->nAirPlanes; i++) {
				if (es.id == data->airPlanes[i].id)
					data->airPlanes[i].flying = true;
			}
			es.answer = true;
			SetEvent(data->hEvent3);
		}

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
	// data->airPlanes[data->nAirPlanes].tm = ap.tm;
	_tcscpy_s(data->airPlanes[data->nAirPlanes].InitialAirport, 100, ap.InitialAirport);
	data->nAirPlanes++;
	total = data->nAirPlanes;
	xAvioes[total - 1] = ap.coordenates.x;
	yAvioes[total - 1] = ap.coordenates.y;
	HDC hdc; // representa a propria janela
	RECT rect;

	// carregar o bitmap
	hBmp = (HBITMAP)LoadImage(NULL, TEXT("bitmap1.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(bmp), &bmp); // vai buscar info sobre o handle do bitmap

	hdc = GetDC(hWndGlobal);
	// criamos copia do device context e colocar em memoria
	bmpDC = CreateCompatibleDC(hdc);
	// aplicamos o bitmap ao device context
	SelectObject(bmpDC, hBmp);
	// SelectObject(bmpDC, hBmpAeroporto);

	ReleaseDC(hWndGlobal, hdc);

	// EXEMPLO
	// 800 px de largura, imagem 40px de largura
	// ponto central da janela 400 px(800/2)
	// imagem centrada, começar no 380px e acabar no 420 px
	// (800/2) - (40/2) = 400 - 20 = 380px

	// definir as posicoes inicias da imagem
	GetClientRect(hWndGlobal, &rect);
	xBitmap = (rect.right / 2) - (bmp.bmWidth / 2);
	yBitmap = (rect.bottom / 2) - (bmp.bmHeight / 2);


	// limite direito é a largura da janela - largura da imagem
	limDir = rect.right - bmp.bmWidth;
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

// Mexe na posição x da imagem de forma a que a imagem se vá movendo
DWORD WINAPI MovimentaImagem(LPVOID lParam) {
	AerialSpace* data = (AerialSpace*)lParam;

	int dir = 1; // 1 para a direita, -1 para a esquerda
	int salto = 2; // quantidade de pixeis que a imagem salta de cada vez

	while (1) {
		// Aguarda que o mutex esteja livre
		WaitForSingleObject(hMutexBitMap, INFINITE);

		// movimentação
		xBitmap = xBitmap + (dir * salto);

		//fronteira À esquerda
		if (xBitmap <= 0) {
			xBitmap = 0;
			dir = 1;
		}
		// limite direito
		else if (xBitmap >= limDir) {
			xBitmap = limDir;
			dir = -1;
		}
		//liberta mutex
		ReleaseMutex(hMutexBitMap);

		// dizemos ao sistema que a posição da imagem mudou e temos entao de fazer o refresh da janela
		InvalidateRect(hWndGlobal, NULL, FALSE);
		Sleep(1);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	MSG lpMsg;
	WNDCLASSEX wcApp;

	wcApp.cbSize = sizeof(WNDCLASSEX);
	wcApp.hInstance = hInst;
	hInstance = hInst;

	wcApp.lpszClassName = TEXT("Control");
	wcApp.lpfnWndProc = TrataEventos;

	wcApp.style = CS_HREDRAW | CS_VREDRAW;
	AerialSpace aerialSpace;
	HANDLE semaphore;
	//Verificação de existir apenas uma instancia
	semaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, TEXT("Semaphore"));
	if (semaphore != NULL) {
		_tprintf(TEXT("Já existe um programa a correr"));
		return -1;
	}

	//Semaforo para controlar que só existe um control
	semaphore = CreateSemaphore(NULL, 1, 1, TEXT("Semaphore"));
	if (semaphore == NULL) {
		_tprintf(TEXT("Creating semaphore failed.\n"));
		return -1;
	}

	//Criação das variaveis
	aerialSpace.maxAirPlanes = createKeyAirplane();
	aerialSpace.maxAirports = createKeyAirport();
	aerialSpace.nPassangers = 0;

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

	if (semaphore == NULL) {
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

	aerialSpace.hEvent4 = CreateEvent(NULL, TRUE, FALSE, TEXT("Evento4"));
	if (aerialSpace.hEvent4 == NULL) {
		_tprintf(TEXT("Erro a criar o evento"));
		return -1;
	}

	aerialSpace.hEvent5 = CreateEvent(NULL, TRUE, FALSE, TEXT("Evento5"));
	if (aerialSpace.hEvent5 == NULL) {
		_tprintf(TEXT("Erro a criar o evento"));
		return -1;
	}


	aerialSpace.hMutex = CreateMutex(NULL, FALSE, TEXT("SO2_MUTEX"));
	if (aerialSpace.hMutex == NULL) {
		_tprintf(TEXT("Erro a criar o mutex"));
		return -1;
	}

	aerialSpace.hMutex2 = CreateMutex(NULL, FALSE, TEXT("SO2_MUTEX2"));
	if (aerialSpace.hMutex2 == NULL) {
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

	aerialSpace.airPlaneMemory = (AirPlane*)MapViewOfFile(aerialSpace.airPlaneObjMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

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

	HANDLE hThread6;
	DWORD idThread6;
	hThread6 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)waitingPassInfoThread, &aerialSpace, 0, &idThread6);
	if (hThread6 == NULL) {
		_tprintf(TEXT("Erro a criar a thread com id: %d"), idThread6);
		return -1;
	}

	HANDLE hThread7;
	DWORD idThread7;
	hThread7 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)passRefreshInfo, &aerialSpace, 0, &idThread7);
	if (hThread6 == NULL) {
		_tprintf(TEXT("Erro a criar a thread com id: %d"), idThread7);
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
	wcApp.lpszMenuName = MAKEINTRESOURCE(ID_MENU);
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcApp.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcApp.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	wcApp.cbClsExtra = sizeof(aerialSpace);
	wcApp.cbWndExtra = 0;
	wcApp.hbrBackground = CreateSolidBrush(RGB(220, 220, 220));

	if (!RegisterClassEx(&wcApp)) {
		_tprintf(TEXT("%d"),GetLastError());
		return(0);
	}

	hWnd = CreateWindow(
		TEXT("Control"),
		TEXT("Control"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1000,
		1000,
		(HWND)HWND_DESKTOP,
		(HMENU)NULL,
		(HINSTANCE)hInstance,
		0);

	HDC hdc; // representa a propria janela
	RECT rect;

	// carregar o bitmap
	hBmp = (HBITMAP)LoadImage(NULL, TEXT("bitmap1.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(bmp), &bmp); // vai buscar info sobre o handle do bitmap

	hdc = GetDC(hWnd);
	// criamos copia do device context e colocar em memoria
	bmpDC = CreateCompatibleDC(hdc);
	// aplicamos o bitmap ao device context
	SelectObject(bmpDC, hBmp);
	// SelectObject(bmpDC, hBmpAeroporto);

	ReleaseDC(hWnd, hdc);

	// EXEMPLO
	// 800 px de largura, imagem 40px de largura
	// ponto central da janela 400 px(800/2)
	// imagem centrada, começar no 380px e acabar no 420 px
	// (800/2) - (40/2) = 400 - 20 = 380px

	// definir as posicoes inicias da imagem
	GetClientRect(hWnd, &rect);
	xBitmap = (rect.right / 2) - (bmp.bmWidth / 2);
	yBitmap = (rect.bottom / 2) - (bmp.bmHeight / 2);


	// limite direito é a largura da janela - largura da imagem
	limDir = rect.right - bmp.bmWidth;
	hWndGlobal = hWnd;

	hMutexBitMap = CreateMutex(NULL, FALSE, NULL);

	// criar a thread;
	CreateThread(NULL, 0, MovimentaImagem, &aerialSpace, 0, NULL);

	ShowWindow(hWnd, nCmdShow);	// "hWnd"= handler da janela, devolvido por
					  // "CreateWindow"; "nCmdShow"= modo de exibição (p.e.
					  // normal/modal); é passado como parâmetro de WinMain()
	UpdateWindow(hWnd);		// Refrescar a janela (Windows envia à janela uma
					  // mensagem para pintar, mostrar dados, (refrescar)


	// dadosPartilhados.numOperacoes = 5; // Apenas para testar...
	LONG_PTR x = SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)&aerialSpace);


	while (GetMessage(&lpMsg, NULL, 0, 0))
	{
		TranslateMessage(&lpMsg);
		DispatchMessage(&lpMsg);
	}

	return((int)lpMsg.wParam);
}

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	HWND aux = GetParent(hWnd); // GetParent vai servir para depois conseguir ir buscar a struct declarada no main e nao usar vars globais
	AerialSpace* aerialSpace = (AerialSpace*)GetWindowLongPtr(aux, GWLP_USERDATA);
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;

	switch (messg)
	{
	case WM_PAINT:
		// Inicio da pintura da janela, que substitui o GetDC
		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &rect);

		// se a copia estiver a NULL, significa que é a 1ª vez que estamos a passar no WM_PAINT e estamos a trabalhar com a copia em memoria
		if (memDC == NULL) {
			// cria copia em memoria
			memDC = CreateCompatibleDC(hdc);
			hBitmapDB = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			// aplicamos na copia em memoria as configs que obtemos com o CreateCompatibleBitmap
			SelectObject(memDC, hBitmapDB);
			DeleteObject(hBitmapDB);
		}
		// operações feitas na copia que é o memDC
		FillRect(memDC, &rect, CreateSolidBrush(RGB(50, 50, 50)));

		WaitForSingleObject(hMutexBitMap, INFINITE);
		// operacoes de escrita da imagem - BitBlt


		BitBlt(memDC, xBitmap, yBitmap, bmp.bmWidth, bmp.bmHeight, bmpDC, 0, 0, SRCCOPY);

		for (int i = 0; i < total; i++) {
			BitBlt(memDC, x[i], y[i], bmpAeroporto.bmWidth, bmpAeroporto.bmHeight, bmpDCAeroporto, 0, 0, SRCCOPY);
		}

		ReleaseMutex(hMutexBitMap);

		// bitblit da copia que esta em memoria para a janela principal - é a unica operação feita na janela principal
		BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);


		// Encerra a pintura, que substitui o ReleaseDC
		EndPaint(hWnd, &ps);
		break;

	case WM_ERASEBKGND:
		return TRUE;
		// redimensiona e calcula novamente o centro
	case WM_SIZE:
		WaitForSingleObject(hMutexBitMap, INFINITE);
		xBitmap = (LOWORD(lParam) / 2) - (bmp.bmWidth / 2);
		yBitmap = (HIWORD(lParam) / 2) - (bmp.bmHeight / 2);
		//yBitmapAeroporto = (HIWORD(lParam) / 2) - (bmp.bmHeight / 2) + 20;
		//xBitmapAeroporto = (LOWORD(lParam) / 2) - (bmp.bmWidth / 2) + 20;
		limDir = LOWORD(lParam) - bmp.bmWidth;
		limDirAeroporto = LOWORD(lParam) - bmpAeroporto.bmWidth;

		memDC = NULL; // metemos novamente a NULL para que caso haja um resize na janela no WM_PAINT a janela em memoria é sempre atualizada com o tamanho novo
		ReleaseMutex(hMutexBitMap);

		break;
		// evento de criação de janela
	case WM_CREATE:
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_CRIAR_AEROPORTO:
			// lida com a dialogbox
			// 1º contexto onde vai ser executada -> NULL porque é no contexto atual
			// 2º ID da dialogbox em si
			// 3º janela mae da dialogbox -> ou NULL(sistema assume que nao estou a usar uma janela modal e permite trabalhar nessa janela e na que está atrás)
			// ou o handle da janela principal
			// 4º serve para tratar os eventos das dialogbox
			DialogBox(NULL, MAKEINTRESOURCE(ID_CRIAR_AEROPORTO), hWnd, TrataEventosCriarAeroporto);
			break;
		case IDM_LISTAVIOES:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_VER_AVIOES), hWnd, TrataEventosVerAvioes);
			break;
		case IDM_LISTAEROPORTOS:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_VERAEROPORTOS), hWnd, TrataEventosVerAeroportos);
			break;
		}

		break;

	case WM_DESTROY:
		free(aerialSpace->airports);
		free(aerialSpace->airPlanes);
		//ReleaseSemaphore(semaphore, 1, NULL);
		aerialSpace->endThreadReceiveInfo = true;
		SetEvent(aerialSpace->hEvent);
		SetEvent(aerialSpace->hEvent3);
		//WaitForSingleObject(hThread, INFINITE);
		//WaitForSingleObject(hThread2, INFINITE);
		//WaitForSingleObject(hThread3, INFINITE);
		//WaitForSingleObject(hThread4, INFINITE);
		//WaitForSingleObject(hThread5, INFINITE);
		//CloseHandle(semaphore);
		UnmapViewOfFile(aerialSpace->mapMemory);
		UnmapViewOfFile(aerialSpace->memPar);
		CloseHandle(aerialSpace->aerialMapObjMap);
		//CloseHandle(hThread);
		//CloseHandle(hThread2);
		//CloseHandle(hThread3);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, messg, wParam, lParam);
		break;
	}

	return(0);
}

LRESULT CALLBACK TrataEventosVerAvioes(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HWND aux = GetParent(hWnd); // GetParent vai servir para depois conseguir ir buscar a struct declarada no main e nao usar vars globais
	AerialSpace* aerial = (AerialSpace*)GetWindowLongPtr(aux, GWLP_USERDATA);
	int i;
	TCHAR message[50];
	HWND hwndList;
	// quando uma dialogbox é inicializava, ela recebe um evento chamado de WM_INITDIALOG
	// no caso das janelas, é um evento chamado de WM_CREATE

	switch (messg)
	{
	case WM_INITDIALOG:

		hwndList = GetDlgItem(hWnd, IDC_VER_AVIOES);
		SendMessage(hwndList, LB_RESETCONTENT, 0, 0);

		// enviamos uma mensagem para cada tipo da lista
		for (i = 0; i < aerial->nAirPlanes; i++) {
			_stprintf(message, TEXT("Avião %d na coordenada x: %d y:%d"), aerial->airPlanes[i].id, aerial->airPlanes[i].coordenates.x, aerial->airPlanes[i].coordenates.y);
			SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)message);
		}

		break;

	case WM_COMMAND:
		// o LOWORD(wParam) traz o ID onde foi carregado 
		// se carregou no OK
		if (LOWORD(wParam) == IDOK)
		{
			// GetDlgItemText() vai à dialogbox e vai buscar o input do user
			//GetDlgItemText(hWnd, ID, username, 16);
			//MessageBox(hWnd, username, TEXT("Username"), MB_OK | MB_ICONINFORMATION);
		}
		// se carregou no CANCEL
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, 0);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return TRUE;
	}

	return FALSE;
}

LRESULT CALLBACK TrataEventosVerAeroportos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HWND aux = GetParent(hWnd); // GetParent vai servir para depois conseguir ir buscar a struct declarada no main e nao usar vars globais
	AerialSpace* aerial = (AerialSpace*)GetWindowLongPtr(aux, GWLP_USERDATA);
	TCHAR message[50];
	HWND hwndList;
	int i;

	switch (messg)
	{
	case WM_INITDIALOG:

		hwndList = GetDlgItem(hWnd, IDC_VER_AEROPORTOS);
		SendMessage(hwndList, LB_RESETCONTENT, 0, 0);

		// enviamos uma mensagem para cada tipo da lista
		for (i = 0; i < aerial->nAirports; i++) {
			_stprintf(message, TEXT("Aeroporto %s na coordenada x: %d y:%d"), aerial->airports[i].name, aerial->airports[i].coordenates.x, aerial->airports[i].coordenates.y);
			SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)message);
		}

		break;
	case WM_COMMAND:
		// o LOWORD(wParam) traz o ID onde foi carregado 
		// se carregou no OK
		if (LOWORD(wParam) == IDOK)
		{
			// GetDlgItemText() vai à dialogbox e vai buscar o input do user
			//GetDlgItemText(hWnd, ID, username, 16);
			//MessageBox(hWnd, username, TEXT("Username"), MB_OK | MB_ICONINFORMATION);
		}
		// se carregou no CANCEL
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, 0);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return TRUE;
	}

	return FALSE;
}

LRESULT CALLBACK TrataEventosCriarAeroporto(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HWND aux = GetParent(hWnd); // GetParent vai servir para depois conseguir ir buscar a struct declarada no main e nao usar vars globais
	AerialSpace* aerial = (AerialSpace*)GetWindowLongPtr(aux, GWLP_USERDATA);
	TCHAR name[100];
	Coordenates coordenates;
	coordenates.x = 0;
	coordenates.y = 0;
	BOOL flag;
	BOOL flagY;

	switch (messg)
	{
	case WM_COMMAND:

		// o LOWORD(wParam) traz o ID onde foi carregado 
		// se carregou no OK
		if (LOWORD(wParam) == IDOK)
		{
			// GetDlgItemText() vai à dialogbox e vai buscar o input do user
			GetDlgItemText(hWnd, IDC_NOME_AERO, name, 16);
			coordenates.x = GetDlgItemInt(hWnd, IDC_COORD_X, &flag, FALSE);
			coordenates.y = GetDlgItemInt(hWnd, IDC_COORD_X, &flagY, FALSE);

			if (flag && flagY) {
				if (createAirport(aerial, name, coordenates) == false) {
					MessageBox(hWnd, TEXT("Invalid airport"), TEXT("Error"), MB_OK | MB_ICONWARNING);
				}
				else {
					RECT rect;
					HDC hdcAeroporto;
					hBmpAeroporto = (HBITMAP)LoadImage(NULL, TEXT("bitmap2.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
					GetObject(hBmpAeroporto, sizeof(bmpAeroporto), &bmpAeroporto);
					hdcAeroporto = GetDC(hWnd);
					// criamos copia do device context e colocar em memoria
					bmpDCAeroporto = CreateCompatibleDC(hdcAeroporto);
					// aplicamos o bitmap ao device context
					SelectObject(bmpDCAeroporto, hBmpAeroporto);
					ReleaseDC(hWnd, hdcAeroporto);
					GetClientRect(hWnd, &rect);

					total = aerial->nAirports;

					x[total - 1] = coordenates.x;
					y[total - 1] = coordenates.y;

					limDirAeroporto = rect.right - bmpAeroporto.bmWidth;

					MessageBox(hWnd, name, TEXT("Created a new airport"), MB_OK | MB_ICONINFORMATION);
				}
			}
			else {
				MessageBox(hWnd, TEXT("Error obtaining value"), TEXT("Error"), MB_ICONEXCLAMATION | MB_OK);
			}
			EndDialog(hWnd, 0);
			return TRUE;
		}
		// se carregou no CANCEL
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, 0);
			return TRUE;
		}

		break;
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}
	