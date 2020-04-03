#ifndef QUERY_H
#define QUERY_H
#include <iostream>
#include <string>
#include <unistd.h>
void ChangeDirectory(std::string path){
 	int returnValue = chdir(path.c_str());
 	if(returnValue < 0){
 		char errorMessage[] = "Error changing directory.\n";
 		write(STDOUT_FILENO, &errorMessage, sizeof(errorMessage));
 	}

}
void FindFiles(std::vector<std::string> v){
	char com[] = "Command:";
	write(STDOUT_FILENO, &com, sizeof(com));
	for(auto itr = v.begin(); itr != v.end(); itr++){
		std::string curCommand = std::string(" " + *itr);
		write(STDOUT_FILENO, curCommand.c_str(), curCommand.size());
	}
	char newLine = '\n';
	write(STDOUT_FILENO, &newLine, sizeof(newLine));
}
void ListFiles(std::vector<std::string> v){
	char com[] = "Command:";
	write(STDOUT_FILENO, &com, sizeof(com));
	for(auto itr = v.begin(); itr != v.end(); itr++){
		std::string curCommand = std::string(" " + *itr);
		write(STDOUT_FILENO, curCommand.c_str(), curCommand.size());
	}
	char newLine = '\n';
	write(STDOUT_FILENO, &newLine, sizeof(newLine));
}
void PrintWorkingDirectory(std::vector<std::string> v){
	char com[] = "Command:";
	write(STDOUT_FILENO, &com, sizeof(com));
	for(auto itr = v.begin(); itr != v.end(); itr++){
		std::string curCommand = std::string(" " + *itr);
		write(STDOUT_FILENO, curCommand.c_str(), curCommand.size());
	}
	char newLine = '\n';
	write(STDOUT_FILENO, &newLine, sizeof(newLine));
}
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
