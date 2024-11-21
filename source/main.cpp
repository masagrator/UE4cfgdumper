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
extern "C" {
#include "armadillo.h"
#include "strext.h"
}

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
bool isUE5 = false;

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

bool checkIfUE4game() {
	size_t i = 0;
	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx && memoryInfoBuffers[i].type == MemType_CodeStatic) {
			if (memoryInfoBuffers[i].size > 200'000'000) {
				continue;
			}
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
				ue4_sdk = result;
				isUE5 = true;
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

	size_t size = utf8_to_utf16(nullptr, (const uint8_t*)UE4settingsArray[0].description, 0);
	char16_t* utf16_string = new char16_t[size+1]();
	utf8_to_utf16((uint16_t*)utf16_string, (const uint8_t*)UE4settingsArray[0].description, size+1);

	size = utf8_to_utf32(nullptr, (const uint8_t*)UE4settingsArray[0].description, 0);
	char32_t* utf32_string = new char32_t[size+1]();
	utf8_to_utf32((uint32_t*)utf32_string, (const uint8_t*)UE4settingsArray[0].description, size+1);

	consoleUpdate(NULL);

	while (i < mappings_count) {
		if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
			if (memoryInfoBuffers[i].size > 200'000'000) {
				continue;
			}
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			char* result = searchString(buffer_c, (char*)UE4settingsArray[0].description, memoryInfoBuffers[i].size);
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
			if (memoryInfoBuffers[k].size > 200'000'000) {
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
						if (memoryInfoBuffers[l].size > 200'000'000) {
							continue;
						}
						buffer_u = new uint64_t[memoryInfoBuffers[l].size / sizeof(uint64_t)];
						dmntchtReadCheatProcessMemory(memoryInfoBuffers[l].addr, (void*)buffer_u, memoryInfoBuffers[l].size);
						uint64_t pointer2_address = 0;
						for (size_t x = 0; x < memoryInfoBuffers[l].size / sizeof(uint64_t); x++) {
							if (buffer_u[x] == pointer_address) {
								if (x+1 != memoryInfoBuffers[l].size / sizeof(uint64_t) 
									&& (int64_t)(buffer_u[x+1]) - buffer_u[x] > 0 
									&& (int64_t)(buffer_u[x+1]) - buffer_u[x] <= 0x98)
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
		char* FFR_result = 0;
		char* CTS_result = 0;
		uint64_t address = 0;
		if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx && (memoryInfoBuffers[i].type == MemType_CodeStatic || memoryInfoBuffers[i].type == MemType_CodeReadOnly)) {
			if (memoryInfoBuffers[i].size > 200'000'000) {
				continue;
			}
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			FFR_result = (char*)searchString(buffer_c, (isUE5 ? "bUseFixedFrameRate" : "FixedFrameRate"), memoryInfoBuffers[i].size, true, true);
			if (FFR_result) {
				if (isUE5) {
					FFR_result = &FFR_result[4]; 
				}
				CTS_result = (char*)searchString(buffer_c, "CustomTimeStep", memoryInfoBuffers[i].size, true, true);
			}
			address = (uint64_t)buffer_c;
			delete[] buffer_c;
		}
		else continue;
		if (!FFR_result) continue;

		ptrdiff_t FFR_diff = (uint64_t)FFR_result - address;
		uint64_t FFR_final_address = memoryInfoBuffers[i].addr + FFR_diff;

		ptrdiff_t CTS_diff = 0;
		uint64_t CTS_final_address = 0;
		if (CTS_result) {
			CTS_diff = (uint64_t)CTS_result - address;
			CTS_final_address = memoryInfoBuffers[i].addr + CTS_diff;
		}
		else {
			printf(CONSOLE_YELLOW "CustomTimeStep" CONSOLE_RESET " not found!\n");
			consoleUpdate(NULL);
		}
		for (size_t x = 0; x < mappings_count; x++) {
			if ((memoryInfoBuffers[x].perm & Perm_Rx) != Perm_Rx && (memoryInfoBuffers[x].type == MemType_CodeMutable || memoryInfoBuffers[x].type == MemType_CodeWritable)) {
				if (memoryInfoBuffers[x].addr < cheatMetadata.main_nso_extents.base) {
					continue;
				}
				if (memoryInfoBuffers[x].addr >= cheatMetadata.main_nso_extents.base + cheatMetadata.main_nso_extents.size) {
					continue;
				}
				if (memoryInfoBuffers[x].size > 200'000'000) {
					continue;
				}
				uint64_t* buffer = new uint64_t[memoryInfoBuffers[x].size / sizeof(uint64_t)];
				dmntchtReadCheatProcessMemory(memoryInfoBuffers[x].addr, (void*)buffer, memoryInfoBuffers[x].size);
				uint32_t offset = 0;
				uint32_t offset2 = 0;
				size_t itr = 0;
				while (itr < (memoryInfoBuffers[x].size / sizeof(uint64_t))) {
					if (buffer[itr] == FFR_final_address) {
						uint32_t offset_temp = 0;
						dmntchtReadCheatProcessMemory(memoryInfoBuffers[x].addr + itr*8 + (isUE5 ? 0x38 : 0x24), (void*)&offset_temp, 4);
						if (offset_temp < 0x600 || offset_temp > 0x1000) {
							if (isUE5) {
								offset_temp = 0;
								dmntchtReadCheatProcessMemory(memoryInfoBuffers[x].addr + itr*8 + 0x32, (void*)&offset_temp, 2);
								if (offset_temp < 0x600 || offset_temp > 0x1000) {
									itr++;
									continue;
								}
							}
							else {
								itr++;
								continue;
							}
						}
						offset = offset_temp;
						break;
					}
					itr++;
				}
				if (!offset) {
					delete[] buffer;
					continue;
				}

				printf("Offset of " CONSOLE_YELLOW "FixedFrameRate" CONSOLE_RESET ": 0x%x\n", offset);
				consoleUpdate(NULL);
				if (CTS_final_address) {
					while (itr < (memoryInfoBuffers[x].size / sizeof(uint64_t))) {
						if (buffer[itr] == CTS_final_address) {
							uint32_t offset_temp = 0;
							dmntchtReadCheatProcessMemory(memoryInfoBuffers[x].addr + itr*8 + (isUE5 ? 0x38 : 0x24), (void*)&offset_temp, 4);
							if (offset_temp < 0x600 || offset_temp > 0x1000) {
								if (isUE5) {
									offset_temp = 0;
									dmntchtReadCheatProcessMemory(memoryInfoBuffers[x].addr + itr*8 + 0x32, (void*)&offset_temp, 2);
									if (offset_temp < 0x600 || offset_temp > 0x1000) {
										itr++;
										continue;
									}
								}
								else {
									itr++;
									continue;
								}
							}
							offset2 = offset_temp;
							break;
						}
						itr++;
					}
				}
				delete[] buffer;
				if (offset2) {
					printf("Offset of " CONSOLE_YELLOW " CustomTimeStep" CONSOLE_RESET ": 0x%x\n", offset2);
				}
				consoleUpdate(NULL);
				for (size_t y = 0; y < mappings_count; y++) {
					if (memoryInfoBuffers[y].addr != cheatMetadata.main_nso_extents.base) {
						continue;
					}
					uint8_t* buffer_two = new uint8_t[memoryInfoBuffers[y].size];
					dmntchtReadCheatProcessMemory(memoryInfoBuffers[y].addr, (void*)buffer_two, memoryInfoBuffers[y].size);
					uint8_t pattern[] = {0xA8, 0x99, 0x99, 0x52, 0x88, 0xB9, 0xA7, 0x72, 0x01, 0x10, 0x2C, 0x1E, 0x00, 0x01, 0x27, 0x1E, 0x60, 0x01, 0x80, 0x52};
					auto it = std::search(buffer_two, &buffer_two[memoryInfoBuffers[y].size], pattern, &pattern[sizeof(pattern)]);
					if (it != &buffer_two[memoryInfoBuffers[y].size]) {
						auto distance = std::distance(buffer_two, it);
						uint32_t first_instruction = *(uint32_t*)&buffer_two[distance-(8 * 4)];
						uint32_t second_instruction = *(uint32_t*)&buffer_two[distance-(7 * 4)];
						ad_insn *insn = NULL;
						uint32_t main_offset = 0;
						ArmadilloDisassemble(first_instruction, distance, &insn);
						if (insn -> instr_id == AD_INSTR_ADRP) {
							main_offset = insn -> operands[1].op_imm.bits;
							ArmadilloDone(&insn);
							ArmadilloDisassemble(second_instruction, distance * 4, &insn);
							if (insn -> instr_id == AD_INSTR_LDR && insn -> num_operands == 3 && insn -> operands[2].type == AD_OP_IMM) {
								main_offset += insn -> operands[2].op_imm.bits;
								ArmadilloDone(&insn);
								uint64_t GameEngine_ptr = 0;
								dmntchtReadCheatProcessMemory(cheatMetadata.main_nso_extents.base + main_offset, (void*)&GameEngine_ptr, 8);
								printf("Main offset of GameEngine pointer: " CONSOLE_YELLOW "0x%lX\n" CONSOLE_RESET, GameEngine_ptr - cheatMetadata.main_nso_extents.base);
								uint64_t GameEngine = 0;
								dmntchtReadCheatProcessMemory(GameEngine_ptr, (void*)&GameEngine, 8);
								uint32_t bitflags = 0;
								dmntchtReadCheatProcessMemory(GameEngine + (offset - 4), (void*)&bitflags, 4);
								printf("Bitflags: " CONSOLE_YELLOW "0x%x\n" CONSOLE_RESET, bitflags);
								printf("bUseFixedFrameRate bool: " CONSOLE_YELLOW "%x\n" CONSOLE_RESET, (bool)(bitflags & 0x40));
								printf("bSmoothFrameRate bool: " CONSOLE_YELLOW "%x\n" CONSOLE_RESET, (bool)(bitflags & 0x20));
								float FixedFrameRate = 0;
								dmntchtReadCheatProcessMemory(GameEngine + offset, (void*)&FixedFrameRate, 4);
								printf("FixedFrameRate: " CONSOLE_YELLOW "%.4f\n" CONSOLE_RESET, FixedFrameRate);
								ue4_vector.push_back({"FixedFrameRate", true, (int)bitflags, FixedFrameRate, (uint32_t)(GameEngine_ptr - cheatMetadata.main_nso_extents.base), offset - 4});
								if (offset2) {
									int CustomTimeStep = 0;
									dmntchtReadCheatProcessMemory(GameEngine + offset2, (void*)&CustomTimeStep, 4);
									printf("CustomTimeStep: " CONSOLE_YELLOW "0x%x\n" CONSOLE_RESET, CustomTimeStep);
									ue4_vector.push_back({"CustomTimeStep", false, CustomTimeStep, 0, (uint32_t)(GameEngine_ptr - cheatMetadata.main_nso_extents.base), offset2});
								}
							}
							else {
								ArmadilloDone(&insn);
								printf("Second instruction is not LDR! %s\n", insn -> decoded);
							}
						}
						else {
							ArmadilloDone(&insn);
							printf("First instruction is not ADRP! %s\n", insn -> decoded);
						}
					}
					else printf("Couldn't find pattern for GameEngine struct!\n");
					consoleUpdate(NULL);
					delete[] buffer_two;
				}
				return;
			}
		}
	}
	printf(CONSOLE_CYAN "FixedFrameRate string was not found!" CONSOLE_RESET);
	if (!isUE5) printf(" On older Unreal Engine 4 games it requires different method.");
	printf("\n");
	consoleUpdate(NULL);
}

void searchDescriptionsInRAM() {
	size_t i = 0;
	bool* UE4checkedList = new bool[UE4settingsArray.size()]();
	size_t ue4checkedCount = 0;
	printf("Mapping %ld / %ld\r", i+1, mappings_count);
	consoleUpdate(NULL);
	while (i < mappings_count) {
		if (ue4checkedCount == UE4settingsArray.size()) {
			return;
		}
		if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
			if (memoryInfoBuffers[i].size > 200'000'000) {
				i++;
				continue;
			}
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			char* result = 0;
			for (size_t itr = 0; itr < UE4settingsArray.size(); itr++) {
				if (UE4checkedList[itr]) {
					continue;
				}
				result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, UE4settingsArray[itr].description);
				if (result) {
					ptrdiff_t diff = (uint64_t)result - (uint64_t)buffer_c;
					uint64_t string_address = memoryInfoBuffers[i].addr + diff;
					if (searchPointerInMappings(string_address, UE4settingsArray[itr].commandName, UE4settingsArray[itr].type, itr)) {
						printf("Mapping %ld / %ld\r", i+1, mappings_count);
						consoleUpdate(NULL);
						ue4checkedCount += 1;
						UE4checkedList[itr] = true;
					}
				}
			}
			delete[] buffer_c;
		}
		i++;
	}
	printf("                                                \n");
	for (size_t x = 0; x < UE4settingsArray.size(); x++) {
		if (!UE4checkedList[x]) {
			if (isUE5) {
				if (UE5DeprecatedUE4Settings.contains(UE4settingsArray[x].commandName)) {
					continue;
				}
			}
			if (UE4alternativeDescriptions1.contains(UE4settingsArray[x].commandName)) {
				i = 0;
				while (i < mappings_count) {
					if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
						if (memoryInfoBuffers[i].size > 200'000'000) {
							i++;
							continue;
						}
						char* buffer_c = new char[memoryInfoBuffers[i].size];
						dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
						char* result = 0;
						result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, UE4alternativeDescriptions1[UE4settingsArray[x].commandName].c_str());
						if (result) {
							ptrdiff_t diff = (uint64_t)result - (uint64_t)buffer_c;
							uint64_t string_address = memoryInfoBuffers[i].addr + diff;
							if (searchPointerInMappings(string_address, UE4settingsArray[x].commandName, UE4settingsArray[x].type, x)) {
								UE4checkedList[x] = true;
								i = mappings_count;
							}
						}
						delete[] buffer_c;
					}
					i++;
				}
			}
		}
		if (isUE5 && !UE4checkedList[x]) {
			if (UE4toUE5alternativeDescriptions1.contains(UE4settingsArray[x].commandName)) {
				i = 0;
				while (i < mappings_count) {
					if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
						if (memoryInfoBuffers[i].size > 200'000'000) {
							i++;
							continue;
						}
						char* buffer_c = new char[memoryInfoBuffers[i].size];
						dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
						char* result = 0;
						result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, UE4toUE5alternativeDescriptions1[UE4settingsArray[x].commandName].c_str());
						if (result) {
							ptrdiff_t diff = (uint64_t)result - (uint64_t)buffer_c;
							uint64_t string_address = memoryInfoBuffers[i].addr + diff;
							if (searchPointerInMappings(string_address, UE4settingsArray[x].commandName, UE4settingsArray[x].type, x)) {
								UE4checkedList[x] = true;
								i = mappings_count;
							}
						}
						delete[] buffer_c;
					}
					i++;
				}
			}
		}
		if (isUE5 && !UE4checkedList[x]) {
			if (UE4toUE5alternativeDescriptions2.contains(UE4settingsArray[x].commandName)) {
				i = 0;
				while (i < mappings_count) {
					if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
						if (memoryInfoBuffers[i].size > 200'000'000) {
							i++;
							continue;
						}
						char* buffer_c = new char[memoryInfoBuffers[i].size];
						dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
						char* result = 0;
						result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, UE4toUE5alternativeDescriptions2[UE4settingsArray[x].commandName].c_str());
						if (result) {
							ptrdiff_t diff = (uint64_t)result - (uint64_t)buffer_c;
							uint64_t string_address = memoryInfoBuffers[i].addr + diff;
							if (searchPointerInMappings(string_address, UE4settingsArray[x].commandName, UE4settingsArray[x].type, x)) {
								UE4checkedList[x] = true;
								i = mappings_count;
							}
						}
						delete[] buffer_c;
					}
					i++;
				}
			}
		}
	}
	for (size_t x = 0; x < UE4settingsArray.size(); x++) {
		if (UE4checkedList[x])
			continue;
		if (UE4alternativeDescriptions1.contains(UE4settingsArray[x].commandName) && !UE4toUE5alternativeDescriptions1.contains(UE4settingsArray[x].commandName) && !UE4toUE5alternativeDescriptions2.contains(UE4settingsArray[x].commandName)) {
			printf(CONSOLE_RED "!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " was not found even with UE4 alt description!\n", UE4settingsArray[x].commandName);
			consoleUpdate(NULL);
			continue;
		}
		if (isUE5 && UE4toUE5alternativeDescriptions1.contains(UE4settingsArray[x].commandName) && !UE4toUE5alternativeDescriptions2.contains(UE4settingsArray[x].commandName)) {
			printf(CONSOLE_RED "!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " was not found even with UE5 alt 1 description!\n", UE4settingsArray[x].commandName);
			consoleUpdate(NULL);
			continue;
		}
		if (isUE5 && UE4toUE5alternativeDescriptions2.contains(UE4settingsArray[x].commandName)) {
			printf(CONSOLE_RED "!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " was not found even with UE5 alt 2 description!\n", UE4settingsArray[x].commandName);
			consoleUpdate(NULL);
			continue;
		}
		if (isUE5 && UE5DeprecatedUE4Settings.contains(UE4settingsArray[x].commandName)) {
			printf(CONSOLE_RED "!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " was deprecated on UE5!", UE4settingsArray[x].commandName);
			if (UE5DeprecatedUE4Settings[UE4settingsArray[x].commandName].compare("")) {
				printf(CONSOLE_CYAN " %s" CONSOLE_RESET, UE5DeprecatedUE4Settings[UE4settingsArray[x].commandName].c_str());
			}
			printf("\n");
			consoleUpdate(NULL);
			continue;
		}
		printf(CONSOLE_RED "!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " was not found!\n", UE4settingsArray[x].commandName);
		consoleUpdate(NULL);
	}
	delete[] UE4checkedList;
}

void searchDescriptionsInRAM_UE5() {
	size_t i = 0;
	bool* UE5checkedList = new bool[UE5settingsArray.size()]();
	size_t ue5checkedCount = 0;
	printf("\n---\nLooking for UE5 specific settings...\n---\n");
	consoleUpdate(NULL);
	printf("Mapping %ld / %ld\r", i+1, mappings_count);
	consoleUpdate(NULL);
	while (i < mappings_count) {
		if (ue5checkedCount == UE5settingsArray.size()) {
			return;
		}
		if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
			if (memoryInfoBuffers[i].size > 200'000'000) {
				i++;
				continue;
			}
			char* buffer_c = new char[memoryInfoBuffers[i].size];
			dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
			char* result = 0;
			for (size_t itr = 0; itr < UE5settingsArray.size(); itr++) {
				if (UE5checkedList[itr]) {
					continue;
				}
				result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, UE5settingsArray[itr].description);
				if (result) {
					ptrdiff_t diff = (uint64_t)result - (uint64_t)buffer_c;
					uint64_t string_address = memoryInfoBuffers[i].addr + diff;
					if (searchPointerInMappings(string_address, UE5settingsArray[itr].commandName, UE5settingsArray[itr].type, itr)) {
						printf("Mapping %ld / %ld\r", i+1, mappings_count);
						consoleUpdate(NULL);
						ue5checkedCount += 1;
						UE5checkedList[itr] = true;
					}
				}
			}
			delete[] buffer_c;
		}
		i++;
	}
	printf("                                                \n");
	for (size_t x = 0; x < UE5settingsArray.size(); x++) {
		if (UE5alternativeDescriptions1.contains(UE5settingsArray[x].commandName)) {
			i = 0;
			while (i < mappings_count) {
				if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw && memoryInfoBuffers[i].type == MemType_Heap) {
					if (memoryInfoBuffers[i].size > 200'000'000) {
						i++;
						continue;
					}
					char* buffer_c = new char[memoryInfoBuffers[i].size];
					dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void*)buffer_c, memoryInfoBuffers[i].size);
					char* result = 0;
					result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, UE5alternativeDescriptions1[UE5settingsArray[x].commandName].c_str());
					if (result) {
						ptrdiff_t diff = (uint64_t)result - (uint64_t)buffer_c;
						uint64_t string_address = memoryInfoBuffers[i].addr + diff;
						if (searchPointerInMappings(string_address, UE5settingsArray[x].commandName, UE5settingsArray[x].type, x)) {
							UE5checkedList[x] = true;
							i = mappings_count;
						}
					}
					delete[] buffer_c;
				}
				i++;
			}
		}
		else if (!UE5checkedList[x]) {
			printf(CONSOLE_RED "!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " was not found!\n", UE5settingsArray[x].commandName);
			consoleUpdate(NULL);
		}
	}
	for (size_t x = 0; x < UE5settingsArray.size(); x++) {
		if (UE5checkedList[x])
			continue;
		if (UE5alternativeDescriptions1.contains(UE5settingsArray[x].commandName)) {
			printf(CONSOLE_RED "!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " was not found even with alt description!\n", UE5settingsArray[x].commandName);
			consoleUpdate(NULL);
			continue;
		}
		printf(CONSOLE_RED "!" CONSOLE_RESET ": " CONSOLE_CYAN "%s" CONSOLE_RESET " was not found!\n", UE5settingsArray[x].commandName);
		consoleUpdate(NULL);
	}
	delete[] UE5checkedList;
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

		if (checkIfUE4game() && (utf_encoding = testRUN())) {
			bool FullScan = true;
			printf("\n----------\nPress A for Full Scan\n");
			printf("Press X for Base Scan (it excludes FixedFrameRate and CustomTimeStep)\n");
			printf("Press + to Exit\n\n");
			consoleUpdate(NULL);
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
				
				if (kDown & HidNpadButton_X) {
					FullScan = false;
					break;
				}
			}
			printf("Searching RAM...\n\n");
			consoleUpdate(NULL);
			appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad);
			searchDescriptionsInRAM();
			if (isUE5) searchDescriptionsInRAM_UE5();
			printf("                                                \n");
			if (FullScan) SearchFramerate();
			printf(CONSOLE_BLUE "\n---------------------------------------------\n\n" CONSOLE_RESET);
			printf(CONSOLE_WHITE "Search is finished!\n");
			consoleUpdate(NULL);
			dumpAsCheats();
			dumpAsLog();
			appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
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
	ue4_vector.clear();
	consoleExit(NULL);
	return 0;
}