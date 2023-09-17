/*******************************************************************

OS Eercises - Project 2 - HOST dispatcher - Exercise 11 and project final

    hostd

        hostd is a four level 'dispatcher' with a Real-Time job queue that
        is dispatched on a first-come-first-served basis and a three level
        feedback dispatcher that deals with the lower priority processes.

        The 'jobs' are read in from a 'dispatch file' specified on the
        command line and the jobs are simulatred by executing the 'process'
        program provided.

        The dispatcher implements memory and i/o resource allocation.

        time resolution is one second (although this can be changed).

        for more details of operarion see the specifications for the second
        project

    usage

        hostd [-mf|-mn|-mb|-mw] <dispatch file>

        where
            <dispatch file> is list of process parameters as specified
                for assignment 2.
            -mx is optional selection of memory allocation algorithm
                -mf First Fit (default)
                -mn Next Fit
                -mb Best Fit
                -mw Worst Fit

    functionality

    1. Initialize  all dispatcher queues;
    2. Initialise memory and resource allocation structures;
    3. Fill dispatcher queue from dispatch list file;
    4. Start dispatcher timer;
    5. While there's anything in any of the queues or there is a currently running process:
        i. Unload any pending processes from the input queue:
           While (head-of-input-queue.arrival-time <= dispatcher timer)
           dequeue process from input queue and enqueue on either:
            a. Real-time queue or
            b. User job queue;
       ii. Unload pending processes from the user job queue:
           While (head-of-user-job-queue.mbytes can be allocated)
            a. dequeue process from user job queue,
            b. allocate memory to the process,
            c. allocate i/o resources to the process, and
            d. enqueue on appropriate priority feedback queue;
      iii. If a process is currently running:
            a. Decrement process remainingcputime;
            b. If times up:
                A. Send SIGINT to the process to terminate it;
                B. Free memory and i/o resources we have allocated to the process (user job only);
                C. Free up process structure memory;
            c. else if it is a user process and other processes are waiting in any of the queues:
                A. Send SIGTSTP to suspend it;
                B. Reduce the priority of the process (if possible) and enqueue it on
                   the appropriate feedback queue;
       iv. If no process currently running && real time queue and feedback queues are not all empty:
            a. Dequeue process from the highest priority queue that is not empty
            b. If already started but suspended, restart it (send SIGCONT to it)
               else start it (fork & exec)
            c. Set it as currently running process;
        v. sleep for one second;
       vi. Increment dispatcher timer;
      vii. Go back to 5.
    6. Exit

********************************************************************

version: 1.4 (exercise 11 and project final)
history:
   v1.0: Original simple FCFS dispatcher (exercise 7)
   v1.1: Simple round-robin dispatcher (exercise 8)
   v1.2: Simple three level feedback dispatcher (exercise 9)
         add CheckQueues fn
   v1.3: Add memory block allocation (exercise 10)
   v1.4: Add resource allocation and merge real-time and feedback
         dispatcher operation (exercise 11 and project final)
*******************************************************************/

#include "hostd.h"

#define VERSION "1.4"

/******************************************************
 
   internal functions
   
 ******************************************************/

int CheckQueues(PcbPtr *);
char * StripPath(char*);
void PrintUsage(FILE *, char *);
void SysErrMsg(char *, char *);
void ErrMsg(char *, char *);
char* InitAnsFile(char *);

/******************************************************

global variables

******************************************************/

Mab  memory = { 0, MEMORY_SIZE, FALSE, NULL, NULL }; // memory arena
Rsrc resources = { MAX_PRINTERS, MAX_SCANNERS, MAX_MODEMS, MAX_CDS };

/******************************************************/

