/* LC-2K Instruction-level simulator */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000 

#define ADD_CODE  0x0
#define NOR_CODE  0x1
#define LW_CODE   0x2
#define SW_CODE	  0x3
#define BEQ_CODE  0x4
#define JALR_CODE 0x5
#define HALT_CODE 0x6
#define NOOP_CODE 0x7

#define OPCODE(MC) \
	(((MC) >> 22) & 0x7)

#define REGA(MC) \
	(((MC) >> 19) & 0x7)

#define REGB(MC) \
	(((MC) >> 16) & 0x7)

#define REGDES(MC) \
	((MC) & 0x7)

#define OFFSET(MC) \
	((MC) & 0xFFFF)

#define RTYPE(TARGET, A, B, DES) \
	(A) = (REGA(TARGET)); \
	(B) = (REGB(TARGET)); \
	(DES) = (REGDES(TARGET)); \

#define ITYPE(TARGET, A, B, IMM) \
	(A) = (REGA(TARGET)); \
	(B) = (REGB(TARGET)); \
	(IMM) = (OFFSET(TARGET));

#define JTYPE(TARGET, A, B) \
	(A) = (REGA(TARGET)); \
	(B) = (REGB(TARGET)); \

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

void printState(stateType *statePtr);
int convertNum(int num);

int fetchAndExecute(stateType*, int executedInstCount);
int isValidRegister(int32_t reg);

void add(stateType* statePtr, int32_t instruction);
void nor(stateType* statePtr, int32_t instruction);
void lw(stateType* statePtr, int32_t instruction);
void sw(stateType* statePtr, int32_t instruction);
void beq(stateType* statePtr, int32_t instruction);
void jalr(stateType* statePtr, int32_t instruction);

int main(int argc, char *argv[])
{
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;
    int executedInstCount;

    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    /* read in the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
            state.numMemory++) {

        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }

		/* TODO: */
    state.pc = 0;
    executedInstCount = 0;
    while (!(fetchAndExecute(&state, executedInstCount))) {
        executedInstCount++;
    }

    if (filePtr) {
		fclose(filePtr);
	}

    return(0);
}

void printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i = 0; i < statePtr->numMemory; i++) {
        printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i = 0; i < NUMREGS; i++) {
        printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

int convertNum(int num)
{
	/* convert a 16-bit number into a 32-bit Linux integer */
	if (num & (1 << 15)) {
		num -= (1 << 16);
	}
	return (num);
}

// return 1 when the program is halted
// else, return 0 
int fetchAndExecute(stateType* statePtr, int executedInstCount) {
    int32_t opcode;
    int32_t instruction; 
    
    printState(statePtr);

    if (statePtr->pc >= statePtr->numMemory) {
        printf("Tried to fetching instruction from out of memory range: %d\n", statePtr->pc);
        exit(1);
    }

    instruction = statePtr->mem[(statePtr->pc)++];

    opcode = OPCODE(instruction);

    switch (opcode)
    {
    case ADD_CODE:
        add(statePtr, instruction);
        break;

    case NOR_CODE:
        nor(statePtr, instruction);
        break;

    case LW_CODE:
        lw(statePtr, instruction);
        break;

    case SW_CODE:
        sw(statePtr, instruction);
        break;

    case BEQ_CODE:
        beq(statePtr, instruction);
        break;

    case JALR_CODE:
        jalr(statePtr, instruction);
        break;

    case HALT_CODE:
        printf("machine halted\n");
        printf("total of %d instructions executed\n", executedInstCount + 1);
        printf("final state of machine:\n");
        printState(statePtr);
        return 1;
        break;

    case NOOP_CODE:
        break;
    
    default:
        printf("Invalid opcode: %d\n", opcode);
        exit(1);
        break;
    }

    return 0;
}

int isValidRegister(int32_t reg) {
    return (reg <= 7) && (reg >=0);
}

void add(stateType* statePtr, int32_t instruction) {
    int32_t regA, regB, regDes;
    RTYPE(instruction, regA, regB, regDes);

    if (!isValidRegister(regA) || !isValidRegister(regB) || !isValidRegister(regDes)) {
        printf("Invalid register number: %d, %d, %d\n", regA, regB, regDes);
        exit(1);
    }

    statePtr->reg[regDes] = (statePtr->reg[regA]) + (statePtr->reg[regB]);
}

void nor(stateType* statePtr, int32_t instruction) {
    int32_t regA, regB, regDes;
    RTYPE(instruction, regA, regB, regDes);

    if (!isValidRegister(regA) || !isValidRegister(regB) || !isValidRegister(regDes)) {
        printf("Invalid register number: %d, %d, %d\n", regA, regB, regDes);
        exit(1);
    }

    statePtr->reg[regDes] = ~((statePtr->reg[regA]) | (statePtr->reg[regB]));
}

void lw(stateType* statePtr, int32_t instruction) {
    int32_t regA, regB, offsetValue, targetAddress;
    ITYPE(instruction, regA, regB, offsetValue);

    offsetValue = convertNum(offsetValue);

    if (!isValidRegister(regA) || !isValidRegister(regB)) {
        printf("Invalid register number: %d, %d\n", regA, regB);
        exit(1);
    }

    targetAddress = statePtr->reg[regA] + offsetValue;

    if (statePtr->numMemory <= targetAddress) {
        printf("Tried to access out of memory range: %d\n", targetAddress);
        exit(1);
    }

    statePtr->reg[regB] = statePtr->mem[targetAddress];
}

void sw(stateType* statePtr, int32_t instruction) {
    int32_t regA, regB, offsetValue, targetAddress;
    ITYPE(instruction, regA, regB, offsetValue);

    offsetValue = convertNum(offsetValue);

    if (!isValidRegister(regA) || !isValidRegister(regB)) {
        printf("Invalid register number: %d, %d\n", regA, regB);
        exit(1);
    }

    targetAddress = statePtr->reg[regA] + offsetValue;

    // if (statePtr->numMemory <= targetAddress) {
    //     printf("Tried to access out of memory range: %d\n", targetAddress);
    //     exit(1);
    // }

    statePtr->mem[targetAddress] = statePtr->reg[regB];
}

void beq(stateType* statePtr, int32_t instruction) {
    int32_t regA, regB, offsetValue, targetAddress;
    ITYPE(instruction, regA, regB, offsetValue);

    offsetValue = convertNum(offsetValue);

    if (!isValidRegister(regA) || !isValidRegister(regB)) {
        printf("Invalid register number: %d, %d\n", regA, regB);
        exit(1);
    }

    targetAddress = statePtr->pc + offsetValue;

    if (statePtr->reg[regA] == statePtr->reg[regB]) {
        statePtr->pc = targetAddress;
    }
}

void jalr(stateType* statePtr, int32_t instruction) {
    int32_t regA, regB, targetAddress;
    JTYPE(instruction, regA, regB);

    if (!isValidRegister(regA) || !isValidRegister(regB)) {
        printf("Invalid register number: %d, %d\n", regA, regB);
        exit(1);
    }

    statePtr->reg[regB] = statePtr->pc;
    statePtr->pc = statePtr->reg[regA];
}