#ifndef QUERY_H
#define QUERY_H
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
void ChangeDirectory(std::vector<std::string> commands){
	if(commands.size() > 1){
		std::string path = commands.at(1);
		int returnValue = chdir(path.c_str());
 		if(returnValue < 0){
 		char errorMessage[] = "Error changing directory.\n";
 		write(STDOUT_FILENO, &errorMessage, sizeof(errorMessage));
 		}
	}else{
		int returnValue = chdir(getenv("HOME"));
 		if(returnValue < 0){
 		char errorMessage[] = "Error changing directory.\n";
 		write(STDOUT_FILENO, &errorMessage, sizeof(errorMessage));
 		}
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

std::string getPermission(const char *filename){
	std::string permission;
	struct stat statbuf;
	if(stat(filename, &statbuf) == -1){
		std::string errorMessage = strerror(errno);
		write(STDOUT_FILENO, errorMessage.c_str(), errorMessage.size());
		return std::string("\n---------- ");
	}
	S_ISDIR(statbuf.st_mode) ? permission.push_back('d'):permission.push_back('-');
	statbuf.st_mode & S_IRUSR ? permission.push_back('r'):permission.push_back('-');
	statbuf.st_mode & S_IWUSR ? permission.push_back('w'):permission.push_back('-');
	statbuf.st_mode & S_IXUSR ? permission.push_back('x'):permission.push_back('-');
	statbuf.st_mode & S_IRGRP ? permission.push_back('r'):permission.push_back('-');
	statbuf.st_mode & S_IWGRP ? permission.push_back('w'):permission.push_back('-');
	statbuf.st_mode & S_IXGRP ? permission.push_back('x'):permission.push_back('-');
	statbuf.st_mode & S_IROTH ? permission.push_back('r'):permission.push_back('-');
	statbuf.st_mode & S_IWOTH ? permission.push_back('w'):permission.push_back('-');
	statbuf.st_mode & S_IXOTH ? permission.push_back('x '):permission.push_back('- ');
	return permission;
}

void ListFiles(std::vector<std::string> commands){
	std::string target = std::string(".");
	if(commands.size() > 1){
		target = commands.at(1);
	}
	auto dir = opendir(target.c_str());
	if(dir == NULL){
		std::string errorMessage = "Failed To Open Directory \"" + target + "/\".\n";
 		write(STDOUT_FILENO, errorMessage.c_str(), errorMessage.size());
 		return;
	}
	while(auto content = readdir(dir)){
		auto filename = content->d_name;
		std::string permission = getPermission(filename);
		write(STDOUT_FILENO, permission.c_str(), permission.size());
		write(STDOUT_FILENO, &(content->d_name), strlen(content->d_name));
		char newLine = '\n';
		write(STDOUT_FILENO, &newLine, sizeof(newLine));
	}
	closedir(dir);
}
void PrintWorkingDirectory(){
	std::string dir = std::string(getcwd(NULL,0));
	write(STDOUT_FILENO, dir.c_str(), dir.size());
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
