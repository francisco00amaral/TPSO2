#include "Passenger.h"
#include <tchar.h>
#include <fcntl.h>

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
	if (argc == 5)
		data.pass.timeMaxWaiting = _ttoi(argv[4]);
	_tprintf(TEXT("%s\n%s\n%s\n%d\n"), data.pass.airportOrigin, data.pass.airportDestiny, data.pass.name, data.pass.timeMaxWaiting);
	return 0;
}