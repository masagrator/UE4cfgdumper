// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include <sys/stat.h>
#include "asmInterpreter.hpp"

DmntCheatProcessMetadata cheatMetadata = {0};
u64 mappings_count = 0;
MemoryInfo* memoryInfoBuffers = 0;

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

template <typename T> T searchString(char* buffer, T string, u64 buffer_size, bool null_terminated = false, bool whole = false) {
	char* buffer_end = &buffer[buffer_size];
	size_t string_len = (std::char_traits<std::remove_pointer_t<std::remove_reference_t<T>>>::length(string) + (null_terminated ? 1 : 0)) * sizeof(std::remove_pointer_t<std::remove_reference_t<T>>);
	T string_end = &string[std::char_traits<std::remove_pointer_t<std::remove_reference_t<T>>>::length(string) + (null_terminated ? 1 : 0)];
	char* result = std::search(buffer, buffer_end, (char*)string, (char*)string_end);
	if (whole) {
		while ((uint64_t)result != (uint64_t)&buffer[buffer_size]) {
			if (!result[-1 * sizeof(std::remove_pointer_t<std::remove_reference_t<T>>)])
				return (T)result;
			result = std::search(result + string_len, buffer_end, (char*)string, (char*)string_end);
		}
	}
	else if ((uint64_t)result != (uint64_t)&buffer[buffer_size]) {
		return (T)result;
	}
	return nullptr;
}

std::string unity_sdk = "";

size_t checkAvailableHeap() {
	size_t startSize = 200 * 1024 * 1024;
	void* allocation = malloc(startSize);
	while (allocation) {
		free(allocation);
		startSize += 1024 * 1024;
		allocation = malloc(startSize);
	}
	return startSize - (1024 * 1024);
}

bool checkIfUnity() {
	size_t i = 0;
	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx && memoryInfoBuffers[i].type == MemType_CodeStatic) {
			if (memoryInfoBuffers[i].size > 200'000'000) {
				continue;
			}
			char test_4[] = "SDK MW+UnityTechnologies+Unity";
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			char* result = searchString(buffer_c, (char*)test_4, memoryInfoBuffers[i].size);
			if (result) {
				printf("%s\n", result);
				unity_sdk = result;
				delete[] buffer_c;
				return true;
			}
			delete[] buffer_c;
		}
		i++;
	}
	printf("This game is not using Unity!\n");
	return false;
}

char* findStringInBuffer(char* buffer_c, size_t buffer_size, const char* description) {
	char* result = 0;
	result = (char*)searchString(buffer_c, (char*)description, buffer_size);
	return result;
}

std::vector<std::string> UnityNames;
std::vector<uint32_t> UnityOffsets;

