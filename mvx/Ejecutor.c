#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

const char* twoOp[] = {
    "mov","add","sub",
    "swap","mul","div","cmp",
    "shl","shr","and","or","xor",
    "slen","smov","scmp"};

const char* oneOp[] = {
    "sys","jmp","jz","jp","jn",
    "jnz","jnp","jnn","ldl",
    "ldh","rnd","not",
    "push","pop","call"};

const char* noOp[] = {"ret","stop"};

const char* nombreReg[] = {
      "DS","SS","ES","CS","HP",
      "IP","SP","BP","CC","AC",
      "A","B","C","D","E","F"};

typedef unsigned int u32;

typedef struct TDisco{

  char nombreArch[100];
  char tArchivo[5];
  u32 numVersion;
  char guid[17];
  u32 fechaCreacion;
  u32 horaCreacion;
  unsigned char tipo;
  unsigned char cilindros;
  unsigned char cabezas;
  unsigned char sectores;
  u32 tamSector;
  u32 ultimoEstado;

}TDisco;

typedef struct Mv{

  TDisco listaVDD[255];

  int contDiscos;

  int reg[16];

  int mem[8192];

}Mv;

//! DECLARACION DE PROTOTIPOS

u32 bStringtoInt(char* string);
void cargaMemoria(Mv* mv, char *argv[]);
void leeHeader(FILE* arch, char* rtadoHeader, Mv* mv);
void DecodificarInstruccion(int instr, int* numOp, int* codInstr,char* mnem);
void Ejecutar(Mv* mv,int codInstr, int numOp,int tOpA,int tOpB,int vOpA,int vOpB,char mnem[],char* argv[],int argc);
void AlmacenaEnRegistro(Mv* mv, int vOp, int dato, int numOp);
int ObtenerValorDeRegistro(Mv mv, int vOp, int numOp);
void step(Mv* mv,char* argv[],int argc);
void DecodificarOperacion(int instr,int *tOpA,int *tOpB,int *vOpA,int *vOpB, int numOp );
void obtenerOperando(int tOp,int vOp,char res[], int numOp);
void disassembler(Mv* mv,int tOpA, int tOpB, int vOpA, int vOpB, char* opA, char* opB,int numOp);
void modCC( Mv* mv, int op );
void mov(Mv* mv,int tOpA,int tOpB,int vOpA,int vOpB);
void add(Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB);
void mul(Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB);
void sub(Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB);
void divi(Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB);
void swap( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB );
void cmp( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB );
void and( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB );
void or( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB );
void xor( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB );
void shl( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB );
void shr( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB );
void sysWrite( Mv* mv );
void sysRead( Mv* mv );
void sysBreakpoint( Mv* mv, char* argv[],int argc,char mnem[], int llamaP);
void sysC(char* argv[], int argc);
void sys( Mv* mv, int tOpA, int vOpA ,char mnem[],char* argv[],int argc);
void falsoStep(Mv* mv,char opA[],char opB[],int i);
int apareceFlag(char* argv[],int argc, char* flag );
void jmp( Mv* mv,int tOpA,int vOpA );
void rnd(Mv *mv, int tOpA, int vOpA);
void slen(Mv *mv,int tOpA,int tOpB,int vOpA,int vOpB);
void smov(Mv *mv,int tOpA,int tOpB,int vOpA,int vOpB);
void scmp(Mv *mv,int tOpA,int tOpB,int vOpA,int vOpB);
void push(Mv *mv,int tOpA,int vOpA);
void pop(Mv *mv,int tOpA,int vOpA);
void call(Mv *mv,int tOpA,int vOpA);
void ret();
int calculaDireccion(Mv mv, int vOp);
int calculaIndireccion(Mv mv, int vOp);
void abreDisco( TDisco *listaVDD, int cantDiscos, char *nombreArch );
void cargaDiscos(int argc, char *argv[], Mv *mv);

int main(int argc, char *argv[]) {

  Mv mv;

  cargaMemoria(&mv, argv);
  cargaDiscos(argc, argv, &mv);

  printf("Codigo: \n");
  do{
    step(&mv,argv,argc);
  }while( (0 <= (mv.reg[5] & 0x0000FFFF) ) && ( (mv.reg[5] & 0x0000FFFF) < (mv.reg[0] & 0x0000FFFF) ));

  return 0;

}

void cargaDiscos(int argc, char *argv[], Mv *mv){

  int contCantDiscos = 0;

  for( int i = 0; i < argc ; i++){
    if( strstr(argv[i], ".vdd") != NULL  ){
      abreDisco(mv->listaVDD, contCantDiscos, argv[i]);
      contCantDiscos++;
    }
  }
  mv->contDiscos = contCantDiscos;

}

void abreDisco( TDisco *listaVDD ,int indiceDiscos, char *nombreArch ){


  unsigned long guidH, guidL;
  u32 fecha, hora, tamSector, numVersion;
  unsigned char cilindros, cabezas, sectores, tipo, relleno;
  char tipoArch[5];

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  FILE *disco = fopen(nombreArch, "rb");
  int existe = access(nombreArch, F_OK);

  //access = 0 si el archivo existe

  //Chequea existencia de archivo
  if( existe != 0 ){

    fclose(disco);
    disco = fopen(nombreArch, "wb+");

    fwrite("VDD0",sizeof("VDD0"), 1, disco);
    numVersion = 1;
    fwrite(&numVersion, sizeof(numVersion), 1, disco);
    guidH = rand();
    guidL = rand();
    fwrite(&guidH, sizeof(guidH), 1, disco);
    fwrite(&guidL, sizeof(guidL), 1, disco);
    fecha = (tm.tm_year + 1900) << 16;
    fecha |= (tm.tm_mon + 1) << 8;
    fecha |= (tm.tm_mday + 1);
    fwrite(&fecha, sizeof(fecha), 1, disco);
    hora = tm.tm_hour << 12;
    hora |= (tm.tm_min << 8);
    hora |= (tm.tm_sec << 4);
    fwrite(&hora, sizeof(hora), 1, disco);
    tipo = 1;
    fwrite(&tipo, sizeof(tipo), 1, disco);
    cilindros = 128;
    fwrite(&cilindros, sizeof(cilindros), 1, disco);
    cabezas = 128;
    fwrite(&cabezas, sizeof(cabezas), 1, disco);
    sectores = 128;
    fwrite(&sectores, sizeof(sectores), 1, disco);
    tamSector = 512;
    fwrite(&tamSector, sizeof(tamSector), 1, disco);
    relleno = 0;
    fwrite(&relleno,sizeof(relleno),472,disco);

  }else{
    fclose(disco);
    disco = fopen(nombreArch, "rb+");
  }

  rewind(disco);
  fread(tipoArch, sizeof(tipoArch), 1, disco);
  fread(&numVersion, sizeof(numVersion), 1, disco);
  fread(&guidH, sizeof(guidH), 1, disco);
  fread(&guidL, sizeof(guidL), 1, disco);
  fread(&fecha, sizeof(fecha), 1, disco);
  fread(&hora, sizeof(hora), 1, disco);
  fread(&tipo, sizeof(tipo), 1, disco);
  fread(&cilindros, sizeof(cilindros), 1, disco);
  fread(&cabezas, sizeof(cabezas), 1, disco);
  fread(&sectores, sizeof(sectores), 1, disco);
  fread(&tamSector, sizeof(tamSector), 1, disco);


  strcpy(listaVDD[indiceDiscos].nombreArch, nombreArch);
  strcpy(listaVDD[indiceDiscos].tArchivo, tipoArch);
  listaVDD[indiceDiscos].numVersion = numVersion;
  char guid[33] = "";
  char auxGuid[17];
  itoa(guidH, guid, 10);
  itoa(guidL, auxGuid, 10);
  strcat(guid,auxGuid);
  strcpy(listaVDD[indiceDiscos].guid, guid);
  listaVDD[indiceDiscos].fechaCreacion = fecha;
  listaVDD[indiceDiscos].horaCreacion = hora;
  listaVDD[indiceDiscos].tipo = tipo;
  listaVDD[indiceDiscos].cilindros = cilindros;
  listaVDD[indiceDiscos].cabezas = cabezas;
  listaVDD[indiceDiscos].sectores = sectores;
  listaVDD[indiceDiscos].tamSector = tamSector;

}

int verificaDesborde(Mv mv, int direccionInicio, int celdas){

  int segmento;
  int direccion;
  int tamSegmento;

  segmento = direccionInicio >> 16;

  direccion = direccionInicio & 0xFFFF;

  tamSegmento = mv.reg[segmento] >> 16;

  return ((direccion + celdas) < tamSegmento);

}

