# README-Summary

## 内存分配方法的描述、选择与讨论

在本次大作业中，一共可以使用的内存分配方法共有4种，他们分别是：首次匹配、下次匹配、最优匹配、最差匹配。
首次匹配指：找到第一个能够满足内存需求的内存块，将其分配。
下次匹配指：在首次匹配的基础上，将其每一次开始查找的位置改变为上一次分配的内存空间，从而使得查找能够分布于全空间。
最优匹配指：找到内存块中能够满足内存需求且最小的内存块，将其分配。
最差匹配指：找到内存块中能够满足内存需求且最大的内存块，将其分配。
据教材描述，下次匹配是对首次匹配的优化，那么首次匹配首先被淘汰。同时最优匹配的表现在大多数情况下都优于最差匹配。其实，从逻辑推导也可以知道，如果每一次都切割较大的内存块，那么剩余的内存块普遍都较小，也就不能够容纳可能到来的一个较大的内存需求。因此范围被限定在下次匹配和最优匹配之中。同时，由于本项目使用的进程数较少，其复杂性并不大，遍历内存块链表消耗的时间不长，最优匹配的缺点能够得到部分补偿。而下次匹配的快速优势被相对抑制。故我选择的内存分配算法是最优匹配。

## HOST调度器的结构

调度器第一层结构是输入队列，该队列存储着从文件中读取的各进程。

第二层结构是实时进程队列和用户进程队列。从输入队列中读取的合法进程按优先级分别进入这两个队列。

第三层结构是feedback队列，该队列又由3个优先级队列组成。对于其中的第1、2级队列，如果其中进程执行完一个时间片后没能完成，其会退出该队列，进入下一级队列。对于第3级队列，其使用轮转算法，直至进程完成。

## 程序框架和各个模块

