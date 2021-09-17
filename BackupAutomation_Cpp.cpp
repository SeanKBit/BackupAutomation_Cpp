//
// Programmer: Sean Bittner
// Version: v1.0
// Release Date: 9/12/2019
// Last Mod Date: 11/17/2020
// Last Mod: Updated to 60 day removal of databases
//			 Previous-Removed /e /s from xcopy command so that only a single directory, and no sub-directories are copied.
//           Added functions to remove /Data folder and /WO folder from current backup directory.
//           Added function to remove all database file from DEFINEMASTER that are older than 60days.
//
//
// BackDatModusUp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// This programs looks in the DEFINEMASTER location for .dmi programs, checks to see if they have been backed-up
// recently within the DEFINEBACKUP location. Proceeds with copying the part-program .dmi 
// if they either do not exist in the backup, or if the .dmi has been edited based on the last modified dates in both locations.
// Improved functionality includes cleaning up unneccarry files from the backup location to save server space, as
// as well as deletes database files from the master locations that are older than 180days.
// Intended to be ran automatically from the Windows Task Scheduler app.
//


#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <chrono>
#include <ctime>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost::filesystem;

// Constants
string const DEFINEMASTER = "C:/Users"; // REMOVED *proprietary*
string const DEFINEBACKUP = "C:/Users"; // REMOVED *proprietary*

// Global
int copyCount = 0;
int garbageCount = 0;

// Function Declarations
bool timeAged(time_t masterFileTime, time_t currentTime);
bool timeSinceMod(time_t masterFileTime, time_t backupFileTime);
void copyDirectory(string fileToCopy, string pathStr);
void removeDirectory(string pathStr);
void deleteData(string toDeleteStr);
string modifyPath(string pathIN);


int main()
{
	path masterPath(DEFINEMASTER);
	string directoryStr;
	string backupFileTxt = DEFINEBACKUP + "BackupInfo.txt";

	auto start = chrono::system_clock::now();
	time_t current = time(0);

	// Main loop beginning at parent directory, MASTER MODUS PROGRAMS
	for (auto i = directory_iterator(masterPath); i != directory_iterator(); i++)
	{
		// Only continue if path is a program folder, not simply a file sitting in the master location.
		if (is_directory(i->path()))
		{
			directoryStr = i->path().string();
			path directoryPath = directoryStr;
			path dmiPath = directoryStr + "/MASTER/";

			if (exists(dmiPath)) // Search inside MASTER folder for .dmi, then use it for comparison of modified date in backup path.
			{
				for (auto j = directory_iterator(dmiPath); j != directory_iterator(); j++)
				{
					string backupDirectoryStr = (j->path().string());
					path backupDirectoryPath = backupDirectoryStr;

					if (backupDirectoryPath.extension() == ".dmi") // Looks for .dmi file to use (may fail if multiple dmi's in location, programmers should not leave multiple .dmi's in master).
					{
						time_t t = last_write_time(backupDirectoryPath);
						path pathToBackupFile = modifyPath(backupDirectoryStr);

						// If .dmi exist above, check to see if it exist in backup and when it was edited last. Exit if recently backed up. 
						if (exists(pathToBackupFile))
						{
							time_t t2 = last_write_time(pathToBackupFile);
							if (timeSinceMod(t, t2))
							{
								copyDirectory(directoryStr, directoryStr);
								removeDirectory(directoryStr);
							}
						}
						else // Copy entire folder if .dmi did not exist in backup (assumed new program directory).
						{
							copyDirectory(directoryStr, directoryStr);
						}

						path dataFolderPath = directoryStr + "/MASTER/Data/"; // The following logic will clear out any old databases that are more then 30 days old from master location.
						
						if (exists(dataFolderPath))
						{
							//cout << "1:  " << directoryStr << endl;
							for (auto k = directory_iterator(dataFolderPath); k != directory_iterator(); k++)
							{
								string dataFileStr = (k->path().string());
								path dataFilePath = dataFileStr;

								if (dataFilePath.extension() == ".bak")
								{
									time_t dataTime = last_write_time(dataFilePath);
									if (timeAged(dataTime, current))
									{
										deleteData(dataFileStr);
										garbageCount++;
									}
								}
							}
						}
					}
				}
			}
		}
		else // Do nothing since it wasn't a path to a program master folder (skips junk/random files that are in the master location I don't care to backup).
		{
			continue;
		}
	}

	auto end = std::chrono::system_clock::now();
	chrono::duration<double> elapsed_seconds = end - start;
	std::ofstream infoFile;
	infoFile.open(backupFileTxt, ios::out, ios::trunc);
	infoFile << "Directories Copied: " << copyCount << "\n";
	infoFile << "Databases Removed: " << garbageCount << "\n";
	infoFile << "Elapsed Time: " << elapsed_seconds.count() << "s\n";
	infoFile.close();

	//std::system("pause");
	return 0;
}