void sysVDD(Mv *mv){

  int numOp;
  int cantSectores;
  int numCilindro;
  int numCabeza;
  int numSector;
  int numDisco;
  int celdaInicio;
  u32 seek;
  int direccion;
  char nombreArchivo[100];

  //AH
  numOp = (mv->reg[0xA] & 0x0000FF00) >> 8;

  //AL
  cantSectores = mv->reg[0xA] & 0xFF;

  //CH
  numCilindro = (mv->reg[0xC] & 0x0000FF00) >> 8;

  //CL
  numCabeza = mv->reg[0xC] & 0xFF;

  //DH
  numSector = (mv->reg[0xD] & 0x0000FF00) >> 8;

  //DL
  numDisco = mv->reg[0xD] & 0xFF;

  //EBX
  celdaInicio = mv->reg[0xB];

  strcpy(nombreArchivo, mv->listaVDD[ numDisco ].nombreArch);

  FILE *disco = fopen(nombreArchivo, "rb+");

  seek = 512 + numCilindro * mv->listaVDD[numDisco].cilindros * mv->listaVDD[numDisco].sectores * 512 + numCabeza * mv->listaVDD[numDisco].sectores * 512 + numSector * 512;

  fseek(disco, seek, SEEK_SET);

  if( numDisco > mv->contDiscos )
    mv->reg[0xA] = 0x31 << 8;
  else{

    if( numOp == 0x0 ){         //Consultar el ultimo estado
      mv->reg[0xA] = mv->listaVDD[numDisco].ultimoEstado & 0xFF00;
    }

    if( numCilindro > mv->listaVDD[numDisco].cilindros )
      mv->listaVDD[numDisco].ultimoEstado = 0xB << 8;

    else if( numCabeza > mv->listaVDD[numDisco].cabezas )
      mv->listaVDD[numDisco].ultimoEstado = 0xC << 8;

    else if( numSector > mv->listaVDD[numDisco].sectores )
      mv->listaVDD[numDisco].ultimoEstado = 0xD << 8;

    else{

      if( numOp == 0x2 ){   //Leer disco

        if( (seek + cantSectores * 512) > 1.074e9 )
          mv->listaVDD[numDisco].ultimoEstado = 0xFF << 8;
        else if( !verificaDesborde(*mv, celdaInicio, cantSectores * 128 ) ) //Como los sectores tienen 512 bytes y las celdas de memoria 4 bytes, necesitamos 128 celdas por cada sector a leer/escribir
          mv->listaVDD[numDisco].ultimoEstado = 0x4 << 8;
        else{
          direccion = calculaDireccion(*mv, celdaInicio);
          int i = direccion;
          int dato;
          //Leo celda a celda el sector entero
          for( int i = direccion ; i < direccion + cantSectores * 128 ; i++ ){
            fread(&dato, sizeof(int), 1, disco);
            mv->mem[i] = dato;
          }
          mv->listaVDD[numDisco].ultimoEstado = 0;
        }

        fclose(disco);

      }else if( numOp == 0x3 ){   //Escribir en disco

        if( (seek + cantSectores * 512) > 1.074e9 )
          mv->listaVDD[numDisco].ultimoEstado = 0xFF << 8;
        else if( !verificaDesborde(*mv, celdaInicio, cantSectores * 128 ) ) //Como los sectores tienen 512 bytes y las celdas de memoria 4 bytes, necesitamos 128 celdas por cada sector a leer/escribir
          mv->listaVDD[numDisco].ultimoEstado = 0xCC << 8;
        else{
          direccion = calculaDireccion(*mv, celdaInicio);
          for( int i = direccion ; i < direccion + cantSectores * 128 ; i++ ){
            int dato = mv->mem[i];
            fwrite(&dato, sizeof(int), 1, disco);
          }
          mv->listaVDD[numDisco].ultimoEstado = 0;
       }
       fclose(disco);

      }

      if( numOp == 0x8){    //Obtener parametros del disco

        mv->reg[0xC] = mv->listaVDD[numDisco].cilindros << 8;
        mv->reg[0xC] |= mv->listaVDD[numDisco].cabezas & 0xFF;
        mv->reg[0xD] = mv->listaVDD[numDisco].sectores << 8;

      }

    }

  }

}


void cargaMemoria(Mv* mv, char *argv[]){

  char linea[34];
  char rtadoHeader[] = "";
  int direccion;

  FILE* arch = fopen(argv[1],"r");

  if( arch ){
    leeHeader(arch, rtadoHeader, mv);
    if( strcmp(rtadoHeader,"") == 0 ){
      direccion = calculaDireccion(*mv, mv->reg[3]);

    while( fgets(linea, 34, arch) ){
        mv->mem[direccion++] = bStringtoInt(linea);
      }
    }else
      printf("%s", rtadoHeader);
  }else
    printf("[!] Error al tratar de abrir el archivo. ");


  fclose(arch);

}


void cargaRegistros(Mv* mv, int* bloquesHeader){

  //Inicializacion de CS
  mv->reg[3] = (bloquesHeader[4] << 16) & 0xFFFF0000; //(H) y (L)

  //DS a continuacion del CS
  mv->reg[0] = (bloquesHeader[1] << 16) & 0xFFFF0000; //(H)
  mv->reg[0] +=  bloquesHeader[4];                    //(L)

  //ES a continuacion del DS
  mv->reg[2] = (bloquesHeader[3] << 16) & 0xFFFF0000; //(H)
  mv->reg[2] +=  bloquesHeader[1];                    //(L)

  //SS a continuacion del ES
  mv->reg[1] = (bloquesHeader[2] << 16) & 0xFFFF0000; //(H)
  mv->reg[1] +=  bloquesHeader[3];                    //(L)

  //Inicializacion de HP
  mv->reg[4] = 0x00020000;

  //Inicializacion de IP
  mv->reg[5] = 0x00030000;

  //Inicializacion de SP
  mv->reg[6] = 0x00010000;
  mv->reg[6] += (mv->reg[1] >> 16);

  //Inicializacion de BP
  mv->reg[7] = mv->reg[7] & 0x00010000;


}

//Lee los bloques del header y verifica si el archivo puede ser leido
void leeHeader(FILE* arch, char* rtadoHeader, Mv* mv){

  char linea[34]; //32 bits
  int bloquesHeader[6];
  int i;
  char bloque0[5];
  char bloque5[5];
  int suma;

  //Lectura de los 6 bloques
  for( i = 0 ; i < 6 ; i++){
      fgets(linea, 34, arch);
      bloquesHeader[i] = bStringtoInt(linea);
  }

  //Cargo el contenido del primer bloque
  bloque0[0] =  (bloquesHeader[0] & 0xFF000000) >> 24; //Me quedo con el primer caracter
  bloque0[1] =  (bloquesHeader[0] & 0x00FF0000) >> 16;
  bloque0[2] =  (bloquesHeader[0] & 0x0000FF00) >> 8;
  bloque0[3] =  (bloquesHeader[0] & 0x000000FF);
  bloque0[4] = '\0';

  //Cargo el contenido del quinto bloque
  bloque5[0] =  (bloquesHeader[5] & 0xFF000000) >> 24; //Me quedo con el primer caracter
  bloque5[1] =  (bloquesHeader[5] & 0x00FF0000) >> 16;
  bloque5[2] =  (bloquesHeader[5] & 0x0000FF00) >> 8;
  bloque5[3] =  (bloquesHeader[5] & 0x000000FF);
  bloque5[4] = '\0';


  if( (strcmp(bloque0,"MV-2") == 0) && (strcmp(bloque5,"V.22") == 0) ){
    suma = 0;
    for( i = 1; i < 5 ; i++){
      suma += bloquesHeader[i];
    }
    if( suma > 8192 )
      strcpy(rtadoHeader,"[!] El proceso no puede ser cargado por memoria insuficiente");
    else
      cargaRegistros(mv, bloquesHeader);
  }else
    strcpy(rtadoHeader,"[!] El formato del archivo .mv2 no es correcto");

}


void pop(Mv *mv, int tOpA, int vOpA){

    //Recupero el tamano del SS
    int tamSS = (mv->reg[1] & 0xFFFF0000) >> 16;

    //Recupero el valor de SP
    int valorSP = calculaDireccion(*mv, mv->reg[6]);

    if( (mv->reg[6] & 0x0000FFFF) >= tamSS ){
      printf (" STACK UNDERFLOW ");
      exit(-1);
    }
    else{
      //mv->mem[valorSP] -> tope de la pila
      mov(mv,tOpA,0,vOpA,mv->mem[valorSP]);
      mv->reg[6]++;
    }
}

void push(Mv *mv, int tOpA, int vOpA){

    int direccion;

    //Decremento SPs
    mv->reg[6]--;

    //Recupero valor SP absoluto a partir del SP relativo
    int valorSP = calculaDireccion(*mv, mv->reg[6]);

    //Si el SP supera el SS, stack overflow
    if( (mv->reg[6] & 0xFFFF) == 0 ){
      printf (" STACK OVERFLOW ");
      exit(-1);
    }
    else{

        if( tOpA == 0 )     //Inmediato
          mv->mem[valorSP] = vOpA;
        else
          if( tOpA == 1)    //De registro
            mv->mem[valorSP] = ObtenerValorDeRegistro(*mv, vOpA, 1);
          else              //Directo
            if( tOpA == 2 ){
              direccion = calculaDireccion(*mv, vOpA);
              mv->mem[valorSP] = mv->mem[ direccion ];
            }
            else{           //Indirecto
              direccion = calculaIndireccion(*mv, vOpA);
              mv->mem[valorSP] = mv->mem[direccion];
            }

    }
}

void ret(Mv *mv){

    //Recupero el tamano del SS
    int tamSS = (mv->reg[1] & 0xFFFF0000) >> 16;

    //Recupero el valor de SP
    int valorSP = calculaDireccion(*mv, mv->reg[6]);

    if( (mv->reg[6] & 0x0000FFFF) >= tamSS ){
      printf (" STACK UNDERFLOW ");
      exit(-1);
    }
    else{
      //"salta" colocando la direccion de retorno en el reg IP
      mv->reg[5] = (0x3 << 16) | mv->mem[valorSP];
      mv->reg[6]++;
    }

}

void call(Mv *mv, int tOpA, int vOpA){

    int direccion;
    int indireccion;

    mv->reg[6]--;

    //Recupero el valor de SP
    int valorSP = calculaDireccion(*mv, mv->reg[6]);

    //Si el SP supera el SS, stack overflow
    if( (mv->reg[6] & 0xFFFF) == 0 ){
      printf (" STACK OVERFLOW ");
      exit(-1);
    }
    else{
        mv->mem[valorSP] = mv->reg[5] & 0xFFFF;
        if( tOpA == 0 ){             //Inmediato
            mv->reg[5] = (0x3 << 16) | vOpA;
        }else
            if( tOpA == 1 ){         //De registro
                mv->reg[5] = (0x3 << 16) | ObtenerValorDeRegistro(*mv,vOpA, 1);
            }else if( tOpA == 2 ){   //Directo
              direccion = calculaDireccion(*mv, vOpA);
              mv->reg[5] = (0x3 << 16) | mv->mem[direccion];
            }else{                   //Indirecto
              indireccion = calculaIndireccion(*mv, vOpA);
              mv->reg[5] = (0x3 << 16) | mv->mem[indireccion];
            }
    }
}