void searchFunctionsUnity2() {
	size_t i = 0;
	const char first_entry[] = "UnityEngineInternal.GIDebugVisualisation::ResetRuntimeInputTextures";
	const char second_entry[] = "UnityEngineInternal.GIDebugVisualisation::PlayCycleMode";
	const char* first_result = 0;
	const char* second_result = 0;
	printf("Base address: 0x%lx\n", cheatMetadata.main_nso_extents.base);
	printf("Mapping %ld / %ld\r", i+1, mappings_count);	
	consoleUpdate(NULL);
	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx && (memoryInfoBuffers[i].type == MemType_CodeStatic || memoryInfoBuffers[i].type == MemType_CodeReadOnly)) {
			printf("Mapping %ld / %ld\r", i+1, mappings_count);
			consoleUpdate(NULL);
			if (memoryInfoBuffers[i].size > 200'000'000) {
				i++;
				continue;
			}
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			char* result = 0;
			result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, first_entry);
			if (result) {
				first_result = (char*)(memoryInfoBuffers[i].addr + (result - buffer_c));
				printf("Found 1. reference string at address 0x%lx\n", (uint64_t)first_result);
				consoleUpdate(NULL);
				result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, second_entry);
				if (result) {
					second_result = (char*)(memoryInfoBuffers[i].addr + (result - buffer_c));
					printf("Found 2. reference string at address 0x%lx\n", (uint64_t)second_result);
					consoleUpdate(NULL);
					delete[] buffer_c;
					break;
				}
			}
			delete[] buffer_c;
		}
		i++;
	}
	if (!first_result || !second_result) {
		printf("Reference strings were not found! Aborting...\n");
		consoleUpdate(NULL);
		return;
	}
	printf("Mapping %ld / %ld\r", i+1, mappings_count);
	consoleUpdate(NULL);
	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx && (memoryInfoBuffers[i].type == MemType_CodeStatic || memoryInfoBuffers[i].type == MemType_CodeReadOnly)) {
			printf("Mapping %ld / %ld\r", i+1, mappings_count);
			consoleUpdate(NULL);
			if (memoryInfoBuffers[i].size > 200'000'000) {
				i++;
				continue;
			}
			int32_t* buffer = new int32_t[memoryInfoBuffers[i].size / sizeof(int32_t)];
			printf("Buffer: 0x%lx, size: 0x%lx\n", (uint64_t)buffer, memoryInfoBuffers[i].size);
			consoleUpdate(NULL);
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer, memoryInfoBuffers[i].size);
			int32_t* result = 0;
			for (size_t x = 0; x+1 < memoryInfoBuffers[i].size / sizeof(uint32_t); x++) {
				int32_t diff1 = (int64_t)first_result - (memoryInfoBuffers[i].addr + (x * sizeof(uint32_t)));
				int32_t diff2 = (int64_t)second_result - (memoryInfoBuffers[i].addr + (x * sizeof(uint32_t)));
				if (buffer[x] == diff1 && buffer[x+1] == diff2) {
					result = &buffer[x];
					break;
				}
			}
			if (!result) {
				delete[] buffer;
				i++;
				continue;
			}
			printf("Found string array at buffer address: 0x%lx\n", (uint64_t)result);
			consoleUpdate(NULL);
			size_t x = 0;
			while(true) {
				int32_t offset = result[x];
				char* address = (char*)((uint64_t)result + offset);
				if (((uint64_t)address > (uint64_t)buffer + memoryInfoBuffers[i].size) || ((uint64_t)address < (uint64_t)buffer)) {
					break;
				}
				if (!strncmp(address, "Unity", 5)) {
					std::string name = address;
					UnityNames.push_back(name);
					x++;
					printf("#%ld: %s\n", x, name.c_str());
					consoleUpdate(NULL);
				}
				else break;
			}
			delete[] buffer;
			break;
		}
		i++;
	}
	printf("Mapping %ld / %ld\r", i+1, mappings_count);
	consoleUpdate(NULL);
	MemoryInfo main = {0};
	dmntchtQueryCheatProcessMemory(&main, cheatMetadata.main_nso_extents.base);
	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && (memoryInfoBuffers[i].type == MemType_CodeMutable || memoryInfoBuffers[i].type == MemType_CodeWritable)) {
			printf("Mapping %ld / %ld\r", i+1, mappings_count);
			consoleUpdate(NULL);
			if (memoryInfoBuffers[i].size > 200'000'000) {
				i++;
				continue;
			}
			uint64_t* buffer = new uint64_t[memoryInfoBuffers[i].size / sizeof(uint64_t)];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer, memoryInfoBuffers[i].size);
			uint16_t count = 0;
			size_t start_index = 0;
			for (size_t x = 0; x < (memoryInfoBuffers[i].size / sizeof(uint64_t)); x++) {
				if (buffer[x] == 0 || (buffer[x] < main.addr) || (buffer[x] > (main.addr + main.size))) {
					if (count == UnityNames.size()) {
						start_index = x - count;
						break;
					}
					count = 0;
					continue;
				}
				count++;
			}
			if (count != UnityNames.size()) {
				delete[] buffer;
				i++;
				continue;
			}
			for (size_t x = 0; x < count; x++) {
				UnityOffsets.push_back(buffer[start_index + x] - cheatMetadata.main_nso_extents.base);
			}
			delete[] buffer;
			printf("Found %ld Unity functions.\n", UnityNames.size());
			return;
		}
		i++;
	}
	return;
}

