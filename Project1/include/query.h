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
std::string traverseLogUp(std::vector<std::string> log, int &index){
	index--;
	if(index >= 0){
		return log.at(index);
	}else{
		char audible = '\a';
		write(STDOUT_FILENO, &audible, sizeof(audible));
		index++;
	}
}
std::string traverseLogDown(std::vector<std::string> log, int &index){
	index++;
	if(index == log.size()){
		return std::string("");
	}else if(index > log.size()){
		char audible = '\a';
		write(STDOUT_FILENO, &audible, sizeof(audible));
		index--;
		return std::string("");
	}else{
		return log.at(index);
	}
}
#endif