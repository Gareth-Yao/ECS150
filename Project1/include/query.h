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
	if(commands.size() > 1 && commands.at(1).compare(">") != 0){
		std::string path = commands.at(1);
		int returnValue = chdir(path.c_str());
 		if(returnValue < 0){
 		char errorMessage[] = "Error changing directory.\n";
 		write(STDERR_FILENO, &errorMessage, sizeof(errorMessage));
 		}
	}else{
		int returnValue = chdir(getenv("HOME"));
 		if(returnValue < 0){
 		char errorMessage[] = "Error changing directory.\n";
 		write(STDERR_FILENO, &errorMessage, sizeof(errorMessage));
 		}
	}
 	

}

void dfs(std::string root, std::string file, std::vector<std::string> &paths){
	auto dir = opendir(root.c_str());
	if(dir == NULL){
		return;
	}

	while(auto content = readdir(dir)){
		std::string curFile = std::string(content->d_name);
		if(curFile.compare("..") == 0 || curFile.compare(".") == 0){
			continue;
		}
		if(DT_DIR == content->d_type){
			dfs(root+"/"+curFile,file,paths);
		}else if(curFile.compare(file) == 0){
			paths.push_back(std::string(root + "\n"));
		}
	}
	closedir(dir);
}


void FindFiles(std::vector<std::string> commands){
	if(commands.size() == 1 || commands.at(1).compare(">") == 0){
		char errorMessage[] = "ff command requires a filename!\n";
		write(STDERR_FILENO, &errorMessage, sizeof(errorMessage));
		return;
	}

	std::string target = std::string(".");
	if(commands.size() > 2  && commands.at(1).compare(">") != 0){
		target = commands.at(2);
	}

	std::vector<std::string> paths;

	dfs(target, commands.at(1), paths);

	for(int i = 0; i<paths.size();i++){
		write(STDOUT_FILENO, paths.at(i).c_str(), paths.at(i).size());
	}
}

std::string getPermission(const char *filename){
	std::string permission;
	struct stat statbuf;
	if(stat(filename, &statbuf) == -1){
		std::string errorMessage = strerror(errno);
		write(STDERR_FILENO, errorMessage.c_str(), errorMessage.size());
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
	if(commands.size() > 1 && commands.at(1).compare(">") != 0){
		target = commands.at(1);
	}
	auto dir = opendir(target.c_str());
	if(dir == NULL){
		std::string errorMessage = "Failed To Open Directory \"" + target + "/\".\n";
 		write(STDERR_FILENO, errorMessage.c_str(), errorMessage.size());
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
