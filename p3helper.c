/* p3helper.c
 Program 3 assignment
 CS570
 SDSU
 Fall 2019
 
 This is the ONLY file you are allowed to change. (In fact, the other
 files should be symbolic links to
 ~cs570/Three/p3main.c
 ~cs570/Three/p3robot.c
 ~cs570/Three/p3.h
 ~cs570/Three/makefile
 ~cs570/Three/CHK.h    )
 */
#include "p3.h"
#include <math.h>

#define COUNTFILE_NAME "countfile"
#define ROWFILE_NAME "rowfile"
#define DOWNFLAG_NAME "downflagfile"
#define NEWCOUNT_NAME "newcountfile"

#define COURSEID "570"

/* You may put declarations/definitions here.
 In particular, you will probably want access to information
 about the job (for details see the assignment and the documentation
 in p3robot.c):
 */
extern int nrRobots;
extern int quota;
extern int seed;

int fd;
int fd2;
int fd3;
int fd4;
// semaphore guarding access to shared data
sem_t *pmutx;
// local copy of counter
int count;
// local copy to keep track of amount of rows
int row;
// flag to see when the number of items per row starts going down from the max
int DOWNFLAG;
// second counter after DOWNFLAG is set, which restarts counting
// each time a new row is started
int newCount = 0;
char semaphoreMutx[SEMNAMESIZE];


void finish(void);
int isPerfectSquare(int number);


/* General documentation for the following functions is in p3.h
 Here you supply the code, and internal documentation:
 */
void initStudentStuff(void) {
    int count = 0;
    
    sprintf(semaphoreMutx,"/%s%ldmutx",COURSEID,(long)getuid());
    // Semaphore Already Exists
    if (SEM_FAILED == (pmutx = sem_open(semaphoreMutx, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0))) {
        pmutx = sem_open(semaphoreMutx, O_RDWR);
        CHK(sem_wait(pmutx));
        CHK(fd = open(COUNTFILE_NAME, O_RDWR));
        CHK(fd2 = open(ROWFILE_NAME, O_RDWR));
        CHK(fd3 = open(DOWNFLAG_NAME, O_RDWR));
        CHK(fd4 = open(NEWCOUNT_NAME, O_RDWR));
        CHK(sem_post(pmutx));
    //Initialize the file
    } else {
        CHK(fd = open(COUNTFILE_NAME,O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR));
        CHK(fd2 = open(ROWFILE_NAME,O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR));
        CHK(fd3 = open(DOWNFLAG_NAME,O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR));
        CHK(fd4 = open(NEWCOUNT_NAME,O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR));
        CHK(lseek(fd,0,SEEK_SET));
        //write count to file
        assert(sizeof(count) == write(fd,&count,sizeof(count)));
        row = 1;
        CHK(lseek(fd2,0,SEEK_SET));
        //write row to file
        assert(sizeof(row) == write(fd2,&row,sizeof(row)));
        CHK(lseek(fd3,0,SEEK_SET));
        // write downflag to file
        assert(sizeof(row) == write(fd3,&DOWNFLAG,sizeof(DOWNFLAG)));
        CHK(lseek(fd4,0,SEEK_SET));
        // write newcounter to file
        assert(sizeof(newCount) == write(fd4,&newCount,sizeof(newCount)));
        //files are now initialized
        CHK(sem_post(pmutx));
    }
}

/* In this braindamaged version of placeWidget, the widget builders don't
 coordinate at all, and merely print a random pattern. You should replace
 this code with something that fully follows the p3 specification. */
void placeWidget(int n) {
    int count;
    // set the last flag, which tells the program that the last
    // widgit has been written
    int last = FALSE;
    int numRows;
    
    /* request access to critical section */
    CHK(sem_wait(pmutx));
    /* begin critical section -- read count, increment count, write count */
    CHK(lseek(fd,0,SEEK_SET));
    assert(sizeof(count) == read(fd,&count,sizeof(count)));
    //rowfile set
    CHK(lseek(fd2,0,SEEK_SET));
    assert(sizeof(row) == read(fd2,&row,sizeof(row)));
    //downflag set
    CHK(lseek(fd3,0,SEEK_SET));
    assert(sizeof(DOWNFLAG) == read(fd3,&DOWNFLAG,sizeof(DOWNFLAG)));
    //newcount set
    CHK(lseek(fd4,0,SEEK_SET));
    assert(sizeof(newCount) == read(fd4,&newCount,sizeof(newCount)));
    
    count++;
    int tmp=count;
    int maxRow = 1;
    // sets the row number for the longest row
    numRows = sqrt(quota * nrRobots);
    if(!isPerfectSquare(quota*nrRobots)){
        numRows = numRows + 1;
    }
    //printf("[Count: %d]", count);
    //printf("[Row: %d]", row);
    if(count == ((nrRobots * quota))){
        printeger(n);
        fflush(stdout);
        last = TRUE;
    } else {
        int i;
        for(i=1; i<row ; i++){
            count=count-i;
        }
//        printf("[Row: %d]", row);
//        printf("[NrRobots: %d]", nrRobots);
        // used when the number of items per row starts decreasing
        if(row > numRows || DOWNFLAG){
            newCount++;
            maxRow = row - 2;
            int j;
            for(j=row-1; j>=1 ; j--){
                count=count+j;
            }
//            printf("[New Count: %d]", newCount);
//            printf("[Max Row: %d]", maxRow);
            // checks when a new line needs to be started
            if(newCount == maxRow){
                printeger(n);
                printf("N\n");
                fflush(stdout);
                row--;
                DOWNFLAG = TRUE;
            }
        }
        // checks when a new line needs to be started
        if(count == row){
            printeger(n);
            printf("N\n");
            fflush(stdout);
            row++;
        }
        else{
            // sets newcounter back to zero to restart for the next row
            if(newCount == maxRow){
                newCount = 0;
            }else{
                // prints PID
                printeger(n);
                fflush(stdout);
            }
        }
        count=tmp;
        CHK(lseek(fd,0,SEEK_SET));
        assert(sizeof(count) == write(fd,&count,sizeof(count)));
        CHK(lseek(fd2,0,SEEK_SET));
        assert(sizeof(row) == write(fd2,&row,sizeof(row)));
        CHK(lseek(fd3,0,SEEK_SET));
        assert(sizeof(DOWNFLAG) == write(fd3,&DOWNFLAG,sizeof(DOWNFLAG)));
        CHK(lseek(fd4,0,SEEK_SET));
        assert(sizeof(newCount) == write(fd4,&newCount,sizeof(newCount)));
        // end critical section
        // release critical section
        CHK(sem_post(pmutx));
    }
    // calls the finish function once the last widget was placed
    if (last) finish();
    fflush(stdout);
    return;
}

/* If you feel the need to create any additional functions, please
 write them below here, with appropriate documentation:
 */
// function to close and unlink
void finish() {
    printf("F\n");
    CHK(close(fd));
    CHK(unlink(COUNTFILE_NAME));
    CHK(close(fd2));
    CHK(unlink(ROWFILE_NAME));
    CHK(close(fd3));
    CHK(unlink(DOWNFLAG_NAME));
    CHK(close(fd4));
    CHK(unlink(NEWCOUNT_NAME));
    CHK(sem_close(pmutx));
    CHK(sem_unlink(semaphoreMutx));
}
// function to check which row will be the longest
int isPerfectSquare(int number){
    int iVar;
    float fVar;
    
    fVar=sqrt((double)number);
    iVar=fVar;
    
    if(iVar==fVar){
        return 1;
    } else{
        return 0;
    }
}
