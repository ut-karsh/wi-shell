#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>

void err_method();
void new_process(const char* path, char *const args[], int valid);
void built_in(char* input[], int num);
char* path_check(const char* name);
void parse_line(char* input, int valid);
void parallel(char* ampersand);

char* paths[50];
int num_paths = 0;
int file_d = -1;
int redir_check = 0;
char* output_file = NULL;
pid_t child_pid = 0;

void parallel(char* ampersand) {
  pid_t pid_arr[50];
  int count = 0;
  char* buffer = strtok(strdup(ampersand), "&");
  if((strcmp(buffer, "\n")) != 0) {
  while((buffer != NULL)&&(strlen(buffer)>0)) {
    if((strcmp(buffer, "\n")) == 0) { break; }
      char* input = strtok(strdup(buffer), "\n"); //Removing newline char to prevent errors
      buffer += strlen(input) + 1;
      input+=strspn(input, " \t\n");
      if(strlen(input)>0) {
        if(strcspn(input, ">") <= 0) {
          err_method();
          continue;
        }
        parse_line(input, 1);
        pid_arr[count++] = child_pid;
        child_pid = 0;
      }

    redir_check = 0;
    buffer = strtok(buffer, "&");
  }
}
  for(int i=0; i<count; i++) {
    waitpid(pid_arr[i], NULL, 0);
  }
}

void err_method() {
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

char* path_check(const char* name) {
  for (int i=0; i<num_paths; i++) {

    if (strlen(paths[i]) > 1) {

      char* path_name = strdup(paths[i]);
      strcat(path_name, "/");
      strcat(path_name, name);

      if((access(path_name, X_OK) == 0)) return path_name;
    }
  }
  return NULL;
}

void built_in(char* input[], int num) {

  if((strcmp(input[0], "exit"))==0) {
    if(num == 1) { exit(0); }
    else { err_method(); }

  } else if((strcmp(input[0], "cd"))==0) {
    if(num == 2) {
      int cd_ret = chdir(input[1]);
      if (cd_ret!=0) { err_method(); }
    }
    else { err_method(); }

  } else if((strcmp(input[0], "path"))==0) {

    if(num == 1) {
      paths[0] = "\0";
      return;
    }

    for (int i=1; i<num; i++) {
      paths[i-1] = strdup(input[i]);
      num_paths = i;
    }
  } else { err_method(); }
}

void new_process(const char* path, char *const args[], int valid) {

  pid_t fork_ret = fork();
  if(fork_ret < 0) {
    err_method();
  }
  else if(fork_ret == 0) { //Child Process
    if(redir_check == 1) {
      file_d = open(output_file, O_CREAT | O_RDWR, 0777);
      if(file_d < 0) {
        err_method();
        return;
      }
      dup2(file_d, 1);
      dup2(file_d, 2);
      close(file_d);
    }

    int execv_ret = execv(path, args);
    if(execv_ret < 0) {
      err_method();
      return;
    }
  }
  else if (valid != 1) { // Parent process

    wait(NULL);
    return;
  }
  else {
    child_pid = fork_ret;
  }
}

void parse_line(char* input, int valid) {

  if ((output_file = strchr(input, '>')) != NULL) {
  //  printf("file before:%s\n", output_file);
    output_file[0] = '\0';
    //printf("%s\n", output_file);
    output_file++;
    output_file+=strspn(output_file, " \t\n");
    //printf("v%d\n", valid);
    //printf("%s\n", output_file);
    if (output_file != NULL) {

      if ((strchr(output_file, '>')) != NULL) {
      //  printf("Reached2\n");
        err_method();
        return;
      }
      if ((strchr(output_file, ' ') != NULL) && (valid != 1)) {

      //  printf("Reached3\n");
        err_method();
        return;
      }

      redir_check = 1;
    }
  }

  char* args[50];

  int num = 0;
  args[num] = strtok(strdup(input), " \n\t");

  while(args[num] != NULL) {
    args[++num] = strtok(NULL, " \n\t");
  }

  if(( (strcmp(args[0],"cd") == 0) || (strcmp(args[0],"path") == 0)
  || (strcmp(args[0],"exit") == 0))) {
    built_in(args, num); }

    else {
      const char* path = path_check(args[0]);

      if(path != NULL) {
        char** const argv = args;

        new_process(path, argv, valid);
      }
      else { err_method(); }
    }
  }

  int main(int argc, char *argv[]) {

    paths[0] = strdup("/bin");
    num_paths++;

    size_t buffer_size = 256;
    char* buffer = malloc(buffer_size);

    if(argc == 1) { // Interactive Mode
      while((feof(stdin)) == 0) {

        fflush(stdin);
        fflush(stdout);
        fflush(stderr);
        printf("wish> ");
        if((getline (&buffer,&buffer_size,stdin)) == EOF) { exit(0); }
        if((strcmp(buffer, "\n")) != 0) {
          if((strchr(buffer, '&')) != NULL) {
            parallel(buffer);
            continue;
          }
          char* input = strtok(strdup(buffer), "\n"); //Removing newline char to prevent errors
          input+=strspn(input, " \t\n");
          if(strlen(input)>0) {
            if(strcspn(input, ">") <= 0) {
              err_method();
              continue;
            }
            parse_line(input, 0);
          }
        }
        redir_check = 0;
      }
    }

    else if(argc == 2) { // Batch mode

      FILE *fpointer = fopen(argv[1], "r");
      if (fpointer == NULL) {
        err_method();
        exit(1);
      }

      while((getline (&buffer,&buffer_size, fpointer)) != EOF) {
        if((strcmp(buffer, "\n")) != 0) {
          if((strchr(buffer, '&')) != NULL) {
            parallel(buffer);
            continue;
          }
          char* input = strtok(strdup(buffer), "\n"); //Removing newline char to prevent errors
          input+=strspn(input, " \t\n");
          if(strlen(input)>0) {
            if(strcspn(input, ">") <= 0) {
              err_method();
              continue;
            }
            parse_line(input, 0);
          }
        }
        redir_check = 0;
      }
      fclose(fpointer);
    }
    else {
      err_method();
      exit(1);
    }
    if(file_d != -1) {
      close(file_d);
    }
    exit(0);
    return(0);
  }
