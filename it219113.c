#include "Counter.h"

int sum = 0;
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {

    signal(SIGINT, catcher);
    signal(SIGTERM, catcher);

    DIR *folder;
    struct dirent *entry;
    int files = 0, dfd,ffd;
    off_t foff;
    FILE *fptr;

    if (argc < 2) {
        //current directory+error checking.
        if ((folder = fdopendir((dfd = open("./", O_RDONLY)))) == NULL) {
            fprintf(stderr, "Cannot open directory\n");
            exit(1);
        }
    } else {
        //current directory+error checking.
        if ((folder = fdopendir((dfd = open(argv[1], O_RDONLY)))) == NULL) {
            fprintf(stderr, "Cannot open directory\n");
            exit(1);
        }
    }

    //error checking.
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

        //Checking if a file contains a non ASCII character and if it does we skip it.
        if ((isASCII(ffd, foff)) != 0) {
            printf("The file with name: %s and size of %lld is not an ascii file", entry->d_name, foff);
            continue;
        }

        if (fork() == 0) {

            pthread_t threads[NTHREADS];

            //creating a table of strucs in order to pass it as an argument at the threads function and have all the necessary information.
            data th[NTHREADS];

            for (int i = 0; i < NTHREADS; ++i) {

                th[i].tnum = i;//keeping the threads number
                th[i].foff = foff;//kepping the total count of chars in the file.
                th[i].entry = entry;/*Passing the name of the file in order to know for which file we need to open the new
                file discriptor for*/
                th[i].dfd = dfd; //Passing the Directory file discriptor in order to open a new file descriptor for every thread.

                //Creating the threads we will use to count the words from this file.(STATIC NUMBER FROM THE HEADER FILE)
                pthread_create(&threads[i], NULL, &thread_func, (void *) &th[i]);

            }

            //Only the children will execute this part of code.
            //every child is beeing created with the same dfd and an updated entry variable.
            for (int i = 0; i < NTHREADS; ++i) {
                pthread_join(threads[i], NULL);
            }

            // opening file in writing mode
            fptr = fopen("../output.txt", "a");

            // exiting program if an error occurs
            if (fptr == NULL) {
                printf("Error!");
                exit(1);
            }

            fprintf(fptr, "%d, %s, %d\n", getpid(), entry->d_name, sum);
            exit(0);
        }
        close(ffd);
    }

    //Parent is waiting for all of his children to finish
    for (int i = 0; i <= files; i++) // loop will run files times how many files we counted inside the loop
        wait(NULL);
    closedir(folder);

    //printing a more visual message to the use that everything went good and the program finished
    printO();
    printf("\n");
    printK();
    printf("..............Im done!!!!..............\n");



}

void *thread_func(void *th) {

    /* seting each threads stack with the right variables from the process is beeing called from */
    data *td = th;

    int limit = td->tnum;
    int partialsum = 0, tfd, s = 1;
    unsigned char c, prev = ' ';

    //opening the file to read with openat(), same as open
    if ((tfd = openat(td->dfd, td->entry->d_name, O_RDONLY)) == -1) {
        perror(td->entry->d_name);
        abort();
    }

    //potitioning the "needle" at the right place every time.
    lseek(tfd, limit * (td->foff / NTHREADS), SEEK_SET);

    /*if we are at the last thread we will read till the end, we will read how many characters are left if the division was not perfect.
    We need to POINT out that even if the "last" thread will be executed first  that does not matter since every thread has it's own file
     decriptor and sets the offset where it needs to read and the "needle" will not be moved by an other thread, and this is a guarantee*/
     if (td->tnum == NTHREADS - 1 && td->foff % NTHREADS != 0) {

        for (long i = limit * (td->foff / NTHREADS);
             i < (limit + 1) * (td->foff / NTHREADS) + (td->foff % NTHREADS); i++) {


            read(tfd, &c, 1);

            /* Here we impleement a logic in which we move step by step but we keep the previous letters in order to know
             * if this is the end of a sentece so we will do a +1 in our word counter(partialsum) or we are still reading a word
             * also we try to cach cases in wich for example we have a (.) and then for the sake of the argument, 3 white
             * spaces these wont be 3 words we have to count zero in this case.*/
            if (c == ' ' || c == '\t' || c == '\n' || c == '.' || c == ',') {
                s++;/*keeping a count of the spaces or tab or new line in order not to get confiused at the final
                            if in the case there are two spaces back to back in between two words */
            }
            if ((c == ' ' || c == '\t' || c == '\n' || c == '.' || c == ',') &&
                (prev != ' ' && prev != '\t' && prev != '\n') &&
                (s < 2)) {
                ++partialsum;
            }
            //making the current letter the previous for the next one we will read.
            if (c != ' ' && c != '\t' && c != '\n') {
                prev = c;
                s = 0;
            }
        }
    } else {

        for (long i = limit * (td->foff / NTHREADS); i < (limit + 1) * (td->foff / NTHREADS); i++) {

            read(tfd, &c, 1);

            /* Here we impleement a logic in which we move step by step but we keep the previous letters in order to know
             * if this is the end of a sentece so we will do a +1 in our word counter(partialsum) or we are still reading a word
             * also we try to cach cases in wich for example we have a (.) and then for the sake of the argument, 3 white
             * spaces these wont be 3 words we have to count zero in this case.*/
            if (c == ' ' || c == '\t' || c == '\n' || c == '.' || c == ',') {
                s++;/*keeping a count of the spaces or tab or new line in order not to get confiused at the final
                            if in the case there are two spaces back to back in between two words */
            }
            if ((c == ' ' || c == '\t' || c == '\n' || c == '.' || c == ',') &&
                (prev != ' ' && prev != '\t' && prev != '\n') &&
                (s < 2)) {
                ++partialsum;
            }
            if (c != ' ' && c != '\t' && c != '\n') {
                prev = c;
                s = 0;
            }
        }
    }

    pthread_mutex_lock(&mymutex);
    sum += partialsum;
    pthread_mutex_unlock(&mymutex);

    /* Print the local copy of the argument */
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

// Function to print the pattern of 'K'
void printK()
{
    int i, j, half = 8 / 2, dummy = half;
    for (i = 0; i < 8; i++) {
        printf("*");
        for (j = 0; j <= half; j++) {
            if (j == abs(dummy))
                printf("*");
            else
                printf(" ");
        }
        printf("\n");
        dummy--;
    }
}

// Function to print the pattern of 'O'
void printO()
{
    int i, j, space = (8 / 3);
    int width = 8 / 2 + 8 / 5 + space + space;
    for (i = 0; i < 8; i++) {
        for (j = 0; j <= width; j++) {
            if (j == width - abs(space) || j == abs(space))
                printf("*");
            else if ((i == 0
                      || i == 8 - 1)
                     && j > abs(space)
                     && j < width - abs(space))
                printf("*");
            else
                printf(" ");
        }
        if (space != 0
            && i < 8 / 2) {
            space--;
        }
        else if (i >= (8 / 2 + 8 / 5))
            space--;
        printf("\n");
    }
}