//Verifica si un numero es negativo, y en caso de serlo, agrega las F faltantes a su izquierda
int complemento2(int vOp, int numOp ){

  int c2 = vOp;

  //Los demas casos fueron considerados usando las variables locales
  //del tamaño correspondiente (los de registro de 8, 16, 32)

  if( (numOp == 2) && (c2 > 0) && ((vOp & 0x800) == 0x800) && ((vOp & 0xF000)  == 0) ){
      c2 = vOp | 0xFFFFF000;
  }
  else if( (numOp == 1) && (c2 > 0) && ((vOp & 0x8000) == 0x8000) && ((vOp & 0xF0000)  == 0) ){
      c2 = vOp | 0xFFFF0000;
  }

  return c2;

}

//Decodifica segun la cantidad de operandos de la funcion, identificando:
//el codigo de instruccion, el numero de operandos, y el correspondiente mnemonico
void DecodificarInstruccion(int instr, int* numOp, int* codInstr,char* mnem){

  if( (instr & 0xFF000000) >> 24 == 0xFF ){ //Instruccion sin operandos
    *numOp = 0;
    *codInstr = (instr & 0x00F00000) >> 20;
    strcpy(mnem,noOp[*codInstr]);
  }else
  if( (instr & 0xF0000000) >> 28 == 0xF ){ //Instruccion con 1 operando
    *numOp = 1;
    *codInstr = (instr & 0x0F000000) >> 24; //
    strcpy(mnem,oneOp[*codInstr]);
  }else{ //Instruccion con 2 operando
    *numOp = 2;
    *codInstr = (instr & 0xF0000000) >> 28;
    strcpy(mnem,twoOp[*codInstr]);
  }

}

