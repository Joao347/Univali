#include <iostream>
#include <sys/stat.h>
#include <fstream>

int main(){
    
    //testando função mkdir no POSIX
    mkdir("pasta de testes", 0777);

    return 0;
}