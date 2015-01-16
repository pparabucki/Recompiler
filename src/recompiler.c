// Project: Sistemsko Programiranje
// File:  recompiler.c
// Date:  December 2014 - January 2015
// Author:  Petar Parabucki 115/06

#include <stdio.h>
#include <lightning.h>
#include <stdbool.h> 

#define MAX_NUM_OF_INSTRUCTIONS 100
#define MEMMORY_SIZE 0x10000          // 2^16 je velicina memorije pC

#define MASK_KO 0xF000
#define MASK_KO_SH 12
#define MASK_A1 0x700
#define MASK_A1_SH 8
#define MASK_A2 0x70
#define MASK_A2_SH 4
#define MASK_A3 0x7
#define MASK_A3_SH 0
#define MASK_i1 0x800
#define MASK_i1_SH 11
#define MASK_i2 0x80
#define MASK_i2_SH 7
#define MASK_i3 0x8
#define MASK_i3_SH 3
#define MASK_X  0xF00
#define MASK_X_SH 8
#define MASK_Y  0xF0
#define MASK_Y_SH 4
#define MASK_N  0xF
#define MASK_N_SH 0
#define MASK_IN 0x007F





static jit_state_t *_jit;
jit_state_t* jit_states[MAX_NUM_OF_INSTRUCTIONS]; // states of jit , max instructions


//--------------------------------------------------------------
//                FUNKCIJE
//--------------------------------------------------------------

typedef int (*MOV)    (int, int);       /* X:=Y */
typedef int (*MOV_C)  (int, int);       /* X:=C */
typedef int (*MOV_AN) (int, int);       /* X[j]:=Y[j] N-1 */
typedef int (*MOV_AC) (int, int);       /* X[j]:=Y[j] C-1*/

typedef int (*ADD)    (int, int, int);  
typedef int (*ADD_I)  (int, int, int); 
typedef int (*SUB)    (int, int, int);  
typedef int (*SUB_I)  (int, int, int);  
typedef int (*MUL)    (int, int, int);  
typedef int (*MUL_I)  (int, int, int);  
typedef int (*DIV)    (int, int, int);  
typedef int (*DIV_I)  (int, int, int);   

typedef int (*BEQ)  (int, int, int);   
typedef int (*BGT)  (int, int, int);   


typedef int (*STOP)   ();               
//--------------------------------------------------------------