int main(int argc, char* argv[])
{
    char* inputfile = NULL;      // job dispatch file
    FILE* inputliststream;
    PcbPtr inputqueue = NULL;     // input queue buffer
    PcbPtr userjobqueue = NULL;   // arrived processes
    PcbPtr dispatcherqueues[N_QUEUES];  // dispatcher queue array
                  // [0] - real-time, [1]-[3] - feedback
    PcbPtr currentprocess = NULL; // current process
    PcbPtr process = NULL;        // working pcb pointer
    MabPtr rtmemory = memAlloc(&memory, RT_MEMORY_SIZE); // fixed RT memory
    int timer = 0;                // dispatcher timer
    int quantum = QUANTUM;        // current time-slice quantum
    int i;                        // working index

//  0. Parse command line

    i = 0;
    while (++i < argc) {
        if (!strcmp(argv[i], "-mf")) {
            MabAlgorithm = FIRST_FIT;
        }
        else
            if (!strcmp(argv[i], "-mn")) {
                MabAlgorithm = NEXT_FIT;
            }
            else
                if (!strcmp(argv[i], "-mb")) {
                    MabAlgorithm = BEST_FIT;
                }
                else
                    if (!strcmp(argv[i], "-mw")) {
                        MabAlgorithm = WORST_FIT;
                    }
                    else
                        if (!strcmp(argv[i], "-mnr")) {
                            memFree(rtmemory);            // dont preallocate RT memory
                            rtmemory = NULL;
                        }
                        else
                            if (!inputfile) {
                                inputfile = argv[i];
                            }
                            else {
                                PrintUsage(stdout, argv[0]);
                            }
    }
    if (!inputfile) PrintUsage(stdout, argv[0]);
    char* ans_file = InitAnsFile(inputfile);

    //  1. Initialize dispatcher queues (all others already initialised) ;

    for (i = 0; i < N_QUEUES; dispatcherqueues[i++] = NULL);

    //  2. Initialise memory and resource allocation structures
    //     (already done)

    //  3. Fill dispatcher queue from dispatch list file;

    if (!(inputliststream = fopen(inputfile, "r"))) { // open it
        SysErrMsg("could not open dispatch list file:", inputfile);
        exit(2);
    }

    while (!feof(inputliststream)) {  // put processes into input_queue
        process = createnullPcb();
        if (fscanf(inputliststream, "%d, %d, %d, %d, %d, %d, %d, %d",
            &(process->arrivaltime), &(process->priority),
            &(process->remainingcputime), &(process->mbytes),
            &(process->req.printers), &(process->req.scanners),
            &(process->req.modems), &(process->req.cds)) != 8) {
            free(process);
            continue;
        }
        process->status = PCB_INITIALIZED;
        process->ans_file = ans_file;
        inputqueue = enqPcb(inputqueue, process);
    }

    // ==================================================================================================================
    // NOTE: Before implement this, please make sure you have implemented the memory allocation algorithms in mab.c !!! |
    // ==================================================================================================================

    //  4. Start dispatcher timer;
    //     (already set to zero above)

    //  //  5. While there's anything in any of the queues or there is a currently running process:
    while (currentprocess || inputqueue ||userjobqueue|| dispatcherqueues[0] || dispatcherqueues[1] || dispatcherqueues[2]||dispatcherqueues[3])
    {
        //      i. Unload any pending processes from the input queue:
        //         While (head-of-input-queue.arrival-time <= dispatcher timer)
        //         dequeue process from input queue and and enqueue on either
        //           a. Real-time queue so check out parameters before enqueueing
        //           b. user job queue - check out parameters before enqueueing
        //           c. unknown priority

                // TODO


        while (inputqueue && inputqueue->arrivaltime <= timer)          
        {
            if (inputqueue->priority == 0 && inputqueue->mbytes == 64 && inputqueue->req.cds == 0  
                && inputqueue->req.modems == 0 && inputqueue->req.printers == 0   //此处检查该实时进程是否合法
                && inputqueue->req.scanners == 0)
            {
                process = deqPcb(&inputqueue);
                dispatcherqueues[0] = enqPcb(dispatcherqueues[0], process);     //进入队列0
            }
            else if (inputqueue->priority >= 1&&inputqueue->priority<=3 && inputqueue->mbytes <= 1024 && inputqueue->req.cds <= 2
                && inputqueue->req.modems <= 1 && inputqueue->req.printers <= 2
                && inputqueue->req.scanners <= 1)                              //检查用户进程是否合法
            {
                process = deqPcb(&inputqueue);                                 //同上
                userjobqueue = enqPcb(userjobqueue, process);
            }
            else
            {
                process=deqPcb(&inputqueue);
                free(process);
                process=NULL;
                printf("unknown priority\n");                                 //进程不合法，丢弃，打印信息         
            }
        }

        //     ii. Unload pending processes from the user job queue:
        //         While (head-of-user-job-queue.mbytes && resources can be allocated            
        //           a. dequeue process from user job queue
        //           b. allocate memory to the process
        //           c. allocate i/o resources to process
        //           d. enqueue on appropriate feedback queue

                // TODO
        if (userjobqueue)
        {
            if ((rsrcChk(&resources, userjobqueue->req)) && (memChk(&memory, userjobqueue->mbytes))) //先检查资源是否充足
            {
                process = deqPcb(&userjobqueue);  //出队
                process->memoryblock = memAlloc(&memory,process->mbytes);        //分配内存
                rsrcAlloc(&resources, process->req);  //分配IO资源
                dispatcherqueues[process->priority] = enqPcb(dispatcherqueues[process->priority], process);  //进入相应的队列
            }
        }
        if (dispatcherqueues[0])         //此处为每一个实时进程分配内存
        {
            dispatcherqueues[0]->memoryblock=rtmemory;
        }
        //    iii. If a process is currently running;
        //          a. Decrement process remainingcputime;            
        //          b. If times up:                
        //             A. Send SIGINT to the process to terminate it;
        //             B. Free memory and resources we have allocated to the process;
        //             C. Free up process structure memory                
        //         c. else if a user process and other processes are waiting in feedback queues:                
        //             A. Send SIGTSTP to suspend it;
        //             B. Reduce the priority of the process (if possible) and enqueue it on
        //                the appropriate feedback queue;;

                // TODO
        if (currentprocess)
        {
            currentprocess->remainingcputime -= quantum;
            if (currentprocess->remainingcputime <= 0)  
            {
                if (currentprocess->priority==0)       //对于实时进程，不释放内存
                {
                    terminatePcb(currentprocess);
                    free(currentprocess);
                    currentprocess = NULL;
                }
                else                                   //对于用户进程，释放内存、IO资源
                {
                    terminatePcb(currentprocess);
                    memFree(currentprocess->memoryblock);
                    rsrcFree(&resources, currentprocess->req);
                    free(currentprocess);
                    currentprocess = NULL;
                }
            }
            else
            {
                if (i == 0)  //如果当前工作进程在下一个时间中仍会运行，就不暂停该进程，但应该改变工作队列序数（即其对应的序列）
                {
                    ;
                }
                else if (i == 1 && (!dispatcherqueues[0]) && (!dispatcherqueues[1]) && (!dispatcherqueues[2]))
                {
                    i++;
                }
                else if (i == 2 && (!dispatcherqueues[0]) && (!dispatcherqueues[1]) && (!dispatcherqueues[2]) && (!dispatcherqueues[3]))
                {
                    i++;
                }
                else if (i == 3 && (!dispatcherqueues[0]) && (!dispatcherqueues[1]) && (!dispatcherqueues[2]) && (!dispatcherqueues[3]))
                {
                    ;
                }
                else   //否则暂停进程，将其放入相应队列中
                {
                    suspendPcb(currentprocess);
                    if (i == 3)
                    {
                        dispatcherqueues[i] = enqPcb(dispatcherqueues[3], currentprocess);
                        currentprocess = NULL;
                    }
                    else if (i <= 2 && i > 0)              //对于第1、2级队列，其中进程运行完时间片之后应该进入下一级队列
                    {
                        dispatcherqueues[i + 1] = enqPcb(dispatcherqueues[i + 1], currentprocess);
                        currentprocess = NULL;
                    }
                }
            }
        }
        //     iv. If no process currently running &&  queues are not empty:
        //         a. Dequeue process from RR queue            
        //         b. If already started but suspended, restart it (send SIGCONT to it)
        //              else start it (fork & exec)
        //         c. Set it as currently running process;

                // TODO
        if ((!currentprocess) && (dispatcherqueues[0] || dispatcherqueues[1] || dispatcherqueues[2] || dispatcherqueues[3]))
        {
            if (dispatcherqueues[0])    //下述各个情况分别对应应该从哪个队列中拿出进程，如如果队列0非空，就该从这个队列中取进程
            {
                process = deqPcb(&dispatcherqueues[0]);
                startPcb(process);
                currentprocess = process;
                i = 0;
            }
            else if (!dispatcherqueues[0] && dispatcherqueues[1])  //队列0为空，队列1非空，应该从队列1取进程
            {
                process = deqPcb(&dispatcherqueues[1]);
                startPcb(process);
                currentprocess = process;
                i = 1;
            }
            else if ((!dispatcherqueues[0]) && (!dispatcherqueues[1]) && dispatcherqueues[2])  //同上
            {
                process = deqPcb(&dispatcherqueues[2]);
                startPcb(process);
                currentprocess = process;
                i = 2;
            }
            else if (!(dispatcherqueues[0]) && !(dispatcherqueues[1]) && !(dispatcherqueues[2]) && (dispatcherqueues[3]))
            {
                process = deqPcb(&dispatcherqueues[3]);
                startPcb(process);
                currentprocess = process;
                i = 3;
            }
        }


        //       v. sleep for quantum;            

        // TODO
        sleep(quantum);

        //      vi. Increment dispatcher timer;

        // TODO
        timer += quantum;

        //     vii. Go back to 5.
    }

    //    6. Exit

    exit(0);
}


