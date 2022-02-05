#include "msh379.h"


static void pr_times(clock_t real, struct tms *tmsstart, struct tms *tmsend);
int argument_identifier(char *input_buffer, char *prog_name, char *arg1, char *arg2, char *arg3, char *arg4);
void execution(char *filename, int num_args, char *arg1, char *arg2, char *arg3, char *arg4);
char *parse_pathname(char *pathname, char *result);


// Storing lstasks information
struct task_info {
    pid_t pid, old_pid;
    char command_line[MAXLINE];
    bool terminated;
};


int main(void) {

    // Set CPU time limit to 10 minutes == 600 seconds
    struct rlimit limit;
    getrlimit(RLIMIT_CPU, &limit);
    limit.rlim_cur = 600;
    limit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &limit);


    int current_num_tasks = 0;  // Current number of accepted tasks
    int temp_int;
    struct task_info task_info_array[NTASK];  // Storing information of accepted tasks

    char input_buffer[MAXLINE], command_buffer[MAXLINE], temp_buffer[MAXLINE], arg1[MAXLINE], arg2[MAXLINE], arg3[MAXLINE], arg4[MAXLINE];
    struct tms tmsstart, tmsend;
    clock_t start, end;
    pid_t pid;
    int fd[2];  // Pipe

    start = times(&tmsstart);  // Starting the clock

    while (true) {
        printf("msh379 [%ld]: ", (long) getpid());

        // Get the entire line of input
        if (fgets(input_buffer, MAXLINE, stdin) != NULL)
            input_buffer[strlen(input_buffer) - 1] = '\0';
        else {
            printf("fgets - Input error\n");
            continue;
        }

        // Extract command
        if (sscanf(input_buffer, "%s", command_buffer) == EOF) {
            // Empty command
            continue;
        }


        // Classify the command and perform an appropriate action
        if (strcmp(command_buffer, "cdir") == 0) {
            // Read pathname to change directory into
            if (sscanf(input_buffer, "%*s%s", temp_buffer) == EOF) {
                printf("Command error\n");
                continue;
            }
            char expanded_pathname[MAXLINE] = "";
            parse_pathname(temp_buffer, expanded_pathname);  // Expand directory alias

            if (chdir(expanded_pathname) < 0) {  // Change directory, fail case
                printf("cdir: failed (pathname= %s)\n", expanded_pathname);
                continue;
            }
            // Success
            printf("cdir: done (pathname= %s)\n", expanded_pathname);
        }



        else if (strcmp(command_buffer, "pdir") == 0) {
            if (getcwd(temp_buffer, MAXLINE * sizeof(char)) == NULL) {
                printf("getcwd failed\n");
                continue;
            }
            printf("%s\n", temp_buffer);  // Print the current working directory

        }

        // lstasks command
        else if (strcmp(command_buffer, "lstasks") == 0) {
            // Print the non-explicitly-terminate head processes
            for (int i = 0; i < current_num_tasks; i++)
                if (!task_info_array[i].terminated)
                    printf("%d: (pid= %ld, cmd= %s)\n", i, (long) task_info_array[i].pid, task_info_array[i].command_line);
        }

        // run command
        else if (strcmp(command_buffer, "run") == 0) {
            if (current_num_tasks >= NTASK)  // If more than 32 tasks, don't run
                continue;

            int num_args = argument_identifier(input_buffer, temp_buffer, arg1, arg2, arg3, arg4);  // Populate arguments
            if (num_args + 1 == EOF) {
                printf("run failed: Cannot fetch program name or arguments\n");
                continue;
            }

            // Establish Pipe for communication with the child process
            if (pipe(fd) < 0) {
                printf("fork error\n");
                continue;
            }
            // Double Fork Technique
            if ((pid = fork()) < 0) {
                printf("fork error\n");
                continue;
            } else if (pid == 0) { // First Child Process
                if ((pid = fork()) < 0) {
                    printf("fork error\n");
                    continue;
                }
                else if (pid > 0) {  // Parent of second child process == first child process
                    char pid_array[MAXLINE];
                    temp_int = snprintf(pid_array, MAXLINE, "%ld", (long) pid);

                    close(fd[0]);  // Close read end
                    write(fd[1], pid_array, temp_int + 1);

                    // Sleep for 0.1 seconds - wait for execlp of the second child to return error if there is
                    struct timespec ts;
                    ts.tv_sec = 0;
                    ts.tv_nsec = 100000000;
                    nanosleep(&ts, NULL);

                    // Wait for the second process to return within 0.1sec (if execlp fail, or the program is just fast to finish)
                    if (waitpid(pid, &temp_int, WNOHANG) == -1) {
                        printf("waitpid() failed\n");
                        exit(1);
                    }

                    // execlp fail
                    if ((WIFEXITED(temp_int) != 0) && (WEXITSTATUS(temp_int) == EXIT_FAILURE))
                            write(fd[1], "false", 6);
                    else  // execlp success, and the program is just really fast to finish
                        write(fd[1], "true", 6);

                    exit(0);  // Exit the first child => make init the parent of the second child
                }

                // Second child process => execute the program
                execution(temp_buffer, num_args, arg1, arg2, arg3, arg4);
            } // End child process

            // Parent process
            close(fd[1]);  // Close write end

            read(fd[0], temp_buffer, MAXLINE);
            temp_int = atoi(temp_buffer);  // temp_int is pid of the new process

            read(fd[0], temp_buffer, MAXLINE);

            // If the process failed to execute for whatever reason, reloop again (don't add to lstasks)
            if (strcmp(temp_buffer, "false") == 0)
                continue;

            // Successfully executing the process
            // Record the new process data to the array of struct (for the usage in lstasks)
            task_info_array[current_num_tasks].pid = temp_int;
            strcpy(task_info_array[current_num_tasks].command_line, input_buffer);
            task_info_array[current_num_tasks].terminated = false;

            current_num_tasks++;  // Increment the number of tasks accepted

            // Clear the zombie - Must be after close the write end
            if (waitpid(pid, NULL, 0) != pid)
                printf("waitpid() error\n");

        }


        else if (strcmp(command_buffer, "stop") == 0) {
            temp_int = -1;
            sscanf(input_buffer, "%*s%d", &temp_int);
            printf("%d\n", temp_int);
            if ((temp_int < 0) || (temp_int >= current_num_tasks)) {
                printf("taskNo invalid\n");
                continue;
            }
            if (kill(task_info_array[temp_int].pid, SIGSTOP) < 0) {
                printf("stop error. Task already killed\n");
                continue;
            }
            printf("stop success\n");
        }


        else if (strcmp(command_buffer, "continue") == 0) {
            sscanf(input_buffer, "%*s%d", &temp_int);
            if ((temp_int < 0) || (temp_int >= current_num_tasks)) {
                printf("taskNo invalid.\n");
                continue;
            }
            if (kill(task_info_array[temp_int].pid, SIGCONT) < 0) {
                printf("continue error. Task already killed\n");
                continue;
            }
            printf("continue success\n");

        }

        // terminate command
        else if (strcmp(command_buffer, "terminate") == 0) {
            sscanf(input_buffer, "%*s%d", &temp_int);

            // temp_int is taskNo
            // Check for valud taskNo
            if ((temp_int < 0) || (temp_int >= current_num_tasks)) {
                printf("taskNo invalid\n");
                continue;
            }
            task_info_array[temp_int].terminated = true;  // delete from lstasks
            if (kill(task_info_array[temp_int].pid, SIGKILL) < 0) { // terminate the process
                printf("Task already killed - remove from lstasks\n");
                continue;
            }
            printf("terminate success - remove from lstasks\n");
        }


        // check command
        else if (strcmp(command_buffer, "check") == 0) {
            if (sscanf(input_buffer, "%*s%s", temp_buffer) == EOF) {
                printf("Command error\n");
                continue;
            }
            int target_id = atoi(temp_buffer);


            char string1[MAXLINE], string2[MAXLINE], string3[MAXLINE], string4[MAXLINE], string5[MAXLINE], string6[MAXLINE];

            // Initialize linked list to store rows of "ps" command
            // Note that "head" has no value, it's a dummy node. The first element in the linked list starts at head->next
            node_t *head = linked_list_init();

            // Open ps command, take the data, populate to linked list
            FILE *fpin = popen("ps -xao user,pid,ppid,state,start,cmd --sort=start", "r");
            fgets(temp_buffer, MAXLINE, fpin);
            bool found = false;  // boolean to indicate whether the target_id is in the process table "ps"

            while (fscanf(fpin, "%s%s%s%s%s%[^\n]\n", string1, string2, string3, string4, string5, string6) != EOF) {
                linked_list_append(head, string1, string2, string3, string4, string5, string6); // populate the linked list

                if (atoi(string2) == target_id) {
                    // target_id founded in "ps"
                    found = true;
                    printf("user\tpid\tppid\tstate\tstart\tcmd\n");
                    printf("%s %s %s %s %s %s\n", string1, string2, string3, string4, string5, string6);

                    if (strcmp(string4, "Z") == 0) {
                        // The process is defuncted/zombie
                        printf("Defunct process\n");
                        break;
                    }
                }
            }
            // target_id founded in "ps"
            if (found) {
                node_t *tmp = head->next;  // the first node in the linked list has no value (since we initialize it that way - see above)
                print_all_descendants(target_id, tmp);  // print all the info of processes in descendant trees root at target_id
            } else
                printf("Process not found\n");
            free_linked_list(head);  // free linked list
            pclose(fpin);
        }


        // exit command
        else if (strcmp(command_buffer, "exit") == 0) {
            for (int i = 0; i < current_num_tasks; i++) {
                if (!task_info_array[i].terminated) {
                    task_info_array[i].terminated = true;  // remove from lstasks only

                    if (kill(task_info_array[i].pid, SIGKILL) < 0)  // task is already finished/dead
                        printf("task %d is already finished/dead - Delete from lstasks\n", i);
                    else  // terminating ongoing task
                        printf("task %d terminated\n", i);
                }
            }
        }

        // quit
        else if (strcmp(command_buffer, "quit") == 0)
            break;  // exit the main loop

        else
            printf("Non-recognized command\n");
    }


    end = times(&tmsend);  // Ending the clock
    pr_times(end - start, &tmsstart, &tmsend);  // Print the timing information

    return 0;
}


