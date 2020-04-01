#include "noncanmode.h"
#include <iostream>
#include "ReadInFunctions.h"
#include "query.h"

int main(){

	while(1){
		//getCwd
		std::cout << ">>";
		std::string input;
		getline(std::cin, input);
		std::vector<std::string> v = parseInput(input);
		std::string functionName = v.at(0);
		if(functionName.compare("ls") == 0){
			ListFiles();
		}else if(functionName.compare("cd") == 0){
			ChangeDirectory();
		}else if(functionName.compare("ff") == 0){
			FindFiles();
		}else if(functionName.compare("pwd") == 0){
			PrintWorkingDirectory();
		}else if(functionName.compare("exit") == 0){
			ExitShell();
		}else{
			std::cout << "Command:";

			for(auto itr = v.begin(); itr != v.end(); itr++){
				std::cout << " ";
				std::cout << *itr;
			}
			std::cout << "\n";
		}
	}
    return 0;
}