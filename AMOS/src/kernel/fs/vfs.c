/*
 *     AAA    M M    OOO    SSSS
 *    A   A  M M M  O   O  S 
 *    AAAAA  M M M  O   O   SSS
 *    A   A  M   M  O   O      S
 *    A   A  M   M   OOO   SSSS 
 *
 *    Author:  Stephen Fewer
 *    Contact: steve [AT] harmonysecurity [DOT] com
 *    Web:     http://amos.harmonysecurity.com/
 *    License: GNU General Public License (GPL)
 */

#include <kernel/fs/vfs.h>
#include <kernel/fs/fat.h>
#include <kernel/mm/mm.h>
#include <kernel/lib/string.h>
#include <kernel/kprintf.h>

struct VFS_FILESYSTEM * vfs_fsTop = NULL;
struct VFS_FILESYSTEM * vfs_fsBottom = NULL;

struct VFS_MOUNTPOINT * vfs_mpTop = NULL;
struct VFS_MOUNTPOINT * vfs_mpBottom = NULL;

// register a new file system with the VFS
int vfs_register( struct VFS_FILESYSTEM * fs )
{
	// add the new file system to a linked list of file system
	// drivers present in the system
	if( vfs_fsBottom == NULL )
		vfs_fsBottom = fs;
	else
		vfs_fsTop->next = fs;
	vfs_fsTop = fs;
	// we can now mount volumes of this file system type
	return VFS_SUCCESS;
}

// unregister a file system driver from the VFS
int vfs_unregister( int fstype )
{
	return VFS_FAIL;
}

// find a file system driver of specified type
struct VFS_FILESYSTEM * vfs_find( int fstype )
{
	struct VFS_FILESYSTEM * fs;
	// search through the linked list of file system drivers
	for( fs=vfs_fsBottom ; fs!=NULL ; fs=fs->next )
	{
		// if we have a match we break from the search
		if( fs->fstype == fstype )
			break;
	}
	// return the fs driver to caller, or NULL if failed to find
	return fs;
}

int vfs_mount( char * device, char * mountpoint, int fstype )
{
	struct VFS_MOUNTPOINT * mount;
	// create our mountpoint structure
	mount = (struct VFS_MOUNTPOINT *)mm_malloc( sizeof(struct VFS_MOUNTPOINT) );
	// find the correct file system driver for the mount command
	mount->fs = vfs_find( fstype );
	if( mount->fs == NULL )
	{
		// failed to find it
		mm_free( mount );
		return VFS_FAIL;	
	}
	mount->mountpoint = (char *)mm_malloc( strlen(mountpoint)+1 );
	strcpy( mount->mountpoint, mountpoint );
	mount->device = (char *)mm_malloc( strlen(device)+1 );
	strcpy( mount->device, device );	
	// add the fs and the mountpoint to a linked list
	if( vfs_mpBottom == NULL )
		vfs_mpBottom = mount;
	else
		vfs_mpTop->next = mount;
	vfs_mpTop = mount;
	// call the file system driver to mount
	if( mount->fs->calltable.mount == NULL )
		return VFS_FAIL;
	return mount->fs->calltable.mount( device, mountpoint, fstype );
}

int vfs_unmount( char * mountpoint )
{
	struct VFS_MOUNTPOINT * mount, * m;
	// find the mountpoint
	for( mount=vfs_mpBottom ; mount!=NULL ; mount=mount->next )
	{
		// if we have a match we break from the search
		if( strcmp( mount->mountpoint, mountpoint ) == 0 )
			break;
	}
	// fail if we cant find it
	if( mount == NULL )
		return VFS_FAIL;
	// call the file system driver to unmount
	if( mount->fs->calltable.unmount == NULL )
		return VFS_FAIL;
	mount->fs->calltable.unmount( mountpoint );
	// remove the mount point from the VFS
	if( mount == vfs_mpBottom )
	{
		vfs_mpBottom = mount->next;
	}
	else
	{
		// search through the linked list of file system drivers
		for( m=vfs_mpBottom ; m!=NULL ; m=m->next )
		{
			// if we have a match we break from the search
			if( m->next == mount )
			{
				// remove the item
				m->next = mount->next;
				break;
			}
		}
	}
	// free the mount structure
	mm_free( mount->mountpoint );
	mm_free( mount->device );
	mm_free( mount );
	return VFS_SUCCESS;	
}

