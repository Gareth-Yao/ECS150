#ifndef QUERY_H
#define QUERY_H
#include <iostream>
#include <string>
#include <unistd.h>
void ChangeDirectory(){ std::cout << "cd" << "\n";}
void FindFiles(){ std::cout << "ff" << "\n";}
void ListFiles(){ std::cout << "ls" << "\n";}
void ExitShell(){ std::cout << "exit" << "\n";}
void PrintWorkingDirectory(){ std::cout << "pwd" << "\n";}
void clearLine(int length){
	char deleteChar[] = "\b \b";
	for(int i = 0; i < length; i++){
		write(STDOUT_FILENO, &deleteChar, sizeof(deleteChar));
	}
}
void traverseLogUp(std::vector<std::string> log, std::string &command, int &index, int &buffersize){
	index--;
	if(index >= 0){
		command = log.at(index);
		clearLine(buffersize);
		write(STDOUT_FILENO, command.c_str(), command.size());
        buffersize = command.size();
	}else{
		char audible = '\a';
		write(STDOUT_FILENO, &audible, sizeof(audible));
		index++;
		return;
	}
}


void traverseLogDown(std::vector<std::string> log, std::string &command, int &index, int &buffersize){
	index++;
	if(index > log.size()){
		char audible = '\a';
		write(STDOUT_FILENO, &audible, sizeof(audible));
		index--;
		return;
	}
	if(index == log.size()){
		command = std::string("");
	}else{
		command = log.at(index);
	}

	clearLine(buffersize);
	write(STDOUT_FILENO, command.c_str(), command.size());
    buffersize = command.size();
}

#endif
