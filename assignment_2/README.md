# Assignment 2: Adding and testing a new system call to Linux kernel

> **Name:** Bhanuj Gandhi
**Roll no.:** 2022201068
> 

In this assignment, our task was to implement the system calls in the Linux kernel and test then out by compiling the kernel.

# Steps Followed

1. Download the compressed file of *Linux Kernel version 4.19.210*.
2. Extract the downloaded kernel file in */usr/src*.

```bash
    $ sudo tar -xvf linux-4.19.210.tar.xz -C/usr/src
```

![Untitled](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled.png)

1. Go to the extracted folder
2. Implement System Calls
    1. **Question 1. : Write syscall to print a welcome message to Linux logs**
        1. Create a Folder for a new system-call *`hello`*
        2. Create a C file inside it. `*hello.c*`
        3. Implement the system call as shown in `*bhanujhello.c*`
            
            ![Untitled](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%201.png)
            
        4. Create a make file in the `*hello*` folder named `*Makefile`* and include the following line in it.
            
            `obj-y := hello.o`
            
        5. Add function call definition in `*./include/syscall.h*` header file.
        `asmlinkage long sys_bhanujhello(void);`
        6. Add the system call entry in `*syscalls_32.tbl*`
        7. Add folder name to `*Makefile*` of kernel
        `core-y += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ hello/`
    2. **Question 2.: Write syscall which will receive string parameter and print it along with some message to kernel logs**
        1. Create a Folder for a new system-call `*bhanujprint*`
        2. Create a C file inside it. `bhanujprint*.c*`
        3. Implement the system call as shown in `*bhanujprint.c*`
            
            ![Untitled](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%202.png)
            
        4. Create a make file in the `bhanujprint` folder named `*Makefile`* and include the following line in it.
            
            `obj-y := bhanujprint.o`
            
        5. Add the system call entry in *`syscalls_32.tbl`*
        6. Add folder name to `*Makefile*` of kernel
            
            `core-y += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ hello/ bhanujprint/`
            
    3. **Question 3.: Write system call to print the parent process id and current process id upon calling it**
        1. Create a Folder for a new system-call `bhanujprocess`
        2. Create a C file inside it. `bhanujprocess.c`
        3. Implement the system call as shown in `bhanujprocess.c`
            
            ![Untitled](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%203.png)
            
        4. Create a make file in the `bhanujprocess` folder named `Makefile`and include the following line in it.
            
            `obj-y := bhanujprocess.o`
            
        5. Add function call definition in `./include/syscall.h` header file.
        `asmlinkage long sys_bhanujprocess(void);`
        6. Add the system call entry in `*syscalls_32.tbl*`
        7. Add folder name to `Makefile` of kernel
        `core-y += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ hello/ bhanujprint/ bhanujprocess/`
    4. **Question 4: Write system call to execute some predefined system call from your written system call**
        
        *In this question, I have implemented `getppid()` function call which is named as `bhanujgetppid()` which will return parent process’ process id(pid).* 
        
        1. Create a Folder for a new system-call `*bhanujgetppid*`
        2. Create a C file inside it. `*bhanujgetppid.c*`
        3. Implement the system call as shown in `bhanujgetppid.c`
            
            ![Untitled](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%204.png)
            
        4. Create a make file in the `bhanujgetppid` folder named `Makefile`and include the following line in it.
            
            `obj-y := bhanujgetppid.o`
            
        5. Add function call definition in `./include/syscall.h` header file.
        `asmlinkage long sys_bhanujgetppid(void);`
        6. Add the system call entry in `*syscalls_32.tbl*`
        7. Add folder name to `Makefile` of kernel
        `core-y += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ hello/ bhanujprint/ bhanujprocess/ bhanujgetppid/`   
        
        <aside>
        💡 Final `*syscalls_32.tbl*` file will look like:
        *Last 4 entries have my implemented system calls for question 1, question 2, question 3, question 4, respectively*
        
        ![syscalls_32.tbl file snapshot](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%205.png)
        
        syscalls_32.tbl file snapshot
        
        Final `Makefile` in root directory of kernel will look like:
        
        ![Makefile snapshot](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%206.png)
        
        Makefile snapshot
        
        </aside>
        
3.  Compile the Kernel

```bash
# Makefile for menuconfig
$ sudo make menuconfig

# Compiling kernel and kernel modules
$ sudo make -j 4 && sudo make modules_install -j 4

# Installing kernel
$ sudo make install -j 4

# *Where -j n defines the number of cores to be give to the process
# that will run this command*

# This is to update the kernel entries in the grub
$ sudo update-grub
$ sudo shutdown now -r

```

![Execution snapshot of *menuconfig and make command*](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled.jpeg)

Execution snapshot of *menuconfig and make command*

![*Completion of make install command*](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%201.jpeg)

*Completion of make install command*

1. Test the system calls
    1. **Question 1:**
    
    ![Driver Code to Test question 1 sys_bhanujhello system call](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%207.png)
    
    Driver Code to Test question 1 sys_bhanujhello system call
    
    ![Upon running the out file after compiling](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%208.png)
    
    Upon running the out file after compiling
    
    ![Output in dmesg by sys_bhanujhello system call](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%209.png)
    
    Output in dmesg by sys_bhanujhello system call
    
    b. **Question 2:**
    
    ![Driver code for bhanujprint system call](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%2010.png)
    
    Driver code for bhanujprint system call
    
    ![Upon compiling and running out file](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%2011.png)
    
    Upon compiling and running out file
    
    ![Output in dmesg by sys_bhanujprint system call](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%2012.png)
    
    Output in dmesg by sys_bhanujprint system call
    
    c. **Question 3:**
    
    ![Driver code for sys_bhanujprocess system call](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%2013.png)
    
    Driver code for sys_bhanujprocess system call
    
    ![Upon compiling and running out file for question 3 system call](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%2014.png)
    
    Upon compiling and running out file for question 3 system call
    
    ![Output in dmesg by sys_bhanujprocess system call](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%2015.png)
    
    Output in dmesg by sys_bhanujprocess system call
    
    d. **Question 4:**
    
    ![Driver code for sys_bhanujgetppid system call](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%2016.png)
    
    Driver code for sys_bhanujgetppid system call
    
    ![Upon compiling and running the out file, the process outputs the pid of parent of the *./q4.o* process.](Assignment%202%20Adding%20and%20testing%20a%20new%20system%20call%20%2055f937de275b4b4f986420543e54d3ed/Untitled%2017.png)
    
    Upon compiling and running the out file, the process outputs the pid of parent of the *./q4.o* process.