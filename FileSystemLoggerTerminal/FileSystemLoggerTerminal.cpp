// FileSystemLoggerTerminal.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <conio.h>
#include <string>
#include <list>
#include <map>
#include "../FileSystemLogger/Public.h"


#define DRIVER_NAME "FileSystemLogger"
#define DRIVER_SYM_NAME "\\\\.\\FileSysLog"
#define DRIVER_FILE_NAME "FileSystemLogger.sys"

class FsOperationEventItem {

public:
    FsOperationEventItem()  {}

    std::string fileName;

    friend std::ostream& operator <<(std::ostream& os, const FsOperationEventItem& pii)
    {
        os << "File: " << pii.fileName << "\n";
        return os;
    }
};

std::list<FsOperationEventItem> fsOpEventList;



HANDLE attemptOpenKernelDriver(const std::string& DriverSymName, DWORD* Status);
std::string convertUnicodeToAsci(const char* Unicode, const size_t Size);
char waitToPressAnyKey();
char wiatToPressСertainKey(const std::string& Keys);
std::string GetAbsFilePath(const std::string& FileName);

void parseReceiveData(const unsigned char* Data, const size_t Size);

void dumpAllInfoInFile();

bool logStart(HANDLE hDriver);
bool logPause(HANDLE hDriver);

int main()
{
    DWORD status;

    const size_t transportBufferSize = 2000;
    DWORD numberOfBytesRead = 0;
    unsigned char* transportBuffer = new unsigned char[transportBufferSize];

    char controlInputCode;

    HANDLE hDriver = INVALID_HANDLE_VALUE;
    bool driverWasLoaded = false;
    SC_HANDLE scManager = NULL;
    SC_HANDLE serviceHandle = NULL;

    size_t readInterval = 200; //mcs
    bool dumpFile = true;

    std::string targetExeFileName;

    std::cout << "           --- ProcMonTerminal ---               " << std::endl;
    std::cout << " To start/cont monitoring press key 's';         " << std::endl;
    std::cout << " Press 'p' for pause                             " << std::endl;
    std::cout << " Press 'm' to get access for options;            " << std::endl;
    std::cout << " Press 'q' to stop monitoring, dump log and exit." << std::endl << std::endl;

    std::cout << ". ReadInterval = " << readInterval << std::endl;
    std::cout << ". DumpFile = " << (dumpFile ? "Yes" : "No") << std::endl << std::endl;

    while ((controlInputCode = wiatToPressСertainKey("smqp")) != 's')
    {
        if (controlInputCode == 'q')
            goto __exit;

        if (controlInputCode == 'm')
        {
            size_t newReadInterval;
            std::cout << "Current read interval: " << readInterval << " mcs" << std::endl;
            std::cout << "New read interval (mcs < 5000): ";
            std::cin >> newReadInterval;

            std::cout << "Dump File (y/n): ";
            dumpFile = (wiatToPressСertainKey("yn") == 'y');
            std::cout << (dumpFile ? "Yes" : "No") << std::endl;

            if (newReadInterval < 5000)
                readInterval = newReadInterval;
        }
    }

    hDriver = attemptOpenKernelDriver(DRIVER_SYM_NAME, &status);
    // Драйвер успешно открыт
    if (hDriver != INVALID_HANDLE_VALUE) {
        std::cout << "Driver already loaded. Open was successful." << std::endl;
    }
    // Драйвер не обнаружен, осуществлуем попутку загрузки его в память
    else if (status == ERROR_FILE_NOT_FOUND)
    {
        std::cout << "Driver not loaded, trying to load it...";

        const std::string driverAbsPath = GetAbsFilePath(DRIVER_FILE_NAME);
        if (driverAbsPath.empty()) {
            std::cout << "Driver file not found (" << DRIVER_FILE_NAME << ")." << std::endl;
            goto __error_exit;
        }

        /*if (!loadNonPnpDriver(DRIVER_FILE_NAME, driverAbsPath, &scManager, &serviceHandle)) {
            std::cout << "Can't load driver, may be access denied." << std::endl;
            goto __error_exit;
        }*/

        //driverWasLoaded = true;

        hDriver = attemptOpenKernelDriver(DRIVER_SYM_NAME, &status);

        if (hDriver == INVALID_HANDLE_VALUE) {
            std::cout << "Unsuccessful attempt to download the driver" << std::endl;
            goto __error_exit;
        }

        std::cout << "Driver loaded successful." << std::endl;
    }
    // Неизвесная ошибка во время открытия драйвера...
    else {
        std::cout << "Application will terminated, because CreateFile return " << status << "." << std::endl;
        goto __error_exit;
    }


    controlInputCode = 'n';
    do {

        if (controlInputCode == 'n') {
            std::cout << "Enter exe file name (with prefix): ";
            std::cin >> targetExeFileName;
        }

        if (targetExeFileName.size() > MAX_TARGET_FILE_NAME_SIZE) {
            std::cout << "Target file name is too big ( size < " << MAX_TARGET_FILE_NAME_SIZE << " )" << std::endl;
            continue;
        }

        std::cout << "Target file: " << targetExeFileName << std::endl;
        std::cout << "If is true press 'y' otherwise press 'n'" << std::endl;
    } while ((controlInputCode = wiatToPressСertainKey("yn")) != 'y');


    if (!DeviceIoControl(hDriver, 
        IOCTL_SET_TARGET_APP,                // ioctl код задания имени рабочего процесса
        PVOID(targetExeFileName.c_str()),    // Задаем имя рабочего процесса для отслеживания
        targetExeFileName.size(),            // 
        NULL, NULL, NULL, NULL)) 
    {
        std::cout << "Error: DeviceIoControl return: " << GetLastError() << std::endl;
        goto __error_exit;
    }

    if (!logStart(hDriver))
        goto __error_exit;
        
    while (true)
    {
        Sleep(readInterval);

        if (_kbhit())
        {
            switch (char(_getch()))
            {
            case 'q':
                logPause(hDriver);
                goto __exit;
                break;
            case 'p':
                if(logPause(hDriver))
                    std::cout << "pause... (to continue press 's')" << std::endl;
                break;
            case 's':
                logStart(hDriver);
            }
        }
            

        if (!ReadFile(hDriver, transportBuffer, transportBufferSize, &numberOfBytesRead, NULL)) {
            std::cout << "Error ReadFile return: " << GetLastError() << std::endl;
            goto __error_exit;
        }

        std::cout << ".NumberOfBytesRead: " << numberOfBytesRead << std::endl;

        if (!numberOfBytesRead)
            continue;

        parseReceiveData(transportBuffer, numberOfBytesRead);
    }


__error_exit:

__exit:
    if (hDriver != INVALID_HANDLE_VALUE)
        CloseHandle(hDriver);

    //if (driverWasLoaded)
    //    unloadNonPnpDriver(scManager, serviceHandle);

    if (dumpFile)
        dumpAllInfoInFile();

    if (transportBuffer != nullptr)
        delete[] transportBuffer;

    std::cout << "Pouse... Press any key." << std::endl;
    waitToPressAnyKey();

    return 0;
}