//En base al numero de operandos y al codigo de instruccion,
//ejecuta la instruccion segun su correspondiente codigo
void Ejecutar(Mv* mv,int codInstr, int numOp,int tOpA,int tOpB,int vOpA,int vOpB,char mnem[],char* argv[],int argc){

  if( numOp == 2 ){
    switch( codInstr ){
      case 0x0: //!mov
        mov(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0x1: //!add
        add(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0x2: //!sub
        sub(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0x3: //!swap
        swap(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0x4: //!mul
        mul(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0x5: //!div
        divi(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0x6: //!cmp
        cmp(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0x7: //!shl
        shl(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0x8: //!shr
        shr(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0x9: //!and
        and(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0xA: //!or
        or(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0xB: //!xor
        xor(mv, tOpA, tOpB, vOpA, vOpB);
        break;
     case 0xC: //!slen
        slen(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0xD: //!smov
        smov(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      case 0xE: //!scmp
        scmp(mv, tOpA, tOpB, vOpA, vOpB);
        break;
      }
  }else
  if( numOp == 1 )
    switch( codInstr ){
      case 0x0: //!sys
          sys(mv,tOpA,vOpA, mnem,argv,argc);
        break;
      case 0x1: //!jmp
          jmp(mv,tOpA,vOpA);
        break;
      case 0x2: //!jz
          if((mv->reg[8] & 0x00000001)==1)
            jmp(mv,tOpA,vOpA);
        break;
      case 0x3: //!jp
          if((mv->reg[8] & 0x80000000)>>31==0)
            jmp(mv,tOpA,vOpA);
        break;
      case 0x4: //!jn
          if((mv->reg[8] & 0x80000000)>>31==1)
            jmp(mv,tOpA,vOpA);
        break;
      case 0x5: //!jnz
          if((mv->reg[8] & 0x00000001) == 0)
            jmp(mv,tOpA,vOpA);
        break;
      case 0x6: //!jnp
          if((mv->reg[8] & 0x80000000)>>31 == 1 || (mv->reg[8] & 0x00000001) == 1)
            jmp(mv,tOpA,vOpA);
        break;
      case 0x7: //!jnn
          if((mv->reg[8] & 0x80000000) >> 31 == 0 || (mv->reg[8] & 0x00000001) == 1)
            jmp(mv,tOpA,vOpA);
        break;
      case 0x8: //!ldl
        vOpB = DevuelveValor(*mv, tOpA, vOpA, 1);
        vOpB = (vOpB & 0x0000FFFF);
        mv->reg[9] = (mv->reg[9] & 0xFFFF0000) | vOpB;
        break;
      case 0x9: //!ldh
        vOpB = DevuelveValor(*mv,tOpA,vOpA, 1);
        vOpB = (vOpB & 0x0000FFFF);
        mv->reg[9] = ( mv->reg[9] & 0x0000FFFF ) | ( vOpB << 16 ) ;
        break;
      case 0xA: //!rnd
        rnd(mv, tOpA, vOpA);
        break;
      case 0xB: //!not
        not( mv, vOpA, tOpA );
        break;
     case 0xC: //!push
        push(mv, tOpA, vOpA);
        break;
      case 0xD: //!pop
        pop(mv, tOpA, vOpA);
        break;
      case 0xE: //!call
        call(mv, tOpA, vOpA);
        break;
    }else
      switch(codInstr){
        case 0x0: //!ret
          ret(mv);
          break;
        case 0x1: //!stop
          stop();
          break;
      }

}



void stop(Mv* mv){

  mv->reg[5] = (0x3 << 16) | (mv->reg[0] & 0x0000FFFF);

}

void not( Mv* mv, int vOp, int tOp ){

  int direccion;
  int valorNeg = ~DevuelveValor(*mv, tOp, vOp);

  if( tOp == 1 ){ //De registro
    AlmacenaEnRegistro( mv, vOp, valorNeg, 1 );
  }else if( tOp == 2 ){ //Directo
    direccion = calculaDireccion(*mv, vOp);
    mv->mem[direccion] = valorNeg;
  }
  modCC(mv,valorNeg);


}

//Almacena en un sector de registro un determinado "dato", valor
void AlmacenaEnRegistro(Mv* mv, int vOp, int dato, int numOp){

  int sectorReg = (vOp & 0x30) >> 4;
  int idReg = (vOp & 0xF);
  char regLL;

  /*
  Todo dato que se almacena en un registro, debe pasar por el if del complemento a 2
  Si es positivo, no se modifica, queda intacto
  */
  dato = complemento2(dato, numOp);

  switch( sectorReg ){
    case 0: //4 bytes
      mv->reg[idReg] = dato;
      break;
    case 1: //4to byte
      mv->reg[idReg] = (mv->reg[idReg] & 0xFFFFFF00) | (dato & 0x000000FF);
      break;
    case 2: //3er byte
      mv->reg[idReg] = (mv->reg[idReg] & 0xFFFF00FF) | ((dato & 0x000000FF) << 8);
      break;
    case 3: //3er y 4to byte
      mv->reg[idReg] = (mv->reg[idReg] & 0xFFFF0000) | (dato & 0x0000FFFF);
      break;
  }
}

//Obtengo un valor entero de un determinado sector de registro
int ObtenerValorDeRegistro(Mv mv, int vOp, int numOp){

  int sectorReg = (vOp & 0x30) >> 4;
  int idReg = (vOp & 0xF);

  int valor;
  char valorLL;
  short int valorX;

  switch( sectorReg ){
    case 0:
      valor = mv.reg[idReg];
      return valor;
    case 1:
      valorLL = mv.reg[idReg] & 0x000000FF;
      return valorLL; //Considero caso negativo de la parte L del reg
    case 2:
      valorLL = (mv.reg[idReg] & 0x0000FF00) >> 8;
      return valorLL; //Considero caso negativo de la parte H del reg
    case 3:
      valorX = mv.reg[idReg] & 0x0000FFFF;
      return valorX; //Considero caso negativo de la parte X del reg
  }

/*
Devuelve el valor ya complementado si resulta negativa
Cambia los ceros de la izquierda por F's,
ya que el VALOR(variable) esta complementado, pero con ceros a la izquierda, no cumpliendo el complemento a 2 "total"
*/

}

void muestraInstruccion( Mv mv, int instr, int numOp, char* opA, char* opB, char* mnem ){

  int direccion = calculaDireccion(mv, mv.reg[5]-1);

  if( numOp == 2 ){
    printf("[%04d]:  %08X   %d: %s   %s,%s\n",direccion,instr,direccion,mnem,opA,opB);
  }else if( numOp == 1 ){
    printf("[%04d]:  %08X   %d: %s   %s\n",direccion,instr,direccion,mnem,opA);
  }else{
    printf("[%04d]:  %08X   %d: %s  \n",direccion,instr,direccion,mnem);
  }

}

void step(Mv* mv,char* argv[],int argc){

  int instr=0;
  int numOp, codInstr;
  char mnem[5], opA[30] = "",opB[30] = "";
  int tOpA, tOpB;
  int vOpA, vOpB;
  int direccionIP = 0;

  direccionIP = calculaDireccion(*mv, mv->reg[5]);
  instr = mv->mem[direccionIP]; //fetch
  DecodificarInstruccion(instr, &numOp, &codInstr, mnem); //Decodifica el numero de operandos de la instruccion
  mv->reg[5]++;
  DecodificarOperacion(instr, &tOpA, &tOpB, &vOpA, &vOpB, numOp);
  disassembler(mv, tOpA, tOpB, vOpA, vOpB, opA, opB, numOp);
  muestraInstruccion(*mv, instr, numOp, opA, opB, mnem);
  Ejecutar(mv, codInstr, numOp, tOpA, tOpB, vOpA, vOpB, mnem, argv, argc);
}

//Obtenemos los operandos que utilizaremos en el disassembler
void obtenerOperando(int tOp, int vOp, char res[], int numOp){

    int aux=0;
    char auxS[7] = "";
    char cadOffset[4] = "";
    int reg;
    char offset; //-128...127

    if( tOp != 3)
      vOp = complemento2(vOp,numOp);
    else
      offset = vOp >> 4;

    if( tOp == 0 ){ //Inmediato
      itoa(vOp, res, 10);

    }else if( tOp == 1 ){ // De registro

      aux = (vOp & 0x30) >> 4; //Obtenemos el sector de registro
      strcpy(auxS, nombreReg[vOp & 0xF] );//obtengo nombre de reg (a,b,c,etc)
      if((vOp & 0xF)<10){
        strcpy(res,auxS);
      }
      else{ //Estamos en los registros de prop. gral (A,B,...,F)
          if( aux == 0 ){ //4 bytes
            res[0]='E';
            res[1]=auxS[0];
            res[2]='X';
         }
         else
           if(aux==1){//4to byte
             res[0]=auxS[0];
             res[1]='L';
           }
           else
            if(aux==2){//3er byte
              res[0]=auxS[0];
              res[1]='H';
            }
            else{//2 bytes
               res[0]=auxS[0];
               res[1]='X';
            }
      }
    }
    else if( tOp == 2 ){//si es directo
        res[0]='[';
        itoa(vOp,auxS,10);
        strcat(res,auxS);
        strcat(res,"]");

    }else if( tOp == 3 ){ //[reg + offset]

       reg = vOp & 0xF;

       strcpy(auxS, nombreReg[reg] );

       res[0] = '[';

       if( reg > 9 ){
         res[1] = 'E';
         res[2] = auxS[0];
         res[3] = 'X';
       }else{
         res[1] = auxS[0];
         res[2] = auxS[1];
       }
       if( offset != 0 ){
         if( offset > 0 )
           strcat(res,"+");
         itoa(offset, cadOffset, 10);
         strcat(res, cadOffset);
       }

       strcat(res,"]");

    }
}

void disassembler(Mv* mv,int tOpA, int tOpB, int vOpA, int vOpB, char* opA, char* opB,int numOp){ //Actualizar numOp

  char resA[30]={""};
  char resB[30]={""};

  if( numOp == 1 ){
    obtenerOperando(tOpA,vOpA,resA, numOp);
    strcpy(opA,resA);
  }else
    if( numOp == 2 ){//con dos operandos
      obtenerOperando(tOpA,vOpA,resA,numOp);
      strcpy(opA,resA);
      obtenerOperando(tOpB,vOpB,resB,numOp);
      strcpy(opB,resB);
    }
 }

//Obtenemos segun el numero de operando:
//tipo de operandos, valor de operandos
void DecodificarOperacion(int instr,int *tOpA,int *tOpB,int *vOpA,int *vOpB, int numOp ){

  int aux;

  switch ( numOp ){
    case 2:
      *tOpA = (instr & 0x0C000000) >> 26;
      *tOpB = (instr & 0x03000000) >> 24;
      *vOpA = (instr & 0x00FFF000) >> 12 ;
      *vOpB = (instr & 0x00000FFF);
      if( *tOpA == 0 )
        *vOpA = complemento2(*vOpA,2);
      if( *tOpB == 0 )
        *vOpB = complemento2(*vOpB,2);
      break;
    case 1:
      *tOpA = (instr & 0x00C00000) >> 22;
      *vOpA = (instr & 0x0000FFFF);
      *vOpA = complemento2(*vOpA, 1);
      *tOpB = 0;
      *vOpB = 0;
      break;
    case 0:
      *tOpA = 0;
      *vOpA = 0;
      *tOpB = 0;
      *vOpB = 0;
      break;
    default:
      printf("[!] Numero de operacion no valido ");
      break;
  }

}

//Convierte las instrucciones "binarias" almacenada en formato string
//a unsigned int para luego almacenarlas en memoria
u32 bStringtoInt(char* string){

  int i;
  u32 ac = 0;
  int exp = 0;

  for( i = 31; i >= 0 ; i-- ){
    if( string[i] == '1'){
       ac += pow(2,exp);
    }
    exp++;
  }
  return ac;
}

int calculaIndireccion(Mv mv, int vOp){

  int codReg = vOp & 0x000F;
  char offset = vOp >> 4;
  int regH = mv.reg[codReg] >> 16;
  int regL = mv.reg[codReg] & 0x0000FFFF;
  int segm;
  int tamSegm;
  int inicioSegm;

  /*[eax]
  eax = 0003XXXX
  eax = 0002XXXX
  eax = 0001XXXX
  eax = 0000XXXX
  */

  //Recupero la info del segmento

  segm = mv.reg[regH]; //CS, DS, SS, ES

  //Obtengo tamano del segmento
  tamSegm = segm >> 16;

  //Obtengo direccion relativa al segmento
  inicioSegm = segm & 0x0000FFFF;

  if( (mv.reg[codReg] < 0) && ((regL + offset) >= tamSegm) && ((regL + offset) <= inicioSegm) ){
    printf("Segmentation fault");
    exit(-1);
  }
  else
    return inicioSegm + regL + offset;
    //Retorno la direccion relativa al segmento

}

int calculaDireccion(Mv mv, int vOp){

int ret;
int seg = vOp >> 16;
int inicio = vOp & 0xFFFF;

 if( (vOp >> 16) < 4 ) // 0000, 0001, 0002, 0003
    if( inicio >= ( mv.reg[seg] >> 16 ) ){
        printf("Segmentation fault");
        exit(-1);
    }else
        ret = inicio + (mv.reg[seg] & 0x0000FFFF); //Devuelve direccion (caso de op. directo)
 else // Tamaño del segmento
    ret = inicio; //Devuelve el inicio del segmento que consulte

 return ret;
}

void mov(Mv* mv,int tOpA,int tOpB,int vOpA,int vOpB){

  //Variables de op de reg.
  int indireccion1, indireccion2;
  int direccion1, direccion2;

  if( tOpA == 3 ){ //! OpA -> Indirecto
    indireccion1 = calculaIndireccion(*mv, vOpA);
      if( tOpB == 0 )//mov [eax + x] = 10
        mv->mem[indireccion1] = vOpB;
      else
        if( tOpB == 1 ) //mov [eax+x] = eax
          mv->mem[indireccion1] = ObtenerValorDeRegistro(*mv, vOpB, 2);
        else
          if( tOpB == 2 ){ //mov [eax+x] = [10]
            direccion2 = calculaDireccion(*mv, vOpB);
            mv->mem[indireccion1] = mv->mem[direccion2];
          }else
            if( tOpB == 3 ){ //mov [eax+x] = [ebx+2];
              indireccion2 = calculaIndireccion(*mv, vOpB);
              mv->mem[indireccion1] = mv->mem[indireccion2];
            }
  }else if( tOpA==2 ){ //!OpA -> directo -> [10]

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ) // mov [10] 10
      mv->mem[direccion1] = vOpB;
    else
    if( tOpB == 1 ) // mov [10] AX
      mv->mem[direccion1] = ObtenerValorDeRegistro(*mv,vOpB,2);
    else
      if( tOpB == 2 ){         // mov [vOpA] [vOpB]
        direccion2 = calculaDireccion(*mv, vOpB);
        mv->mem[direccion1] = mv->mem[direccion2];
      }else{
        indireccion2 = calculaIndireccion(*mv, vOpB);
        mv->mem[direccion1] = mv->mem[indireccion2];
      }

  }else if( tOpA == 1 ){          //!Op -> De registro -> EAX

      if( tOpB == 0 ){ // mov AX 10
        AlmacenaEnRegistro(mv, vOpA, vOpB, 2);
      }else
        if( tOpB == 2){ // mov AX [10]
          direccion2 = calculaDireccion(*mv, vOpB);
          AlmacenaEnRegistro( mv, vOpA, mv->mem[direccion2], 2 );
        }else
          if( tOpB == 1){ //mov AX EAX
            AlmacenaEnRegistro( mv, vOpA, ObtenerValorDeRegistro(*mv,vOpB,2), 2 );
          }else {
            indireccion2 = calculaIndireccion(*mv, vOpB);
            AlmacenaEnRegistro(mv, vOpA, mv->mem[indireccion2], 2);
          }

      }
}

void add(Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB){

  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;

  if( tOpA == 3 ){ //! OpA -> Indirecto
    indireccion1 = calculaIndireccion(*mv, vOpA);
      if( tOpB == 0 )//mov [eax + x] = 10
        mv->mem[indireccion1] += vOpB;
      else
        if( tOpB == 1 ) //mov [eax+x] = eax
          mv->mem[indireccion1] += ObtenerValorDeRegistro(*mv, vOpB, 2);
        else
          if( tOpB == 2 ){ //mov [eax+x] = [10]
            direccion2 = calculaDireccion(*mv, vOpB);
            mv->mem[indireccion1] += mv->mem[direccion2];
          }else
            if( tOpB == 3 ){ //mov [eax+x] = [ebx+2];
              indireccion2 = calculaIndireccion(*mv, vOpB);
              mv->mem[indireccion1] += mv->mem[indireccion2];
            }
  }
  else if( tOpA==2 ){ //! tOpA -> Directo [vOpA]

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ){ // add [10] 10
      mv->mem[direccion1] += vOpB;
    }else
      if( tOpB == 2){ //add [vOpA] [vOpB]
        direccion2 = calculaDireccion(*mv, vOpB);
        mv->mem[direccion1] += mv->mem[direccion2];
      }else
        if( tOpB == 1 )//directo y reg
          mv->mem[direccion1] += ObtenerValorDeRegistro(*mv,vOpB,2);
        else{
          indireccion2 = calculaIndireccion(*mv, vOpB);
          mv->mem[direccion1] += mv->mem[indireccion2];
        }


    modCC(mv, mv->mem[direccion1]);

  }else
    if( tOpA == 1 ){ //! tOpA -> DeRegistro EAX

      aux = ObtenerValorDeRegistro(*mv,vOpA,2);
      if( tOpB == 0 ){ // add reg 10
        aux += vOpB;
      }else if( tOpB == 2){ //add reg [vOpB]
        direccion2 = calculaDireccion(*mv, vOpB);
        aux += mv->mem[direccion2];
      }else if( tOpB == 1 ) //reg y reg
        aux += ObtenerValorDeRegistro(*mv,vOpB,2);
      else{
        indireccion2 = calculaIndireccion(*mv, vOpB);
        aux += mv->mem[indireccion2];
      }
      AlmacenaEnRegistro(mv, vOpA, aux, 2);
      modCC(mv, aux);
    }
}

void modCC( Mv* mv, int op ){

  mv->reg[8] = 0;

  if( op == 0 )      // = 0 -> Prende bit menos significativo
    mv->reg[8] = 0x00000001;
  else if( op < 0 )  // < 0 -> Prende bit mas significativo
    mv->reg[8] = 0x80000000;
}

void mul(Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB){

  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;


  if( tOpA == 3 ){ //! OpA -> Indirecto
    indireccion1 = calculaIndireccion(*mv, vOpA);
      if( tOpB == 0 )//mov [eax + x] = 10
        mv->mem[indireccion1] *= vOpB;
      else
        if( tOpB == 1 ) //mov [eax+x] = eax
          mv->mem[indireccion1] *= ObtenerValorDeRegistro(*mv, vOpB, 2);
        else
          if( tOpB == 2 ){ //mov [eax+x] = [10]
            direccion2 = calculaDireccion(*mv, vOpB);
            mv->mem[indireccion1] *= mv->mem[direccion2];
          }else
            if( tOpB == 3 ){ //mov [eax+x] = [ebx+2];
              indireccion2 = calculaIndireccion(*mv, vOpB);
              mv->mem[indireccion1] *= mv->mem[indireccion2];
            }
  }
  else if( tOpA==2 ){ //! tOpA -> Directo [vOpA]

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ){ // add [10] 10
      mv->mem[direccion1] *= vOpB;
    }
    else if( tOpB == 2){ //add [vOpA] [vOpB]
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[direccion1] *= mv->mem[direccion2];
    }
    else if( tOpB == 1 ){//directo y de reg
      mv->mem[direccion1] *= ObtenerValorDeRegistro(*mv,vOpB,2);
    }
    else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[direccion1] *= mv->mem[indireccion2];
    }
    modCC(mv, mv->mem[direccion1]);

  }else
    if( tOpA == 1 ){ //! tOpA -> DeRegistro EAX

      aux = ObtenerValorDeRegistro(*mv,vOpA,2);
      if( tOpB == 0 ){ // add reg 10
        aux *= vOpB;
      }else if( tOpB == 2){ //add reg [vOpB]
        direccion2 = calculaDireccion(*mv, vOpB);
        aux *= mv->mem[direccion2];
      }else if( tOpB == 1){// reg reg
        aux *= ObtenerValorDeRegistro(*mv,vOpB,2);
      }else{
        indireccion2 = calculaIndireccion(*mv, vOpB);
        aux *= mv->mem[indireccion2];
      }
      AlmacenaEnRegistro(mv, vOpA, aux, 2);
      modCC(mv, aux);
    }
}

void sub(Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB){

  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;

    if( tOpA == 3 ){ //! OpA -> Indirecto

    indireccion1 = calculaIndireccion(*mv, vOpA);
      if( tOpB == 0 )//mov [eax + x] = 10
        mv->mem[indireccion1] -= vOpB;
      else
        if( tOpB == 1 ) //mov [eax+x] = eax
          mv->mem[indireccion1] -= ObtenerValorDeRegistro(*mv, vOpB, 2);
        else
          if( tOpB == 2 ){ //mov [eax+x] = [10]
            direccion2 = calculaDireccion(*mv, vOpB);
            mv->mem[indireccion1] -= mv->mem[direccion2];
          }else
            if( tOpB == 3 ){ //mov [eax+x] = [ebx+2];
              indireccion2 = calculaIndireccion(*mv, vOpB);
              mv->mem[indireccion1] -= mv->mem[indireccion2];
            }
            modCC(mv,mv->mem[indireccion1]);

  }else if( tOpA==2 ){ //! tOpA -> Directo [vOpA]

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ){ // add [10] 10
      mv->mem[direccion1] -= vOpB;
    }
    else if( tOpB == 2){ //add [vOpA] [vOpB]
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[direccion1] -= mv->mem[direccion2];
    }
    else if( tOpB == 1){
     mv->mem[direccion1] -= ObtenerValorDeRegistro(*mv,vOpB,2);
    }
    else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[direccion1] -= mv->mem[indireccion2];
    }
    modCC(mv, mv->mem[direccion1]);

  }else if( tOpA == 1 ){ //! tOpA -> DeRegistro EAX

      aux = ObtenerValorDeRegistro(*mv,vOpA,2);
      if( tOpB == 0 ){ // add [10] 10
        aux -= vOpB;
      }
      else if( tOpB == 2){ //add [vOpA] [vOpB]
        direccion2 = calculaDireccion(*mv, vOpB);
        aux -= mv->mem[direccion2];
      }
      else if( tOpB == 1 ){
        aux -= ObtenerValorDeRegistro(*mv,vOpB,2);
      }
      else{
        indireccion2 = calculaIndireccion(*mv, vOpB);
        aux -= mv->mem[indireccion2];
      }
      AlmacenaEnRegistro(mv, vOpA, aux, 2);
      modCC(mv, aux);
    }
}

void divi(Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB){

  int aux;
  int auxB;
  int cero = 0;
  int indireccion1, indireccion2;
  int direccion1, direccion2;
  int direccionIP = calculaDireccion(*mv, vOpB);

  if( tOpA == 3 ){ //! OpA -> Indirecto

    indireccion1 = calculaIndireccion(*mv, vOpA);
    if( tOpB == 0 ){ // add [10] 10
      if( vOpB ){
        mv->reg[9] = mv->mem[indireccion1] % vOpB;
        mv->mem[indireccion1] /= vOpB;
      }
      else
        cero = 1;
    }else if( tOpB == 2){ //add [vOpA] [vOpB]
      direccion2 = calculaDireccion(*mv, vOpB);
      if( mv->mem[direccion2] ){
        mv->reg[9] = mv->mem[indireccion1] % mv->mem[direccion2];
        mv->mem[indireccion1] /= mv->mem[direccion2];
      }
      else
        cero = 1;
    }else if( tOpB == 1 ){
       auxB = ObtenerValorDeRegistro(*mv,vOpB,2);
       if( auxB ){
         mv->reg[9] = mv->mem[indireccion1] % auxB;
         mv->mem[indireccion1] /= auxB;
       }
       else
         cero = 1;
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      if( mv->mem[indireccion2] ){
        mv->reg[9] = mv->mem[indireccion1] % mv->mem[indireccion2];
        mv->mem[indireccion1] /= mv->mem[indireccion2];
      }else
        cero = 1;
    }

    modCC(mv, mv->mem[indireccion1]);
  }
  else if( tOpA==2 ){ //! tOpA -> Directo [vOpA]

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ){ // add [10] 10
      if( vOpB ){
        mv->reg[9] = mv->mem[direccion1] % vOpB;
        mv->mem[direccion1] /= vOpB;
      }
      else
        cero = 1;
    }else if( tOpB == 2){ //add [vOpA] [vOpB]
      direccion2 = calculaDireccion(*mv, vOpB);
      if( mv->mem[direccion2] ){
        mv->reg[9] = mv->mem[direccion1] % mv->mem[direccion2];
        mv->mem[direccion1] /= mv->mem[direccion2];
      }
      else
        cero = 1;
    }else if( tOpB == 1 ){
       auxB = ObtenerValorDeRegistro(*mv,vOpB,2);
       if( auxB ){
         mv->reg[9] = mv->mem[direccion1] % auxB;
         mv->mem[direccion1] /= auxB;
       }
       else
         cero = 1;
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      if( mv->mem[indireccion2] ){
        mv->reg[9] = mv->mem[direccion1] % mv->mem[indireccion2];
        mv->mem[direccion1] /= mv->mem[indireccion2];
      }else
        cero = 1;
    }
    modCC(mv, mv->mem[direccion1]);
  }

  else
    if( tOpA == 1 ){ //! tOpA -> DeRegistro EAX

      aux = ObtenerValorDeRegistro(*mv,vOpA,2);

      if( tOpB == 0 ){
        if( vOpB ){
          mv->reg[9] = aux % vOpB;
          aux /= vOpB;
        }
        else
          cero = 1;
      }else if( tOpB == 2){
        direccion2 = calculaDireccion(*mv, vOpB);
        if( mv->mem[direccion2] ){
          mv->reg[9] = aux % mv->mem[direccion2];
          aux /= mv->mem[direccion2];
        }
        else
          cero = 1;
      }else if( tOpB == 1 ){
        auxB = ObtenerValorDeRegistro(*mv,vOpB,2);
        if( auxB ){
          mv->reg[9] = aux % auxB;
          aux /= auxB;
        }
        else
          cero = 1;
      }else{
        indireccion2 = calculaIndireccion(*mv, vOpB);
        if( mv->mem[indireccion2] ){
          mv->reg[9] = aux % mv->mem[indireccion2];
          aux /= mv->mem[indireccion2];
        }
        else
          cero = 1;
      }
      if( !cero ) {
        AlmacenaEnRegistro(mv, vOpA, aux, 2);
        modCC(mv, aux);
      }
    }

    if( cero ){
        printf("\n[%04d] [!] Division por cero\n",direccionIP-1);
        exit(-1);
    }
}

void swap( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ){

  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;


  if( tOpA == 3 ){

    indireccion1 = calculaIndireccion(*mv, vOpA);
    aux = mv->mem[indireccion1];

    if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[indireccion1] = mv->mem[direccion2];
      mv->mem[direccion2] = aux;
    }else if( tOpB == 1 ){
      mv->mem[indireccion1] = ObtenerValorDeRegistro(*mv, vOpB, 2);
      AlmacenaEnRegistro(mv, vOpB, aux,2);
    }else if ( tOpB == 3 ){
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[indireccion1] = mv->mem[indireccion2];
      mv->mem[indireccion2] = aux;
    }

  }else if( tOpA==2 ){ //! Op directo

    direccion1 = calculaDireccion(*mv,vOpA);
    aux = mv->mem[direccion1];

    if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[direccion1] = mv->mem[direccion2];
      mv->mem[direccion2] = aux;
    }else if( tOpB == 1 ){
      mv->mem[direccion1] = ObtenerValorDeRegistro(*mv, vOpB, 2);
      AlmacenaEnRegistro(mv, vOpB, aux,2);
    }else if ( tOpB == 3 ){
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[direccion1] = mv->mem[indireccion2];
      mv->mem[indireccion2] = aux;
    }

  }else if( tOpA ==1 ){ //!De registro

    aux = ObtenerValorDeRegistro(*mv, vOpA,2);

    if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      AlmacenaEnRegistro(mv,vOpA,mv->mem[direccion2],2);
      mv->mem[direccion2] = aux;
    }else if( tOpB == 1 ){
      AlmacenaEnRegistro(mv, vOpA, ObtenerValorDeRegistro(*mv, vOpB,2),2);
      AlmacenaEnRegistro(mv, vOpB, aux,2);
    }else if( tOpB == 3){
      indireccion2 = calculaIndireccion(*mv, vOpB);
      AlmacenaEnRegistro(mv, vOpA, mv->mem[indireccion2] ,2);
      mv->mem[indireccion2] = aux;
    }

  }

}

void cmp( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ){

  int sub;
  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;

  if( tOpA == 0 ){                 //! Op inmediato

    if( tOpB == 0 ){               // cmp 10, 11
      sub = vOpA - vOpB;
    }else if( tOpB == 2 ){         // cmp 10, [11]
      direccion2 = calculaDireccion(*mv, vOpB);
      sub = vOpA - mv->mem[direccion2];
    }else if( tOpB == 1) {                         // cmp 10, EAX
      sub = vOpA - ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      sub = vOpA - mv->mem[indireccion2];
    }

  }else if( tOpA == 2 ){           //! Op directo

     direccion1 = calculaDireccion(*mv, vOpA);
     aux = mv->mem[direccion1];

     if( tOpB == 0 ){              // cmp [10], 11
      sub = aux - vOpB;
    }else if( tOpB == 2 ){         // cmp [10], [11]
      direccion2 = calculaDireccion(*mv, vOpB);
      sub = aux - mv->mem[direccion2];
    }else if( tOpB == 1){                         // cmp [10], EAX
      sub = aux - ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      sub = aux - mv->mem[indireccion2];
    }

  }else if( tOpA == 1 ){                           //! Op de registro

    aux = ObtenerValorDeRegistro(*mv, vOpA,2);

    if( tOpB == 0 ){               // cmp EAX, 11
      sub = aux - vOpB;
    }else if( tOpB == 2 ){         // cmp EAX, [11]
      direccion2 = calculaDireccion(*mv, vOpB);
      sub = aux - mv->mem[direccion2];
    }else if( tOpB == 1){                         // cmp EAX, AX
      sub = aux - ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      sub = aux - mv->mem[indireccion2];
    }
  }else if( tOpA == 3){

    indireccion1 = calculaIndireccion(*mv, vOpA);
    aux = mv->mem[indireccion1];

    if( tOpB == 0 ){
      sub = aux - vOpB;
    }else if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      sub = aux - mv->mem[direccion2];
    }else if( tOpB == 1){
      sub = aux - ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      sub = aux - mv->mem[indireccion2];
    }
  }

  modCC(mv, sub);

}

void and( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ){

  int op;
  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;

  if( tOpA == 3){

    indireccion1 = calculaIndireccion(*mv, vOpA);

    if( tOpB == 0 ){
      mv->mem[indireccion1] &= vOpB;
    }else if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[indireccion1] &= mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[indireccion1] &= ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[indireccion1] &= mv->mem[indireccion2];
    }
    op = mv->mem[indireccion1];

  }else if( tOpA==2 ){ //! Op directo

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ){
      mv->mem[direccion1] &= vOpB;
    }else if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[direccion1] &= mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[direccion1] &= ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[direccion1] &= mv->mem[indireccion2];
    }

    op = mv->mem[direccion1];

  }else if( tOpA == 1 ){ //! Op de registro
    aux = ObtenerValorDeRegistro(*mv, vOpA,2);

    if( tOpB == 0 ){
      op = aux & vOpB;
    }else if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      op = aux & mv->mem[direccion2];
    }else if( tOpB == 1 ){
      op = aux & ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      op = aux & mv->mem[indireccion2];
    }
    AlmacenaEnRegistro(mv, vOpA, op,2);

  }
  modCC(mv, op);

}

