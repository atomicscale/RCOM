#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	
	
	if (argc != 2) {
        printf("Usage: %s [PORT NUMBER]\n", argv[0]);
        return -1;
    }
	
	int port = argv[1];
	int connection = -1;
	// Some function to establish the connection
	
	if(connection > 0){
		
		printf("---------------------");
		printf("Conection established");
		printf("---------------------");
		
		
	} else {
		printf("Conection failed! \n");
		exit(-1);
	}
		
	return 0;
}