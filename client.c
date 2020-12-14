
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>  // for getcwd(), read(), write()
//#include <dirent.h>  // for dirent struct and readdir()

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>  //for sockaddr_in struct
#include <arpa/inet.h>   //for inet_addr and inet_ntoa functions
//#include <sys/stat.h>
#include <fcntl.h>      // for open()
#include <errno.h>    // for detecting errors

extern int errno;



int readPacket(int socket, char *buffer){
    
    short lungime;

    read(socket, &lungime, sizeof(short));
    read(socket, buffer, lungime);

    return lungime;

}


void writePacketWithLength(int socket, char *from, short nrBytes){

    write(socket, &nrBytes, sizeof(short));
    write(socket, from, nrBytes);

}


void writePacket(int socket, char *from){

    short lungime = strlen(from)+1;

    write(socket, &lungime, sizeof(short));
    write(socket, from, lungime);

}

char* extractFilename(char *comanda){

    if(comanda[strlen(comanda)-1]=='\n')    // a newline character at the end messed up the filename (nu apare to timpul, de ex. daca se foloseste functia asta de 2 ori pe
        comanda[strlen(comanda)-1]='\0';    // aceeasi comanda, o data la client inainte sa o trimita si o data la server dupa ce acesta o primeste)
    int i=0;
    while(comanda[i]!=' ')
        i++;
    char *filename=comanda+i+1;

    return filename;
    
}

void get(int socket, char *comanda){

    char *buffer=(char*)calloc(120,sizeof(char));
    int lungime;

    writePacket(socket, comanda);
    readPacket(socket, buffer);     // raspunsul daca fisierul exista sau nu


    if(strcmp(buffer,"NO_FILE")==0){
        printf("File does not exist\n");
        return;
    }

    FILE *fisier=fopen(extractFilename(comanda), "wb");

    while(1){
        lungime=readPacket(socket, buffer);
        if(lungime==0)
            break;
        fwrite(buffer, sizeof(char), lungime, fisier);
    }

    printf("fisier copiat OK\n");

    free(buffer);
    fclose(fisier);

}

void add(int socket, char *comanda){

    char *buffer=(char*)calloc(120,sizeof(char));
    int lungime;

    char *filename=extractFilename(comanda);

    //printf("filename=!%s!\n", filename);

    FILE *fisier=fopen(filename, "rb");

    if(fisier==0){
        printf("File does not exist\n\n");
        return;
    }

    writePacket(socket, comanda);

    while(1){
        lungime=fread(buffer, sizeof(char), 100, fisier);
        writePacketWithLength(socket, buffer, lungime);
        if(lungime==0)
            break;
    }

    printf("fisier trimis ok\n\n");

    free(buffer);
    fclose(fisier);


}

void delete(int socket, char *comanda){

    char *buffer=(char*)calloc(120,sizeof(char));

    writePacket(socket, comanda);
    readPacket(socket, buffer);

    /*
    if(strncmp(buffer,"NO_LOGIN",8)==0){
        printf("You have to be logged in to use this command.\n");
        return;
    }

    */

    if(strncmp(buffer,"NO_FILE",7)==0){
        printf("File does not exist. Nothing deleted\n\n");
        return;
    }
    else if(strncmp(buffer,"ERROR",5)==0){
        printf("Could not delete file %s\n\n", extractFilename(comanda));
    }
    else 
        printf("File %s deleted\n\n", extractFilename(comanda));

    free(buffer);

}



int login(int socket, char *comanda){

    char *buffer=(char*)calloc(120,sizeof(char));

    char username[20], password[20];

    writePacket(socket, comanda);

    printf("Username: ");
    scanf("%s", username);
    writePacket(socket, username);

    printf("Password: ");
    scanf("%s", password);
    writePacket(socket, password);

    readPacket(socket, buffer);

    if(strncmp(buffer,"USER_OK",7)==0){
        printf("You are logged in as user \"%s\".\n\n", username);
        free(buffer);
        memset(comanda-1,0,100);
        return 1;
    }

    else{
        printf("Bad user or password.\n\n");
        free(buffer);
        memset(comanda-1,0,100);
        return 0;
    }


}


