#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FILE_SIZE 65536
#define LINE_SIZE 8

unsigned short _memory[MAX_FILE_SIZE / 2];
unsigned short _fileSize;
unsigned short _regs[8]={};
unsigned short _pc = 0x0034, _sp = 0, _IR = 0;
signed short _result,_regB;
unsigned short _regA, *_regD;
int _wb = 0, _memCode = 0, _sa_IR = 0, _neg = 0, _zero = 0;

enum execcodes {ADD, SUB, MOV, RIGHT, LEFT, NONE} execcode;

FILE *arquivo;



//Car_regA arquivo binario na memoria
int loadBinary(const char *filename) {
    //Open the file for reading in binary mode
    FILE *fIn = fopen(filename, "rb");
    long position;

    if (fIn != NULL) {
        //Go to the end of the file
        const int fseek_end_value = fseek(fIn, 0, SEEK_END);
        if (fseek_end_value != -1) {

            //Get the current position in the file (in bytes)
            position = ftell(fIn);
            if (position != -1) {

                //Go back to the beginning of the file
                const int fseek_set_value = fseek(fIn, 0, SEEK_SET);

                //If error, exit
                if (fseek_set_value == -1) {
                    printf("Error reading file.");
                    exit(1);
                }

                //If file too big, exit
                if (fseek_set_value > MAX_FILE_SIZE) {
                    printf("Maximum allowed file size is 64KB.");
                    exit(1);
                }

                //Read the whole file to _memory
                _fileSize = fread(_memory, 1, position, fIn);
            }
        }
        fclose(fIn);
    }
}