void searchFunctionsUnity() {
	size_t i = 0;
	const char first_entry[] = "UnityEngineInternal.GIDebugVisualisation::ResetRuntimeInputTextures";
	printf("Base address: 0x%lx\n", cheatMetadata.main_nso_extents.base);
	printf("Mapping %ld / %ld\r", i+1, mappings_count);
	consoleUpdate(NULL);
	char* found_string = 0;
	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx && (memoryInfoBuffers[i].type == MemType_CodeStatic || memoryInfoBuffers[i].type == MemType_CodeReadOnly)) {
			printf("Mapping %ld / %ld\r", i+1, mappings_count);
			consoleUpdate(NULL);
			if (memoryInfoBuffers[i].size > 200'000'000) {
				i++;
				continue;
			}
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			char* result = 0;
			result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, first_entry);
			if (result) {
				found_string = (char*)(memoryInfoBuffers[i].addr + (result - buffer_c));
				printf("Found reference string at offset 0x%lx\n", (uint64_t)found_string);
				consoleUpdate(NULL);
				delete[] buffer_c;
				break;
			}
			delete[] buffer_c;
		}
		i++;
	}
	if (!found_string) {
		printf("Didn't found reference string.\n");
		consoleUpdate(NULL);
		return;
	}
	i = 0;
	uint64_t* array_start = 0;
	printf("Mapping %ld / %ld\r", i+1, mappings_count);
	consoleUpdate(NULL);
	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && (memoryInfoBuffers[i].type == MemType_CodeMutable || memoryInfoBuffers[i].type == MemType_CodeWritable)) {
			printf("Mapping %ld / %ld\r", i+1, mappings_count);
			consoleUpdate(NULL);
			if (memoryInfoBuffers[i].size > 200'000'000) {
				i++;
				continue;
			}
			uint64_t* buffer = new uint64_t[memoryInfoBuffers[i].size / sizeof(uint64_t)];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer, memoryInfoBuffers[i].size);
			uint64_t* result = 0;
			for (size_t x = 0; x < memoryInfoBuffers[i].size / sizeof(uint64_t); x++) {
				if (buffer[x] == (uint64_t)found_string) {
					result = (uint64_t*)(memoryInfoBuffers[i].addr + (sizeof(uint64_t) * x));
					break;
				}
			}
			delete[] buffer;
			if (result) {
				array_start = result;
				printf("Found string array at offset 0x%lx\n", (uint64_t)array_start);
				break;
			}
		}
		i++;
	}
	if (!array_start) {
		printf("Didn't found array string. Initiating second method...\n");
		consoleUpdate(NULL);
		return searchFunctionsUnity2();
	}
	i = 0;
	while(true) {
		uint64_t address = (u64)array_start + (i * 8);
		uint64_t string_address = 0;
		dmntchtReadCheatProcessMemory(address, (void*)&string_address, sizeof(uint64_t));
		i++;
		char UnityCheck[6] = "";
		dmntchtReadCheatProcessMemory(string_address, (void*)UnityCheck, 5);
		if (!strncmp(UnityCheck, "Unity", 5)) {
			char buffer[1024] = "";
			dmntchtReadCheatProcessMemory(string_address, (void*)buffer, 1024);
			std::string string = buffer;
			if (string.length() > 0) {
				UnityNames.push_back(string);
				printf("#%ld: %s\n", UnityNames.size(), UnityNames.back().c_str());
				consoleUpdate(NULL);				
			}
		}
		else break;
	}
	printf("Found %ld Unity functions.\n", UnityNames.size());
	consoleUpdate(NULL);
	uint64_t second_array = (u64)array_start + ((i-1) * 8);
	for (size_t x = 0; x < UnityNames.size(); x++) {
		uint64_t address = second_array + (x * 8);
		uint64_t function_address = 0;
		if (dmntchtReadCheatProcessMemory(address, (void*)&function_address, sizeof(uint64_t))) {
			printf("Something went wrong.\n");
			break;
		}
		UnityOffsets.push_back(function_address - cheatMetadata.main_nso_extents.base);
	}
	return;
}

