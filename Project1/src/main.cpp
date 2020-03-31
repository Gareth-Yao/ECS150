#include "noncanmode.h"
#include <iostream>
int main(){
	std::cout << "Hello World\n";
	struct termios SavedTermAttributes;
    char RXChar;
    
    SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    
    while(1){
        read(STDIN_FILENO, &RXChar, 1);
        if(0x04 == RXChar){ // C-d
            break;
        }
        else{
            if(isprint(RXChar)){
                printf("RX: '%c' 0x%02X\n",RXChar, RXChar);   
            }
            else{
                printf("RX: ' ' 0x%02X\n",RXChar);
            }
        }
    }
    
    ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    return 0;
}