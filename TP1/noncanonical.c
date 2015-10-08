/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 
#define FALSE 0
#define TRUE 1
#define A 0x01
#define FLAG 0x10F46

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

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
    newtio.c_cc[VMIN]     = 4;   /* blocking read until 4 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);
    

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }


    /* Verificar flags */

    res = read(fd,buf,5);
    if(buf[0]=FLAG && buf[1]=A && buf[2]=C && buf[3]=BCC && buf[4]=FLAG){
      printf("okapa");
    }
    else{
      printf("non okapa");
    }

    printf("New termios structure set\n");


    // Send a string até "/n"

    res = 0;

    while (STOP==FALSE) {       /* loop for input */
      res += read(fd,buf+res,1);   /* returns after 1 char has been input */
      if (buf[res-1] == '\0') {
        STOP = TRUE;
      }
    }

    printf("Read message: %s (%d bytes)\n", buf, res);

    res = write(fd, buf, res);
    printf("Message resent (%d bytes)\n", res);

    sleep(5);

  tcsetattr(fd,TCSANOW,&oldtio);
  close(fd);
  return 0;
}