HANDLE attemptOpenKernelDriver(const std::string& DriverName, DWORD* Status)
{
    HANDLE hHandl = CreateFileA(DriverName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL
    );

    if (hHandl == INVALID_HANDLE_VALUE)
        *Status = GetLastError();
    else
        *Status = ERROR_SUCCESS;

    return hHandl;
}


std::string convertUnicodeToAsci(const char* Unicode, const size_t Size)
{
    std::string res;
    res.reserve(Size / 2);

    for (int i = 0; i < Size && isprint(Unicode[i]); i += 2)
        res += Unicode[i];

    return res;
}


char waitToPressAnyKey() {
    return _getch();
}


char wiatToPressСertainKey(const std::string& Keys)
{
    while (true)
    {
        char key = waitToPressAnyKey();
        if (Keys.find(key) != std::string::npos)
            return key;
    }
}


std::string GetAbsFilePath(const std::string& FileName)
{
    char fullPath[MAX_PATH];

    if (0 == GetFullPathNameA(FileName.c_str(), MAX_PATH, fullPath, NULL))
        return "";

    return std::string(fullPath);
}


void parseReceiveData(const unsigned char* Data, const size_t Size)
{
    for (int i = 0; i < Size; ) {
        FsOperationEvent* item = (FsOperationEvent*)(&Data[i]);

        if (item->size > Size - i)
            return;

        FsOperationEventItem elem;
        elem.fileName = convertUnicodeToAsci(PCHAR(item) + item->targetFileNameOffset, item->targetFileNameLength);
        fsOpEventList.push_back(elem);

        std::cout << elem;

        i += item->size;
        std::cout << std::endl;
    }
}


void dumpAllInfoInFile()
{
    std::ofstream fout("process-dump.txt");

    for (auto elem : fsOpEventList)
        fout << elem << std::endl;
}


bool logStart(HANDLE hDriver)
{
    if (!DeviceIoControl(hDriver,
        IOCTL_START_LOG,
        NULL, NULL, NULL, NULL, NULL, NULL))
    {
        std::cout << "[ Start logging ] Error: DeviceIoControl return: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}


bool logPause(HANDLE hDriver)
{
    if (!DeviceIoControl(hDriver, 
        IOCTL_STOP_LOG,
        NULL, NULL, NULL, NULL, NULL, NULL)) 
    {
        std::cout << "[ Pause logging ] Error: DeviceIoControl return: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}
