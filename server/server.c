
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>  // for getcwd(), read(), write()
#include <dirent.h>  // for dirent struct and readdir()

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>  //for sockaddr_in struct
#include <arpa/inet.h>   //for inet_addr and inet_ntoa functions
#include <fcntl.h>      // for open()
#include <errno.h>    // for detecting errors

//in_addr_t = unsigned int
//socklen_t = unsigned int



// welcoming socket will be in main

extern int errno;



// citeste lungimea si apoi mesajul

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

// trimite lungimea unui mesaj inainte de mesajul propriu-zis pentru a ajuta la citire

int writePacket(int socket, char *from){

    short lungime = strlen(from)+1;
    
    //printf("lungime=%d\n", lungime);
    write(socket, &lungime, sizeof(short));
    write(socket, from, lungime);
    return lungime;

}

// functie pentru ls (va genera rezulatatul si il va trimite clientului, deci trebuie sa primeasca socketul de comunicare cu acel client ca parametru)

void list(int clientSocket){

    DIR *currentDir;
    char currentDirPath[100];
    char *mesaj=(char*) calloc(1000, sizeof(char)); 
    int i=0;

    struct dirent *currentFile;
    currentDir=opendir(getcwd(currentDirPath,sizeof(currentDirPath)));

    while((currentFile=readdir(currentDir))!=NULL){
        if(currentFile->d_name[0]!='.'){            // don't show files whose names start with '.'
            strcpy(mesaj+i, currentFile->d_name);
            i+=strlen(currentFile->d_name) + 1;
            *(mesaj+i-1)='\n';
            
        }
        
    }


    writePacket(clientSocket, mesaj);
    printf("Lista trimisa.\n");



}


char* extractFilename(char *comanda){

    if(comanda[strlen(comanda)-1]=='\n')    // un caracter '\n' strica numele fisierului (nu apare to timpul, de ex. daca se foloseste functia asta de 2 ori pe
        comanda[strlen(comanda)-1]='\0';    // aceeasi comanda, o data la client inainte sa o trimita si o data la server dupa ce acesta o primeste)
    int i=0;
    while(comanda[i]!=' ')
        i++;
    char *filename=comanda+i+1;

    return filename;
    
}

void get(int socket, char *filename){

    char *buffer=(char*)calloc(120,sizeof(char));
    int lungime;

    // try to open file
    printf("Nume fisier:!%s!\n", filename);

    FILE *fisier=fopen(filename, "rb");
    if(fisier==0){
        //printf("nu exista, trimit vorba la client\n");
        writePacket(socket, "NO_FILE"); 
        return;
    }
    else
        writePacket(socket,"OK");

    while(1){
        lungime=fread(buffer, sizeof(char), 100, fisier);
        writePacketWithLength(socket, buffer, lungime);
        if(lungime==0)
            break;
    }

    printf("fisier trimis ok\n");

    free(buffer);
    fclose(fisier);

    

}


void add(int socket, char *filename){

    char *buffer=(char*)calloc(120,sizeof(char));
    int lungime;

    //printf("filename=!%s!\n", filename);

    FILE *fisier=fopen(filename, "wb");

    while(1){
        lungime=readPacket(socket, buffer);
        if(lungime==0)
            break;
        fwrite(buffer, sizeof(char), lungime, fisier);
    }

    printf("fisier copiat OK\n\n");

    free(buffer);
    fclose(fisier);


}


void delete(int socket, char *filename){

    FILE *fisier=fopen(filename, "rb");
    
    if(fisier==0)
        writePacket(socket, "NO_FILE");
    else{
        fclose(fisier);
        if(-1==remove(filename))
            writePacket(socket,"ERROR");
        else
            writePacket(socket, "OK");
    }

}

void login(int socket, char *comanda){

    FILE *fisier=fopen("users.txt", "rb");

    char *buffer=(char*)calloc(120,sizeof(char));
    char username[20]={0}, password[20]={0};
    char userPass[40]={0};
    int lungime;
    //readPacket(socket, username);
    //readPacket(socket, password);

    readPacket(socket, userPass);
    userPass[strlen(userPass)]=' ';
    readPacket(socket, userPass+strlen(userPass));
    lungime=strlen(userPass);
    userPass[lungime]='\n';
    userPass[lungime+1]='\0';


    while(1){
        if(fgets(buffer, 100, fisier)==0)
            break;
        if(strcmp(buffer, userPass)==0){
            writePacket(socket, "USER_OK");
            fclose(fisier);
            free(buffer);
            return;
        } 
    }
    
    writePacket(socket, "BAD_USER");
    fclose(fisier);
    free(buffer);

    //printf("user=!%s!\n", username);
    //printf("pass=!%s!\n", password);

    //printf("user=!%s!\n", userPass);

    //fgets(buffer, 100, fisier);

    

}



int main(){


    char *buffer=(char*) calloc(1000, sizeof(char));   //remember to free

    int monitorSocket=socket(AF_INET, SOCK_STREAM, 0); // socket TCP pentru acceptarea clientilor
    int commSocket; // socketul de comunicare
    int pid=getpid();


    struct sockaddr_in msAddress;
    struct sockaddr_in clientAddress;
    int clientAddressSize=sizeof(clientAddress);


    msAddress.sin_family=AF_INET;
    msAddress.sin_addr.s_addr=inet_addr("127.0.0.1");
    msAddress.sin_port=htons(11000);
    memset(msAddress.sin_zero,0,sizeof(msAddress.sin_zero));

    if(bind(monitorSocket, (struct sockaddr *)&msAddress,sizeof(msAddress))<0)   // fixam socketul pe portul specificat in msAddress
    {
        perror("Eroare la bind\n");
        return errno;
    }
    printf("Monitor socket at %s on port %d.\n", inet_ntoa(msAddress.sin_addr), ntohs(msAddress.sin_port));

    listen(monitorSocket,10);  // ascultam pentru conexiuni din partea clientilor
    printf("Listening...\n");


    while(1){

    
    if(pid!=0){
        commSocket=accept(monitorSocket,(struct sockaddr *) &clientAddress, &clientAddressSize);
        printf("Accepted connection from %s port %d.\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        pid=fork(); //generam un proces copil pentru a trata conexiunea cu fiecare client

        if(pid!=0){
            close(commSocket);
        }
        
        else{

            // proces copil. aici primim si procesam comenzile de  la client
            close(monitorSocket);


            while(1){
                //printf("ready\n");
                readPacket(commSocket, buffer);


                //printf("comanda=!%s!\n", buffer);

                if(strncmp(buffer,"list",4)==0)
                    
                    list(commSocket);
                //else if(strncmp(buffer,"ping",4)==0)
                   // writePacket(commSocket, "pong\n");
                else if(strncmp(buffer,"get",3)==0)
                    get(commSocket, extractFilename(buffer));
                else if(strncmp(buffer,"add",3)==0)
                    add(commSocket, extractFilename(buffer));
                else if(strncmp(buffer,"delete",6)==0)
                    delete(commSocket, extractFilename(buffer));
                else if(strncmp(buffer,"login",5)==0)
                    login(commSocket,buffer);
                else if(strncmp(buffer,"USER_EXIT",9)==0){
                    close(commSocket);
                    break;
                }

                else 
                    writePacket(commSocket, "Unsupported command\n\n");

            free(buffer);


            }
        

        }


    }  

    

    }
    
    
    return 0;

}