int main(){

    /*
    int argc, c       at connection succeeds.
    if(argc<3){       at connection succeeds.
        printf("Usage: <executable_name> <server_ip_address> <port>\n");
        return 0;
    }


    char ip[20], port[20];
    strcpy(ip,argv[1]);
    strcpy(ip,argv[2]);
    */

    printf("Command list:\n");
    printf("login - autentificarea la server\n");
    printf("list - lista cu fisiere de pe server\n");
    printf("get - copiaza o pagina web de pe server\n");
    printf("delete - sterge o pagina web de pe server\n");
    printf("add - uploadeaza pe server o pagina web\n");
    printf("exit - close the client program\n\n");
    

    int clientSocket=socket(AF_INET, SOCK_STREAM, 0);


    char *buffer=(char*) calloc(1000, sizeof(char));   //remember to free
    char comanda[100];

    struct sockaddr_in serverAddress;

    serverAddress.sin_family=AF_INET;
    serverAddress.sin_addr.s_addr=inet_addr("127.0.0.1");
    serverAddress.sin_port=htons(11000);
    memset(serverAddress.sin_zero,0,sizeof(serverAddress.sin_zero));

    
    if(connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress))<0){
        printf("Eroare la connect\n");
        return 0;
    }
    

    printf("Connected to server at %s on port %d.\n", inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));

    int loggedIn=0;
    


    while(1){
        printf(">");
        fgets(comanda,100,stdin);

        /*
        printf("comanda=!%s!\n", comanda);

        for(int i=0;i<20;i++)
            printf("%d ", comanda[i]);
        printf("\n");
        */
        
        if(strncmp(comanda,"\n",1)==0)
            continue;

        if(loggedIn==0 && strncmp(comanda,"login",5)!=0 && strncmp(comanda,"exit",4)!=0){
            printf("You have to be logged in to use commands other than 'exit' and 'login'.\n");
            continue;
        }


        if(strncmp(comanda,"list",4)==0){
            strcpy(buffer,comanda);
            writePacket(clientSocket, buffer);  //trimitem mesajul la server
            readPacket(clientSocket, buffer);   // citim raspunsul
            printf("%s\n", buffer);
        }

         
        else if(strncmp(comanda,"ping",4)==0){
            strcpy(buffer,comanda);
            writePacket(clientSocket, buffer);  //trimitem mesajul la server
            readPacket(clientSocket, buffer);   // citim raspunsul
            printf("%s\n", buffer);
        }
            
        else if(strncmp(comanda,"get",3)==0){
            get(clientSocket, comanda);
        }
        
        else if(strncmp(comanda,"add",3)==0){
            add(clientSocket, comanda);
        }

        else if(strncmp(comanda,"delete",6)==0){
            delete(clientSocket, comanda);
        }
            
        else if(strncmp(comanda,"exit",4)==0){
            writePacket(clientSocket, "USER_EXIT");
            break;
        }

        else if(strncmp(comanda, "login", 5)==0){
            if(login(clientSocket, comanda)==1)
                loggedIn=1;
  
        }
        else if(strcmp(comanda,"\n")==0)
            continue;
            

        else
            printf("Unsupported command\n");
        

        
    }

    shutdown(clientSocket, SHUT_RDWR);    
    close(clientSocket);
    free(buffer);
    
    

    return 0;

}

/*

struct sockaddr_in {
               sa_family_t    sin_family; // address family: AF_INET 
               in_port_t      sin_port;   // port in network byte order 
               struct in_addr sin_addr;   // internet address 
               unsigned char  sin_zero;   // padding bytes (used to make sockaddr_in the same size as sockaddr, for casts)
           };

           // Internet address. 
           struct in_addr {
               uint32_t       s_addr;     // address in network byte order 
           };

*/
