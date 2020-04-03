#ifndef READINFUNCTIONS_H
#define READINFUNCTIONS_H
#include <vector>
#include <string>
#include <iostream>
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


#endif