// Running the assigned task
void execution(char *filename, int num_args, char *arg1, char *arg2, char *arg3, char *arg4) {
    // Depends on the number of arguments (max 4), we use execlp() appropriately
    switch (num_args) {
        case 0:
            if (execlp(filename, filename, (char *) 0) < 0) {
                // Failure executing the file
                printf("execlp error\n");
                _exit(EXIT_FAILURE);
            }
            break;
        case 1:
            if (execlp(filename, filename, arg1, (char *) 0) < 0) {
                // Failure executing the file
                printf("execlp error\n");
                _exit(EXIT_FAILURE);
            }
            break;
        case 2:
            if (execlp(filename, filename, arg1, arg2, (char *) 0) < 0) {
                // Failure executing the file
                printf("execlp error\n");
                _exit(EXIT_FAILURE);
            }
            break;
        case 3:
            if (execlp(filename, filename, arg1, arg2, arg3, (char *) 0) < 0) {
                // Failure executing the file
                printf("execlp error\n");
                _exit(EXIT_FAILURE);
            }
            break;
        case 4:
            if (execlp(filename, filename, arg1, arg2, arg3, arg4, (char *) 0) < 0) {
                // Failure executing the file
                printf("execlp error\n");
                _exit(EXIT_FAILURE);
            }
            break;
        default:
            _exit(EXIT_FAILURE);
    } // End switch
}


