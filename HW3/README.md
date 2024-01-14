# Assignment 3: System Information Fetching Kernel Module

## Getting Started

### 0. Compile the kernel module

```shell
$ make
```

### 1. Load the kernel module into the system

```shell
$ make load
```

### 2. Check the module has been loaded into the system

```shell
$ lsmod | grep "kfetch"
```

![Alt text](./Reference%20Image/check.png)

### 3. Test the Program

```shell
$ cc kfetch.c -o kfetch
$ sudo ./kfetch
```

![Alt text](./Reference%20Image/Info.png)

### 4. Program Usage

```shell
Usage:
        sudo ./kfetch [options]
Options:
        -a  Show all information
        -c  Show CPU model name
        -m  Show memory information
        -n  Show the number of CPU cores
        -p  Show the number of processes
        -r  Show the kernel release information
        -u  Show how long the system has been running
```

### 5. Unload the kernel module

```shell
$ make unload
```