void or( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ){

  int op;
  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;

  if( tOpA == 3){

    indireccion1 = calculaIndireccion(*mv, vOpA);

    if( tOpB == 0 ){
      mv->mem[indireccion1] |= vOpB;
    }else if( tOpB == 2 ){//b directo
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[indireccion1] |= mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[indireccion1] |= ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[indireccion1] |= mv->mem[indireccion2];
    }
    op = mv->mem[indireccion1];

  }else if( tOpA==2 ){ //! Op directo

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ){
      mv->mem[direccion1] |= vOpB;
    }else if( tOpB == 2 ){//b directo
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[direccion1] |= mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[direccion1] |= ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[direccion1] |= mv->mem[indireccion2];
    }
    op = mv->mem[direccion1];

  }else if( tOpA == 1 ){ //! Op de registro
    aux = ObtenerValorDeRegistro(*mv, vOpA,2);

    if( tOpB == 0 ){
      op = aux | vOpB;
    }else if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      op = aux | mv->mem[direccion2];
    }else if( tOpB == 1 ){
      op = aux | ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      op = aux | mv->mem[indireccion2];
    }
    AlmacenaEnRegistro(mv, vOpA, op,2);

  }
  modCC(mv, op);

}

void xor( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ){

  int op;
  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;

    if( tOpA == 3){ //!Op indirecto

    indireccion1 = calculaIndireccion(*mv, vOpA);

    if( tOpB == 0 ){
      mv->mem[indireccion1] ^= vOpB;
    }else if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[indireccion1] ^= mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[indireccion1] ^= ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[indireccion1] ^= mv->mem[indireccion2];
    }
    op = mv->mem[indireccion1];

  }else if( tOpA==2 ){ //! Op directo

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ){
      mv->mem[direccion1] ^= vOpB;
    }else if( tOpB == 2 ){//b directo
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[direccion1] ^= mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[direccion1] ^= ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[direccion1] ^= mv->mem[indireccion2];
    }
    op = mv->mem[mv->reg[0] + vOpA];

  }else if( tOpA == 1 ){ //! Op de registro
    aux = ObtenerValorDeRegistro(*mv, vOpA,2);

    if( tOpB == 0 ){
      op = aux ^ vOpB;
    }else if( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      op = aux ^ mv->mem[direccion2];
    }else if( tOpB == 1 ){
      op = aux ^ ObtenerValorDeRegistro(*mv, vOpB,2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      op = aux ^ mv->mem[indireccion2];
    }
    AlmacenaEnRegistro(mv, vOpA, op,2);

  }
  modCC(mv, op);

}

