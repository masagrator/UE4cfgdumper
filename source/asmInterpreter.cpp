#include "asmInterpreter.hpp"
#include "UnityDumps.hpp"

void wrongOperand(ad_insn* insn, const DmntCheatProcessMetadata cheatMetadata, uint64_t address) {
	printf("Unsupported instruction! %s\n", insn -> decoded);
	printf("offset: 0x%lx\n", address - cheatMetadata.main_nso_extents.base);
	printf("Operands:\n");
	for (int i = 0; i < insn -> num_operands; i++) {
		printf("%d. type: ", i);
		if (insn -> operands[i].type == AD_OP_REG) {
			printf("REG: %s\n", insn -> operands[i].op_reg.rtbl[0]);
		}
		else if (insn -> operands[i].type == AD_OP_IMM) {
			printf("IMM: 0x%lx\n", insn -> operands[i].op_imm.bits);
		}
		else if (insn -> operands[i].type == AD_OP_SHIFT) {
			printf("SHIFT: type: %d, amt: %d\n", insn -> operands[i].op_shift.type, insn -> operands[i].op_shift.amt);
		}
		else printf("MEM\n");
	}
	ArmadilloDone(&insn);
}

MachineState machineState_copy = {0};
int registerPointer = 0;
long immediate_offset = 0;
bool cmp_flag = false;

struct pointerDump {
	UnityData* data;
	std::string toPrint;
	std::string register_print;
};

