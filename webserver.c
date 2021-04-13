//NAME: Khoa Quach
//EMAIL: khoaquachschool@gmail.com
//ID: 105123806
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
// new header files for CS111 Project 1a:
#include <sys/types.h>
#include <sys/wait.h>
// new header files for CS111 Project 1b:
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
// 118 Project 1: 
#include <sys/stat.h>
#include <time.h>

/*=========CONSTANTS=============*/
// https://stackoverflow.com/questions/2659952/maximum-length-of-http-get-request#:~:text=Most%20web%20servers%20have%20a,specification%20even%20warns%20about%20this.
#define GET_BLOCK 8192
//https://www.ibm.com/support/knowledgecenter/SSEQVQ_8.1.11/client/c_cmd_filespecsyntax.html#:~:text=On%20Linux%3A%20The%20maximum%20length,path%20name%20is%204096%20bytes.
#define PATH_BLOCK 4092

/*=========GLOBALS===============*/ // For easy access in functions
struct sockaddr_in serveraddr;
struct sockaddr_in client_addr;
socklen_t client_size;
int sockfd;
int new_fd;
int port_number;
char curr_time[32] = "Date: ";
char last_time[32] = "Last-Modified: ";
char status[32] = "HTTP/1.1 200 OK\r\n";
char bad_status[32] = "HTTP/1.1 404 Not Found\r\n";
char four_o_four[48] = "\r\n<html><h1>404 Not Found</h1></html>";
char get_request[GET_BLOCK];
char file_name[PATH_BLOCK];
char file_type[32] = "bin\0";

/*=========HelperFunctions=======*/
void print_error(const char* msg){
	fprintf(stderr, "%s , error number: %d, strerror: %s\n ",msg, errno, strerror(errno));
	exit(1);
}
void write_with_check(int fd, const void *buf, size_t size){
	if(write(fd,buf,size) < 0){ 
		print_error("Error: writing error");
	} 
}
//https://fdiv.net/2009/01/14/memset-vs-bzero-ultimate-showdown
void zero_out(){
	bzero(get_request, GET_BLOCK);
	bzero(file_name, PATH_BLOCK);
	bzero(file_type, 32);
}
// resetting, port no, binding, listen , and size
void socket_setup() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		print_error("Error: opening the socket");
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
	// important
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(port_number);
	if (bind(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0){
		print_error("Error: bind error");
	}
	if (listen(sockfd,5) < 0){
		print_error("Error: listen error");
	}
	client_size = sizeof(client_addr);
}

