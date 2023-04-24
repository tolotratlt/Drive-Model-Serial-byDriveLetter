#define _WIN32_DCOM
#include <Windows.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <vector>
#include <string>
#include <sstream>
#include <comdef.h>
#include <Wbemidl.h>
#include <regex>


#pragma comment(lib, "wbemuuid.lib")

struct Partition
{
    std::wstring partitionId;
    std::wstring driveLetter;
};

struct Drive
{
    std::wstring model;
    std::wstring serial;
    std::wstring deviceId;
    std::vector<Partition> partitions;
};

bool GetModelAndSerialForDrive(const std::vector<Drive>& drives, const std::wstring& driveLetter, std::wstring& model, std::wstring& serial)
{
    model.clear();
    serial.clear();

    if (drives.empty())
    {
        return false;
    }

    for (const auto& drive : drives)
    {
        for (const auto& partition : drive.partitions)
        {
            if (partition.driveLetter == driveLetter)
            {
                model = drive.model;
                serial = drive.serial;
                return true;
            }
        }
    }

    return false;
}


int main()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

    IWbemLocator* pLoc = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

    IWbemServices* pSvc = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
    }

    if (SUCCEEDED(hr))
    {
        hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    }

    std::vector<Drive> drives;

    if (SUCCEEDED(hr))
    {
        IEnumWbemClassObject* pEnumerator = nullptr;
        hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT Model, SerialNumber, DeviceID FROM Win32_DiskDrive WHERE MediaType='Fixed hard disk media' OR MediaType='Removable media'"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);

        if (SUCCEEDED(hr))
        {
            IWbemClassObject* pDisk = nullptr;
            ULONG uReturn = 0;

            while (pEnumerator)
            {
                HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pDisk, &uReturn);
                if (0 == uReturn)
                {
                    break;
                }

                Drive drive;
                VARIANT vtModel, vtSerial, vtDeviceId;
                hr = pDisk->Get(L"Model", 0, &vtModel, nullptr, nullptr);
                hr = pDisk->Get(L"SerialNumber", 0, &vtSerial, nullptr, nullptr);
                hr = pDisk->Get(L"DeviceID", 0, &vtDeviceId, nullptr, nullptr);

                drive.model = vtModel.bstrVal;
                drive.serial = vtSerial.bstrVal;
                drive.deviceId = vtDeviceId.bstrVal;

                IEnumWbemClassObject* pPartitionEnumerator = nullptr;
                std::wstring partitionQuery = L"ASSOCIATORS OF {Win32_DiskDrive.DeviceID='" + drive.deviceId + L"'} WHERE AssocClass = Win32_DiskDriveToDiskPartition";
                hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(partitionQuery.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pPartitionEnumerator);

                if (SUCCEEDED(hr))
                {
                    IWbemClassObject* pPartition = nullptr;
                    ULONG uPartitionReturn = 0;

                    while (pPartitionEnumerator)
                    {
                        hr = pPartitionEnumerator->Next(WBEM_INFINITE, 1, &pPartition, &uPartitionReturn);
                        if (0 == uPartitionReturn)
                        {
                            break;
                        }

                        Partition part;
                        VARIANT vtPartitionId;
                        hr = pPartition->Get(L"DeviceID", 0, &vtPartitionId, nullptr, nullptr);

                        part.partitionId = vtPartitionId.bstrVal;

                        IEnumWbemClassObject* pLogicalDiskEnumerator = nullptr;
                        std::wstring logicalDiskQuery = L"ASSOCIATORS OF {Win32_DiskPartition.DeviceID='" + part.partitionId + L"'} WHERE AssocClass = Win32_LogicalDiskToPartition";
                        hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(logicalDiskQuery.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pLogicalDiskEnumerator);

                        if (SUCCEEDED(hr))
                        {
                            IWbemClassObject* pLogicalDisk = nullptr;
                            ULONG uLogicalDiskReturn = 0;

                            while (pLogicalDiskEnumerator)
                            {
                                hr = pLogicalDiskEnumerator->Next(WBEM_INFINITE, 1, &pLogicalDisk, &uLogicalDiskReturn);
                                if (0 == uLogicalDiskReturn)
                                {
                                    break;
                                }

                                VARIANT vtDriveLetter;
                                hr = pLogicalDisk->Get(L"Name", 0, &vtDriveLetter, nullptr, nullptr);

                                part.driveLetter = vtDriveLetter.bstrVal;

                                pLogicalDisk->Release();
                            }

                            pLogicalDiskEnumerator->Release();
                        }

                        drive.partitions.push_back(part);
                        pPartition->Release();
                    }

                    pPartitionEnumerator->Release();
                }

                drives.push_back(drive);
                pDisk->Release();
            }

            pEnumerator->Release();
        }
    }

    std::wstring driveLetter = L"C:";
    std::wstring model, serial;
    if (GetModelAndSerialForDrive(drives, driveLetter, model, serial))
    {
        std::wcout << L"Model: " << model << std::endl;
        std::wcout << L"Serial: " << serial << std::endl;
    }
    else
    {
        std::wcout << L"Drive not found" << std::endl;
    }

    if (pSvc != nullptr)
    {
        pSvc->Release();
    }

    if (pLoc != nullptr)
    {
        pLoc->Release();
    }

    CoUninitialize();
    return 0;
}
