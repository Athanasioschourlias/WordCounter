#include "Counter.h"

int sum = 0;
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char *argv[]) {

    signal(SIGINT, catcher);
    signal(SIGTERM, catcher);

    DIR *folder;
    struct dirent *entry;
    int files = 0, dfd;
    struct stat statbuf;
    int count = 0, ffd;
    off_t foff, culoff;
    unsigned char buf;
    FILE *fptr;

    if (argc < 2) {
        //current directory.
        if ((folder = fdopendir((dfd = open(".", O_RDONLY)))) == NULL) {
            fprintf(stderr, "Cannot open directory\n");
            exit(1);
        }
    } else {
        if ((folder = fdopendir((dfd = open(argv[1], O_RDONLY)))) == NULL) {
            fprintf(stderr, "Cannot open directory\n");
            exit(1);
        }
    }


    if (folder == NULL) {
        fprintf(stderr, "Error : Failed to open common_file - %s\n", strerror(errno));
        return (1);
    }

    while ((entry = readdir(folder))) {
        //each child we want it to have its own total word sum.
        files++;
        /* On linux/Unix we don't want current and parent directories
             * If you're on Windows machine remove this two lines
             * also we do not want to look at hidden files.
             */
        if (entry->d_name[0] == 46)
            continue;

        /* there is a possible race condition here as the file
         * could be renamed between the readdir and the open */
        if (entry->d_type != DT_REG)
            continue;

        //opening the file to read with openat(), same as open
        if ((ffd = openat(dfd, entry->d_name, O_RDONLY)) == -1) {
            perror(entry->d_name);
            abort();
        }

        //checking if it is an executable file.
        if (fstat(ffd, &statbuf) == 0 && !(statbuf.st_mode & S_IXUSR)) {

            //Find how many characters are in this file.
            if ((foff = lseek(ffd, 0, SEEK_END)) == -1) {
                printf("There was an error");
                abort();
            }

            if (foff == 0) {
                //is the file does not have any characters inside it.
                printf("This file %s contains no words.\n", entry->d_name);
                continue;
            }

            if ((isASCII(ffd, foff)) != 0) {
                printf("The file with name: %s and size of %ld is not an ascii file", entry->d_name, foff);
            }

            //reset the "needle" at the start of the file because we moved it with the last if.
            lseek(ffd, 0, SEEK_SET);

            if (fork() == 0) {
                pthread_t threads[NTHREADS];

                data th[NTHREADS];

                for (int i = 0; i < NTHREADS; ++i) {
                    th[i].tnum = i;//keeping the threads number
                    th[i].foff = foff;//kepping the total count of chars in the file.
                    th[i].ffd = ffd;//keeping the file descripton in order to open if latter a any thread with the right offset.
                    th[i].entry = entry;
                    th[i].dfd =dfd;

                    pthread_create(&threads[i], NULL, &thread_func, (void *) &th[i]);

                }

                //Only the children will execute this part of code.
                //every child is beeing created with the same dfd and an updated entry variable.
                for (int i = 0; i < NTHREADS; ++i) {
                    pthread_join(threads[i], NULL);
                }


                lseek(ffd, foff-1,SEEK_SET);
                read(ffd, &buf, 1);

                // opening file in writing mode
                fptr = fopen("../output.txt", "w");

                // exiting program
                if (fptr == NULL) {
                    printf("Error!");
                    exit(1);
                }

                if (buf == '.' || buf == ' ' || buf == '\n' || buf == '\t' || buf == '/' || buf == ',' ) {
                    fprintf(fptr,"%d, %s, %d\n", getpid(), entry->d_name, --sum);
                } else {
                    fprintf(fptr,"%d, %s, %d\n", getpid(), entry->d_name, sum);
                }
                exit(0);
            }

        }

    }
    //Parent is waiting for all of his children to finish
    for (int i = 0; i <= files; i++) // loop will run files times how many files we counted inside the loop
        wait(NULL);
    closedir(folder);

}

void * thread_func(void *th) {

    /* seting each threads stack with the right variables from the process is beeing called from */
    data *td = th;

    int limit = td->tnum;
    int partialsum = 0,ffd,bc = 0;
    unsigned char buff[100],c;


    if (td->tnum == NTHREADS - 1 && td->foff%NTHREADS != 0){

        pthread_mutex_lock(&mymutex);
        for (long i = limit * (td->foff / NTHREADS); i < (limit + 1) * (td->foff / NTHREADS) + (td->foff%NTHREADS); i++){

            read(td->ffd, &c, 1);
            if(c != ' ' && c != '\t' && c != '\n' && c != '.' && c != ',' && c != ':' && c != '/'){
                buff[bc++] = c;
            } else {
                for (int j = 0; j < 100; j++){
                    buff[i] = 0;
                }
                partialsum += 1;
            }

        }
        pthread_mutex_unlock(&mymutex);
    } else {
        pthread_mutex_lock(&mymutex);
        for (long i = limit * (td->foff / NTHREADS); i < (limit + 1) * (td->foff / NTHREADS); i++) {

            read(td->ffd, &c, 1);
            if(c != ' ' && c != '\t' && c != '\n' && c != '.' && c != ',' && c != ':' && c != '/'){
                buff[bc++] = c;
            } else {
                for (int j = 0; j < 100; j++){
                    buff[i] = 0;
                }
                partialsum += 1;
            }

        }
        pthread_mutex_unlock(&mymutex);
    }

    pthread_mutex_lock(&mymutex);

    sum += partialsum;

    pthread_mutex_unlock(&mymutex);

    /* Print the local copy of the argument */
//    printf("Im the thread with number: %d\n", td->tnum);
    pthread_exit(NULL);
}

int isASCII(int ffd, off_t foff) {

    unsigned char buff;
    int bytes;

    for (int i = 0; i < foff && (bytes = read(ffd, &buff, 1)) != 0; i++) {
        if (buff < 0 || buff > 128) {
            //we found a non ascii char.
            lseek(ffd, 0, SEEK_SET);
            return -1;
        }
    }

    //we havent found a non ASCII char.
    return 0;
}

//Display a message to the user and will continue to run
void catcher(int sig) {

    switch (sig) {
        case SIGINT:
            signal(SIGINT, SIG_IGN);
            printf("Caught signal %d im ignoring it!\n", sig);
            break;

        case SIGTERM:
            signal(SIGTERM, SIG_IGN);
            printf("Caught signal %d im ignoring it!\n", sig);
            break;


    }
}