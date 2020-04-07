#include "noncanmode.h"
#include "ReadInFunctions.h"
#include "query.h"

int main(int argc, char *argv[]){
	std::vector<std::string> entryLog;
	const std::string deleteChar = std::string("\b \b");
	struct termios SavedTermAttributes;
	SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
	char RXChar;
	while(1){
		//Print prompt
		std::string dir = std::string(getcwd(NULL,0));
		if(dir.size() > 16){
			dir= shrinkChar(dir);
		}
		dir += "% ";
		write(STDOUT_FILENO, dir.c_str(), dir.size());


		std::string command;
		bool checkArrow;
		int buffersize = 0;
		int index = entryLog.size();
		while(1){
	        read(STDIN_FILENO, &RXChar, 1);
	        if(RXChar == 0x1B){
	        	checkArrow = true;
	        }else if(checkArrow && RXChar == '['){
	        	char upOrDown;
	        	read(STDIN_FILENO, &upOrDown, 1);
	        	if(upOrDown == 'A'){
	        		traverseLogUp(entryLog, command, index, buffersize);	        		        		
	        	}else if(upOrDown == 'B'){
	        		traverseLogDown(entryLog, command, index, buffersize);
	        	}else{
	        		write(STDOUT_FILENO, &RXChar, sizeof(RXChar));
	        		buffersize++;
	        	}
	        }else if(RXChar == '\n'){
	        	write(STDOUT_FILENO, &RXChar, sizeof(RXChar));
	        	break;
	        }else if(RXChar == 0x7F && buffersize>0){
	        	write(STDOUT_FILENO, deleteChar.c_str(), deleteChar.size());
	        	buffersize--;
	        	if(!command.empty()){
	        		command.pop_back();
	        	}
	        }else if(isprint(RXChar)){
	        	command.push_back(RXChar);
	        	write(STDOUT_FILENO, &RXChar, sizeof(RXChar));
	        	buffersize++;
	        }
	        
    	}
    	if(entryLog.size() > 10){
    		entryLog.erase(entryLog.begin());
    	}
    	entryLog.push_back(command);

    	//check piping
    	std::vector<std::string> commands = parseInput(command,'|');

    	for(auto temp : commands){
    		std::vector<std::string> v = parseInput(temp,' ');
			std::string functionName = v.at(0);
			if(functionName.compare("ls") == 0){
				ListFiles(v);
			}else if(functionName.compare("cd") == 0){
				ChangeDirectory(v);
			}else if(functionName.compare("ff") == 0){
				FindFiles(v);
			}else if(functionName.compare("pwd") == 0){
				PrintWorkingDirectory();
			}else if(functionName.compare("exit") == 0){
				goto EXIT;
			}else{
				char com[] = "Command:";
				write(STDOUT_FILENO, &com, sizeof(com));
				for(auto itr = v.begin(); itr != v.end(); itr++){
					std::string curCommand = std::string(" " + *itr);
					write(STDOUT_FILENO, curCommand.c_str(), curCommand.size());
				}

				char newLine = '\n';
				write(STDOUT_FILENO, &newLine, sizeof(newLine));
			}
    	}
		
	}
	EXIT:ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    return 0;
}