//Escreve arquivo binario em um aruivo legível
void writeFileAsText(const char *basename) {
    char filename[256] = "txt_";
    FILE * fp;
    int i, j;

    strcat(filename, basename);
    strcat(filename, ".txt");

    fp = fopen(filename, "w+");
    i = 0;
    for (i = 0; i < _fileSize / 2; i += LINE_SIZE) {
        fprintf(fp, "%04X    ", 2 * i);
        for (j = 0; j < LINE_SIZE; j++) {
            fprintf(fp, "%04X ", _memory[i + j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

//Busca a instrução na memória
void instructionFetch(){
    _IR = _memory[_pc>>1];
    fprintf(arquivo, "ri %04x\n", _pc);
}

// MUDAR DAQUI PARA BAIXO \/

/*Decodifica a instrução, e informa a operação a ser feita na ULA por meio da 
variável execcode*/
void instructionDecode() {
    switch (_IR & 0xF000) {
        case 0x0000:
            switch (_IR & 0x0800) {
                case 0x0800: // Formato 1: LSR
                    _regA = _regs[(_IR & 0x0038) >> 3];
                    _regB = (_IR & 0x07C0) >> 6;
                    _regD = &_regs[(_IR & 0x0007)];
                    execcode = LEFT;
                    _memCode = 0;
                    _wb = 1;
                    break;
                case 0x0000: // Formato 1: LSL
                    _regA = _regs[(_IR & 0x0038) >> 3];
                    _regB = (_IR & 0x07C0) >> 6;
                    _regD = &_regs[(_IR & 0x0007)];
                    execcode = RIGHT;
                    _memCode = 0;
                    _wb = 1;
                    break;
            }
            break;
        case 0x1000:
            switch (_IR & 0x0E00) {
                case 0x0800: //Formato 2: ADD
                    _regA = _regs[(_IR & 0x0038) >> 3];
                    _regB = _regs[(_IR & 0x01C0) >> 6];
                    _regD = &_regs[(_IR & 0x0007)];
                    execcode = ADD;
                    _memCode = 0;
                    _wb = 1;
                    break;
            }
            break;
        case 0x2000:
            switch (_IR & 0x0800) {
                case 0x0800: // Formato 3: CMP
                    _regA = _regs[(_IR & 0x0700) >> 8];
                    _regB = (_IR & 0x00FF);
                    execcode = SUB;
                    _memCode = 0;
                    _wb = 0;
                    break;
                case 0x0000: // Formato 3: MOV
                    _regB = (_IR & 0x00FF);
                    _regD = &_regs[(_IR & 0x0700) >> 8];
                    execcode = MOV;
                    _memCode = 0;
                    _wb = 1;
                    break;
            }
            break;
        case 0x3000:
            switch (_IR & 0x0800) {
                case 0x0000: // Formato 3: ADD
                    _regA = _regs[(_IR & 0x0700) >> 8];
                    _regB = (_IR & 0x00FF);
                    _regD = &_regs[(_IR & 0x0700) >> 8];
                    execcode = ADD;
                    _memCode = 0;
                    _wb = 1;
                    break;
            }
            break;
        case 0x4000:
            switch (_IR & 0x0800) {
                case 0x0800: // Fromato 6: LDR
                    _regA = _pc + 4;
                    _regB = (_IR & 0x00FF) << 2;
                    _regD = &_regs[(_IR & 0x0700) >> 8];
                    execcode = ADD;
                    _wb = 1;
                    _memCode = 1;
                    break;
            }
            switch (_IR & 0x0F00) {
                case 0x0600:
                    switch (_IR & 0x00C0) {
                        case 0x0080: // Formato 5: MOV
                            _regB = _regs[(_IR & 0x0038) >> 3];
                            _regD = &_regs[(_IR & 0x0007)];
                            execcode = MOV;
                            _memCode = 0;
                            _wb = 1;
                            break;
                    }
                    break;
            }
            break;
        case 0x5000:
            switch (_IR & 0x0E00) {
                case 0x0A00: // Formato 8: LDRH
                    _regA = _regs[(_IR & 0x0038) >> 3];
                    _regB = _regs[(_IR & 0x01C0) >> 6];
                    _regD = &_regs[(_IR & 0x0007)];
                    execcode = ADD;
                    _memCode = 1;
                    _wb = 1;
                    break;
            }
            break;
        case 0x6000:
            switch (_IR & 0x0800) {
                case 0x0800: // Formato 9: LDR
                    _regA = _regs[(_IR & 0x0038) >> 3];
                    _regB = (_IR & 0x07C0) >> 4; 
                    _regD = &_regs[(_IR & 0x0007)];
                    execcode = ADD;
                    _memCode = 1;
                    _wb = 1;
                    break;
                case 0x0000: // Formato 9: STR
                    _regA = _regs[(_IR & 0x0038) >> 3];
                    _regB = (((_IR & 0x07C0) >> 6) << 2);
                    _regD = &_regs[(_IR & 0x0007)];
                    execcode = ADD;
                    _memCode = 2;
                    _wb = 0;
                    break;
            }
            break;
        case 0x8000:
            switch (_IR & 0x0800) {
                case 0x0800: // Formato 10: LDRH
                    _regA = _regs[(_IR & 0x0038) >> 3];
                    _regB = (_IR & 0x07C0) >> 5; 
                    _regD = &_regs[(_IR & 0x0007)];
                    execcode = ADD;
                    _memCode = 1;
                    _wb = 1;
                    break;
                case 0x0000: // Formato 10: STRH
                    _regA = _regs[(_IR & 0x0038) >> 3];
                    _regB = _IR & 0x07C0 >> 5; 
                    _regD = &_regs[(_IR & 0x0007)];
                    execcode = ADD;
                    _memCode = 2;
                    _wb = 0;
                    break;
            }
            break;
        case 0xA000:
            switch (_IR & 0x0800) {
                case 0x0800:// Formato 12: ADD 
                    _regA = _sp;
                    _regB = (_IR & 0x00FF) << 2;
                    _regD = &_regs[(_IR & 0x0700) >> 8];
                    execcode = ADD;
                    _memCode = 0;
                    _wb = 1;
                    break;
            }
            break;
        case 0xB000:
            switch (_IR & 0x0D00) {
                case 0x0000:
                    switch (_IR & 0x0080) {
                        case 0x0080: // Formato 13: ADD _neg
                            _regA = _sp;
                            _regB = (_IR & 0x007F) << 2;
                            _regD = &_sp;
                            execcode = SUB;
                            _memCode = 0;
                            _wb = 1;
                            break;
                        case 0x0000://Formato 13: ADD POS
                            _regA = _sp;
                            _regB = (_IR & 0x007F) << 2;
                            _regD = &_sp;
                            execcode = ADD;
                            _memCode = 0;
                            _wb = 1;
                            break;
                    }
                    break;
                case 0x0500: //Formato 14: PUSH
                    break;
                case 0x0D00: //Formato 14: POP
                    _sa_IR = 1;
                    break;
            }
            break;
        case 0xD000:
            switch (_IR & 0x0F00) {
                case 0x0D00: // Formato 16: BLE
                    _regA = _pc + 2;
                    _regB = (_IR & 0x00FF) << 1;
                    _regD = &_pc;
                    _wb = 0;
                    execcode = NONE;
                    if (_neg || _zero) {
                        execcode = ADD;
                        _wb = 1;
                        if (_IR & 0x0100) {
                            signed short b_neg = _regB | 0xFE00;
                            _regB = -b_neg;
                            execcode = SUB;
                        }
                    }
                    _memCode = 0;
                    break;
            }
            break;
        case 0xE000: // Formato 18: B
            _regA = _pc + 2;
            _regB = (_IR & 0x07FF) << 1;
            _regD = &_pc;
            if (_IR & 0x0800) {
                signed short b_neg = _regB | 0xF000;
                _regB = -b_neg;
            }
            execcode = ADD;
            _memCode = 0;
            _wb = 1;
            break;
    }
}

//Executa a operação e_specificada por execcode, e realiza os acessos à memória
void execMem(){
    switch (execcode){
    case ADD:
        _result = _regA+_regB;
        break;
    case SUB:
        _result = _regA-_regB;
        if (_result<0)   _neg = 1;
        else _neg = 0;
        if (_result==0) _zero = 1;
        else _zero = 0;
        break;
    case RIGHT:
    _result = _regA>>_regB;
    break;
    case LEFT:
    _result = _regA<<_regB;
    break;
    case MOV:
    _result = _regB;
    break;
    case NONE:
        break;
    }
    
    
    switch (_memCode){
    case 1: //Leitura na memória        
        _result = _memory[_result>>1];
        fprintf(arquivo, "rd %04x\n",_result);
    break;
    case 2: //Escrita na memória
        _memory[_result>>1] = *_regD;
        fprintf(arquivo, "wd %04x\n",_result);
    break;
    }
    _memCode = 0;
}

//Coloca o _resultado da operação no registrador destino, quando necessário
void writeBack(){
    if (_wb) {
        *_regD = _result;
    }
}


int main(int argc, char *argv[]) {

    char *filename = "summation.o"; 
    loadBinary(filename);
    writeFileAsText(filename);          
    arquivo = fopen("LOG.txt", "w");    //Arquivo de saída
    
    while (!_sa_IR){
        instructionFetch();
        instructionDecode();
        execMem();
        writeBack();
        _pc = _pc + 2;
    }    
    fclose(arquivo);
    
    return 0;
}