bool timeAged(time_t dataFileTime, time_t currentTime)
{
	long timeAged = 0;
	timeAged = difftime(currentTime, dataFileTime);
	
	if (timeAged > 5184000) // approx. 60 days
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool timeSinceMod(time_t masterFileTime, time_t backupFileTime)
{
	long timePassed = 0;
	timePassed = difftime(masterFileTime, backupFileTime);

	// 43200 seconds in 12 hours (typically use 7200)
	if (timePassed > 7200)
	{
		return true;
	}
	else
	{
		return false;
	}
}


void copyDirectory(string fileToCopy, string pathStr)
{
	string copyFrom = fileToCopy + "/MASTER";
	replace(copyFrom.begin(), copyFrom.end(), '/', '\\');
	pathStr = pathStr + "/MASTER/";
	string copyToMod = modifyPath(pathStr);

	string completeCommand = ("xcopy \"" + copyFrom + "\" " + copyToMod + " /q /y /r /d"); // add /q after debugging, remove /f
	const char* c = completeCommand.c_str();
	std::system(c);
	copyCount++;
	//copy_file(mySourcePath, myTargetPath, copy_option::overwrite_if_exists);
}


// Removes Data and File directories from MASTER folder in backup location. Way to many files in these subfolders, needed to clean up
void removeDirectory(string pathStr)
{
	string pathStrData, pathStrWO, pathStrARCHIVE,pathStrTEST;
	pathStrData = pathStr + "/MASTER/Data";
	pathStrWO = pathStr + "/WO";
	pathStrARCHIVE = pathStr + "/ARCHIVE";
	pathStrTEST = pathStr + "/TEST";

	string toRemoveDataStr = modifyPath(pathStrData);
	path toRemoveDataPath = toRemoveDataStr;
	remove_all(toRemoveDataPath);

	string toRemoveWOStr = modifyPath(pathStrWO);
	path toRemoveWOPath = toRemoveWOStr;
	remove_all(toRemoveWOPath);

	string toRemoveARCHIVEStr = modifyPath(pathStrARCHIVE);
	path toRemoveARCHIVEPath = toRemoveARCHIVEStr;
	remove_all(toRemoveARCHIVEPath);

	string toRemoveTESTStr = modifyPath(pathStrTEST);
	path toRemoveTESTPath = toRemoveTESTStr;
	remove_all(toRemoveTESTPath);
}



void deleteData(string toDeleteStr)
{
	replace(toDeleteStr.begin(), toDeleteStr.end(), '\\', '/');
	path destructFilePath = toDeleteStr;
	remove_all(destructFilePath);

	boost::erase_all(toDeleteStr, "Insp_");
	boost::erase_all(toDeleteStr, ".bak");
	path destructFolderPath = toDeleteStr;
	remove_all(destructFolderPath);
	//cout << "2:  " << toDeleteStr << endl;
}



// Modifies master location path name to the backup directory, formats slashes.
string modifyPath(string pathIN)
{
	string toReplace = DEFINEMASTER;
	string replaceWith = DEFINEBACKUP;
	replace(pathIN.begin(), pathIN.end(), '\\', '/');
	boost::replace_all(pathIN, toReplace, replaceWith);
	replace(pathIN.begin(), pathIN.end(), '/', '\\');
	return pathIN;
}