HOST程序在运行时是靠两个独立的程序共同支持而得到的。其中一个程序由sigtrap.c编译而来，另一个程序由hostd.c和其配套的pcb.c mab.c rsrc.c编译而来。sigtrap.c这个程序主要用于捕获某些信号，同时进行计时、指定颜色等操作，而hostd.c程序及其配套文件则是HOST的主体。
pcb.c/pcb.h:这个头文件及其C文件主要定义了进程这个结构体，同时定义并实现了对于进程(队列）的各个操作。
包括了进程创建、开始、暂停、终止、入队、出队、打印等。
mab.c/mab.h：对于这个头文件，其主要定义了内存这个结构体及队列（指针），同时实现了针对内存的各个操作
如：内存检查、内存分割、内存释放、内存合并等。
rsrc.c/rsrc.h：对于这个文件，其主要定义了资源这个结构体和对其的操作。例如资源检查（是否充足）、资源分配、
资源释放等操作。
而在hostd.c这个文件中，我们主要利用了上述各个模块所提供的操作，用一个大循环和时间计数实现了类似与进程实际
时间中的分配的一个过程。在hostd.c这个文件中，我们会按照各个条件执行某些操作，包括从实际文件中读取进程进入
输入队列，从输入队列讲进程分队，选择进程同时执行、打印信息。

## 进程调度、内存分配、资源分配等策略的总结与讨论

#### 进程调度

对于本HOST程序的进程调度策略，我认为大致上较为符合真实的调度情况。但是，我认为在某些小细节上能够有所改进。比如，我认为在用户进程入队时，可以有所改进。现在我们采取的方式是如果用户进程队列的头部进程内存、资源要求不能被满足，就一直等待。实际上，当这种情况发生时可以遍历队列，查询队列中的每一个进程，看其内存等需求是否满足，一旦满足，就可以让其入队同时分配资源。这个改进，可以使得CPU的使用效率进一步提升。此外，在本次HOST程序中，我们考虑了IO资源，但是没有考虑进程可能会存在的IO交互时间，如果将此编入程序，将使得该程序进一步符合现实。最后，由于我们采用的文件中进程数较少，而生活中的实际情况是进程数很多，只要电脑在使用，几乎就在运行进程。我们的简化使得我们忽视了一个严重的问题：饥饿。如果有很多个实时进程（最高优先级进程）不断的进入我们的调度器，那么所有用户程序都不能运行，这将会导致饥饿。而解决办法就是每隔一段时间提升剩余进程的优先级，同时取消实时进程的特权，使其变成一个优先级最高的用户进程，而不是一个凌驾在用户进程之上的特殊进程。

#### 内存分配

由于我们现在采用的内存分配是连续式的，实际上和现代的操作系统经常采用的分页式方法不同，我认为可以采用分页式内存分配，这样会增加我们这个程序的实际性同时提升其面对复杂内存分配时的性能。当然，这样需要很多修正，同时会较大地增加程序复杂度。

#### 资源分配

我认为本次程序使用的资源分配是简单而且实用的。在我们现实生活中，我们实际上只需把程序中使用到的静态资源结构体完全复制同时根据I/O设备的使用情况进行修正即可，不需要太大的改动。



# README-Summary

## Description, Selection and Discussion of Memory Allocation Methods

There are a total of 4 memory allocation methods that can be used in this big assignment, they are: `first match`, `next match`, `optimal match`, and `worst match`.
`First match` means: find the first memory block that can satisfy the memory requirement and allocate it.
`Next match` means: based on the first match, change the location of each search to the last allocated memory space, so that the search can be distributed in the whole space.
`Best match` means: find the smallest memory block that can satisfy the memory requirement and allocate it.
`Worst match` means: find the largest block of memory that meets the memory requirement and allocate it.
According to the textbook description, the `next match` method is an optimisation of the first match.Hence, the `first match` can not be the best approach. Meanwhile the `optimal match` performs better than the `worst match` in most cases. In fact, it is also known from the logical deduction that if larger memory blocks are cut each time, then the remaining memory blocks are generally smaller and will not be able to accommodate a larger memory requirement that may come. Therefore the range is limited to the `next match` and the `optimal match`. At the same time, the disadvantage of `optimal matching` is partially compensated for by the fact that the number of processes used in this project is small, its complexity is not large, and traversing the chained list of memory blocks does not consume much time. The fast advantage of `next matching` is relatively suppressed. Therefore, the best memory allocation algorithm I chose is optimal matching.

## Structure of the HOST scheduler

The first level of structure of the scheduler is the input queue, which stores each process reading from the file.

The second level of structure is the real-time process queue and the user process queue. Legitimate processes read from the input queue go to each of these two queues according to their priority.

The third level of structure is the feedback queue, which in turn consists of three priority queues. For level 1 and 2 queues, if a process fails to complete after a time slice, it exits the queue and enters the next level queue. For level 3 queue, it uses the rotation algorithm until the process completes.

## Program framework and modules

The HOST program is supported at runtime by two separate programs working together. One program is compiled from `sigtrap.c` and the other program is compiled from `hostd.c` and its companion `pcb.c` `mab.c` `rsrc.c`. 

### `sigtrap.c`

The program `sigtrap.c` is mainly used to capture certain signals and also to perform operations such as timing, assigning colours, etc., whereas the program `hostd.c` and its companion files are the main body of HOST.

### `pcb.c/pcb.h`

This header file and its C file define the process structure, and define and implement various operations on the process (queue).Including process creation, start, pause, terminate, queue in, queue out, print and so on.

### `mab.c/mab.h`:

For this header file, it mainly defines the memory structure and queues (pointers), and implements various operations on memory.such as memory checking, memory splitting, memory freeing, memory merging, etc.

### `rsrc.c/rsrc.h`

This file defines the resource structure and its operations. For example, resource checking (adequacy), resource allocation,resource allocation, resource release, and so on.

### `hostd.c`

In hostd.c, we make use of the operations provided by the above modules, and use a big loop and time counting to implement a process similar to the process actual allocation in time. In the hostd.c file, we perform certain actions according to various conditions, including reading the process into the input queue from the actual file, reading the process out of the input queue, and reading the process out of the input queue.

## Summary and discussion of strategies for process scheduling, memory allocation, resource allocation, etc.

#### Process Scheduling

For the process scheduling strategy of this HOST application, I think it is generally more in line with the real scheduling situation. However, I think it can be improved in some small details. For example, I think it could be improved when user processes are queued up. Right now we take the approach that if the memory and resource requirements of the process at the head of the user process queue cannot be met, it waits. In fact, when this happens you can traverse the queue and query each process in the queue to see if its memory and other requirements are met, and once they are met, you can let it enter the queue while allocating resources. This improvement can make the CPU usage more efficient. Additionally, in this HOST program, we considered IO resources, but did not consider the IO interaction time that processes may have, which, if coded into the program, would make the program further realistic. Finally, since we used a small number of processes in the file, the reality of life is that the number of processes is large and processes are running almost as long as the computer is in use. Our simplification allows us to ignore a serious problem: starvation. If there are many real-time processes (highest priority processes) constantly entering our scheduler, then all user programs cannot run, which will lead to starvation. And the solution is to raise the priority of the remaining processes every now and then, while removing the privileges of the real-time process, so that it becomes a user process with the highest priority, rather than a special process that overrides the user processes.

#### Memory Allocation

Since the memory allocation we are using now is sequential, which is actually different from the paged approach often used by modern operating systems, I think it would be possible to use paged memory allocation, which would increase the practicability of our program and improve its performance in the face of complex memory allocations. Of course, this would require a lot of fixes and would increase the complexity of the program considerably.

#### Resource Allocation

I think that the resource allocation used in this programme is simple and practical. In our real life, we can actually just copy the static resource structures used in the program and modify them according to the usage of the I/O devices, without much modification.
