/* Assembler code fragment for LC-2K */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define MAXLINELENGTH 1000

#define ADD_CODE  0x0
#define NOR_CODE  0x1
#define LW_CODE   0x2
#define SW_CODE	  0x3
#define BEQ_CODE  0x4
#define JALR_CODE 0x5
#define HALT_CODE 0x6
#define NOOP_CODE 0x7
#define FILL_CODE 0x8

#define OFFSET_NUM_MIN -0x8000
#define OFFSET_NUM_MAX  0x7FFF
#define FILL_NUM_MIN   -0x80000000LL
#define FILL_NUM_MAX	0x7FFFFFFFLL

#define OPCODE(OP) \
	(((OP) & 0x7) << 22)

#define REGA(REG) \
	(((REG) & 0x7) << 19)

#define REGB(REG) \
	(((REG) & 0x7) << 16)

#define REGDES(REG) \
	((REG) & 0x7)

#define OFFSET(IMM) \
	((IMM) & 0xFFFF)

#define RTYPE(TARGET, OP, A, B, DES) \
	(TARGET) = 0; \
	(TARGET) |= (OPCODE(OP)); \
	(TARGET) |= (REGA(A)); \
	(TARGET) |= (REGB(B)); \
	(TARGET) |= (REGDES(DES));

#define ITYPE(TARGET, OP, A, B, IMM) \
	(TARGET) = 0; \
	(TARGET) |= (OPCODE(OP)); \
	(TARGET) |= (REGA(A)); \
	(TARGET) |= (REGB(B)); \
	(TARGET) |= (OFFSET(IMM));

#define JTYPE(TARGET, OP, A, B) \
	(TARGET) = 0; \
	(TARGET) |= (OPCODE(OP)); \
	(TARGET) |= (REGA(A)); \
	(TARGET) |= (REGB(B)); \

#define OTYPE(TARGET, OP) \
	(TARGET) = 0; \
	(TARGET) |= (OPCODE(OP));

#define FILLTYPE(TARGET, FILLVALUE) \
	(TARGET) = (FILLVALUE);

typedef struct {
	int32_t address;
	char label[10];
} LabelInfo;

typedef struct {
	int capacity;
	int size;
	LabelInfo* raw;
} LabelVector;

typedef enum {
	REGBASE,
	PCBASE
} OffsetType;

void initVector(LabelVector* vec);
void releaseVector(LabelVector* vec);
void pushBack(LabelVector* vec, LabelInfo* inst);
void resizeVector(LabelVector* vec);
LabelInfo* searchVector(LabelVector* vec, char* label);
void checkAndPutLabel(LabelVector* vec, char* label, int32_t address);
int32_t getOpcode(char* opcodeStr);
int32_t getRegNum(char* regStr);
int32_t getOffsetValue(char* offsetStr, OffsetType offsetType, LabelVector* vec, int32_t pcPlusOne);
int32_t getFillValue(char* fillStr, LabelVector* vec);

int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);

