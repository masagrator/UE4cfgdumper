// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <vector>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include "dmntcht.h"
#include "ue4settings.hpp"
#include <string>
#include <sys/stat.h>

DmntCheatProcessMetadata cheatMetadata = {0};
u64 mappings_count = 0;
MemoryInfo* memoryInfoBuffers = 0;
uint8_t utf_encoding = 0;
struct ue4Results {
	size_t iterator;
	bool isFloat = false;
	int default_value_int;
	float default_value_float;
	uint32_t offset;
};

std::vector<ue4Results> ue4_vector;

bool isServiceRunning(const char *serviceName) {	
	Handle handle;	
	SmServiceName service_name = smEncodeName(serviceName);	
	if (R_FAILED(smRegisterService(&handle, service_name, false, 1))) 
		return true;
	else {
		svcCloseHandle(handle);	
		smUnregisterService(service_name);
		return false;
	}
}

template <typename T> T searchString(char* buffer, T string, u64 buffer_size) {
	char* buffer_end = &buffer[buffer_size];
	T string_end = &string[std::char_traits<std::remove_pointer_t<std::remove_reference_t<T>>>::length(string)];
	char* result = std::search(buffer, buffer_end, (char*)string, (char*)string_end);
	if ((uint64_t)result != (uint64_t)&buffer[buffer_size])
		return (T)result;
	return nullptr;
}

std::string ue4_sdk = "";

bool checkIfUE4game() {
	size_t i = 0;
	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && memoryInfoBuffers[i].type == MemType_CodeStatic) {
			char test_4[] = "SDK MW+EpicGames+UnrealEngine-4";
			char test_5[] = "SDK MW+EpicGames+UnrealEngine-5";
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			char* result = searchString(buffer_c, (char*)test_4, memoryInfoBuffers[i].size);
			if (result) {
				printf("%s\n", result);
				ue4_sdk = result;
				delete[] buffer_c;
				return true;
			}
			result = searchString(buffer_c, (char*)test_5, memoryInfoBuffers[i].size);
			if (result) {
				printf("%s\n", result);
				printf("Unreal Engine 5 was not tested with this tool, program will still execute.\n");
				ue4_sdk = result;
				delete[] buffer_c;
				return true;
			}
			delete[] buffer_c;
		}
		i++;
	}
	printf("This game is not using Unreal Engine 4 or 5!\n");
	return false;
}

uint8_t testRUN() {
	size_t i = 0;
	uint8_t encoding = 0;

	size_t size = utf8_to_utf16(nullptr, (const uint8_t*)settingsArray[0].description, 0);
	char16_t* utf16_string = new char16_t[size+1]();
	utf8_to_utf16((uint16_t*)utf16_string, (const uint8_t*)settingsArray[0].description, size+1);

	size = utf8_to_utf32(nullptr, (const uint8_t*)settingsArray[0].description, 0);
	char32_t* utf32_string = new char32_t[size+1]();
	utf8_to_utf32((uint32_t*)utf32_string, (const uint8_t*)settingsArray[0].description, size+1);

	consoleUpdate(NULL);

	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			char* result = searchString(buffer_c, (char*)settingsArray[0].description, memoryInfoBuffers[i].size);
			if (result) {
				printf("Encoding: UTF-8\n");
				encoding = 8;
				consoleUpdate(NULL);
			}
			if (!encoding) {
				char16_t* result16 = searchString(buffer_c, utf16_string, memoryInfoBuffers[i].size);
				if (result16) {
					printf("Encoding: UTF-16\n");
					encoding = 16;
					consoleUpdate(NULL);
				}
			}
			if (!encoding) {
				char32_t* result32 = searchString(buffer_c, utf32_string, memoryInfoBuffers[i].size);
				if (result32) {
					printf("Encoding: UTF-32\n");
					encoding = 32;
					consoleUpdate(NULL);
				}
			}
			delete[] buffer_c;
			if (encoding) {
				delete[] utf16_string;
				delete[] utf32_string;
				return encoding;
			}
		}
		i++;
	}	
	delete[] utf16_string;
	delete[] utf32_string;
	printf("Encoding not detected...");
	return encoding;
}

