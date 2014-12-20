#ifndef _GRAPHENE_IPC_H
#define _GRAPHENE_IPC_H
#ifdef __linux__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define GIPC_FILE   "/dev/gipc"
#define GIPC_MINOR		240


/* Ioctl codes */
#define GIPC_SEND   _IOW('k', 0, void *)
#define GIPC_RECV   _IOR('k', 1, void *)
#define GIPC_CREATE _IOR('k', 2, void *)
#define GIPC_JOIN   _IOR('k', 3, void *)

// Must be a power of 2!
#define PAGE_BUFS 2048
#define PAGE_BITS (PAGE_BUFS / sizeof(unsigned long))
#define ADDR_ENTS 32

#define PAGE_PRESENT 1

/* Argument Structures */
typedef struct gipc_send {
	unsigned long entries;
	unsigned long *addr;
	unsigned long *len; // Rounded up to PAGE_SIZE
} gipc_send;

typedef struct gipc_recv {
	unsigned long entries;
	unsigned long *addr;
	unsigned long *len; // Rounded up to PAGE_SIZE
	int *prot;
} gipc_recv;

struct file_operations;

#ifdef __KERNEL__
struct gipc_queue {
	struct list_head list;
	s64 token;
	u32 owner;
};
#endif

#endif // _GRAPHENE_IPC_H
