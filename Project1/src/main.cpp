#include "noncanmode.h"
#include "ReadInFunctions.h"
#include "query.h"
#include <sys/wait.h>

int main(int argc, char *argv[]){

	std::vector<std::string> entryLog;
	const std::string deleteChar = std::string("\b \b");
	struct termios SavedTermAttributes;
	SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
	int fdpipe[2];
	char RXChar;
	pid_t child_pid;
	int status;
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
	        }else if(RXChar == 0x7F){
	        	if(buffersize == 0){
	        		char audible = '\a';
					write(STDOUT_FILENO, &audible, sizeof(audible));
					continue;
	        	}
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
 
    	int saved_stdout = dup(STDOUT_FILENO);
    	pipe(fdpipe);
    	int prev = 0;
    	for(int i = 0; i < commands.size();i++){	
    		std::string temp = commands.at(i);
    		std::vector<std::string> v = parseInput(temp,' ');
			std::string functionName = v.at(0);

			
			

			if(functionName.compare("exit") == 0){
				goto EXIT;
			}else if(functionName.compare("ls") == 0){
				pipe(fdpipe);
				child_pid = fork();
				if(child_pid == 0){
					dup2(prev, 0);
					// close(fdpipe[0]);
					if(i != commands.size() - 1){
						dup2(fdpipe[1],STDOUT_FILENO);
						close(fdpipe[1]);
					}
					checkRedirection(v);
					ListFiles(v);
					exit(0);
				}else{
					// dup2(STDOUT_FILENO, saved_stdout);
					// close(fdpipe[1]);
					while(child_pid != wait(&status)){}
				}
				close(fdpipe[1]);
				prev = fdpipe[0];
			}else if(functionName.compare("cd") == 0){
				ChangeDirectory(v);
			}else if(functionName.compare("ff") == 0){
				pipe(fdpipe);
				child_pid = fork();
				if(child_pid == 0){
					dup2(prev, 0);
					// close(fdpipe[0]);

					if(i != commands.size() - 1){
						dup2(fdpipe[1],STDOUT_FILENO);
						close(fdpipe[1]);
					}
					checkRedirection(v);
					FindFiles(v);
					exit(0);
				}else{
					// dup2(STDOUT_FILENO, saved_stdout);
					// close(fdpipe[1]);
					while(child_pid != wait(&status)){}
				}
				close(fdpipe[1]);
				prev = fdpipe[0];
			}else if(functionName.compare("pwd") == 0){
				pipe(fdpipe);
				child_pid = fork();
				if(child_pid == 0){
					dup2(prev, 0);
					// close(fdpipe[0]);
					if(i != commands.size() - 1){
						dup2(fdpipe[1],STDOUT_FILENO);
						close(fdpipe[1]);
					}
					checkRedirection(v);
					PrintWorkingDirectory();
					exit(0);
				}else{
					// dup2(STDOUT_FILENO, saved_stdout);
					// close(fdpipe[1]);
					while(child_pid != wait(&status)){}
				}
				close(fdpipe[1]);
				prev = fdpipe[0];
			}else{
				pipe(fdpipe);
				child_pid = fork();
				if(child_pid == 0){
					dup2(prev, 0);
					if(i != commands.size() - 1){
						dup2(fdpipe[1],STDOUT_FILENO);
						close(fdpipe[1]);
					}
					checkRedirection(v);
					std::vector<char*> argv;
					for(const auto arg : v){
						if (arg.compare(">") == 0 || arg.compare("<") == 0){
							break;
						}
						argv.push_back((char*)arg.data());
					}
					argv.push_back(nullptr);
					// while(read(STDIN_FILENO, &RXChar, 1) > 0){
    	// 				write(STDOUT_FILENO, &RXChar, 1);
    	// 			}

					if(execvp(v.at(0).c_str(), argv.data()) < 0){
						std::string errorMessage = std::string("Failed To execute " + functionName + "\n");
						write(STDERR_FILENO, errorMessage.c_str(), errorMessage.size());
						exit(0);
					}
				} else{
					//dup2(STDOUT_FILENO, saved_stdout);
					//close(fdpipe[1]);
					wait(NULL);
				}
				close(fdpipe[1]);
				prev = fdpipe[0];
			}
    	}
    	// dup2(saved_stdout, STDOUT_FILENO);
    	// close(saved_stdout);
    	close(fdpipe[1]);
    	// while(read(fdpipe[0], &RXChar, 1) > 0){
    	// 	write(STDOUT_FILENO, &RXChar, 1);
    	// }
    	close(fdpipe[0]);

	}
	EXIT:ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
    return 0;
}