bool searchPointerInMappings(uint64_t string_address, const char* commandName, uint8_t type, size_t itr) {
	size_t k = 0;
	uint64_t pointer_address = 0;
	uint64_t* buffer_u = 0;

	while(k < mappings_count) {
		if ((memoryInfoBuffers[k].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[k].type == MemType_Heap) {
			if (memoryInfoBuffers[k].size > 100'000'000) {
				k++;
				continue;
			}
			buffer_u = new uint64_t[memoryInfoBuffers[k].size / sizeof(uint64_t)];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[k].addr, (void*)buffer_u, memoryInfoBuffers[k].size);
			for (size_t x = 0; x < memoryInfoBuffers[k].size / sizeof(uint64_t); x++) {
				if (buffer_u[x] == string_address) {
					pointer_address = memoryInfoBuffers[k].addr + x*8 - 8;
					consoleUpdate(NULL);
					break;
				}
			}
			delete[] buffer_u;
			if (pointer_address) {
				size_t l = 0;
				while (l < mappings_count) {
					if ((memoryInfoBuffers[l].perm & Perm_Rw) == Perm_Rw && (memoryInfoBuffers[l].type == MemType_CodeMutable || memoryInfoBuffers[l].type == MemType_CodeWritable)) {
						buffer_u = new uint64_t[memoryInfoBuffers[l].size / sizeof(uint64_t)];
						dmntchtReadCheatProcessMemory(memoryInfoBuffers[l].addr, (void*)buffer_u, memoryInfoBuffers[l].size);
						uint64_t pointer2_address = 0;
						for (size_t x = 0; x < memoryInfoBuffers[l].size / sizeof(uint64_t); x++) {
							if (buffer_u[x] == pointer_address) {
								if (x+1 != memoryInfoBuffers[l].size / sizeof(uint64_t) 
									&& (int64_t)(buffer_u[x+1]) - buffer_u[x] > 0 
									&& (int64_t)(buffer_u[x+1]) - buffer_u[x] < 0x48)
								{
									pointer2_address = memoryInfoBuffers[l].addr + (x+1)*8;
									printf("Main offset: 0x%lX, cmd: %s ", pointer2_address - cheatMetadata.main_nso_extents.base, commandName);
									uint64_t pointer = 0;
									dmntchtReadCheatProcessMemory(pointer2_address, (void*)&pointer, 8);
									uint32_t main_offset = pointer2_address - cheatMetadata.main_nso_extents.base;
									if (pointer) {
										if (type == 1) {
											int data = 0;
											dmntchtReadCheatProcessMemory(pointer, (void*)&data, 4);
											printf("int: %d\n", data);
											ue4_vector.push_back({itr, false, data, 0.0, main_offset});
										}
										else if (type == 2) {
											float data = 0;
											dmntchtReadCheatProcessMemory(pointer, (void*)&data, 4);
											printf("float: %.4f\n", data);
											ue4_vector.push_back({itr, true, 0, data, main_offset});
										}
										else {
											printf("Unknown type: %d\n", type);
										}
									}
									consoleUpdate(NULL);
									delete[] buffer_u;
									return true;
								}
							}	
						}
						delete[] buffer_u;
						if (pointer2_address) {
							k = mappings_count;
							l = mappings_count;
						}									
					}
					l++;
				}
			}
		}
		k++;
	}
	return false;
}

char* findStringInBuffer(char* buffer_c, size_t buffer_size, const char* description) {
	char* result = 0;
	if (utf_encoding == 8) {
		result = (char*)searchString(buffer_c, (char*)description, buffer_size);
	}
	else if (utf_encoding == 16) {
		size_t size = utf8_to_utf16(nullptr, (const uint8_t*)description, 0);
		char16_t* utf16_string = new char16_t[size+1]();
		utf8_to_utf16((uint16_t*)utf16_string, (const uint8_t*)description, size+1);
		result = (char*)searchString(buffer_c, utf16_string, buffer_size);
		delete[] utf16_string;
	}
	else {
		size_t size = utf8_to_utf32(nullptr, (const uint8_t*)description, 0);
		char32_t* utf32_string = new char32_t[size+1]();
		utf8_to_utf32((uint32_t*)utf32_string, (const uint8_t*)description, size+1);
		result = (char*)searchString(buffer_c, utf32_string, buffer_size);
		delete[] utf32_string;
	}
	return result;
}

