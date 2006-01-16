#ifndef _KERNEL_FS_VFS_H_
#define _KERNEL_FS_VFS_H_

#include <sys/types.h>

#define VFS_SUCCESS		-1
#define VFS_FAIL		-1

struct VFS_HANDLE
{
	struct VFS_MOUNTPOINT * mount;
	void * data_ptr;
};

struct VFS_FILESYSTEM_CALLTABLE
{
	struct VFS_HANDLE * (*open)( struct VFS_HANDLE *, char * );
	int (*close)(struct VFS_HANDLE *);
	int (*read)(struct VFS_HANDLE *, BYTE *, DWORD );
	int (*write)(struct VFS_HANDLE *, BYTE *, DWORD );
	int (*seek)(struct VFS_HANDLE *, DWORD, BYTE );
	int (*control)(struct VFS_HANDLE *, DWORD, DWORD );
	int (*create)( char *, int );
	int (*delete)( char * );
	int (*rename)( char *, char * );
	int (*copy)( char *, char * );
	struct VFS_DIRLIST_ENTRY * (*list)( char * );
	int (*mount)( char *, char *, int );
	int (*unmount)( char * );
};

struct VFS_FILESYSTEM
{
	int fstype;
	struct VFS_FILESYSTEM_CALLTABLE calltable;
	struct VFS_FILESYSTEM * next;
};

struct VFS_MOUNTPOINT
{
	struct VFS_FILESYSTEM * fs;
	char * mountpoint;
	char * device;
	struct VFS_MOUNTPOINT * next;
};

struct VFS_DIRLIST_ENTRY
{
	char name[32];	
};

int vfs_init();

int vfs_register( struct VFS_FILESYSTEM * );

int vfs_unregister( int );

// volume operations
int vfs_mount( char *, char *, int );

int vfs_unmount( char * );

// file operations
struct VFS_HANDLE * vfs_open( char * );

int vfs_close( struct VFS_HANDLE * );

int vfs_read( struct VFS_HANDLE *, BYTE *, DWORD );

int vfs_write( struct VFS_HANDLE *, BYTE *, DWORD );

int vfs_seek( struct VFS_HANDLE *, DWORD, BYTE );

int vfs_control( struct VFS_HANDLE *, DWORD, DWORD );

// fs operations
int vfs_create( char *, int );

int vfs_delete( char * );

int vfs_rename( char *, char * );

int vfs_copy( char *, char * );

struct VFS_DIRLIST_ENTRY * vfs_list( char * );

#endif