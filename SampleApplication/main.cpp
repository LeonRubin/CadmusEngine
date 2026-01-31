
#include <Engine/Application/Application.h>
#include <iostream>


int main() {
    Init();
    std::cout<< "Hello, Cadmus Engine!" << std::endl;

    while(!IterateEngineLoop()) {
        
    }

    DeInit();
    return 0;
}