void searchRAM() {
	size_t i = 0;
	printf("Searching RAM...\n\n");
	bool* checkedList = new bool[settingsArray.size()](); 
	while (i < mappings_count) {
		printf("Mapping %ld / %ld\r", i, mappings_count);
		if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
			if (memoryInfoBuffers[i].size > 100'000'000) {
				i++;
				continue;
			}
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			char* result = 0;
			for (size_t itr = 0; itr < settingsArray.size(); itr++) {
				if (checkedList[itr]) {
					continue;
				}
				result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, settingsArray[itr].description);
				if (result) {
					ptrdiff_t diff = (uint64_t)result - (uint64_t)buffer_c;
					uint64_t string_address = memoryInfoBuffers[i].addr + diff;
					if (searchPointerInMappings(string_address, settingsArray[itr].commandName, settingsArray[itr].type, itr)) {
						checkedList[itr] = true;
					}
				}
			}
			delete[] buffer_c;
		}
		i++;
	}
	printf("\n");
	for (size_t x = 0; x < settingsArray.size(); x++) {
		if (!checkedList[x]) {
			printf("%s was not found!\n", settingsArray[x].commandName);
			consoleUpdate(NULL);
			if (alternativeDescriptions1.contains(settingsArray[x].commandName)) {
				printf("%s has alternative description, searching again...\n", settingsArray[x].commandName);
				consoleUpdate(NULL);
				i = 0;
				while (i < mappings_count) {
					if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
						if (memoryInfoBuffers[i].size > 100'000'000) {
							i++;
							continue;
						}
						char* buffer_c = new char[memoryInfoBuffers[i].size];
						dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
						char* result = 0;
						result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, alternativeDescriptions1[settingsArray[x].commandName].c_str());
						if (result) {
							ptrdiff_t diff = (uint64_t)result - (uint64_t)buffer_c;
							uint64_t string_address = memoryInfoBuffers[i].addr + diff;
							if (searchPointerInMappings(string_address, settingsArray[x].commandName, settingsArray[x].type, x)) {
								checkedList[x] = true;
								i = mappings_count;
								continue;
							}
						}
						delete[] buffer_c;
					}
					i++;
				}
				if (!checkedList[x]) {
					printf("%s alternative description failed!\n", settingsArray[x].commandName);
					consoleUpdate(NULL);
				}
			}
		}
	}
	delete[] checkedList;
	printf("Search is finished!\n");
	consoleUpdate(NULL);
}

void dumpAsCheats() {
	uint64_t BID = 0;
	memcpy(&BID, &(cheatMetadata.main_nso_build_id), 8);
	char path[128] = "";
	mkdir("sdmc:/switch/UE4cfgdumper/", 777);
	snprintf(path, sizeof(path), "sdmc:/switch/UE4cfgdumper/%016lX/", cheatMetadata.title_id);
	mkdir(path, 777);
	snprintf(path, sizeof(path), "sdmc:/switch/UE4cfgdumper/%016lX/%016lX.txt", cheatMetadata.title_id, __builtin_bswap64(BID));
	FILE* text_file = fopen(path, "w");
	if (!text_file) {
		printf("Couldn't create cheat file!");
		return;
	}
	for (size_t i = 0; i < ue4_vector.size(); i++) {
		fwrite("[", 1, 1, text_file);
		fwrite(settingsArray[ue4_vector[i].iterator].commandName, strlen(settingsArray[ue4_vector[i].iterator].commandName), 1, text_file);
		fwrite("]\n", 2, 1, text_file);
		fwrite("580F0000 ", 9, 1, text_file);
		char temp[24] = "";
		snprintf(temp, sizeof(temp), "%08X\n", ue4_vector[i].offset);
		fwrite(temp, 9, 1, text_file);
		fwrite("680F0000 ", 9, 1, text_file);
		if (ue4_vector[i].isFloat) {
			int temp_val = 0;
			memcpy(&temp_val, &ue4_vector[i].default_value_float, 4);
			snprintf(temp, sizeof(temp), "%08X %08X\n\n", temp_val, temp_val);
		}
		else {
			snprintf(temp, sizeof(temp), "%08X %08X\n\n", ue4_vector[i].default_value_int, ue4_vector[i].default_value_int);
		}
		fwrite(temp, strlen(temp), 1, text_file);
	}
	fclose(text_file);
	printf("Dumped cheat file to:\n");
	printf(path);
	printf("\n");
}