// Parse the command line into arguments (arg1, arg2, arg3, arg4)
// Return the number of arguments
int argument_identifier(char *input_buffer, char *prog_name, char *arg1, char *arg2, char *arg3, char *arg4) {
    int num_args = sscanf(input_buffer, "%*s%s%s%s%s%s", prog_name, arg1, arg2, arg3, arg4);
    return num_args - 1;
}


// Printing the timing information
static void pr_times(clock_t real, struct tms *tmsstart, struct tms *tmsend) {
    static long clktck = 0;

    if (clktck == 0) /* fetch clock ticks per second first time */
        if ((clktck = sysconf(_SC_CLK_TCK)) < 0) {
            perror("sysconf error");
            exit(1);
        }

    printf(" real: %7.2f sec\n", real / (double) clktck);
    printf(" user: %7.2f sec\n", (tmsend->tms_utime - tmsstart->tms_utime) / (double) clktck);
    printf(" sys: %7.2f sec\n", (tmsend->tms_stime - tmsstart->tms_stime) / (double) clktck);
    printf(" child user: %7.2f sec\n", (tmsend->tms_cutime - tmsstart->tms_cutime) / (double) clktck);
    printf(" child sys: %7.2f sec\n", (tmsend->tms_cstime - tmsstart->tms_cstime) / (double) clktck);
}


// Expand the pathname from directory aliases (if there is)
// Return the expanded pathname to result
char *parse_pathname(char *pathname, char *result) {
    char *token = strtok(pathname, "/");
    while (token != NULL) {
        if (token[0] == '$') {  // If the token starts with "$" => directory alias
            strcat(result, getenv(token + 1));
        } else // Not a directory alias
            strcat(result, token);
        strcat(result, "/");
        token = strtok(NULL, "/");
    }
    return result;
}