void shl( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ){

  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;

  if( tOpA == 3 ){

    indireccion1 = calculaIndireccion(*mv, vOpA);

    if( tOpB == 0 ){
      mv->mem[indireccion1] = mv->mem[indireccion1] << vOpB;
    }else if (tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[indireccion1] = mv->mem[indireccion1] << mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[indireccion1] = mv->mem[indireccion1] << ObtenerValorDeRegistro(*mv, vOpB, 2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[indireccion1] = mv->mem[indireccion1] << mv->mem[indireccion2];
    }

  }else if( tOpA==2 ){ //! Op directo

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ){ //shl [#] 10
      mv->mem[direccion1] = mv->mem[direccion1] << vOpB;
    }else if (tOpB == 2 ){ //shl [#] [#2]
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[direccion1] = mv->mem[direccion1] << mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[direccion1] = mv->mem[direccion1] << ObtenerValorDeRegistro(*mv, vOpB, 2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[direccion1] = mv->mem[direccion1] << mv->mem[indireccion2];
    }

    aux = mv->mem[direccion1];

  }else if( tOpA == 1 ){ //! Op de registro

     aux = ObtenerValorDeRegistro(*mv, vOpA, 2);

     if( tOpB == 0 ){ //shl EAX 10
      aux = aux << vOpB;
    }else if (tOpB == 2 ){ //shl EAX [#]
      direccion2 = calculaDireccion(*mv, vOpB);
      aux = aux << mv->mem[direccion2];
    }else if( tOpB == 1 ){
      aux = aux << ObtenerValorDeRegistro(*mv, vOpB, 2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      aux = aux << mv->mem[indireccion2];
    }
    AlmacenaEnRegistro(mv, vOpA, aux, 2);
  }

  modCC(mv, aux);

}

void shr( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ){

  int aux;
  int indireccion1, indireccion2;
  int direccion1, direccion2;

  if( tOpA == 3 ){

    indireccion1 = calculaIndireccion(*mv, vOpA);

    if( tOpB == 0 ){
      mv->mem[indireccion1] = mv->mem[indireccion1] >> vOpB;
    }else if (tOpB == 2 ){
      direccion2 = calculaDireccion(*mv,vOpB);
      mv->mem[indireccion1] = mv->mem[indireccion1] >> mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[indireccion1] = mv->mem[indireccion1] >> ObtenerValorDeRegistro(*mv, vOpB, 2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[indireccion1] = mv->mem[indireccion1] >> mv->mem[indireccion2];
    }

  }else if( tOpA==2 ){ //! Op directo

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 0 ){ //shl [#] 10
      mv->mem[direccion1] = mv->mem[direccion1] >> vOpB;
    }else if (tOpB == 2 ){ //shl [#] [#2]
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[direccion1] = mv->mem[direccion1] >> mv->mem[direccion2];
    }else if( tOpB == 1 ){
      mv->mem[direccion1] = mv->mem[direccion1] >> ObtenerValorDeRegistro(*mv, vOpB, 2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[direccion1] = mv->mem[direccion1] >> mv->mem[indireccion2];
    }

    aux = mv->mem[mv->reg[0] + vOpA];

  }else if( tOpA == 1 ){ //! Op de registro

     aux = ObtenerValorDeRegistro(*mv, vOpA, 2);

     if( tOpB == 0 ){ //shl EAX 10
      aux = aux >> vOpB;
    }else if (tOpB == 2 ){ //shl EAX [#]
      direccion2 = calculaDireccion(*mv, vOpB);
      aux = aux >> mv->mem[direccion2];
    }else if( tOpB == 1 ){
      aux = aux >> ObtenerValorDeRegistro(*mv, vOpB, 2);
    }else{
      indireccion2 = calculaIndireccion(*mv, vOpB);
      aux = aux >> mv->mem[indireccion2];
    }
    AlmacenaEnRegistro(mv, vOpA, aux, 2);
  }

  modCC(mv, aux);
}

void slen( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ){

  int indireccion1, indireccion2;
  int direccion1, direccion2;
  char aux;
  int len;

  if( tOpA == 3 ){         //! opA indirecto

    indireccion1 = calculaIndireccion(*mv, vOpA);
    if( tOpB == 3 ){        //opB indirecto
      indireccion2 = calculaIndireccion(*mv, vOpB);
      mv->mem[indireccion1] = strlen(mv->mem[indireccion2]);
    }else if( tOpB == 2 ){   //opB directo
      direccion2 = calculaDireccion(*mv, vOpB);
      mv->mem[indireccion1] = strlen(mv->mem[direccion2]);
    }

  }else if( tOpA == 2 ){   //! opA directo

     direccion1 = calculaDireccion(*mv, vOpA);

     if( tOpB == 3 ){       //opB indirecto
       indireccion2 = calculaIndireccion(*mv, vOpB);
       mv->mem[direccion1] = strlen(mv->mem[indireccion2]);
     }else if( tOpB == 2 ){ //opB directo
       direccion2 = calculaDireccion(*mv, vOpB);
       mv->mem[direccion1] = strlen(mv->mem[direccion2]);
     }

  }else if( tOpA == 1 ){  //! opA de registro

     if ( tOpB == 3 ){    // opB indirecto
       indireccion2 = calculaIndireccion(*mv, vOpB);
       len = strlen( mv->mem[indireccion2] );
     }else if( tOpB == 2 ){  //opB directo
       direccion2 = calculaDireccion(*mv, vOpB);
       len = strlen( mv->mem[direccion2] );
     }

     AlmacenaEnRegistro(mv, vOpA, len, 2);

  }

}

void smov( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ) {

  int indireccion1, indireccion2;
  int direccion1, direccion2;

  if ( tOpA == 3 ){

    indireccion1 = calculaIndireccion(*mv, vOpA);
    if( tOpB == 3 ){
      indireccion2 = calculaIndireccion(*mv, vOpB);
      while( mv->mem[indireccion2] != '\0' ){
        mv->mem[indireccion1++] = mv->mem[indireccion2++];
      }
    }else if ( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      while( mv->mem[direccion2] != '\0' ){
        mv->mem[indireccion1++] = mv->mem[direccion2++];
      }
    }

  }else if( tOpA == 2 ){

    direccion1 = calculaDireccion(*mv, vOpA);

    if( tOpB == 3 ){
      indireccion2 = calculaIndireccion(*mv, vOpB);
      while( mv->mem[indireccion2] != '\0' ){
        mv->mem[direccion1++] = mv->mem[indireccion2++];
      }
    }else if ( tOpB == 2 ){
      direccion2 = calculaDireccion(*mv, vOpB);
      while( mv->mem[direccion2] != '\0' ){
        mv->mem[direccion1++] = mv->mem[direccion2++];
      }
    }

  }

}

void scmp( Mv* mv, int tOpA, int tOpB, int vOpA, int vOpB ) {

  int indireccion1, indireccion2;
  int direccion1, direccion2;
  int rtado;
  char str1Aux[300] = "";
  char str2Aux[300] = "";
  int i=0;

  if( tOpA == 3 ){

      indireccion1 = calculaIndireccion(*mv, vOpA);
      while( mv->mem[indireccion1] != '\0'){
        str1Aux[i++] = mv->mem[indireccion1++];
      }

      if( tOpB == 3 ){
        indireccion2 = calculaIndireccion(*mv, vOpB);
        i=0;
        while( mv->mem[indireccion2] != '\0'){
            str2Aux[i++] = mv->mem[indireccion2++];
        }
      }else if( tOpB == 2 ){
        direccion2 = calculaDireccion(*mv, vOpB);
        i=0;
        while( mv->mem[direccion2] != '\0' ){
            str2Aux[i++] = mv->mem[direccion2++];
        }
      }

  }else if( tOpA == 2 ){

      direccion1 = calculaDireccion(*mv, vOpA);
      while( mv->mem[direccion1] != '\0'){
        str1Aux[i++] = mv->mem[direccion1++];
      }

      if( tOpB == 3 ){
        indireccion2 = calculaIndireccion(*mv, vOpB);
        i=0;
        while( mv->mem[indireccion2] != '\0'){
            str2Aux[i++] = mv->mem[indireccion2++];
        }
      }else if( tOpB == 2 ){
        direccion2 = calculaDireccion(*mv, vOpB);
        i=0;
        while( mv->mem[direccion2] != '\0'){
            str2Aux[i++] = mv->mem[direccion2++];
        }
      }
      rtado = strcmp(str1Aux,str2Aux);
      modCC(mv, rtado);
  }

}

void sysWrite( Mv* mv ){

    u32 celda, celdaMax, aux, aux2;
    int i;
    char endl[3];
    int segmento, tamSegmento;

    //Los 12 bits menos significativos me daran informacion de modo de lectura
    aux = mv->reg[0xA] & 0x00000FFF;      //12 bits de AX
    celda = calculaDireccion(*mv, mv->reg[0xD]); //EDX -> Posicion de mem inicial desde donde empiezo la lect
    segmento = mv->reg[0xD] >> 16;
    tamSegmento = mv->reg[segmento] >> 16;
    celdaMax = mv->reg[0xC] & 0x0000FFFF; //CX (Cuantas posiciones de mem como max)

    if( celdaMax >= tamSegmento ){
      printf("Segmentation fault");
      exit(-1);
    }else{

     for( i = celda; i < celda + celdaMax ; i++ ){

       if( (aux & 0x800) == 0 ){ //prompt
         printf("[%04d]:",i);
       }

        if( (aux & 0x100) == 0x100 )
          strcpy(endl,"");
        else
          strcpy(endl,"\n");


        if ( (aux & 0x0F0) == 0x010 ){ //Imprime caracter
          int aux2 = mv->mem[i] & 0xFF; // 1er byte
          if( aux2 != 127 && aux2 >= 32 && aux2 <= 255 ) //Verifico rango del ascii
            printf("%c%s ", aux2, endl );
          else
            printf(".%s",endl);
        }else {

            if( (aux & 0x001) == 0x001 ) //Imprime decimal
              printf(" %d%s", mv->mem[i], endl);
            if( (aux & 0x004) == 0x004) //Imprime octal
              printf(" %O%s", mv->mem[i], endl);
            if( (aux & 0x008) == 0x008) //Imprime hexadecimal
              printf(" %X%s", mv->mem[i], endl);
        }


      }
      printf("\n");
    }
}

void sysRead( Mv* mv ){

    u32 celda, celdaMax, aux;
    int i;
    int num;
    int segmento, tamSegmento;

    //Los 12 bits menos significativos me daran informacion de modo de lectura
    aux = mv->reg[0xA] & 0x00000FFF;      //12 bits de AX
    celda = calculaDireccion(*mv, mv->reg[0xD]); //EDX -> Posicion de mem inicial desde donde empiezo la lect
    segmento = mv->reg[0xD] >> 16;
    tamSegmento = mv->reg[segmento] >> 16;
    celdaMax = mv->reg[0xC] & 0x0000FFFF; //CX (Cuantas posiciones de mem como max)
    printf("\n");
    if( celdaMax >= tamSegmento ){
      printf("Segmentation fault");
      exit(-1);
    }else{

    //Prendido octavo bit -> Leo string, guardo caracter a caracter
    if( (aux & 0x100) == 0x100 ){

      char *string = (char*)malloc(celdaMax*sizeof(char));

      if( (aux & 0x800) == 0x000 ) //prompt
        printf("[%04d]:",celda);

      scanf("%s",string);

      i = 0;
      while( i <= strlen(string) ){
        mv->mem[ celda + i ] = string[i];
        i++;
      }

    }else

      for( i = celda; i < celda + celdaMax ; i++ ){

        if( (aux & 0x800) == 0x00 ) //prompt
          printf("[%04d]:",i);

        if( (aux & 0x00F) == 0x001 )     //Interpreta decimal
          scanf("%d", &num);
        else if( (aux & 0x00F) == 0x004) //Interpreta octal
          scanf("%O", &num);
        else if( (aux & 0x00F) == 0x008) //Interpreta hexadecimal
          scanf("%X", &num);

        mv->mem[i] = num;

      }
    }
}

void sysStringRead(Mv* mv){

    int direccion;
    int charMax;
    int bitPrompt;
    int HDreg, tamSegmento;
    int i;
    int longStr;

    char *str;

    //Recupero EDX y calculo la direccion relativa al segmento correspondiente
    direccion = calculaDireccion(*mv, mv->reg[0xD]);

    //Recupero la cantidad max de caracteres a leer
    charMax = mv->reg[0xC];

    str = (char*)malloc(charMax*sizeof(char));

    //Recupero bit de prompt
    bitPrompt = (mv->reg[0xA] & 0x800) >> 11;

    if( bitPrompt == 0 )
      printf("[%04d]: ", direccion);

    fgets(str, charMax, stdin);

    //Obtengo la parte alta del registro D (me indica el segmento)
    HDreg = mv->reg[0xD] >> 16;

    //Obtengo el tamaño del segmento
    tamSegmento = mv->reg[HDreg] >> 16;

    longStr = strlen(str);

    if( longStr > (tamSegmento - direccion) ){
      printf("Segmentation fault");
      exit(-1);
    }else{
      i = 0;
      while( str[i] != '\0' ){
        mv->mem[direccion++] = str[i++];
      }
    }

}

void sysStringWrite(Mv *mv){

  int str;
  int segmento;
  char letra;
  int bitPrompt, bitEndl;

  segmento = mv->reg[0xD] >> 16;

  //Recupero posicion de inicio de string
  str = calculaDireccion(*mv, mv->reg[0xD]);

  //Recupero bit de prompt
  bitPrompt = (mv->reg[0xA] & 0x800) >> 11;

  //Recupero bit de endline
  bitEndl = (mv->reg[0xA] & 0x100) >> 8;

  if( bitPrompt == 0 )
    printf("[%04d]:  ",str);

  letra = mv->mem[str];
  while( letra != '\0' ){
    printf("%c", letra);
    letra = mv->mem[++str];
  }

  if( bitEndl == 0 )
    printf("\n");

}

void sysCls(){

  system("cls");

}


void sys( Mv* mv, int tOpA, int vOpA ,char mnem[],char* argv[],int argc){

  if( vOpA == 0x2 )      //! Write
    sysWrite( mv );
  else if( vOpA == 0x1 ) //! Read
    sysRead( mv );
  else if( vOpA == 0xF ) //! BreakPoint
    sysBreakpoint(mv,argv,argc,mnem,0);
  else if( vOpA == 0x3 ) //! StringRead
    sysStringRead(mv);
  else if( vOpA == 0x4)  //! StringWrite
    sysStringWrite(mv);
  else if( vOpA == 0x7)  //! ClearScreen
    sysCls();
  else if ( vOpA == 0xD ){//! VDD
    sysVDD(mv);
  }

}

//Recorre los argumentos ingresados desde el ejecutable
//y si existe retorna 1
int apareceFlag(char* argv[],int argc, char* flag ){

  int i = 0;
  int aparece = 0;

  while( i < argc && aparece == 0){
    if( strcmp(argv[i], flag) == 0 )
      aparece = 1;
    i++;
  }
  return aparece;

}

void sysBreakpoint( Mv* mv, char* argv[],int argc,char mnem[],int llamaP){ //! Sin probar

  char aux[10]={""},opA[7]={""},opB[7]={""}; //max 4096#9862
  int i=0;
  int direccionDS, direccionIP, IPinicial, direccion, direccion1, direccion2;

  if( !llamaP )
    sysC( argv, argc );

  if( apareceFlag( argv, argc, "-b" ) ){
    direccionIP = calculaDireccion(*mv, mv->reg[5]);
    printf("[%04d] cmd: ", direccionIP );//print de IP
    printf("Ingresa un comando: ");
    //Scanf detiene la lecutra cuando encuentra un espacio en blanco
    //gets me permite leer hasta encontrar un salto de linea
    gets(aux);
    if( aux[0] == 'p' ){
      step(mv,argv,argc);
      sysBreakpoint(mv, argv, argc, mnem, 1);
    }else if( aux[0] == 'r' ){
    }
      else{ //Numero entero positivo
        char *token = strtok(aux, " ");
        int cantArg = 0;
        int dms[2];
        while ( token != NULL ) {
          dms[cantArg] = atoi(token);
          cantArg++;
          token = strtok(NULL, " ");
        }
        if (cantArg == 1) {
            direccion = calculaDireccion(*mv, dms[0]);
            printf("[%04d] %08X %d\n", direccion, mv->mem[direccion],mv->mem[direccion]);
        } else { //dos argumentos numericos
            if (dms[0] < dms[1]) {
                direccion1 = calculaDireccion(*mv, dms[0]);
                direccion2 = calculaDireccion(*mv, dms[1]);
                for(int i = direccion1; i <= direccion2; i++ ){
                    printf("[%04d] %08X %d\n", i, mv->mem[i],mv->mem[i]);
                }
            }
        }
    }
  }

   if ( apareceFlag( argv, argc, "-d" ) ){

    IPinicial = calculaDireccion(*mv, mv->reg[5]);
    direccionDS = calculaDireccion(*mv, mv->reg[0]);
    i=0;
    while( i < 10 && IPinicial+i < direccionDS ){
      falsoStep(mv,opA,opB,i); //Muestro los operandos y paso a la siguiente opeeracion sin ejecutar
      i++;
    }
    mv->reg[5] = (0x0003 << 16) | IPinicial;
    direccionIP = calculaDireccion(*mv, mv->reg[5]);
    printf("\nRegistros: \n");
    printf("DS =    %06d  |              |                 |                |\n",direccionDS);
    printf("                | IP =   %06d|                 |                |\n",direccionIP);
    printf("CC  =    %06d | AC =   %06d|EAX =     %06d |EBX =     %06d|\n",mv->reg[8],mv->reg[9],mv->reg[10],mv->reg[11]);
    printf("ECX =   %06d  |EDX =   %06d|EEX =     %06d |EFX =     %06d|\n",mv->reg[12],mv->reg[13],mv->reg[14],mv->reg[15]);

  }
}

//Step utilizado para poder realizar en el sys %F -d
//Es un step pero sin la ejecucion
void falsoStep(Mv* mv,char opA[],char opB[],int i){ //! Sin probar

  int instr;
  int numOp, codInstr;
  char mnem[5];
  int tOpA, tOpB;
  int vOpA, vOpB;
  int direccionIP;

  direccionIP = calculaDireccion(*mv, mv->reg[5]);
  instr = mv->mem[direccionIP]; //fetch
  DecodificarInstruccion(instr,&numOp,&codInstr,mnem); //Decodifica el numero de operandos de la instruccion
  mv->reg[5]++;
  DecodificarOperacion(instr,&tOpA,&tOpB,&vOpA,&vOpB,numOp);
  disassembler(mv, tOpA, tOpB, vOpA, vOpB, opA, opB, numOp);
  if(i==0)
    printf("\n>[%04d]:  %x   %d: %s   %s,%s",direccionIP-1,instr,direccionIP,mnem,opA,opB);
  else
    printf("\n [%04d]:  %x   %d: %s   %s,%s",direccionIP-1,instr,direccionIP,mnem,opA,opB);

}

//Sys%F -c -> Ejecuta el clearscreen
void sysC(char* argv[], int argc){

  if( apareceFlag(argv, argc, "-c") )
    system("cls");

}

void jmp( Mv* mv,int tOpA,int vOpA){

      int indireccion;
      int direccion;

     if( tOpA == 0 ){//Inmediato
       mv->reg[5] = (0x3 << 16) | vOpA;
     }
     else if( tOpA == 1 ){//De registro
       mv->reg[5] = (0x3 << 16) | ObtenerValorDeRegistro(*mv,vOpA, 2);
     }
     else if( tOpA == 2 ){//Directo
       direccion = calculaDireccion(*mv, vOpA);
       mv->reg[5] = (0x3 << 16) | mv->mem[direccion];
     }
     else{                //Indirecto
       indireccion = calculaIndireccion(*mv, vOpA);
       mv->reg[5] = (0x3 << 16) | mv->mem[indireccion];
     }

}

//Devuelve segun el tOp el valor dentro del registro, memoria o bien como cte
int DevuelveValor(Mv mv,int tOpA,int vOpA, int numOp){

    int aux;
    int direccion1;
    int indireccion1;

    if( tOpA == 0 ) //Inmediato
        aux = vOpA;
    else if( tOpA == 1 )//De Registro
        aux = ObtenerValorDeRegistro(mv,vOpA,numOp);
    else if( tOpA == 2){
        direccion1 = calculaDireccion(mv, vOpA);
        aux = mv.mem[direccion1];
    }else{
        indireccion1 = calculaIndireccion(mv, vOpA);
        aux = mv.mem[indireccion1];
    }

    return complemento2(aux,numOp);
}

void rnd(Mv *mv, int tOpA, int vOpA){

  int direccion;

  int aux;
  if( tOpA == 0 )         //Inmediato
    aux = vOpA;
  else
    if (tOpA == 1)        //De registro
      aux = ObtenerValorDeRegistro(*mv, vOpA, 1);
    else
      if (tOpA == 2) {    //Directo
        direccion = calculaDireccion(*mv, vOpA);
        aux = mv->mem[direccion];
      }else
        if( tOpA == 3 ){  //Indirecto
          direccion = calculaIndireccion(*mv, vOpA);
          aux = mv->mem[direccion];
        }

  //Cargo en AC el valor aleatorio entre 0 y el valor del operando
  mv->reg[9] = rand() % (aux + 1);

}

