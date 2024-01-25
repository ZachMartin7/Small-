/******************************************************
 * ** Program: SmallShell.c 
 * ** Author: Zachary Martin 
 * ** Date: 11/15/23
 * ********************************************************/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>



// main
// while
// prompt
// check if built in, comment, or blank; do it if so
// fork
// If parent, wait
// If child, exec then exit

#define MAX_Args 512
#define MAX_Input 2048
int Z = 0;
int foreground = -7;
int background = 0; 
int L_exit = -1;
pid_t background_pid = -1;


//Built in commands 

//exit the shell
void shellExit(){
   //taken from slides 
    killpg(0,15);
}

//cd 
void directory_change(char** v, int count){

    //last arguement is & remove it 
    if((strcmp(v[count - 1], "&") == 0)){
        v[count - 1] = NULL;
        //decrement count
        count--; 
    }
   
    //one arguement change it to the home directory 
    if(count == 1){

        char* H_dir = getenv("HOME");
        chdir(H_dir);

    }    
    //more than one argument
    else{
        char cwd [MAX_Input];
        char s[] = "/";
        //cwd with null 
        memset(cwd, '\0', sizeof(cwd));
    //if second arguement is / change the directory 
    if(v[1][0] == '/'){
        strcpy(cwd, v[1]);
    }
    //doesn't have a /
    else{
        //get the current directory
        getcwd(cwd, sizeof(cwd));
        //add / to end of directory
        strcat(cwd, s);
        strcat(cwd, v[1]);
    }

    //change the directory
    chdir(cwd);
   }
}

//status 

void status(){
   
    //exitted abnormal
    if(WIFEXITED(L_exit)){
        
        //get exit status & print it 
        L_exit = WEXITSTATUS(L_exit);
        printf("exit value %d\n", L_exit);
        
        //output buffer flush
        fflush(stdout);
    }
    //exitted normal 
    else if(L_exit = -1){
        //print exit value is 0
        printf("exit value 0\n");

        fflush(stdout);
    }

    //terminated by the signal 
    else{
        //get signal normal 
        L_exit = WTERMSIG(L_exit);
        //print the terminated signal number 
        printf("terminated by signal %d\n", L_exit);

        fflush(stdout);
    }
}

//expand variable function
void expand(char** v, int count){
    //buffer for pid
    char pid[16];
    //string that means expand 
    char* expand = "$$";

    //get the pid and convert to string 
    sprintf(pid, "%d", getpid());

    //loop through the command line arguements 
    for(int i = 0; i < count; i++){
        char* ptr = v[i];
        
        //while $$ is found in the string 
        while ((ptr = strstr(ptr, expand)) != NULL){

            //finds the index of $$
            int index = ptr - v[i];

            // Create a new string 
            char* newArg = malloc(strlen(v[i]) + strlen(pid) + 1);

            // Copy the part of the string before $$
            strncpy(newArg, v[i], index);

            // Append the PID
            strcat(newArg, pid);

            // Append the rest of the original string after $$
            strcat(newArg, v[i] + index + strlen(expand));

            // original string is = new string
            free(v[i]);
            v[i] = newArg;

            //move the pointer to end of the new string 
            ptr = newArg + index + strlen(pid);
        }
    }
}


//opens a fie for reading 
int Read_file(char* file){
    
    //read only file openening 
    int file_d = open(file, O_RDONLY);
    //If it fails return -1 
    if (file_d == -1){
        return -1; 
    }
    //eturn the file descriptor of the file
    return file_d;
}

//opens a file in write only mode 
int Write_file(char* file){
    
    //write the file where permissions are set to 644 meaning owner can read and write, but others can only read 
    int file_d = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    //if it fails return -1
    if (file_d == -1){
        return -1; 
    }
    //return the file descriptor of the file
    return file_d;
}

//function that removes arguments 
void argument_remove(char** v, int* count, int* indR, int number){
    //loop through the number of arguments that need to be removed
    for(int i = 0; i < number; i++){
        //remove argument by setting the index to null
        v[indR[i]] = NULL;
    }
    //decrement number of arguments
    *count -= number;
    
}

//Input Output redirection
int I_O_redirection(char** v, int* count, int b){
    //desciptors that are set to -10
    int ofile = -10; 
    int ifile = -10;

    //array of indexes to be removed
    int indR[MAX_Args];
    //counter
    int c = 0;
    
    // open dev null for both of the reading a wrting 
    int OutD = open("/dev/null", O_WRONLY);
    int InD = open("/dev/null", O_RDONLY);

    //looping through all of the arguments 
    for(int i = 1; i < *count - 1; i++){
        
        //arguement is < 
        if(strcmp(v[i], "<") == 0){
            //open file for reading 
            ifile = Read_file(v[i + 1]);
            //if it fails return 1
            if(ifile == -1){
                return 1;
            }
            //add index of current and next argument to be the next to be removed
            indR[c++] = i;
            indR[c++] = i + 1;
        }
    
    
        // if arguement is > open the file for writing 
        else if(strcmp(v[i], ">") == 0){

            //open the file for writing 
            ofile = Write_file(v[i + 1]);
            //if it fails return 1
            if(ofile == -1){
               return 1;
            }
            //add index of current and next argument to be the next to be removed 
            indR[c++] = i;
            indR[c++] = i + 1;
        }

    }

    //output file was found redirect output to the file
    if(ofile != -10){
        dup2(ofile, 1);
    }

    //input file was found redirect input to the file
    if(ifile != -10){
        dup2(ifile, 0);
    }

    //no output file and the background process is running redirect output to dev/null
    if(ofile == -10 && b == 1){
       dup2(OutD, 1);
    }

    //no input file and the background process is running redirect input to dev/null
    if(ifile == -10 && b == 1){
       dup2(InD, 0);
    }

    //remove the arguements that were use for the redirection process
    argument_remove(v, count, indR, c);

    return 0;
}