struct VFS_MOUNTPOINT * vfs_file2mountpoint( char * filename )
{
	struct VFS_MOUNTPOINT * mount;
	// find the mountpoint
	for( mount=vfs_mpBottom ; mount!=NULL ; mount=mount->next )
	{
		// if we have a match we break from the search. we use strncmp instead of
		// strcmp to avoid comparing the null char at the end of the mountpoint
		if( strncmp( mount->mountpoint, filename, strlen(mount->mountpoint) ) == 0 )
			break;
	}
	// return the mountpoint
	return mount;
}



struct VFS_HANDLE * vfs_open( char * filename, int mode )
{
	struct VFS_HANDLE * handle;
	struct VFS_MOUNTPOINT * mount;
	// find the correct mountpoint for this file
	mount = vfs_file2mountpoint( filename );
	if( mount == NULL )
		return NULL;
	// advance the filname past the mount point
	filename = (char *)( filename + strlen(mount->mountpoint) );
	// call the file system driver to open
	if( mount->fs->calltable.open == NULL )
		return NULL;
	// create the new virtual file handle	
	handle = (struct VFS_HANDLE *)mm_malloc( sizeof(struct VFS_HANDLE) );
	handle->mount = mount;
	handle->mode = mode;
	// try to open the file on the mounted file system
	if( mount->fs->calltable.open( handle, filename ) != NULL )
	{	
		// set the file position to the end of the file if in append mode
		if( (handle->mode & VFS_MODE_APPEND) == VFS_MODE_APPEND )
			vfs_seek( handle, 0, VFS_SEEK_END );
		return handle;
	}
	else
	{
		// if we fail to open the file but are in create mode, we can create the file
		//     TO-DO: test the failed open() result for a value like FILE_NOT_FOUND
		//     otherwise we could end up in an infinite recurive loop
		if( (handle->mode & VFS_MODE_CREATE) == VFS_MODE_CREATE )
		{	
			if( mount->fs->calltable.create != NULL )
			{
				// try to create it
				if( mount->fs->calltable.create( filename, 0 ) != VFS_FAIL )
				{
					if( mount->fs->calltable.open( handle, filename ) != NULL )
						return VFS_SUCCESS;
				}
			}
		}		
	}
	// if we fail, free the handle and return NULL
	mm_free( handle );
	return NULL;
}

int vfs_close( struct VFS_HANDLE * handle )
{
	if( handle->mount->fs->calltable.close != NULL )
	{
		int ret;
		ret = handle->mount->fs->calltable.close( handle );
		mm_free( handle );
		return ret;
	}
	return VFS_FAIL;	
}

int vfs_read( struct VFS_HANDLE * handle, BYTE * buffer, DWORD size  )
{
	// test if the file has been opened in read mode first
	if( (handle->mode & VFS_MODE_READ) != VFS_MODE_READ )
		return VFS_FAIL;
	// try to call the file system driver to read
	if( handle->mount->fs->calltable.read != NULL )
		return handle->mount->fs->calltable.read( handle, buffer, size  );
	// if we get here we have failed
	return VFS_FAIL;
}

int vfs_write( struct VFS_HANDLE * handle, BYTE * buffer, DWORD size )
{
	int ret;
	// test if the file ha been opened in read mode first
	if( (handle->mode & VFS_MODE_WRITE) != VFS_MODE_WRITE )
		return VFS_FAIL;
	// try to call the file system driver to write
	if( handle->mount->fs->calltable.write != NULL )
	{
		ret = handle->mount->fs->calltable.write( handle, buffer, size );
		if( ret != VFS_FAIL )
		{
			// set the file position to the end of the file if in append mode
			if( (handle->mode & VFS_MODE_APPEND) == VFS_MODE_APPEND )
				vfs_seek( handle, 0, VFS_SEEK_END );
			// return the write result
			return ret;
		}
	}
	// if we get here we have failed
	return VFS_FAIL;
}

int vfs_seek( struct VFS_HANDLE * handle, DWORD offset, BYTE origin )
{
	if( handle->mount->fs->calltable.seek != NULL )
		return handle->mount->fs->calltable.seek( handle, offset, origin  );
	return VFS_FAIL;
}

