#ifndef READINFUNCTIONS_H
#define READINFUNCTIONS_H
#include <vector>
#include <string>

std::vector<std::string> parseInput(std::string input){
	int start = 0;
	int end = 0;
	char prev;
	std::vector<std::string> v;
	while(end < input.length()){
		char CurChar = input.at(end);
		if(CurChar == ' ' && prev != ' '){
			v.push_back(input.substr(start, end - start));
			start = end;
		}else if(prev == ' '){
			start = end;
		}
		prev = CurChar;
		end++;
	}

	if(input.at(start) != ' '){
		v.push_back(input.substr(start, end - start));
	}

	return v;
}


#endif