int main(int argc, char *argv[]) 
{
	LabelVector labelVec;
	char *inFileString, *outFileString;
	FILE *inFilePtr, *outFilePtr;
	char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH], 
			 arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];
	int32_t addressCount;
	int32_t opcodeValue, regAValue, regBValue, regDesValue, offsetValue, fillValue;
	int32_t resultMachineCode;

	if (argc != 3) {
		printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
				argv[0]);
		exit(1);
	}

	inFileString = argv[1];
	outFileString = argv[2];

	inFilePtr = fopen(inFileString, "r");
	if (inFilePtr == NULL) {
		printf("error in opening %s\n", inFileString);
		exit(1);
	}
	outFilePtr = fopen(outFileString, "w");
	if (outFilePtr == NULL) {
		printf("error in opening %s\n", outFileString);
		exit(1);
	}

	initVector(&labelVec);

	/* TODO: Phase-1 label calculation */

	/* this is how to rewind the file ptr so that you start reading from the
		 beginning of the file */

	addressCount = 0;
	while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
		if (strcmp(label, "")) {
			checkAndPutLabel(&labelVec, label, addressCount);
		}
		addressCount++;
	}

	rewind(inFilePtr);

	/* TODO: Phase-2 generate machine codes to outfile */

	addressCount = 0;
	while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
		opcodeValue = getOpcode(opcode);
		
		switch (opcodeValue) {
			case ADD_CODE:
			case NOR_CODE:
				regAValue = getRegNum(arg0);
				regBValue = getRegNum(arg1);
				regDesValue = getRegNum(arg2);
				RTYPE(resultMachineCode, opcodeValue, regAValue, regBValue, regDesValue);
				break;
			
			case LW_CODE:
			case SW_CODE:
			case BEQ_CODE:
				regAValue = getRegNum(arg0);
				regBValue = getRegNum(arg1);
				offsetValue = getOffsetValue(arg2, opcodeValue == BEQ_CODE ? PCBASE : REGBASE, &labelVec, addressCount + 1);
				ITYPE(resultMachineCode, opcodeValue, regAValue, regBValue, offsetValue);
				break;

			case JALR_CODE:
				regAValue = getRegNum(arg0);
				regBValue = getRegNum(arg1);
				JTYPE(resultMachineCode, opcodeValue, regAValue, regBValue);
				break;
			
			case HALT_CODE:
			case NOOP_CODE:
				OTYPE(resultMachineCode, opcodeValue);
				break;
			
			case FILL_CODE:
				fillValue = getFillValue(arg0, &labelVec);
				FILLTYPE(resultMachineCode, fillValue);
				break;
			
			default:
				printf("Invalid opcode %d\n", opcodeValue);
				exit(1);
				break;
		}

		fprintf(outFilePtr, "%d\n", resultMachineCode);
		addressCount++;
	}

	releaseVector(&labelVec);

	if (inFilePtr) {
		fclose(inFilePtr);
	}
	if (outFilePtr) {
		fclose(outFilePtr);
	}
	return(0);
}

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
		char *arg1, char *arg2)
{
	char line[MAXLINELENGTH];
	char *ptr = line;

	/* delete prior values */
	label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

	/* read the line from the assembly-language file */
	if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
		/* reached end of file */
		return(0);
	}

	/* check for line too long (by looking for a \n) */
	if (strchr(line, '\n') == NULL) {
		/* line too long */
		printf("error: line too long\n");
		exit(1);
	}

	/* is there a label? */
	ptr = line;
	if (sscanf(ptr, "%[^\t\n\r ]", label)) {
		/* successfully read label; advance pointer over the label */
		ptr += strlen(label);
	}

	/*
	 * Parse the rest of the line.  Would be nice to have real regular
	 * expressions, but scanf will suffice.
	 */
	sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%"
			"[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]", opcode, arg0, arg1, arg2);
	return(1);
}

int isNumber(char *string)
{
	/* return 1 if string is a number */
	int i;
	return( (sscanf(string, "%d", &i)) == 1);
}

void initVector(LabelVector* vec) {
	vec->capacity = 1;
	vec->size = 0;
	if (!(vec->raw = (LabelInfo *)malloc((vec->capacity) * sizeof(LabelInfo)))) {
		printf("malloc failed\n");
		exit(1);
	}
}

void releaseVector(LabelVector* vec) {
	free(vec->raw);
}

void pushBack(LabelVector* vec, LabelInfo* inst) {
	if (vec->capacity <= vec->size) {
		resizeVector(vec);
	}

	vec->raw[(vec->size)++] = *inst;
}

void resizeVector(LabelVector* vec) {
	vec->capacity = 2 * vec->capacity;

	if (!(vec->raw = (LabelInfo *)realloc(vec->raw, (vec->capacity) * sizeof(LabelInfo)))) {
		printf("realloc failed\n");
		exit(1);
	}
}

LabelInfo* searchVector(LabelVector* vec, char* label) {
	for (int i = 0; i < vec->size; i++) {
		if (strcmp(vec->raw[i].label, label) == 0) {
			return &(vec->raw[i]);
		}
	}

	return NULL;
}