//Fork function 
void fork_F(char** v, int count){
    //exit status and id 
    int CExit = -10;
    pid_t ID = -10;

    //backround 
    int B = 0;

    //exit status
    int ExitS = 0;

    //if the last arguement is & remove it and decrement count
    if(strcmp(v[count - 1], "&") == 0){
       if(!Z) {
              B = 1;
       }
        v[count - 1] = NULL;
        count--;
    }

    //fork the new process 
    ID = fork();

    switch(ID){
        case 0:
            //if the background process is running set up SIGINT
            if(!B){
                foreground = getpid();
                struct sigaction ignore_ctrl_c = {0};
                sigemptyset(&ignore_ctrl_c.sa_mask);
                ignore_ctrl_c.sa_flags = 0;

                sigaction(SIGINT, &ignore_ctrl_c, NULL);

            }

            //handle redirection 
            ExitS = I_O_redirection(v, &count, B);

            //if it fails print error message and exit
            if(ExitS){
                fprintf(stderr, "cannot open file\n");
                exit(1);
            }
            
            //execute command typed in shell
            execvp(v[0], v);

            //command cannot be executed print out an error message exit
            fprintf(stderr, "command not recognized\n");
            exit(1);
            break;

        default:
            // Save the PID of the child process
            background_pid = ID;

            //if the background process is running print out the pid
            if(B){
                printf("background pid is %d\n", ID);
                background++;

            }
            //not running in background wait for it to finish and save status 
            else{

                foreground = ID;
                ID = waitpid(ID, &CExit, 0);
                L_exit = CExit;

                //terminated by signal print signal number 
                if (WIFSIGNALED(CExit)){
                    printf("\nterminated by signal %d\n", WTERMSIG(CExit));
                    fflush(stdout);
                }
        }
        break;
    }

}


void comandprompt_built_in(char** argv, int argc){

//line begins with #, or is blank repeat prompt
    if(argv[0][0] == '#'){
        return; 
    }
    else if(argv[0][0] == '\n'){
        return; 
    }

    size_t size = strlen(argv[argc - 1]);
    if (size > 0 && argv[argc - 1][size - 1] == '\n') {
        argv[argc - 1][size - 1] = '\0';
    }

    //expand varibale function 
    expand(argv, argc);

    

    //command is cd 
    if (strcmp(argv[0], "cd") == 0){
        //call cd function
        directory_change(argv, argc);
    }

    //command is status 
    else if(strcmp(argv[0], "status") == 0){
        //call status function
        status();
    }

    //command is exit
    else if(strcmp(argv[0], "exit") == 0){
        //call exit function
        shellExit();
    }

    //command is not built in
    else{
        //call fork function
        fork_F(argv, argc);

    }


};

//SIGTSTP function
void control_z(int handle){

    //entering foreground only mode if 0 
    if(Z == 0){

        //print message to user
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 49);
        //set to 1 showing we are in foreground only mode
        Z = 1;
        fflush(stdout);
    }
    //exiting foreground only mode if we are in it
    else{
        //print message to user
        char* message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 29);

        //not in foreground only mode anymore 
        Z = 0;
        fflush(stdout);
    }
}


int main(){

    // SIGINT and SIGTSTP set up 
    struct sigaction ignoreSIGNT = {0}, ignoreSIGTSTP = {0};

    //ignore sigint signal
    ignoreSIGNT.sa_handler = SIG_IGN;
    
    //ignore sigtstp signal and print message to user 
    ignoreSIGTSTP.sa_handler = control_z;

    //add signals to mask
    sigfillset(&ignoreSIGTSTP.sa_mask);

    //set flags to restart 
    ignoreSIGTSTP.sa_flags = SA_RESTART;

    //change action for sigint
    sigaction(SIGINT, &ignoreSIGNT, NULL);

    //change action for sigtstp
    sigaction(SIGTSTP, &ignoreSIGTSTP, NULL);

    


    

    printf("$ smallsh\n"); 

    //Program continues to run until user exits 
    while(1){

    //check children for background processes
    if(background){

        pid_t child = waitpid(-1, &L_exit, WNOHANG);

        if(child > 0){
            printf("background pid %d is done: ", child);
            fflush(stdout);
            
            if(WIFEXITED(L_exit)){
                L_exit = WEXITSTATUS(L_exit);
                printf("exit value %d\n", L_exit);
                fflush(stdout);
            }
            else{
                L_exit = WTERMSIG(L_exit);
                printf("terminated by signal %d\n", L_exit);
                fflush(stdout);
            }
        }
    }
        // print the prompt 
        printf(": "); 
        fflush(stdout);
        
        //get the input from the user 
        char* inpt = NULL;
        size_t size = 0;
        char** v = malloc(MAX_Args * sizeof(char*));
        int c = 0;
        getline(&inpt, &size, stdin);

        //tokenize input into array of strings meaning arguements
        char* token = strtok(inpt, " ");

        while(c < MAX_Args && token != NULL){
            v[c] = malloc(strlen(token) + 1);
            strcpy(v[c], token);
            c++;
            token = strtok(NULL, " ");
        }

        v[c] = NULL;

        //handle built in commands and fork if not built in
        comandprompt_built_in(v, c);
   }


    return 0; 
}

