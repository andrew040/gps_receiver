 #include <stdio.h>
 #include <ctype.h>
 #include <stdlib.h>
 #include <string.h>

#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART


char *strdup(const char *src)
{
    char *tmp = malloc(strlen(src) + 1);
    if(tmp)
        strcpy(tmp, src);
    return tmp;
}

void explode(const char *src, const char *tokens, char ***list, size_t *len)
{   
    if(src == NULL || list == NULL || len == NULL)
        return;

    char *str, *copy, **_list = NULL, **tmp;
    *list = NULL;
    *len  = 0;

    copy = strdup(src);
    if(copy == NULL)
        return;

    str = strtok(copy, tokens);
    if(str == NULL)
        goto free_and_exit;

    _list = realloc(NULL, sizeof *_list);
    if(_list == NULL)
        goto free_and_exit;

    _list[*len] = strdup(str);
    if(_list[*len] == NULL)
        goto free_and_exit;
    (*len)++;


    while((str = strtok(NULL, tokens)))
    {   
        tmp = realloc(_list, (sizeof *_list) * (*len + 1));
        if(tmp == NULL)
            goto free_and_exit;

        _list = tmp;

        _list[*len] = strdup(str);
        if(_list[*len] == NULL)
            goto free_and_exit;
        (*len)++;
    }


free_and_exit:
    *list = _list;
    free(copy);
}



int main (void)
{

	//-------------------------
	//----- SETUP USART 0 -----
	//-------------------------
	//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
	int uart0_filestream = -1;
	 
	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//	O_RDONLY - Open for reading only.
	//	O_RDWR - Open for reading and writing.
	//	O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//	if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//	immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	close(uart0_filestream);
	uart0_filestream = open("/dev/ttyAMA0", O_RDONLY | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}else{
		printf("DTG:******/******.000Z*POS:N****.**** E*****.*****SPD:*.**\n");
	}
	
	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag |= ICANON;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);

        /*
	//----- TX BYTES -----
	unsigned char tx_buffer[20];
	unsigned char *p_tx_buffer;
	
	p_tx_buffer = &tx_buffer[0];
	*p_tx_buffer++ = 'H';
	*p_tx_buffer++ = 'e';
	*p_tx_buffer++ = 'l';
	*p_tx_buffer++ = 'l';
	*p_tx_buffer++ = 'o';
	
	if (uart0_filestream != -1)
	{
		int count = write(uart0_filestream, &tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));		//Filestream, bytes to write, number of bytes to write
		if (count < 0)
		{
			printf("UART TX error\n");
		}
	}

	*/


	while(1){
	//----- CHECK FOR ANY RX BYTES -----
	if (uart0_filestream != -1)
	{
		// Read up to 255 characters from the port if they are there
		unsigned char rx_buffer[256];
		unsigned char * match;
		int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);		//Filestream, buffer to store in, number of bytes to read (max)

		FILE * fp;

		if (rx_length < 0)
		{
			//printf("Error ocurred: %i bytes read\n", rx_length);	//An error occured (will occur if there are no bytes)
		}
		else if (rx_length == 0)
		{
			//printf(" no data waiting\n");//No data waiting
		}
		else
		{
			//Bytes received, terminate string with null
			rx_buffer[rx_length] = '\0'; 
			match = strstr (rx_buffer,"GPRMC");
			if(match){
				char **list;
				size_t i, len;
				explode(rx_buffer, ",", &list, &len);
				
				
				/*
				0: $GPRMC
				1: 115206.000
				2: A
				3: 5127.9049
				4: N
				5: 00528.6147
				6: E
				7: 0.00
				8: 78.93
				9: 190817
				10: A*52
				*/
				
				if(strcmp(list[2],"A")){
					fp = fopen ("file.txt", "a");
					printf("DTG:%s/%sZ*POS:%s%s %s%s*SPD:%s\n",list[9],list[1],list[4],list[3],list[6],list[5],list[7]);
					fprintf(fp,"%s;%s;%s;%s;%s;%s;%s\r\n",list[9],list[1],list[4],list[3],list[6],list[5],list[7]);
					fclose(fp);
				}
				} else{
					printf("POSITION INVALID");
				}
				
				/* free list */
				for(i = 0; i < len; ++i)
					free(list[i]);
				free(list);

			}
		}
	}
	}
	//----- CLOSE THE UART -----
	close(uart0_filestream);


  printf ("Hello, world!\n");
  return 0;
}