FILE* getFile(char* argv){

  FILE *ifp;

  ifp = fopen(argv, "r");  // open file for read

  if (ifp == NULL) {
    printf("Nije moguce otvoriti fajl - sledi izlazak iz programa!\n");
    printf("%s \n",argv);
    exit(1);
  }

  return ifp;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

void coolPrint(){

  printf("************************************************************************\n");
  printf(" ____  ____            ____            _      _         _ \n"); 
  printf("/ ___||  _ \\          |  _ \\ _ __ ___ (_) ___| | ____ _| |_\n"); 
  printf("\\___ \\| |_| |  _____  | |_) | '__/ _ \\| |/ _ \\ |/ / _` | __|\n"); 
  printf(" ___) |  __/  |_____| |  __/| | | (_) | |  __/   < (_| | |_\n");  
  printf("|____/|_|             |_|   |_|  \\___// |\\___|_|\\_\\__,_|\\__|\n"); 
  printf("                                    |__/\n");                    
  printf("************************************************************************\n");
}

//--------------------------------------------------------------

int main(int argc, char* argv[]) {
  

  FILE *fajl;
  int rec;
  short int numOfI=0;
  short int numOfIJIT=0;
  bool isIns = true; 


// Struktura masinske instrukcije (16 bita) :
// u komentaru stoji kako ce biti ucitavani pocevsi od index-a 0
  char kodOp = 0;  // biti 12 13 14 15
  char i1 = 0;     // biti 11
  char a1 = 0;     // biti 8 9 10
  char i2 = 0;     // biti 7
  char a2 = 0 ;    // biti 4 5 6
  char i3 = 0;     // biti 3
  char a3 = 0;     // biti 0 1 2
//-------------------------------

// fiksne adrese
 // potrebno 2^16

  int arg1,arg2,arg3;
  int vr1,vr2,vr3;
  bool kraj = false;
  int adr[MEMMORY_SIZE] = {};  // oduzeto 8 zbog toga sto fiksne adrese cuvam u fAdr
  int fAdr[8] ={}  ;
  int startAdr = 0;              // pokazuje od koje adrese pocinje program u memoriji
  int PC = 0;                    // pokazuje do koje instrukcije smo stigli 
  int SP = MEMMORY_SIZE-1;       // pokazuje na prvu slobodnu adresu

  jit_node_t *sp;

  char uX,uY,uN,izX,izY,izN;
  int pom=0,pocAdr=0;

// funkcije :
  MOV     mov; 
  MOV_C   mov_c;
  MOV_AN  mov_an;
  MOV_AC  mov_ac;

  ADD     add;
  ADD_I   add_i;
  SUB     sub;
  SUB_I   sub_i;
  MUL     mul;
  MUL_I   mul_i;
  DIV     divv;
  DIV_I   divv_i;

  BEQ     beq;
  BGT     bgt;

  STOP    stop;

// da se definise samo jednom 

  bool    dMov    = true;
  bool    dMov_c  = true;
  bool    dMov_an = true;
  bool    dMov_ac = true;

  bool    dAdd    = true;
  bool    dAdd_i  = true;
  bool    dSub    = true;
  bool    dSub_i  = true;
  bool    dMul    = true;
  bool    dMul_i  = true;  
  bool    dDiv    = true;
  bool    dDiv_i  = true;

  bool    dBeq  = true;
  bool    dBgt  = true;
  coolPrint();

//--------------------------------------------------------------
//                Dohvatanje fajla 
//--------------------------------------------------------------

  fajl = getFile(argv[1]);
  printf("Fajl %s je dohvacen, sledi ucitavanje isntrukcija.\n",argv[1]);

//--------------------------------------------------------------

//--------------------------------------------------------------
//               Ucitavanje instrukcija
//--------------------------------------------------------------
  fscanf(fajl, "%d", &startAdr);  // od kojeg mesta u memoriji pocinje kod programa
  while (fscanf(fajl, "%x", &adr[numOfI+startAdr]) != EOF ) {
    numOfI++;
  }
  
  printf("Ukupan broj ucitanih isntrukcija je : %d \n",numOfI);

//--------------------------------------------------------------

//--------------------------------------------------------------
//               Ispis instrukcija
//--------------------------------------------------------------

 printf("Sledi ispis ucitanih isntrukcija:\n\n");
 int i=0;
  while(i<numOfI){
    printf("\t%2.d. instrukcija %.4x \n",i+startAdr,adr[i+startAdr] );
    i++;
  }
  printf("\n");

//--------------------------------------------------------------
  

//--------------------------------------------------------------
//              Dekodovanje instrukcija
//--------------------------------------------------------------

i=0;
PC = startAdr;

init_jit(argv[0]);

while((PC-startAdr)<numOfI && !kraj){
  
//--------------------------------------------------------------
//              Instrukcijska rec razlozena  
//--------------------------------------------------------------

  kodOp = (adr[PC] & MASK_KO)>>MASK_KO_SH;
  a1    = (adr[PC] & MASK_A1)>>MASK_A1_SH;
  a2    = (adr[PC] & MASK_A2)>>MASK_A2_SH;
  a3    = (adr[PC] & MASK_A3)>>MASK_A3_SH;
  i1    = (adr[PC] & MASK_i1)>>MASK_i1_SH;
  i2    = (adr[PC] & MASK_i2)>>MASK_i2_SH;
  i3    = (adr[PC] & MASK_i3)>>MASK_i3_SH;

  switch(kodOp){

    //  OPERACIJA MOV
    case 0x0 : 
              printf("OPERACIJA MOV\n"); 
                    
              if(i3==0x0 && a3 == 0x0 ){
                printf("X:=Y\n");

                uX  = (adr[PC] & MASK_A1)>>MASK_A1_SH;
                izY = (adr[PC] & MASK_A2)>>MASK_A2_SH;

                if(dMov==true){

                jit_states[numOfIJIT] = _jit = jit_new_state();
             
                jit_prolog   ();
         arg1 = jit_arg();  // uX
         arg2 = jit_arg();  // PC+1
                jit_getarg   (JIT_R0, arg1);              
                jit_getarg   (JIT_R1, arg2);
                jit_movr     (JIT_R0,JIT_R1);
                jit_retr     (JIT_R0);

               //kod za generisanje prve funkcije
                mov = jit_emit(); 
                jit_clear_state();

                numOfIJIT++;
                dMov=false;

                }

                if(i1==0x1 && i2==0x0){ 
                  //printf("W1 INDIREKTNO!!!\n");
                  adr[adr[uX]] = mov(adr[adr[uX]],adr[izY]); // sledeca rec je konstanta
                }else if(i1==0x0 && i2==0x0){
                  //printf("DIREKTNO!!!\n");
                  adr[uX] = mov(adr[uX],adr[izY]); // sledeca rec je konstanta
                }else if(i1==0x0 && i2==0x1){
                  //printf("W2 INDIREKTNO!!! %d\n");
                  adr[uX] = mov(adr[uX],adr[adr[izY]]); // sledeca rec je konstanta
                }else if(i1 =1 && i2==1){
                  //printf("W1 i W2 INDIREKTNO!!!\n");
                  adr[adr[uX]] = mov(adr[adr[uX]],adr[adr[izY]]);
                }

                

              }else if(i3==0x1 && a3 == 0x0 ){ 
                // DVE RECI
                printf("X:=C , sledeca rec je konstanta : %x\n",adr[PC+1]);  // konstanta

                uX = (adr[PC] & MASK_A1)>>MASK_A1_SH;

                if(dMov_c==true){
                jit_states[numOfIJIT] = _jit = jit_new_state();
            
                jit_prolog   ();
         arg1 = jit_arg();  // uX
         arg2 = jit_arg();  // PC+1
                jit_getarg   (JIT_R0, arg1);              
                jit_getarg   (JIT_R1, arg2);
                jit_movr     (JIT_R0,JIT_R1);
                jit_retr     (JIT_R0);

               //kod za generisanje prve funkcije
                mov_c = jit_emit(); 
                jit_clear_state();

                numOfIJIT++;
                dMov_c = false;
               }
               
                //printf("adr[adr[PC]] = %d , PC = %d adr[PC]=%d\n",adr[adr[PC]] ,PC,adr[PC]);
                
                if(i1==0x1){ 
                  printf("W1 INDIREKTNO!!!\n");
                  adr[adr[uX]] = mov_c(adr[adr[uX]],adr[PC+1]); // sledeca rec je konstanta
                }else{
                  //printf("W1 DIREKTNO!!!\n");
                  adr[uX] = mov_c(adr[uX],adr[PC+1]); // sledeca rec je konstanta
                }

               
                PC++;

              }else if(i3==0x0 && a3 != 0x0 ){

                printf("X[j]:=Y[j] , j=0,...,N-1 , max 7\n");  // niz sa brojem koji je konstanta 
               
                uX  = (adr[PC] & MASK_X)>>MASK_X_SH;
                izY = (adr[PC] & MASK_Y)>>MASK_Y_SH;
                izN = (adr[PC] & MASK_A3)>>MASK_A3_SH;

                if(dMov_an==true){

                jit_states[numOfIJIT] = _jit = jit_new_state();
             
                jit_prolog   ();
         arg1 = jit_arg();  // uX
         arg2 = jit_arg();  // PC+1
                jit_getarg   (JIT_R0, arg1);              
                jit_getarg   (JIT_R1, arg2);
                jit_movr     (JIT_R0,JIT_R1);
                jit_retr     (JIT_R0);

                //kod za generisanje prve funkcije
                mov_an = jit_emit(); 
                jit_clear_state();

                numOfIJIT++;
                dMov_an = false;
              }


                if(i1==0x1 && i2==0x0){ 

                  printf("X INDIREKTNO!!!\n");

                  pocAdr = adr[uX];
                  for(i=0;i<izN;i++){
                    adr[pocAdr+i] = mov_an(adr[pocAdr+i],adr[izY+i]); // ucitava niz
                    printf("Ucitan adr[%d] = %d\n",i,adr[pocAdr+i] );
                  }

                }else if(i1==0x0 && i2==0x0){
                  
                  printf("X i Y DIREKTNO!!!\n");

                  for(i=0;i<izN;i++){
                    adr[uX+i] = mov_an(adr[uX+i],adr[izY+i]); // ucitava niz
                    printf("Ucitan adr[%d] = %d\n",i,adr[uX+i] );
                  }

                }else if(i1==0x0 && i2==0x1){

                  printf("Y INDIREKTNO!!! %d\n");
                  pocAdr = adr[izY];
                  for(i=0;i<izN;i++){
                    adr[uX+i] = mov_an(adr[uX+i],adr[pocAdr+i]); // ucitava niz
                    printf("Ucitan adr[%d] = %d\n",i,adr[pocAdr+i] );
                  }

                }else if(i1==1 && i2==1){
                  int pocAdrX = 0;
                  pocAdrX = adr[uX];
                  printf("X i Y INDIREKTNO!!! %d\n");
                  pocAdr = adr[izY];
                  for(i=0;i<izN;i++){
                    adr[pocAdrX+i] = mov_an(adr[pocAdrX+i],adr[pocAdr+i]); // ucitava niz
                    printf("Ucitan adr[%d] = %d\n",i,adr[pocAdr+i] );
                  }
                }

              
                

              }else if(i3==0x1 && a3 == 0x7 ){
                // DVE RECI

                printf("X[j]:=Y[j] , j=0,...,C-1 , sledeca rec je konstanta\n");  // niz sa brojem koji je konstanta 

                 uX  = (adr[PC] & MASK_X)>>MASK_X_SH;
                izY = (adr[PC] & MASK_Y)>>MASK_Y_SH;
                izN = adr[PC+1];  // ucitava konstantu iz sledece reci

                if(dMov_ac==true){

                jit_states[numOfIJIT] = _jit = jit_new_state();
             
                jit_prolog   ();
         arg1 = jit_arg();  // uX
         arg2 = jit_arg();  // PC+1
                jit_getarg   (JIT_R0, arg1);              
                jit_getarg   (JIT_R1, arg2);
                jit_movr     (JIT_R0,JIT_R1);
                jit_retr     (JIT_R0);

                //kod za generisanje prve funkcije
                mov_ac = jit_emit(); 
                jit_clear_state();

                numOfIJIT++;
                dMov_ac = false;
              }


                if(i1==0x1 && i2==0x0){ 

                  printf("X INDIREKTNO!!!\n");

                  pocAdr = adr[uX];
                  for(i=0;i<izN;i++){
                    adr[pocAdr+i] = mov_ac(adr[pocAdr+i],adr[izY+i]); // ucitava niz
                    printf("Ucitan adr[%d] = %d\n",i,adr[pocAdr+i] );
                  }

                }else if(i1==0x0 && i2==0x0){
                  
                  printf("X i Y DIREKTNO!!!\n");

                  for(i=0;i<izN;i++){
                    adr[uX+i] = mov_ac(adr[uX+i],adr[izY+i]); // ucitava niz
                    printf("Ucitan adr[%d] = %d\n",i,adr[uX+i] );
                  }

                }else if(i1==0x0 && i2==0x1){

                  printf("Y INDIREKTNO!!! %d\n");
                  pocAdr = adr[izY];
                  for(i=0;i<izN;i++){
                    adr[uX+i] = mov_ac(adr[uX+i],adr[pocAdr+i]); // ucitava niz
                    printf("Ucitan adr[%d] = %d\n",i,adr[pocAdr+i] );
                  }

                }else if(i1==1 && i2==1){
                  int pocAdrX = 0;
                  pocAdrX = adr[uX];
                  printf("X i Y INDIREKTNO!!! %d\n");
                  pocAdr = adr[izY];
                  for(i=0;i<izN;i++){
                    adr[pocAdrX+i] = mov_ac(adr[pocAdrX+i],adr[pocAdr+i]); // ucitava niz
                    printf("Ucitan adr[%d] = %d\n",i,adr[pocAdr+i] );
                  }
                }

              

                PC++;
              }

    break;

    //  OPERACIJA ADD
    case 0x1 : 
              printf("OPERACIJA ADD A1 = A2 + A3 (format)\n"); 

              if(dAdd == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_addr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

              //kod za generisanje prve funkcije
              add = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dAdd = false;

              }
              // provera
              if(i1==0){
                vr1 = adr[a1];
              }
              else{
                vr1 = adr[adr[a1]];
              }

              if(i2==0){
                vr2 = adr[a2];
              }
              else{
                vr2 = adr[adr[a2]];
              }

              if(i3==0){
                vr3 = adr[a3];
              }
              else{
                vr3 = adr[adr[a3]];
              }
              
              if(i1==0)
              adr[a1] = add(vr1,vr2,vr3); // ako je x direktno
              else
              adr[adr[a1]] = add(vr1,vr2,vr3);  // ako je x indirektno

    break; 

    //  OPERACIJA ADD INDIREKTNO
    case 0x9 : 
              printf("OPERACIJA ADD A1:= A2 + val(val(PC)) ili A1:= val(val(PC)) + A3 , mem indirektno\n");

              if(dAdd_i == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_addr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

              //kod za generisanje prve funkcije
              add_i = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dAdd_i = false;
              }

              // provera
              if(i1==0){
                vr1 = adr[a1];
              }else{
                vr1 = adr[adr[a1]];
              }
              if(i2==0){
                vr2 = adr[a2];
              }else{
                vr2 = adr[adr[a2]];
              }
              if(i3==0){
                vr3 = adr[a3];
              }else{
                vr3 = adr[adr[a3]];
              }

              if(a2==0){  // ako je y = 0

                if(i1==0)
                  adr[a1] = add_i(vr1,vr3,adr[PC+1]); // ako je x direktno
                else
                  adr[adr[a1]] = add_i(vr1,vr3,adr[PC+1]);  // ako je x indirektno
                  
              }
              else{       // ako je z = 0 

                if(i1==0)
                  adr[a1] = add_i(vr1,vr2,adr[PC+1]); // ako je x direktno
                else
                  adr[adr[a1]] = add_i(vr1,vr2,adr[PC+1]); // ako je x indirektno                
              }
              
              
              PC++;

    break;

    //  OPERACIJA SUB
    case 0x2 : 
              printf("OPERACIJA SUB\n"); 

              if(dSub == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_subr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

      //kod za generisanje prve funkcije
              sub = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dSub = false;

              }
             // provera
              if(i1==0){
                vr1 = adr[a1];
              }
              else{
                vr1 = adr[adr[a1]];
              }

              if(i2==0){
                vr2 = adr[a2];
              }
              else{
                vr2 = adr[adr[a2]];
              }

              if(i3==0){
                vr3 = adr[a3];
              }
              else{
                vr3 = adr[adr[a3]];
              }
              
              if(i1==0)
              adr[a1] = sub(vr1,vr2,vr3); // ako je x direktno
              else
              adr[adr[a1]] = sub(vr1,vr2,vr3);  // ako je x indirektno

    break;

    //  OPERACIJA SUB INDIREKTNO
    case 0xA : 
              printf("OPERACIJA SUB, mem indirektno\n"); 
              
              if(dSub_i == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_subr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

              //kod za generisanje prve funkcije
              sub_i = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dSub_i = false;
              }
              // provera
              if(i1==0){
                vr1 = adr[a1];
              }else{
                vr1 = adr[adr[a1]];
              }
              if(i2==0){
                vr2 = adr[a2];
              }else{
                vr2 = adr[adr[a2]];
              }
              if(i3==0){
                vr3 = adr[a3];
              }else{
                vr3 = adr[adr[a3]];
              }

              if(a2==0){  // ako je y = 0

                if(i1==0)
                  adr[a1] = sub_i(vr1,vr3,adr[PC+1]); // ako je x direktno
                else
                  adr[adr[a1]] = sub_i(vr1,vr3,adr[PC+1]);  // ako je x indirektno
                  
              }
              else{       // ako je z = 0 

                if(i1==0)
                  adr[a1] = sub_i(vr1,vr2,adr[PC+1]); // ako je x direktno
                else
                  adr[adr[a1]] = sub_i(vr1,vr2,adr[PC+1]); // ako je x indirektno                
              }
              
              
              PC++;
    break;

    //  OPERACIJA MUL
    case 0x3 : 
              printf("OPERACIJA MUL\n"); 

              if(dMul == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_mulr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

      //kod za generisanje prve funkcije
              mul = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dMul = false;

              }
              // provera
              if(i1==0){
                vr1 = adr[a1];
              }
              else{
                vr1 = adr[adr[a1]];
              }

              if(i2==0){
                vr2 = adr[a2];
              }
              else{
                vr2 = adr[adr[a2]];
              }

              if(i3==0){
                vr3 = adr[a3];
              }
              else{
                vr3 = adr[adr[a3]];
              }
              
              if(i1==0)
              adr[a1] = mul(vr1,vr2,vr3); // ako je x direktno
              else
              adr[adr[a1]] = mul(vr1,vr2,vr3);  // ako je x indirektno

    break;

    //  OPERACIJA MUL INDIREKTNO
    case 0xB : 
              printf("OPERACIJA MUL, mem indirektno\n"); 
              if(dMul_i == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_mulr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

              //kod za generisanje prve funkcije
              mul_i = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dMul_i = false;
              }
              // provera
              if(i1==0){
                vr1 = adr[a1];
              }else{
                vr1 = adr[adr[a1]];
              }
              if(i2==0){
                vr2 = adr[a2];
              }else{
                vr2 = adr[adr[a2]];
              }
              if(i3==0){
                vr3 = adr[a3];
              }else{
                vr3 = adr[adr[a3]];
              }

              if(a2==0){  // ako je y = 0

                if(i1==0)
                  adr[a1] = mul_i(vr1,vr3,adr[PC+1]); // ako je x direktno
                else
                  adr[adr[a1]] = mul_i(vr1,vr3,adr[PC+1]);  // ako je x indirektno
                  
              }
              else{       // ako je z = 0 

                if(i1==0)
                  adr[a1] = mul_i(vr1,vr2,adr[PC+1]); // ako je x direktno
                else
                  adr[adr[a1]] = mul_i(vr1,vr2,adr[PC+1]); // ako je x indirektno                
              }
              
              
              PC++;
    break;

    //  OPERACIJA DIV
    case 0x4 : 
              printf("OPERACIJA DIV\n"); 

              if(dDiv == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_divr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

              //kod za generisanje prve funkcije
              divv = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dDiv = false;

              }

             // provera
              if(i1==0){
                vr1 = adr[a1];
              }
              else{
                vr1 = adr[adr[a1]];
              }

              if(i2==0){
                vr2 = adr[a2];
              }
              else{
                vr2 = adr[adr[a2]];
              }

              if(i3==0){
                vr3 = adr[a3];
              }
              else{
                vr3 = adr[adr[a3]];
              }
              
              if(i1==0)
              adr[a1] = divv(vr1,vr2,vr3); // ako je x direktno
              else
              adr[adr[a1]] = divv(vr1,vr2,vr3);  // ako je x indirektno


    break;

    //  OPERACIJA DIV INDIREKTNO
    case 0xC : 
              printf("OPERACIJA DIV, mem indirektno\n"); 
              if(dDiv_i == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_divr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

              //kod za generisanje prve funkcije
              divv_i = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dDiv_i = false;
              }
              // provera
              if(i1==0){
                vr1 = adr[a1];
              }else{
                vr1 = adr[adr[a1]];
              }
              if(i2==0){
                vr2 = adr[a2];
              }else{
                vr2 = adr[adr[a2]];
              }
              if(i3==0){
                vr3 = adr[a3];
              }else{
                vr3 = adr[adr[a3]];
              }

              if(a2==0){  // ako je y = 0

                if(i1==0)
                  adr[a1] = divv_i(vr1,vr3,adr[PC+1]); // ako je x direktno
                else
                  adr[adr[a1]] = divv_i(vr1,vr3,adr[PC+1]);  // ako je x indirektno
                  
              }
              else{       // ako je z = 0 

                if(i1==0)
                  adr[a1] = divv_i(vr1,vr2,adr[PC+1]); // ako je x direktno
                else
                  adr[adr[a1]] = divv_i(vr1,vr2,adr[PC+1]); // ako je x indirektno                
              }
              
              
              PC++;
    break;

    //  OPERACIJA BEQ
    case 0x5 : 
              printf("OPERACIJA BEQ\n"); 

              if(dBeq == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_subr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

      //kod za generisanje prve funkcije
              beq = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dBeq = false;
              }


              if(i1==0){
                vr1 = adr[a1];
              }else{
                vr1 = adr[adr[a1]];
              }

              if(i2==0){
                vr2 = adr[a2];
              }else{
                vr2 = adr[adr[a2]];
              }

              if(a1>0 && a2>0){ // kada su ucitani X i Y
                if(beq(vr1,vr1,vr2)==0){ 
                  if(i3==1){
                    PC = adr[PC+1]-1;
                  }else{
                    PC = adr[a3];
                  }
                  //printf(" PC = %d\n",PC );
                }else if(i3==1){  // ovo preskace adresu
                  PC++;
                  //printf("PC++ = %d\n",PC );
                }
              }else if(a1>0 && i2==0 && a2==0){  // kada je ucitano X i 0
                if(beq(vr1,vr1,0)==0){ 
                  if(i3==1){
                    PC = adr[PC+1]-1;
                  }else{
                    PC = adr[a3];
                  }
                  //printf(" PC = %d\n",PC );
                }else if(i3==1){  // ovo preskace adresu
                  PC++;
                  //printf("PC++ = %d\n",PC );
                }
              }else if(a2>0 && i1==0 && a1==0){  // kada je ucitano Y i 0
                if(beq(0,0,vr2)==0){ 
                  if(i3==1){
                    PC = adr[PC+1]-1;
                  }else{
                    PC = adr[a3];
                  }
                  //printf(" PC = %d\n",PC );
                }else if(i3==1){  // ovo preskace adresu
                  PC++;
                  //printf("PC++ = %d\n",PC );
                }                

              }
    break;

    //  OPERACIJA BGT
    case 0x6 : 
              printf("OPERACIJA BGT\n"); 

              if(dBgt == true){

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
      arg1  = jit_arg();
      arg2  = jit_arg();
      arg3  = jit_arg();
              jit_getarg   (JIT_R0, arg1);              
              jit_getarg   (JIT_R1, arg2); 
              jit_getarg   (JIT_R2, arg3); 
              jit_subr     (JIT_R0,JIT_R1,JIT_R2);
              jit_retr     (JIT_R0);

              //kod za generisanje prve funkcije
              bgt = jit_emit();
              jit_clear_state();
              numOfIJIT++;
              dBgt = false;
              }


              if(i1==0){
                vr1 = adr[a1];
              }else{
                vr1 = adr[adr[a1]];
              }

              if(i2==0){
                vr2 = adr[a2];
              }else{
                vr2 = adr[adr[a2]];
              }

              if(a1>0 && a2>0){ // kada su ucitani X i Y
               
                if(bgt(vr1,vr1,vr2)>0){ 
                  if(i3==1){
                    PC = adr[PC+1]-1;
                  }else{
                    PC = adr[a3];
                  }
                  //printf(" PC = %d\n",PC );
                }else if(i3==1){  // ovo preskace adresu
                  PC++;
                  //printf("PC++ = %d\n",PC );
                }
              }else if(a1>0 && i2==0 && a2==0){  // kada je ucitano X i 0
                
                if(bgt(vr1,vr1,0)>0){ 
                  if(i3==1){
                    PC = adr[PC+1]-1;
                  }else{
                    PC = adr[a3];
                  }
                  //printf(" PC = %d\n",PC );
                }else if(i3==1){  // ovo preskace adresu
                  PC++;
                  //printf("PC++ = %d\n",PC );
                }
              }else if(a2>0 && i1==0 && a1==0){  // kada je ucitano Y i 0
                
                if(bgt(0,0,vr2)>0){ 
                  if(i3==1){
                    PC = adr[PC+1]-1;
                  }else{
                    PC = adr[a3];
                  }
                  //printf(" PC = %d\n",PC );
                }else if(i3==1){  // ovo preskace adresu
                  PC++;
                 // printf("PC++ = %d\n",PC );
                }                

              }
    break;

    //  OPERACIJA IN
    case 0x7 : 
              printf("OPERACIJA IN\n"); 

              uX  = (adr[PC] & MASK_X)>>MASK_X_SH;
              izN = (adr[PC] & MASK_IN);

              if(i1 == 0 ){
                printf("DIREKTNO\n");
                for(i=0;i<izN;i++){
                
                  printf("Unesite vrednost : "); scanf("%d",&pom); printf("\n");

                  adr[uX+i] = pom;

                  printf("adr[%d+%d]=%d\n", uX,i,adr[uX+i] ); 
                
                }
              }else{

                printf("INDIREKTNO\n");
                uX  = (adr[PC] & 0x0700)>>MASK_X_SH;
                pom = (adr[PC] & MASK_N);
                izN = adr[pom];
                //printf("izN = %d\n", izN);
                pocAdr = adr[uX];
                //printf("pocadr = %d\n",pocAdr );
                //printf("uX=%d\n",uX );
                for(i=0;i<izN;i++){
                
                  printf("Unesite vrednost : "); scanf("%d",&pom); printf("\n");

                  adr[pocAdr+i] = pom;

                  printf("adr[%d+%d]=%d\n",pocAdr,i,adr[pocAdr+i]); 
              }
            }
    break;

    //  OPERACIJA OUT
    case 0x8 : 
              printf("OPERACIJA OUT\n"); 
              uX  = (adr[PC] & MASK_X)>>MASK_X_SH;
              izN = (adr[PC] & MASK_IN);


              if(i1 == 0 ){
                //printf("DIREKTNO\n");
                printf("--------------------------------------\n");
                for(i=0;i<izN;i++){
                  printf("adr[%d+%d]=%d\n", uX,i,adr[uX+i] ); 
                }
                printf("--------------------------------------\n");
              }else{

                //printf("INDIREKTNO\n");
                uX  = (adr[PC] & 0x0700)>>MASK_X_SH;
                pom = (adr[PC] & MASK_N);
                izN = adr[pom];
                pocAdr = adr[uX];
                printf("--------------------------------------\n");
                for(i=0;i<izN;i++){
                  printf("adr[%d+%d]=%d\n",pocAdr,i,adr[pocAdr+i]); 
                }
                printf("--------------------------------------\n");
            }
    break;

    //  OPERACIJA JSR
    case 0xD : 
              printf("OPERACIJA JSR\n");

              //printf("SP = %d\n", SP);
              adr[SP] = PC+2;   // sledeca instrukcija
              //printf("adr[SP] = %d\n", adr[SP]);
              //printf("SP = %d\n", SP);
              SP--;
              //printf("SP = %d\n", SP);
              PC = adr[PC+1]-1;    // adresa skoka postaje adresa PC-ja
              //printf("PC = %d\n", PC);
              


    break;

    //  OPERACIJA RTS
    case 0xE : 
              //printf("OPERACIJA RTS\n"); 
              SP++;
              PC = adr[SP]-1;
              //printf("SP = %d\n", adr[SP]);
              //printf("PC = %d\n", PC);


    break;

    //  OPERACIJA STOP
    case 0xF : 
              printf("OPERACIJA STOP\n"); 

              jit_states[numOfIJIT] = _jit = jit_new_state();
              
              jit_prolog   ();
  arg1      = jit_arg();
  arg2      = jit_arg();
  arg3      = jit_arg();
              jit_getarg   (JIT_R0, arg1);
              jit_getarg   (JIT_R1, arg2);
              jit_getarg   (JIT_R2, arg3); 
              
              short int numPrint=0;

              if(adr[a1]!=0 && a1!=0){
                numPrint++;  
              }
              if(adr[a2]!=0 && a2!=0){
                numPrint++;
              }
              if(adr[a3]!=0 && a3!=0){
                numPrint++;
              }

              switch(numPrint){

                case 1: jit_pushargi("STOP : %d\n"); break;
                case 2: jit_pushargi("STOP : %d , %d\n"); break;
                case 3: jit_pushargi("STOP : %d , %d , %d\n"); break;
                default: jit_pushargi("STOP : nema argumenata\n"); break;
              }

              jit_ellipsis();

              if(adr[a1]!=0){
                jit_pushargr(JIT_R0);
              }
              if(adr[a2]!=0){
                jit_pushargr(JIT_R1);
              }
              if(adr[a3]!=0){
                jit_pushargr(JIT_R2);
              }
              jit_finishi(printf);
              jit_ret();
              stop = jit_emit();
              jit_clear_state();

              stop(adr[a1],adr[a2],adr[a3]);
              
              numOfIJIT++;

              kraj = true;
              
    break;

    default: printf("ERROR\n"); break;
  }


  PC++;
}

//--------------------------------------------------------------

printf("\nFIKSNA MEMORIJA, STAMPA:\n");
for(i = 0; i<7; i++){
  printf("adr[%d]=%d \n",i,adr[i]);
  //if(i%10==0) printf("\n");  
}
/*
for(i = 200; i<228; i++){
  printf("adr[%d]=%d \n",i,adr[i]);
  //if(i%10==0) printf("\n");  
}
*/
printf("\n");
printf("numOfIJIT = %d:\n",numOfIJIT);
for(i = 0; i<numOfIJIT; i++){
  _jit = jit_states[i];
  jit_destroy_state();
}

  finish_jit();

return 0;
}