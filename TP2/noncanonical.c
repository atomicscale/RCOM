/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BAUDRATE B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A_SEND 0x03   
#define A_REC 0x01  
#define C_SET 0x07
#define C_UA 0x03
 

volatile int STOP=FALSE;
volatile int STOP_REC=FALSE;


int send_ua(int fd) {
    unsigned char UA[5];
    UA[0] = FLAG;
    UA[1] = A_SEND;
    UA[2] = C_UA;
    UA[3] = C_UA;
    UA[4] = FLAG;

    //SEND UA
    write(fd,UA,5);
    printf("Sent response!\n");

    return 0;
}

int receive_set(int fd, int count){


    int res;
    char buf[255];   

    //RECEIVE SET
    strcpy(buf,"");
    while (STOP_REC == FALSE) {               
        res = read(fd,buf,1);
	
	printf("%x\n", buf[res-1]);
	switch(count){
	case 0:
	   if(buf[res-1] == FLAG) 
              count++;  
           break; 
        case 1:
           if(buf[res-1] == A_SEND) 
              count++; 
           else if(buf[res-1] == FLAG) 
              break; 
           else count=0; 
           break; 
        case 2:
           if(buf[res-1] == C_SET) 
              count++; 
           else if(buf[res-1] == FLAG) 
              count=1; 
           else count=0; 
           break; 
        case 3:
           if(buf[res-1] == (A_SEND^C_SET)) 
              count++; 
           else if (buf[res-1] == FLAG)
	      count = 1;
           else count = 0;
           break; 
        case 4:
           if (buf[res-1] == FLAG) {
	      printf("Received SET!\n");
	      return send_ua(fd);
           }
           else count = 0;
           break;
        }    
    }

    return -1;

 }

int main(int argc, char** argv)
{
    int fd, res;
    struct termios oldtio,newtio;
    char buf[255];
char buf1[255];

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS4", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS4\n");
      exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



    /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
    */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    //RECEIVE SET

      if(receive_set(fd, 0) != 0){
        printf("Error receiving SET!\n");

      }


    strcpy(buf1,"");
    

    while (STOP==FALSE) {       /* loop for input */
      res = read(fd,buf,1);

      if(buf[0]== FLAG){
        receive_set(fd,1);
      }
      else{
         strncat(buf1, buf, res);
         if (buf[res-1]=='\0') STOP=TRUE;
         else buf[res]=0;
      }
    }

    printf("%s\n", buf1);
    res = write(fd,buf1,strlen(buf1) + 1);  
    printf("%d bytes written\n", res);





    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