int vfs_control( struct VFS_HANDLE * handle, DWORD request, DWORD arg )
{
	if( handle->mount->fs->calltable.control != NULL )
		return handle->mount->fs->calltable.control( handle, request, arg  );
	return VFS_FAIL;
}

int vfs_create( char * filename, int mode )
{
	struct VFS_MOUNTPOINT * mount;
	// find the correct mountpoint for this file
	mount = vfs_file2mountpoint( filename );
	if( mount == NULL )
		return VFS_FAIL;
	// advance the filname past the mount point
	filename = (char *)( filename + strlen(mount->mountpoint) );
	// try to create the file on the mounted file system
	if( mount->fs->calltable.create != NULL )
		return mount->fs->calltable.create( filename, mode );
	// return fail
	return VFS_FAIL;	
}

int vfs_delete( char * filename )
{
	struct VFS_MOUNTPOINT * mount;
	// find the correct mountpoint for this file
	mount = vfs_file2mountpoint( filename );
	if( mount == NULL )
		return VFS_FAIL;
	// advance the filname past the mount point
	filename = (char *)( filename + strlen(mount->mountpoint) );
	// try to delete the file on the mounted file system
	if( mount->fs->calltable.delete != NULL )
		return mount->fs->calltable.delete( filename );
	// return fail
	return VFS_FAIL;		
}

int vfs_rename( char * src, char * dest )
{
	struct VFS_MOUNTPOINT * mount;
	// find the correct mountpoint for this file
	mount = vfs_file2mountpoint( src );
	if( mount == NULL )
		return VFS_FAIL;
	// advance the filnames past the mount point, we should sanity check this better
	src = (char *)( src + strlen(mount->mountpoint) );
	dest = (char *)( dest + strlen(mount->mountpoint) );
	// try to rename the file on the mounted file system
	if( mount->fs->calltable.rename != NULL )
		return mount->fs->calltable.rename( src, dest );
	// return fail
	return VFS_FAIL;
}

int vfs_copy( char * src, char * dest )
{
	struct VFS_MOUNTPOINT * mount;
	// find the correct mountpoint for this file
	mount = vfs_file2mountpoint( src );
	if( mount == NULL )
		return VFS_FAIL;
	// advance the filnames past the mount point, we should sanity check this better
	// also dest may be on another mountpoint
	src = (char *)( src + strlen(mount->mountpoint) );
	dest = (char *)( dest + strlen(mount->mountpoint) );
	// try to copy the file on the mounted file system
	if( mount->fs->calltable.copy != NULL )
		return mount->fs->calltable.copy( src, dest );
	// return fail
	return VFS_FAIL;	
}

struct VFS_DIRLIST_ENTRY * vfs_list( char * dir )
{
	struct VFS_MOUNTPOINT * mount;
	// sanity check that this is a dir and not a file
	if( dir[ strlen(dir)-1 ] != '/' )
		return NULL;
	// find the correct mountpoint for this dir
	mount = vfs_file2mountpoint( dir );
	if( mount == NULL )
		return NULL;
	// advance the dir name past the mount point
	dir = (char *)( dir + strlen(mount->mountpoint) );
	// try to list the dir on the mounted file system
	if( mount->fs->calltable.list != NULL )
	{
		struct VFS_DIRLIST_ENTRY * entry;
		entry = mount->fs->calltable.list( dir );
		/*
		// to-do: add any virtual mount points
		//        add in a ".." 
		struct VFS_MOUNTPOINT * mount;
		// find the mountpoint
		for( mount=vfs_mpBottom ; mount!=NULL ; mount=mount->next )
		{
			if( strncmp( mount->mountpoint, dir, strlen(mount->mountpoint) ) == 0 )
			{
				int i,c;
				for(  i=0, c=0;i<strlen(dir);i++)
				{
					if( dir[i] == '/' )
						c++;
				}
				if( c <= 2 )
					kprintf("vfs: mountpoint %s\n", mount->mountpoint );
			}
		}*/		
		
		// to-do: maby sort the list...
		
		return entry;
	}
	// return fail
	return NULL;
}

int vfs_init()
{
	// initilize Device File System driver
	dfs_init();
	// mount the DFS
	vfs_mount( NULL, "/device/", DFS_TYPE );

	// initilize FAT File System driver
	fat_init();

	return VFS_SUCCESS;
}
