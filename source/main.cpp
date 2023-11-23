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
	const char* iterator;
	bool isFloat = false;
	int default_value_int;
	float default_value_float;
	uint32_t offset;
	uint32_t add;
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
									&& (int64_t)(buffer_u[x+1]) - buffer_u[x] <= 0x64)
								{
									pointer2_address = memoryInfoBuffers[l].addr + (x+1)*8;
									printf(CONSOLE_GREEN "*" CONSOLE_RESET "Main offset: " CONSOLE_YELLOW "0x%lX" CONSOLE_RESET", cmd: " CONSOLE_YELLOW "%s " CONSOLE_RESET, pointer2_address - cheatMetadata.main_nso_extents.base, commandName);
									uint64_t pointer = 0;
									dmntchtReadCheatProcessMemory(pointer2_address, (void*)&pointer, 8);
									uint32_t main_offset = pointer2_address - cheatMetadata.main_nso_extents.base;
									if (pointer) {
										if (type == 1) {
											int data = 0;
											dmntchtReadCheatProcessMemory(pointer, (void*)&data, 4);
											printf("int: " CONSOLE_YELLOW "%d\n" CONSOLE_RESET, data);
											ue4_vector.push_back({commandName, false, data, 0.0, main_offset, 0});
										}
										else if (type == 2) {
											float data = 0;
											dmntchtReadCheatProcessMemory(pointer, (void*)&data, 4);
											printf("float: " CONSOLE_YELLOW "%.4f\n" CONSOLE_RESET, data);
											ue4_vector.push_back({commandName, true, 0, data, main_offset, 0});
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

void SearchFramerate() {
	for (size_t i = 0; i < mappings_count; i++) {
		if (memoryInfoBuffers[i].addr < cheatMetadata.main_nso_extents.base) {
			continue;
		}
		if (memoryInfoBuffers[i].addr >= cheatMetadata.main_nso_extents.base + cheatMetadata.main_nso_extents.size) {
			continue;
		}
		char* result = 0;
		uint64_t address = 0;
		if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].type == MemType_CodeStatic || memoryInfoBuffers[i].type == MemType_CodeReadOnly)) {
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			result = (char*)searchString(buffer_c, "FixedFrameRate",  memoryInfoBuffers[i].size, true, true);
			address = (uint64_t)buffer_c;
			delete[] buffer_c;
		}
		if (result) {
			ptrdiff_t diff = (uint64_t)result - address;
			uint64_t final_address = memoryInfoBuffers[i].addr + diff;
			for (size_t x = 0; x < mappings_count; x++) {
				if ((memoryInfoBuffers[x].perm & Perm_Rw) == Perm_Rw && (memoryInfoBuffers[x].type == MemType_CodeMutable || memoryInfoBuffers[x].type == MemType_CodeWritable)) {
					if (memoryInfoBuffers[x].addr < cheatMetadata.main_nso_extents.base) {
						continue;
					}
					if (memoryInfoBuffers[x].addr >= cheatMetadata.main_nso_extents.base + cheatMetadata.main_nso_extents.size) {
						continue;
					}
					uint64_t* buffer = new uint64_t[memoryInfoBuffers[x].size / sizeof(uint64_t)];
					dmntchtReadCheatProcessMemory(memoryInfoBuffers[x].addr, (void*)buffer, memoryInfoBuffers[x].size);
					for (size_t y = 0; y < (memoryInfoBuffers[x].size / sizeof(uint64_t)); y++) {
						if (buffer[y] == final_address) {
							uint32_t offset = 0;
							dmntchtReadCheatProcessMemory(memoryInfoBuffers[x].addr + y*8 + 0x24, (void*)&offset, 4);
							if (offset < 0x600 || offset > 0x1000) {
								continue;
							}
							printf("Offset of FixedFrameRate: 0x%x\nPossible offset of CustomTimeStep: 0x%x\nSearching for main pointer, " CONSOLE_WHITE "OS may not respond until finished...\n\n" CONSOLE_RESET, offset, offset+0x18);
							consoleUpdate(NULL);
							delete[] buffer;

							uint32_t findings = 0;
							for (size_t y = 0; y < mappings_count; y++) {
								if (memoryInfoBuffers[y].addr < cheatMetadata.main_nso_extents.base) {
									continue;
								}
								if (memoryInfoBuffers[y].addr >= cheatMetadata.main_nso_extents.base + cheatMetadata.main_nso_extents.size) {
									continue;
								}
								if ((memoryInfoBuffers[y].perm & Perm_Rw) == Perm_Rw && (memoryInfoBuffers[y].type == MemType_CodeMutable || memoryInfoBuffers[y].type == MemType_CodeWritable)) {
									printf("Mapping %ld / %ld\r", y+1, mappings_count);
									consoleUpdate(NULL);
									buffer = new uint64_t[memoryInfoBuffers[y].size / sizeof(uint64_t)];
									dmntchtReadCheatProcessMemory(memoryInfoBuffers[y].addr, (void*)buffer, memoryInfoBuffers[y].size);
									for (size_t z = 0; z < (memoryInfoBuffers[y].size / sizeof(uint64_t)); z++) {
										if (buffer[z] % 0x1000 == 0) {
											uint32_t bitflags = 0;
											float float_value = 0;
											dmntchtReadCheatProcessMemory(buffer[z] + offset, (void*)&float_value, 4);
											dmntchtReadCheatProcessMemory(buffer[z] + offset - 4, (void*)&bitflags, 4);
											int32_t CustomTimeStep = 0;
											dmntchtReadCheatProcessMemory(buffer[z] + offset + 0x18, (void*)&CustomTimeStep, 4);
											if ((bitflags == 7 || bitflags == 0x27 || bitflags == 0x47 || bitflags == 0x67) && (float_value == 0.0 || float_value == 30.0 || float_value == 60.0)) {
												printf("FFR potential main offset: " CONSOLE_YELLOW "0x%lx" CONSOLE_RESET", float: " CONSOLE_YELLOW"%.2f" CONSOLE_RESET"\nFlags: " CONSOLE_YELLOW"0x%x\n" CONSOLE_RESET, 
													(memoryInfoBuffers[y].addr + (z * 8)) - cheatMetadata.main_nso_extents.base, float_value, bitflags);
												printf("bUseFixedFrameRate bool: " CONSOLE_YELLOW "%x\n" CONSOLE_RESET, (bool)(bitflags & 0x40));
												printf("bSmoothFrameRate bool: " CONSOLE_YELLOW "%x\n" CONSOLE_RESET, (bool)(bitflags & 0x20));
												printf("CustomTimeStep bool: " CONSOLE_YELLOW "%d\n\n" CONSOLE_RESET, CustomTimeStep);
												consoleUpdate(NULL);
												ue4_vector.push_back({"FixedFrameRate", true, (int)bitflags, float_value, (uint32_t)(memoryInfoBuffers[y].addr + (z * 8) - cheatMetadata.main_nso_extents.base), offset - 4});
												ue4_vector.push_back({"CustomTimeStep", false, CustomTimeStep, 0, (uint32_t)(memoryInfoBuffers[y].addr + (z * 8) - cheatMetadata.main_nso_extents.base), offset + 0x18});
												findings += 1;
											}
										}
									}
									delete[] buffer;
								}
							}
							if (findings > 1) {
								printf(CONSOLE_MAGENTA "?: " CONSOLE_WHITE "There are more than 1 possible candidate for FixedFrameRate address!\n" CONSOLE_RESET);
							}
							return;
						}
					}
					delete[] buffer;
				}
			}
		}
	}
	printf(CONSOLE_CYAN "FixedFrameRate string was not found!" CONSOLE_RESET " On older Unreal Engine 4 games it requires different method.\n" CONSOLE_RESET);
}