void dumpAsLog() {
	uint64_t BID = 0;
	memcpy(&BID, &(cheatMetadata.main_nso_build_id), 8);
	char path[128] = "";
	mkdir("sdmc:/switch/UE4cfgdumper/", 777);
	snprintf(path, sizeof(path), "sdmc:/switch/UE4cfgdumper/%016lX/", cheatMetadata.title_id);
	mkdir(path, 777);
	snprintf(path, sizeof(path), "sdmc:/switch/UE4cfgdumper/%016lX/%016lX.log", cheatMetadata.title_id, __builtin_bswap64(BID));	
	FILE* text_file = fopen(path, "w");
	if (!text_file) {
		printf("Couldn't create log file!");
		return;
	}
	fwrite(ue4_sdk.c_str(), ue4_sdk.size(), 1, text_file);
	fwrite("\n\n", 2, 1, text_file);
	for (size_t i = 0; i < ue4_vector.size(); i++) {
		fwrite(settingsArray[ue4_vector[i].iterator].commandName, strlen(settingsArray[ue4_vector[i].iterator].commandName), 1, text_file);
		char temp[48] = "";
		snprintf(temp, sizeof(temp), ", main_offset: 0x%X, type: ", ue4_vector[i].offset);
		fwrite(temp, strlen(temp), 1, text_file);
		if (ue4_vector[i].isFloat) {
			int temp_val = 0;
			memcpy(&temp_val, &ue4_vector[i].default_value_float, 4);
			snprintf(temp, sizeof(temp), "float: %.5f / 0x%X\n", ue4_vector[i].default_value_float, temp_val);
		}
		else {
			snprintf(temp, sizeof(temp), "int: %d / 0x%X\n", ue4_vector[i].default_value_int, ue4_vector[i].default_value_int);
		}
		fwrite(temp, strlen(temp), 1, text_file);
	}
	fclose(text_file);
	printf("Dumped log file to:\n");
	printf(path);	
	printf("\n");
}

// Main program entrypoint
int main(int argc, char* argv[])
{
	// This example uses a text console, as a simple way to output text to the screen.
	// If you want to write a software-rendered graphics application,
	//   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
	// If on the other hand you want to write an OpenGL based application,
	//   take a look at the graphics/opengl set of examples, which uses EGL instead.
	consoleInit(NULL);

	// Configure our supported input layout: a single player with standard controller styles
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);

	// Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
	PadState pad;
	padInitializeDefault(&pad);

	bool error = false;
	if (!isServiceRunning("dmnt:cht")) {
		printf("DMNT:CHT not detected!\n");
		error = true;
	}
	pmdmntInitialize();
	uint64_t PID = 0;
	if (R_FAILED(pmdmntGetApplicationProcessId(&PID))) {
		printf("Game not initialized.\n");
		error = true;
	}
	pmdmntExit();
	if (error) {
		printf("Press + to exit.");
		while (appletMainLoop()) {   
			// Scan the gamepad. This should be done once for each frame
			padUpdate(&pad);

			// padGetButtonsDown returns the set of buttons that have been
			// newly pressed in this frame compared to the previous one
			u64 kDown = padGetButtonsDown(&pad);

			if (kDown & HidNpadButton_Plus)
				break; // break in order to return to hbmenu

			// Your code goes here

			// Update the console, sending a new frame to the display
			consoleUpdate(NULL);
		}
	}
	else {
		pmdmntExit();
		dmntchtInitialize();
		bool hasCheatProcess = false;
		dmntchtHasCheatProcess(&hasCheatProcess);
		if (!hasCheatProcess) {
			dmntchtForceOpenCheatProcess();
		}

		Result res = dmntchtGetCheatProcessMetadata(&cheatMetadata);
		if (res)
			printf("dmntchtGetCheatProcessMetadata ret: 0x%x\n", res);

		res = dmntchtGetCheatProcessMappingCount(&mappings_count);
		if (res)
			printf("dmntchtGetCheatProcessMappingCount ret: 0x%x\n", res);
		else printf("Mapping count: %ld\n", mappings_count);

		memoryInfoBuffers = new MemoryInfo[mappings_count];

		res = dmntchtGetCheatProcessMappings(memoryInfoBuffers, mappings_count, 0, &mappings_count);
		if (res)
			printf("dmntchtGetCheatProcessMappings ret: 0x%x\n", res);

		//Test run

		if (checkIfUE4game() && (utf_encoding = testRUN())) {
			searchRAM();
			dumpAsCheats();
			dumpAsLog();
		}
		
		delete[] memoryInfoBuffers;
		dmntchtExit();
		printf("Press + to exit.");
		while (appletMainLoop()) {   
			// Scan the gamepad. This should be done once for each frame
			padUpdate(&pad);

			// padGetButtonsDown returns the set of buttons that have been
			// newly pressed in this frame compared to the previous one
			u64 kDown = padGetButtonsDown(&pad);

			if (kDown & HidNpadButton_Plus)
				break; // break in order to return to hbmenu

			// Your code goes here

			// Update the console, sending a new frame to the display
			consoleUpdate(NULL);
		}
	}

	// Deinitialize and clean up resources used by the console (important!)
	consoleExit(NULL);
	return 0;
}
