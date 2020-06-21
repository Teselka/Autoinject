#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <shlwapi.h>
#include <vector>
#include <tlhelp32.h>
#pragma comment(lib, "shlwapi")

using namespace std;


bool StatDirectoryExists(const LPCTSTR &szPath)
{
	struct stat info;

	if (stat(szPath, &info))
		return false;
	else if (info.st_mode & S_IFDIR)
		return true;

	return false;
}

BOOL DirectoryExists(const LPCTSTR &szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) && StatDirectoryExists(szPath);
}

bool FileExists(const LPCTSTR &szPath)
{
	return ::PathFileExists(szPath) == TRUE;
}

void LaunchFile(const LPCTSTR &szPath, const LPCTSTR &szDrive)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	if (!CreateProcessA(szPath, NULL, NULL, NULL, FALSE, NULL, NULL, szDrive, &si, &pi)) { // Checking if we doesn't have permissions to start steam process
		cout << "Failed to create loader process! Error " << GetLastError() << '\n';
	}
}

// Returning us all string in file
vector<string> IterateFile(const LPCTSTR &szFname)
{
	vector<string> strings;
	fstream In(szFname);
	if (!In)
		return strings;

	string str;
	while (getline(In, str))
	{
		strings.push_back(str);
	}

	In.close();
	return strings;
}


vector<string> FindDirectoryFiles(const LPCTSTR &szPath)
{
	vector<string> Strings;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFile;
	string Path = szPath + string("\\*");
	hFile = FindFirstFile(Path.c_str(), &FindFileData);
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			Strings.push_back(FindFileData.cFileName);
		} while (FindNextFile(hFile, &FindFileData) != 0);
		FindClose(hFile);
	}
	return Strings;
}

DWORD GetProcessByExeName(const LPCTSTR &ExeName)
{
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	if (Process32First(hProcessSnap, &pe32))
	{
		do
		{
			if (strstr(pe32.szExeFile, ExeName))
			{
				CloseHandle(hProcessSnap);
				return pe32.th32ProcessID;
			}
		} while (Process32Next(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);
	return 0;
}

void CheckConfig(const string &Settings, string &SteamPath, string &DrivePath, const ifstream &File)
{
	vector<string> strings = IterateFile(Settings.c_str());
	if (strings.size() > 1) {
		SteamPath = strings.at(0);
		DrivePath = strings.at(1);
	}

	if (!FileExists(Settings.c_str()) || !File.is_open() || !FileExists(SteamPath.c_str()) || SteamPath.size() <= 1)
	{
		cout << "It seems you running autoinject first time(or file config is not valid).\nCreating config file...\n";
		if (FileExists(Settings.c_str())) {
			remove(Settings.c_str());
		}
		ofstream NewFile(Settings);
		cout << "File created! File path: " << Settings << '\n';
		cout << "Please enter path to steam .exe >> ";
		getline(cin, SteamPath);
		if (!FileExists(SteamPath.c_str())) {
			do
			{
				cout << "It seems file you entered doesn't exists!\n";
				cout << "Please enter path to steam .exe >> ";
				getline(cin, SteamPath);
			} while (!FileExists(SteamPath.c_str()));
		}

		cout << "Please enter path to the flash drive >> ";
		getline(cin, DrivePath);
		if (!DirectoryExists(DrivePath.c_str())) {
			do
			{
				cout << "Directory you entered doesn't exists!(You must put your flash drive in USB slot)\n";
				cout << "Please enter path to the flash drive >> ";
				getline(cin, DrivePath);
			} while (!DirectoryExists(DrivePath.c_str()));
		}

		NewFile << SteamPath << endl;
		NewFile << DrivePath << endl;
	}
}

int main()
{
	setlocale(LC_CTYPE, "ru");
	string Settings = (string)(getenv("APPDATA")) + "\\Autoinject.ini";
	string SteamPath;
	string DrivePath;
	ifstream File(Settings);
	File.open(Settings);

	CheckConfig(Settings, SteamPath, DrivePath, File);

	cout << "Put your flash drive in USB slot and press ENTER...";
	cin.get();

	if (!DirectoryExists(DrivePath.c_str()))
	{
		cout << "Drive path not founded! Please put your flash drive in USB slot!\n";
		cin.get();
		exit(0);
	}

	string LoaderFile;
	for (const auto str : FindDirectoryFiles(DrivePath.c_str()))
	{
		if (strstr(str.c_str(), ".exe"))
		{
			LoaderFile = str;
		}
	}

	string LoaderPath = DrivePath + LoaderFile;
	if (!LoaderPath.size())
	{
		cout << "Loader file not founded!\n";
		cin.get();
		exit(0);
	}

	WORD PID = GetProcessByExeName("steam.exe");
	if (PID)
	{
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, PID);
		if (hProcess) {
			cout << "Closing steam process..\n";
			TerminateProcess(hProcess, 0);
		}
	}

	LaunchFile(LoaderPath.c_str(), DrivePath.c_str());

	while (GetProcessByExeName(LoaderFile.c_str()))
	{
		Sleep(300);
	}
	
	cout << "Waiting for notepad process opening...\n";
	while (!GetProcessByExeName("notepad.exe"))
	{
		Sleep(300);
	}

	LaunchFile(SteamPath.c_str(), NULL);

	cout << "Waiting for notepad process close...\n";
	while (GetProcessByExeName("notepad.exe"))
	{
		Sleep(300);
	}

	cout << "Running game...\n";

	ShellExecute(0, 0, "steam://rungameid/730", 0, 0, SW_SHOW);

	cout << "Succefully injected!\n";

	File.close();
	Sleep(3000);
	return EXIT_SUCCESS;
}