void checkAndPutLabel(LabelVector* vec, char* label, int32_t address) {
	LabelInfo newLabelInfo;
	int labelLen;

	labelLen = strlen(label);

	if (labelLen > 6 || labelLen <= 0) {
		printf("Invalid length of label %s: %d\n", label, labelLen);
		exit(1);
	}

	if (!((label[0] >= 'a' && label[0] <= 'z') || (label[0] >= 'A' && label[0] <= 'Z'))) {
		printf("Invalid format of label %s\n", label);
		exit(1);
	}

	if (searchVector(vec, label)) {
		printf("Duplicated definition of label %s\n", label);
		exit(1);
	}

	newLabelInfo.address = address;
	strcpy(newLabelInfo.label, label);

	pushBack(vec, &newLabelInfo);
}

int32_t getOpcode(char* opcodeStr) {
	if (!strcmp(opcodeStr, "add")) {
		return ADD_CODE;
	}

	if (!strcmp(opcodeStr, "nor")) {
		return NOR_CODE;
	}

	if (!strcmp(opcodeStr, "lw")) {
		return LW_CODE;
	}

	if (!strcmp(opcodeStr, "sw")) {
		return SW_CODE;
	}

	if (!strcmp(opcodeStr, "beq")) {
		return BEQ_CODE;
	}

	if (!strcmp(opcodeStr, "jalr")) {
		return JALR_CODE;
	}

	if (!strcmp(opcodeStr, "halt")) {
		return HALT_CODE;
	}

	if (!strcmp(opcodeStr, "noop")) {
		return NOOP_CODE;
	}

	if (!strcmp(opcodeStr, ".fill")) {
		return FILL_CODE;
	}

	printf("Unrecognized opcode: %s\n", opcodeStr);
	exit(1);
}

int32_t getRegNum(char* regStr) {
	int regnum, nread;

	if(!isNumber(regStr)) {
		printf("Invalid register number: %s\n", regStr);
		exit(1);
	}

	sscanf(regStr, "%d%n", &regnum, &nread);
	if (strcmp(regStr + nread, "")) {
		printf("Invalid register number: %s\n", regStr);
		exit(1);
	}

	if (regnum > 7 || regnum < 0) {
		printf("Invalid register number %d (out of range)\n", regnum);
		exit(1);
	}
	
	return regnum;
}

int32_t getOffsetValue(char* offsetStr, OffsetType offsetType, LabelVector* vec, int32_t pcPlusOne) {
	int offsetNum, nread;
	LabelInfo* labelInfo;

	if (isNumber(offsetStr)) {
		sscanf(offsetStr, "%d%n", &offsetNum, &nread);
		if (strcmp(offsetStr + nread, "")) {
			printf("Invalid offset number: %s\n", offsetStr);
			exit(1);
		}

		if (offsetNum < OFFSET_NUM_MIN || offsetNum > OFFSET_NUM_MAX) {
			printf("Invalid offset number: %d (out of range)\n", offsetNum);
			exit(1);
		}

		return offsetNum;
	} else {
		labelInfo = searchVector(vec, offsetStr);
		if (labelInfo == NULL) {
			printf("Use of undefined label %s\n", offsetStr);
			exit(1);
		}

		switch (offsetType) {
			case REGBASE:
				return labelInfo->address;
				break;

			case PCBASE:
				return labelInfo->address - pcPlusOne;
				break;
			
			default:
				printf("Wrong offset type %d\n", offsetType);
				exit(1);
				break;
		}
	}
}

int32_t getFillValue(char* fillStr, LabelVector* vec) {
	int64_t fillValue;
	int nread;
	LabelInfo* labelInfo;

	if (isNumber(fillStr)) {
		sscanf(fillStr, "%lld%n", &fillValue, &nread);
		if (strcmp(fillStr + nread, "")) {
			printf("Invalid fill value: %s\n", fillStr);
			exit(1);
		}

		if(fillValue < FILL_NUM_MIN || fillValue > FILL_NUM_MAX) {
			printf("Invalid offset value: %lld (out of range)\n", fillValue);
			exit(1);
		}

		return fillValue;
	} else {
		labelInfo = searchVector(vec, fillStr);
		if (labelInfo == NULL) {
			printf("Use of undefined label %s\n", fillStr);
			exit(1);
		}

		return labelInfo->address;
	}
}