void dumpPointers(const std::vector<std::string> UnityNames, const std::vector<uint32_t> UnityOffsets, const DmntCheatProcessMetadata cheatMetadata, std::string unity_sdk) {
	ad_insn *insn = NULL;
	MachineState machineState = {0};
	std::vector<pointerDump> result;
	std::vector<std::string> forPass;
	for (size_t i = 0; i < unityDataStruct.size(); i++) {
		forPass.clear();
		auto itr = std::find(UnityNames.begin(), UnityNames.end(), unityDataStruct[i].search_name);
		if (itr == UnityNames.end()) {
			printf("%s was not found!\n", unityDataStruct[i].search_name);
			consoleUpdate(NULL);
			continue;
		}
		size_t index = itr - UnityNames.begin();
		uint64_t start_address = cheatMetadata.main_nso_extents.base + UnityOffsets[index];
		uint32_t instruction = 0;
		std::vector<uint64_t> returns;
		while(true) {
			dmntchtReadCheatProcessMemory(start_address, (void*)&instruction, sizeof(uint32_t));
			if (returns.size() == 0 && instruction == 0xD65F03C0)
				break;
			if (insn) {
				ArmadilloDone(&insn);
			}
			int rc = ArmadilloDisassemble(instruction, start_address, &insn);
			if (rc) {
				printf("Disassembler error! 0x%x/%d\n", rc, rc);
				ArmadilloDone(&insn);
				return;
			}
			else {
				forPass.push_back(insn -> decoded);
			}
			uint8_t readSize = 0;
			switch(insn -> instr_id) {
				case AD_INSTR_ADD: {
					if (insn -> operands[2].type == AD_OP_REG) {
						machineState.X[insn -> operands[0].op_reg.rn] = machineState.X[insn -> operands[1].op_reg.rn] + machineState.X[insn -> operands[2].op_reg.rn];
					}
					else if (insn -> operands[2].type == AD_OP_IMM) {
						machineState.X[insn -> operands[0].op_reg.rn] = machineState.X[insn -> operands[1].op_reg.rn] + insn -> operands[2].op_imm.bits;
					}
					else {
						wrongOperand(insn, cheatMetadata, start_address);
						return;						
					}
					break;
				}
				case AD_INSTR_ADRP: {
					machineState.X[insn -> operands[0].op_reg.rn] = insn -> operands[1].op_imm.bits;
					break;
				}
				case AD_INSTR_B: {
					start_address = insn -> operands[0].op_imm.bits - 4;
					break;
				}
				case AD_INSTR_BL: {
					if (insn -> num_operands != 1) {
						wrongOperand(insn, cheatMetadata, start_address);
						return;									
					}
					returns.push_back(start_address);
					start_address = insn -> operands[0].op_imm.bits - 4;
					break;
				}
				case AD_INSTR_BR: {
					start_address = machineState.X[insn -> operands[0].op_reg.rn] - 4;
					break;
				}
				case AD_INSTR_CMP: {
					cmp_flag = machineState.X[insn -> operands[0].op_reg.rn] == machineState.X[insn -> operands[1].op_reg.rn];
					break;
				}
				case AD_INSTR_LDP: {
					break;
				}
				case AD_INSTR_LDRB:
					readSize = 1;
				case AD_INSTR_LDRH:
					if (!readSize) readSize = 2;
				case AD_INSTR_LDR: {
					if (insn -> num_operands == 4) {
						if (insn -> operands[0].op_reg.fp) {
							if ((insn -> operands[3].type != AD_OP_IMM) || (insn -> operands[3].op_imm.bits < 0)) {
								wrongOperand(insn, cheatMetadata, start_address);
								return;
							}
							if (insn -> operands[0].op_reg.rtbl[0][0] == 's') {
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn] + (machineState.X[insn -> operands[2].op_reg.rn] << insn -> operands[3].op_imm.bits), (void*)&machineState.S[insn -> operands[0].op_reg.rn], sizeof(uint64_t));
							}
							else if (insn -> operands[0].op_reg.rtbl[0][0] == 'd') {
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn] + (machineState.X[insn -> operands[2].op_reg.rn] << insn -> operands[3].op_imm.bits), (void*)&machineState.D[insn -> operands[0].op_reg.rn], sizeof(uint64_t));
							}
							else {
								wrongOperand(insn, cheatMetadata, start_address);
								return;
							}
						}
						else {
							if ((insn -> operands[3].type != AD_OP_IMM) || (insn -> operands[3].op_imm.bits < 0)) {
								wrongOperand(insn, cheatMetadata, start_address);
								return;
							}
							if (insn -> operands[0].op_reg.rtbl[0][0] == 'w') {
								machineState.X[insn -> operands[0].op_reg.rn] = 0;
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn] + (machineState.X[insn -> operands[2].op_reg.rn] << insn -> operands[3].op_imm.bits), (void*)&machineState.X[insn -> operands[0].op_reg.rn], (readSize ? readSize : sizeof(uint32_t)));
							}
							else if (insn -> operands[0].op_reg.rtbl[0][0] == 'x')  {
								machineState.X[insn -> operands[0].op_reg.rn] = 0;
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn] + (machineState.X[insn -> operands[2].op_reg.rn] << insn -> operands[3].op_imm.bits), (void*)&machineState.X[insn -> operands[0].op_reg.rn], (readSize ? readSize : sizeof(uint64_t)));
							}
							else {
								wrongOperand(insn, cheatMetadata, start_address);
								return;
							}
						}
					}
					
					else if (insn -> num_operands == 3) {
						if (insn -> operands[0].op_reg.fp) {
							if (insn -> operands[0].op_reg.rtbl[0][0] == 's') {
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn] + (insn -> operands[2].op_imm.bits), (void*)&machineState.S[insn -> operands[0].op_reg.rn], sizeof(uint64_t));
								if (insn -> operands[0].op_reg.rtbl[0][0] == unityDataStruct[i].data_type && insn -> operands[0].op_reg.rn == 0) {
									memcpy(&machineState_copy, &machineState, sizeof(machineState_copy));
									registerPointer = insn -> operands[1].op_reg.rn;
									immediate_offset = insn -> operands[2].op_imm.bits;
								}
							}
							else if (insn -> operands[0].op_reg.rtbl[0][0] == 'd') {
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn] + (insn -> operands[2].op_imm.bits), (void*)&machineState.D[insn -> operands[0].op_reg.rn], sizeof(uint64_t));
								if (insn -> operands[0].op_reg.rtbl[0][0] == unityDataStruct[i].data_type && insn -> operands[0].op_reg.rn == 0) {
									memcpy(&machineState_copy, &machineState, sizeof(machineState_copy));
									registerPointer = insn -> operands[1].op_reg.rn;
									immediate_offset = insn -> operands[2].op_imm.bits;
								}
							}
							else {
								wrongOperand(insn, cheatMetadata, start_address);
								return;
							}
						}
						else {
							if (insn -> operands[0].op_reg.rtbl[0][0] == 'w') {
								machineState.X[insn -> operands[0].op_reg.rn] = 0;
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn] + (insn -> operands[2].op_imm.bits), (void*)&machineState.X[insn -> operands[0].op_reg.rn], (readSize ? readSize : sizeof(uint32_t)));
								if (insn -> operands[0].op_reg.rtbl[0][0] == unityDataStruct[i].data_type && insn -> operands[0].op_reg.rn == 0) {
									memcpy(&machineState_copy, &machineState, sizeof(machineState_copy));
									registerPointer = insn -> operands[1].op_reg.rn;
									immediate_offset = insn -> operands[2].op_imm.bits;
								}
							}
							else if (insn -> operands[0].op_reg.rtbl[0][0] == 'x')  {
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn] + (insn -> operands[2].op_imm.bits), (void*)&machineState.X[insn -> operands[0].op_reg.rn], (readSize ? readSize : sizeof(uint64_t)));
								if (insn -> operands[0].op_reg.rtbl[0][0] == unityDataStruct[i].data_type && insn -> operands[0].op_reg.rn == 0) {
									memcpy(&machineState_copy, &machineState, sizeof(machineState_copy));
									registerPointer = insn -> operands[1].op_reg.rn;
									immediate_offset = insn -> operands[2].op_imm.bits;
								}
							}
							else {
								wrongOperand(insn, cheatMetadata, start_address);
								return;
							}
						}
					}
					else if (insn -> num_operands == 2) {
						if (insn -> operands[0].op_reg.fp) {
							if (insn -> operands[0].op_reg.rtbl[0][0] == 's') {
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn], (void*)&machineState.S[insn -> operands[0].op_reg.rn], sizeof(uint64_t));
								if (insn -> operands[0].op_reg.rtbl[0][0] == unityDataStruct[i].data_type && insn -> operands[0].op_reg.rn == 0) {
									memcpy(&machineState_copy, &machineState, sizeof(machineState_copy));
									registerPointer = insn -> operands[1].op_reg.rn;
									immediate_offset = 0;
								}
							}
							else if (insn -> operands[0].op_reg.rtbl[0][0] == 'd') {
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn], (void*)&machineState.D[insn -> operands[0].op_reg.rn], sizeof(uint64_t));
								if (insn -> operands[0].op_reg.rtbl[0][0] == unityDataStruct[i].data_type && insn -> operands[0].op_reg.rn == 0) {
									memcpy(&machineState_copy, &machineState, sizeof(machineState_copy));
									registerPointer = insn -> operands[1].op_reg.rn;
									immediate_offset = 0;
								}
							}
							else {
								wrongOperand(insn, cheatMetadata, start_address);
								return;
							}
						}
						else {
							if (insn -> operands[0].op_reg.rtbl[0][0] == 'w') {
								machineState.X[insn -> operands[0].op_reg.rn] = 0;
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn], (void*)&machineState.X[insn -> operands[0].op_reg.rn], (readSize ? readSize : sizeof(uint32_t)));
								if (insn -> operands[0].op_reg.rtbl[0][0] == unityDataStruct[i].data_type && insn -> operands[0].op_reg.rn == 0) {
									memcpy(&machineState_copy, &machineState, sizeof(machineState_copy));
									registerPointer = insn -> operands[1].op_reg.rn;
									immediate_offset = 0;
								}
							}
							else if (insn -> operands[0].op_reg.rtbl[0][0] == 'x') {
								dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn], (void*)&machineState.X[insn -> operands[0].op_reg.rn], (readSize ? readSize : sizeof(uint64_t)));
								if (insn -> operands[0].op_reg.rtbl[0][0] == unityDataStruct[i].data_type && insn -> operands[0].op_reg.rn == 0) {
									memcpy(&machineState_copy, &machineState, sizeof(machineState_copy));
									registerPointer = insn -> operands[1].op_reg.rn;
									immediate_offset = 0;
								}
							}
							else {
								wrongOperand(insn, cheatMetadata, start_address);
								return;
							}
						}
					}
					else {
						wrongOperand(insn, cheatMetadata, start_address);
						return;		
					}
					break;
				}
				case AD_INSTR_LDRSW: {
					if (insn -> num_operands > 3 || insn -> operands[0].op_reg.rtbl[0][0] != 'x') {
						wrongOperand(insn, cheatMetadata, start_address);
						return;
					}
					machineState.X[insn -> operands[0].op_reg.rn] = 0;
					int32_t temp_word = 0;
					if (insn -> num_operands == 3) {
						dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn] + (insn -> operands[2].op_imm.bits), (void*)&temp_word, sizeof(int32_t));
					}
					else {
						dmntchtReadCheatProcessMemory((uint64_t)machineState.X[insn -> operands[1].op_reg.rn], (void*)&temp_word, sizeof(int32_t));
					}
					int64_t new_word = temp_word;
					memcpy(&machineState.X[insn -> operands[0].op_reg.rn], &new_word, sizeof(int64_t));
					break;
				}
				case AD_INSTR_MADD: {
					if (insn -> num_operands != 4 || ((insn -> operands[0].op_reg.rtbl[0][0] | insn -> operands[1].op_reg.rtbl[0][0] | insn -> operands[2].op_reg.rtbl[0][0] | insn -> operands[3].op_reg.rtbl[0][0]) != insn -> operands[0].op_reg.rtbl[0][0])) {
						wrongOperand(insn, cheatMetadata, start_address);
						return;
					}
					machineState.X[insn -> operands[0].op_reg.rn] = (machineState.X[insn -> operands[1].op_reg.rn] * machineState.X[insn -> operands[2].op_reg.rn]) + machineState.X[insn -> operands[3].op_reg.rn];
					break;
				}				
				case AD_INSTR_MOV: {
					if (insn -> num_operands != 2) {
						wrongOperand(insn, cheatMetadata, start_address);
						return;
					}
					if (insn -> operands[1].type == AD_OP_REG) {
						machineState.X[insn -> operands[0].op_reg.rn] = machineState.X[insn -> operands[1].op_reg.rn];
					}
					else if (insn -> operands[1].type == AD_OP_IMM) {
						machineState.X[insn -> operands[0].op_reg.rn] = insn -> operands[1].op_imm.bits;
					}
					else {
						wrongOperand(insn, cheatMetadata, start_address);
						return;					
					}
					break;
				}
				case AD_INSTR_RET: {
					start_address = returns.back();
					returns.pop_back();
					break;
				}
				case AD_INSTR_SMADDL: {
					if (insn -> num_operands != 4 || insn -> operands[0].op_reg.rtbl[0][0] != 'x' || insn -> operands[1].op_reg.rtbl[0][0] != 'w' || insn -> operands[2].op_reg.rtbl[0][0] != 'w' || insn -> operands[3].op_reg.rtbl[0][0] != 'x') {
						wrongOperand(insn, cheatMetadata, start_address);
						return;
					}
					int32_t Wn = machineState.X[insn -> operands[1].op_reg.rn];
					int32_t Wm = machineState.X[insn -> operands[2].op_reg.rn];
					Wn *= Wm;
					int64_t Xa = machineState.X[insn -> operands[3].op_reg.rn];
					Xa += Wn;
					machineState.X[insn -> operands[0].op_reg.rn] = Xa;
					break;
				}
				case AD_INSTR_STP:
				case AD_INSTR_STUR:
				case AD_INSTR_STR: {
					break;
				}
				case AD_INSTR_SUB: {
					break;
				}
				default: {
					wrongOperand(insn, cheatMetadata, start_address);
					return;
				}
			}
			start_address += 4;
		}
		char smallToPrint[64] = "";
		if (unityDataStruct[i].get == false)
			printf("%s: set = no register dump\n", unityDataStruct[i].output_name);
		else {
			printf("%s: Register %c0=", unityDataStruct[i].output_name, unityDataStruct[i].data_type);
			switch(unityDataStruct[i].data_type) {
				case 'w':
					printf("%d\n", (uint32_t)machineState_copy.X[0]);
					if (unityDataStruct[i].get)
						snprintf(smallToPrint, sizeof(smallToPrint), "int32=%d", (uint32_t)machineState_copy.X[0]);
					break;
				case 'x':
					printf("%ld\n", machineState_copy.X[0]);
					if (unityDataStruct[i].get)
						snprintf(smallToPrint, sizeof(smallToPrint), "int64=%ld", machineState_copy.X[0]);
					break;
				case 's':
					printf("%f\n", machineState_copy.S[0]);
					if (unityDataStruct[i].get)
						snprintf(smallToPrint, sizeof(smallToPrint), "float=%f", machineState_copy.S[0]);
					break;
				case 'd':
					printf("%f\n", machineState_copy.D[0]);
					if (unityDataStruct[i].get)
						snprintf(smallToPrint, sizeof(smallToPrint), "double=%f", machineState_copy.D[0]);
					break;
			}
		}
		if (insn)
			ArmadilloDone(&insn);
		std::string toReturn = "";
		for (size_t x = 0; x < forPass.size(); x++) {
			toReturn += forPass[x];
			if (x+1 < forPass.size()) toReturn += "\n";
		}
		result.push_back({&unityDataStruct[i], toReturn, smallToPrint});
		consoleUpdate(NULL);
	}
	uint64_t BID = 0;
	char path[128] = "";
	memcpy(&BID, &(cheatMetadata.main_nso_build_id), 8);
	snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/%016lX.txt", cheatMetadata.title_id, __builtin_bswap64(BID));	
	FILE* text_file = fopen(path, "w");
	printf("Results: %ld\n", result.size());
	if (!text_file) {
		printf("Couldn't create txt file!\n");
	}
	else {
		fwrite("{", 1, 1, text_file);
		fwrite(unity_sdk.c_str(), unity_sdk.size(), 1, text_file);
		fwrite("}", 1, 1, text_file);
		fwrite("\n", 1, 1, text_file);
		char address_temp[17] = "";
		snprintf(address_temp, sizeof(address_temp), "0x%lx", cheatMetadata.main_nso_extents.base);
		fwrite("{MAIN: ", 7, 1, text_file);
		fwrite(address_temp, strlen(address_temp), 1, text_file);
		fwrite("}", 1, 1, text_file);
		fwrite("\n\n", 2, 1, text_file);		
		for (size_t i = 0; i < result.size(); i++) {
			if (result[i].data -> get) {
				fwrite("{", 1, 1, text_file);
				fwrite(result[i].register_print.c_str(), result[i].register_print.size(), 1, text_file);
				fwrite("}\n", 2, 1, text_file);
			}
			fwrite("[", 1, 1, text_file);
			fwrite(result[i].data -> search_name, strlen(result[i].data -> search_name), 1, text_file);
			fwrite("]\n", 2, 1, text_file);
			fwrite(result[i].toPrint.c_str(), result[i].toPrint.size(), 1, text_file);
			fwrite("\n", 1, 1, text_file);
			if (i+1 < result.size()) fwrite("\n", 1, 1 , text_file);
		}
		fclose(text_file);
		printf("Dumped instructions to txt file:\n");
		printf(path);	
		printf("\n");
	}
	result.clear();
	forPass.clear();
}