#ifndef LIBNETFILES_H_
#define LIBNETFILES_H_

#define UNRESTRICTED 0

#define EXCLUSIVE 1

#define TRANSACTION 2

int netserverinit(char * hostname , int filemode); //ext. A requires the extra argument

int netopen(const char* pathname , int flags);

ssize_t netread(int fildes , void * buf , size_t nbytes);

ssize_t netwrite(int fildes , const void* buf , size_t nbytes);

int netclose(int fd);

#endif