/*******************************************************************

int CheckQueues(PcbPtr * queues)

  check array of dispatcher queues

  return priority of highest non-empty queue
          -1 if all queues are empty
*******************************************************************/
int CheckQueues(PcbPtr * queues)
{
    int n;

    for (n = 0; n < N_QUEUES; n++)
        if (queues[n]) return n;
    return -1;
}

/*******************************************************************

char * StripPath(char * pathname);

  strip path from file name

  pathname - file name, with or without leading path

  returns pointer to file name part of pathname
    if NULL or pathname is a directory ending in a '/'
        returns NULL
*******************************************************************/

char * StripPath(char * pathname)
{
    char * filename = pathname;\

    if (filename && *filename) {           // non-zero length string
        filename = strrchr(filename, '/'); // look for last '/'
        if (filename)                      // found it
            if (*(++filename))             //  AND file name exists
                return filename;
            else
                return NULL;
        else
            return pathname;               // no '/' but non-zero length string
    }                                      // original must be file name only
    return NULL;
}

/*******************************************************
 * print usage
 ******************************************************/
void PrintUsage(FILE * stream, char * progname)
{
    if(!(progname = StripPath(progname))) progname = DEFAULT_NAME;
    
    fprintf(stream,"\n"
"%s process dispatcher ( version " VERSION "); usage:\n\n"
"  %s [-mf|-mn|-mb|-mw] <dispatch file>\n"
" \n"
"  where \n"
"    <dispatch file> is list of process parameters \n"
"    -mx is optional selection of memory allocation algorithm \n"
"      -mf First Fit (default) \n"
"      -mn Next Fit \n"
"      -mb Best Fit \n"
"      -mw Worst Fit \n"
"    -mnr don\'t preallocate real-time memory\n\n",
    progname,progname);
    exit(127);
}
/********************************************************
 * print an error message on stderr
 *******************************************************/

void ErrMsg(char * msg1, char * msg2)
{
    if (msg2)
        fprintf(stderr,"ERROR - %s %s\n", msg1, msg2);
    else
        fprintf(stderr,"ERROR - %s\n", msg1);
    return;
}

/*********************************************************
 * print an error message on stderr followed by system message
 *********************************************************/

void SysErrMsg(char * msg1, char * msg2)
{
    if (msg2)
        fprintf(stderr,"ERROR - %s %s; ", msg1, msg2);
    else
        fprintf(stderr,"ERROR - %s; ", msg1);
    perror(NULL);
    return;
}

/*********************************************************
 * Create answer file and return its file name
 *********************************************************/

char* InitAnsFile(char * inputfile)
{
    char* ans_file = malloc(sizeof(char) * 100);
    strcpy(ans_file + strlen(ans_file), inputfile);
    strcpy(ans_file + strlen(ans_file), ".ans");
    fopen(ans_file, "w");

    return ans_file;
}             
