#ifndef __FS_H__
#define __FS_H__

#include "fs/minix.h"

int readSuperBlock (SuperBlock *superBlock);
//TODO
int allocInode (SuperBlock *superBlock,
                Inode *fatherInode,
                int fatherInodeOffset,
                Inode *destInode,
                int *destInodeOffset,
                const char *destFilename,
                int destFiletype);
//TODO
int freeInode (SuperBlock *superBlock,
               Inode *fatherInode,
               int fatherInodeOffset,
               Inode *destInode,
               int *destInodeOffset,
               const char *destFilename,
               int destFiletype);

int readInode (SuperBlock *superBlock,
               Inode *destInode,
               int *inodeOffset,
               const char *destFilePath);
//TODO
int allocBlock (SuperBlock *superBlock,
                Inode *inode,
                int inodeOffset);

int readBlock (SuperBlock *superBlock,
               Inode *inode,
               int blockIndex,
               uint8_t *buffer);
//TODO
int writeBlock (SuperBlock *superBlock,
                Inode *inode,
                int blockIndex,
                uint8_t *buffer);
//TODO
int getDirEntry (SuperBlock *superBlock,
                 Inode *inode,
                 int dirIndex,
                 DirEntry *destDirEntry);

void initFS (void);
//TODO
void initFile (void);

//TODO
void ls(char *path, char *tmp, int *len);

#endif /* __FS_H__ */
