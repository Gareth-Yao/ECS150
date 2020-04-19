#ifndef READINFUNCTIONS_H
#define READINFUNCTIONS_H
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

std::vector<std::string> parseInput(std::string input, char delimiter){
	int start = 0;
	int end = 0;
	char prev = delimiter;
	std::vector<std::string> v;
	while(end < input.length()){
		char CurChar = input.at(end);
		if(CurChar == delimiter && prev != delimiter){
			v.push_back(input.substr(start, end - start));
			start = end;
		}else if(prev == delimiter){
			start = end;
		}
		prev = CurChar;
		end++;
	}

	if(input.at(start) != delimiter){
		v.push_back(input.substr(start, end - start));
	}

	return v;
}

std::string shrinkChar(std::string dirName){
	std::vector<std::string> v = parseInput(dirName, '/');
	std::string ans = std::string("/.../");
	ans += v.at(v.size() - 1);
	return ans;
}

void checkRedirection(std::vector<std::string> commands){
	int fd[2];
	for(int i = 0; i < commands.size(); i++){
		if (commands.at(i).compare(">") == 0){
			std::cout << commands.at(i+1);
			fd[1] = open(commands.at(i+1).c_str(), O_CREAT | O_WRONLY, S_IRWXU);
			if(fd[1] < 0){
				std::string errorMessage = strerror(errno);
				write(STDERR_FILENO, errorMessage.c_str(), errorMessage.size());
				write(STDERR_FILENO, "\n", 1);
				return;
			}

			dup2(fd[1],STDOUT_FILENO);
			close(fd[1]);
		}else if(commands.at(i).compare("<") == 0){
			fd[0] = open(commands.at(i+1).c_str(), O_CREAT | O_RDONLY);
			if(fd[0] < 0){
				std::string errorMessage = strerror(errno);
				write(STDERR_FILENO, errorMessage.c_str(), errorMessage.size());
				write(STDERR_FILENO, "\n", 1);
				return;
			}

			dup2(fd[0],STDIN_FILENO);
		}
	}
}
#endif