void searchDescriptionsInRAM() {
	size_t i = 0;
	bool* checkedList = new bool[settingsArray.size()]();
	size_t checkedCount = 0;
	printf("Mapping %ld / %ld\r", i+1, mappings_count);
	consoleUpdate(NULL);
	while (i < mappings_count) {
		if (checkedCount == settingsArray.size()) {
			return;
		}
		if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
			if (memoryInfoBuffers[i].size > 100'000'000) {
				i++;
				continue;
			}
			printf("Mapping %ld / %ld\r", i+1, mappings_count);
			consoleUpdate(NULL);
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
						checkedCount += 1;
						checkedList[itr] = true;
					}
				}
			}
			delete[] buffer_c;
		}
		i++;
	}
	printf("                                                \n");
	for (size_t x = 0; x < settingsArray.size(); x++) {
		if (!checkedList[x]) {
			printf(CONSOLE_RED "!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " was not found!\n", settingsArray[x].commandName);
			consoleUpdate(NULL);
			if (alternativeDescriptions1.contains(settingsArray[x].commandName)) {
				printf(CONSOLE_MAGENTA "?" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " has alternative description, searching again...\n", settingsArray[x].commandName);
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
					printf(CONSOLE_RED "!!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " alternative description search failed!\n", settingsArray[x].commandName);
					consoleUpdate(NULL);
				}
			}
		}
	}
	delete[] checkedList;
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
		fwrite(ue4_vector[i].iterator, strlen(ue4_vector[i].iterator), 1, text_file);
		fwrite("]\n", 2, 1, text_file);
		fwrite("580F0000 ", 9, 1, text_file);
		char temp[24] = "";
		snprintf(temp, sizeof(temp), "%08X\n", ue4_vector[i].offset);
		fwrite(temp, 9, 1, text_file);
		if (ue4_vector[i].add) {
			fwrite("780F0000 ", 9, 1, text_file);
			snprintf(temp, sizeof(temp), "%08X\n", ue4_vector[i].add);
			fwrite(temp, 9, 1, text_file);
		}
		if (!strcmp("CustomTimeStep", ue4_vector[i].iterator)) {
			fwrite("640F0000 00000000 ", 18, 1, text_file);
			snprintf(temp, sizeof(temp), "%08X\n\n", ue4_vector[i].default_value_int);
			fwrite(temp, 10, 1, text_file);
		}
		else {
			fwrite("680F0000 ", 9, 1, text_file);
			if (ue4_vector[i].isFloat) {
				if (!ue4_vector[i].default_value_int) {
					int temp_val = 0;
					memcpy(&temp_val, &ue4_vector[i].default_value_float, 4);
					snprintf(temp, sizeof(temp), "%08X %08X\n\n", temp_val, temp_val);
				}
				else {
					int temp_val = 0;
					memcpy(&temp_val, &ue4_vector[i].default_value_float, 4);
					snprintf(temp, sizeof(temp), "%08X %08X\n\n", temp_val, ue4_vector[i].default_value_int);
				}
			}
			else {
				snprintf(temp, sizeof(temp), "%08X %08X\n\n", ue4_vector[i].default_value_int, ue4_vector[i].default_value_int);
			}
			fwrite(temp, strlen(temp), 1, text_file);
		}
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
		fwrite(ue4_vector[i].iterator, strlen(ue4_vector[i].iterator), 1, text_file);
		char temp[128] = "";
		snprintf(temp, sizeof(temp), ", main_offset: 0x%X + 0x%X, ", ue4_vector[i].offset, ue4_vector[i].add);
		fwrite(temp, strlen(temp), 1, text_file);
		if (!strcmp("FixedFrameRate", ue4_vector[i].iterator)) {
			snprintf(temp, sizeof(temp), "flags: 0x%x, bUseFixedFrameRate: %d, bSmoothFrameRate: %d, ", ue4_vector[i].default_value_int, (bool)(ue4_vector[i].default_value_int & 0x40), (bool)(ue4_vector[i].default_value_int & 0x20));
			fwrite(temp, strlen(temp), 1, text_file);
		}
		if (ue4_vector[i].isFloat) {
			int temp_val = 0;
			memcpy(&temp_val, &ue4_vector[i].default_value_float, 4);
			snprintf(temp, sizeof(temp), "type: float %.5f / 0x%X\n", ue4_vector[i].default_value_float, temp_val);
		}
		else {
			snprintf(temp, sizeof(temp), "type: int %d / 0x%X\n", ue4_vector[i].default_value_int, ue4_vector[i].default_value_int);
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

		appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad);

		if (checkIfUE4game() && (utf_encoding = testRUN())) {
			printf("Searching RAM...\n\n");
			consoleUpdate(NULL);
			searchDescriptionsInRAM();
			printf("                                                \n");
			SearchFramerate();
			printf(CONSOLE_BLUE "\n---------------------------------------------\n\n" CONSOLE_RESET);
			printf(CONSOLE_WHITE "Search is finished!\n");
			consoleUpdate(NULL);
			dumpAsCheats();
			dumpAsLog();
		}

		appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
		
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
	ue4_vector.clear();
	consoleExit(NULL);
	return 0;
}
