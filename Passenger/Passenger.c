#include "Passenger.h"
#include <tchar.h>
#include <stdbool.h>
#include <fcntl.h>
#define PIPE_CONTROL_TO_PASS TEXT("\\\\.\\pipe\\ControlToPass")
#define PIPE_PASS_TO_CONTROL TEXT("\\\\.\\pipe\\PassToControl")

BOOL compare(TCHAR* arg1, TCHAR* arg2) {
	if (_tcscmp(arg1, arg2) == 0) {
		return true;
	}
	return false;
}

//DWORD WINAPI verificaTimeout(LPVOID params){
//	DataPassenger* data = (DataPassenger*)params;
//	_tprintf(TEXT("Vou dormir"));
//	Sleep(50000);
//	_tprintf(TEXT("asoddkaoa"));
//}

DWORD WINAPI threadMenu(LPVOID params){
	DataPassenger* data = (DataPassenger*)params;
	while(1){
		TCHAR command[256];
		_fgetts(command, 256, stdin);

		command[_tcslen(command) - 1] = '\0';
		fseek(stdin, 0, SEEK_END); // limpar o buffer

		//Mete tudo para minusculas para comparar
		for (unsigned int i = 0; i < _tcslen(command); i++) {
			command[i] = tolower(command[i]);
		}

		if (compare(command, TEXT("exit"))) {
			// SAI DA CENA
		}
	}
	return 0;
}

int _tmain(int argc, LPTSTR argv[]) {

#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif

	//Verificação se existe um controlador a correr
	HANDLE semaphore;
	semaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, TEXT("Semaphore"));
	if (semaphore == NULL) {
		_tprintf(TEXT("Não existe nenhum controlador a correr"));
		return -1;
	}

	DataPassenger data;
	

	if (argc < 4)
		return 0;
	_tcscpy_s(data.pass.airportOrigin, 100, argv[1]);
	_tcscpy_s(data.pass.airportDestiny, 100, argv[2]);
	_tcscpy_s(data.pass.name, 100, argv[3]);
	//HANDLE hThread;
	if (argc == 5) {
		data.pass.timeMaxWaiting = _ttoi(argv[4]);
		//DWORD idThread;
		//hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)verificaTimeout, &data, 0, &idThread);
	}
	_tprintf(TEXT("%s\n%s\n%s\n%d\n"), data.pass.airportOrigin, data.pass.airportDestiny, data.pass.name, data.pass.timeMaxWaiting);
	
	if(compare(data.pass.airportOrigin,data.pass.airportDestiny)){
		_tprintf(TEXT("Aeroporto destino e origem não pode ser o mesmo\n"));
		return 0;
	}

	if (!WaitNamedPipe(PIPE_CONTROL_TO_PASS, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("Errooo\n"));
		return -1;
	}
	_tprintf(TEXT("1\n"));

	data.pipeControlToPass = CreateFile(PIPE_CONTROL_TO_PASS, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (data.pipeControlToPass == NULL) {
		_tprintf(TEXT("Errooo\n"));
		return -1;
	}

	if (!WaitNamedPipe(PIPE_PASS_TO_CONTROL, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("Errooo\n"));
		return -1;
	}

	_tprintf(TEXT("2\n"));

	data.pipePassToControl = CreateFile(PIPE_PASS_TO_CONTROL, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (data.pipePassToControl == NULL) {
		_tprintf(TEXT("Errooo\n"));
		return -1;
	}
	_tprintf(TEXT("3\n"));
	DWORD n;
	data.pass.pedidoEntrada = true;
	data.pass.pedidoSaida = false;
	if (!WriteFile(data.pipePassToControl, &data.pass, sizeof(Passanger), &n, NULL)) {
		_tprintf(TEXT("Erro a escrever\n"));
		return -1;
	}

	_tprintf(TEXT("4\n"));

	BOOL ret = ReadFile(data.pipeControlToPass, &data.pass, sizeof(Passanger), &n, NULL);
	if(ret && n > 0) {
		if (data.pass.answer == false) {
			_tprintf(TEXT("Informações de aeroporto erradas"));
			return -1;
		}
	}
	_tprintf(TEXT("%s\n"), ret);

	HANDLE hThreadMenu;
	DWORD idThreadMenu;
	hThreadMenu = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadMenu, &data, 0, &idThreadMenu);
	
	//WaitForSingleObject(hThread, INFINITE);
	WaitForSingleObject(threadMenu, INFINITE);

	return 0;
}