char path[128] = "";

void dumpAsLog() {
	if (UnityNames.size() != UnityOffsets.size()) {
		printf("Cannot produce log, Names and Offsets count doesn't match.\n");
		return;
	}
	uint64_t BID = 0;
	memcpy(&BID, &(cheatMetadata.main_nso_build_id), 8);
	mkdir("sdmc:/switch/UnityFuncDumper/", 777);
	snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/", cheatMetadata.title_id);
	mkdir(path, 777);
	snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/%016lX.log", cheatMetadata.title_id, __builtin_bswap64(BID));	
	FILE* text_file = fopen(path, "w");
	if (!text_file) {
		printf("Couldn't create log file!\n");
		return;
	}
	fwrite(unity_sdk.c_str(), unity_sdk.size(), 1, text_file);
	fwrite("\n\n", 2, 1, text_file);
	for (size_t i = 0; i < UnityNames.size(); i++) {
		fwrite(UnityNames[i].c_str(), UnityNames[i].length(), 1, text_file);
		fwrite(": ", 2, 1, text_file);
		char temp[128] = "";
		snprintf(temp, sizeof(temp), "0x%X\n", UnityOffsets[i]);
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
		size_t availableHeap = checkAvailableHeap();
		printf("Available Heap: %ld MB\n", (availableHeap / (1024 * 1024)));
		consoleUpdate(NULL);
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

		if (checkIfUnity()) {

			uint64_t BID = 0;
			memcpy(&BID, &(cheatMetadata.main_nso_build_id), 8);
			mkdir("sdmc:/switch/UnityFuncDumper/", 777);
			snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/", cheatMetadata.title_id);
			mkdir(path, 777);
			snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/%016lX.log", cheatMetadata.title_id, __builtin_bswap64(BID));	
			bool file_exists = false;
			FILE* text_file = fopen(path, "r");
			if (text_file) {
				file_exists = true;
				fclose(text_file);
			}
			if (file_exists) {
				printf("\nFunctions offsets were already dumped.\nPress A to overwrite them.\nPress X to dump data.\nPress + to Exit\n\n");
			}
			else printf("\n----------\nPress A to Start\nPress + to Exit\n\n");
			consoleUpdate(NULL);
			bool overwrite = true;
			while (appletMainLoop()) {   
				padUpdate(&pad);

				u64 kDown = padGetButtonsDown(&pad);

				if (kDown & HidNpadButton_A)
					break;

				if (kDown & HidNpadButton_Plus) {
					dmntchtExit();
					consoleExit(NULL);
					return 0;
				}

				if (file_exists && (kDown & HidNpadButton_X)) {
					printf("Restoring offsets to program...\n");
					consoleUpdate(NULL);
					text_file = fopen(path, "r");
					if (!text_file) {
						printf("It didn't open?!\n");
						consoleUpdate(NULL);
					}
					else {
						char line[256] = "";
						while (fgets(line, sizeof(line), text_file)) {
							if (strncmp("Unity", line, 5))
								continue;
							char* ptr = strchr(line, ' ');
							char temp[256] = "";
							memcpy(temp, line, ptr-(line+1));
							UnityOffsets.push_back(std::stoi(ptr, nullptr, 16));
							UnityNames.push_back(temp);
						}
						fclose(text_file);
					}
					overwrite = false;
					break;
				}
				
			}
			if (overwrite) {
				printf("Searching RAM...\n\n");
				consoleUpdate(NULL);
				appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad);
				searchFunctionsUnity();
				printf(CONSOLE_BLUE "\n---------------------------------------------\n\n" CONSOLE_RESET);
				printf(CONSOLE_WHITE "Search is finished!\n");
				consoleUpdate(NULL);
				dumpAsLog();
				appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
				delete[] memoryInfoBuffers;
			}
		}
		
		dumpPointers(UnityNames, UnityOffsets, cheatMetadata, unity_sdk);
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
	UnityNames.clear();
	UnityOffsets.clear();
	consoleExit(NULL);
	return 0;
}