//https://stackoverflow.com/questions/30825491/does-0-appear-naturally-in-text-files
//https://stackoverflow.com/questions/40821419/when-why-is-0-necessary-to-mark-end-of-an-char-array
/*
"But sometimes char arrays are used to store strings. A string is a sequence of characters terminated by \0. 
So, if you want to use your char array as a string you have to terminate your string with a \0.
So, the answer to the question about \0 being "necessary" depends on what you are storing in your char array.
 If you are storing a string, then you will have to terminate it with a \0. If you are storing something that is not a string, then \0 has no special meaning at all.""
*/
int main(int argc, char *argv[])
{
	if (argc < 2){
		fprintf(stderr, "Error: Specify a port number greater than 1024\n");
		exit(1);
	}
	port_number = atoi(argv[1]);
	socket_setup();
	while(1){
		new_fd = accept(sockfd,(struct sockaddr *) &client_addr, &client_size);
		if (new_fd < 0){
			print_error("Error: accept");
		}
		zero_out(); // clearing buffers in case
		if(read(new_fd,get_request,GET_BLOCK) < 0){
			print_error("Error: read request (from new_fd socket)");
		}
		fprintf(stdout, "%s\n", get_request); // Print out the http request to terminal
		// HTTP Request manipulation
		int filename_i = 0;
		//int extension_i = 0;
		int name_len = 0;
		int ext_flag = 0;
		for (int i = 5; i < PATH_BLOCK; i++){
			if(get_request[i] == ' '){ // edge case
				break;
			}
			if (strncmp(&get_request[i], "%20", 3) == 0){
				file_name[filename_i++] = ' ';
				i += 2;
			} else {
				file_name[filename_i++] = get_request[i];
			}
		}
		file_name[filename_i++] = '\0';
		name_len = strlen(file_name);
		for (int i = 0; i < name_len; i++){
			if(file_name[i] == '.'){
				ext_flag = 1;
				if (ext_flag){
					int extension_i = 0;
					for (int j = i+1; file_name[j] != '\0'; j++, extension_i++) {
						file_type[extension_i] = file_name[j];
					}
					file_type[extension_i] = '\0';
				}
			}
		}
		//file_type[extension_i++] = '\0';
			/*========HTTP-RESPONSE=========*/
		int fd = open(file_name, O_RDONLY);
		if (fd <= 0){
			write_with_check(new_fd, bad_status, strlen(bad_status));
			char html_str[32] = "Content-Type: text/html\r\n";
			write_with_check(new_fd, html_str, strlen(html_str));
			write_with_check(new_fd, four_o_four, strlen(four_o_four));
			write_with_check(new_fd, "\r\n\r\n", 4);
			//char no_exist[32] = "File doesn't exist\n";
	        //fprintf(stderr,no_exist,strlen(no_exist));
	        close(new_fd);
		} else {
			// https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
			// https://stackoverflow.com/questions/8236/how-do-you-determine-the-size-of-a-file-in-c
			// Get the size
			struct stat st;
			stat(file_name, &st);
			long file_size = st.st_size;
			// https://stackoverflow.com/questions/10138192/malloc-the-size-of-file-http_request-base-on-an-existing-file
            // https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
			char* file_buf = (char *)malloc(file_size * sizeof(char) + 1);
			FILE *f = fopen(file_name,"r");
			if (fread(file_buf,1,file_size,f) < 0){
				print_error("Error: fread file content");
			}
			/*===========Time==========*/

			/*=======Content-Length====*/
			char contlen_buf[32] = "Content-Length: ";
			char actual_len[32];
			sprintf(actual_len, "%ld", file_size);
			strcat(contlen_buf,actual_len);
			strcat(contlen_buf,"\r\n");

			/*=======HTML-RESPONSE======*/
			char conn_close[32] = "Connection: Closed\r\n";
			char server[32] = "Server: Khoa-Ubuntu\r\n";
			char html[64];
			char txt[64];
			char jpeg[64];
			char png[64];
			char gif[64];
			char binary[64];
			strcpy(html,"Content-Type: text/html\r\n");
			strcpy(txt, "Content-Type: text/plain\r\n");
			strcpy(jpeg,"Content-Type: image/jpeg\r\n");
			strcpy(png, "Content-Type: image/png\r\n");
			strcpy(gif, "Content-Type: image/gif\r\n");
			strcpy(binary, "Content-Type: application/octet-stream\r\n");

			write_with_check(new_fd, status, strlen(status)); // status 200
			write_with_check(new_fd, conn_close, strlen(conn_close)); // Connection: closed
			//write_with_check(new_fd, curr_time , strlen(curr_time)); // Date:
			write_with_check(new_fd,server,strlen(server)); // Server:
			//write_with_check(new_fd, last_time , strlen(curr_time)); // Last-Modified:
			write_with_check(new_fd, contlen_buf, strlen(contlen_buf)); // Content Length:

			/*==== Content-Type====*/
			if (!strcmp(file_type, "html") || !strcmp(file_type, "htm")){
				write_with_check(new_fd, html, strlen(html));
			} else if (!strcmp(file_type, "txt")) {
				write_with_check(new_fd, txt, strlen(txt));
			} else if (!strcmp(file_type, "jpeg") || !strcmp(file_type,"jpg")){
				write_with_check(new_fd, jpeg, strlen(jpeg));
			} else if (!strcmp(file_type, "png")){
				write_with_check(new_fd, png, strlen(png));
			} else if (!strcmp(file_type, "gif")){
				write_with_check(new_fd, gif, strlen(gif));
			} else if (!strcmp(file_type, "bin")){
				write_with_check(new_fd, binary, strlen(binary));
			} else if (fd <= 0) {
				write_with_check(new_fd, bad_status, strlen(bad_status));
				char html_str[32] = "Content-Type: text/html\r\n";
				write_with_check(new_fd, html_str, strlen(html_str));
				write_with_check(new_fd, four_o_four, strlen(four_o_four));
				write_with_check(new_fd, "\r\n\r\n", 4);
				//char no_exist[32] = "File doesn't exist\n";
				//fprintf(stderr,no_exist,strlen(no_exist));
				close(new_fd);
			} else {
				write_with_check(new_fd, binary, strlen(binary));
			}
			write_with_check(new_fd, "\r\n\r\n", 2);
			// Print file.txt contents
			write_with_check(new_fd, file_buf, file_size);
			free(file_buf);
			close(new_fd);
		}
	}
	exit(0);
}
