#include "x86.h"
#include "device.h"
#include "fs.h"

int readSuperBlock(SuperBlock *superBlock) {
    diskRead((void*)superBlock, sizeof(SuperBlock), 1, 0);
    return 0;
}

int readBlock (SuperBlock *superBlock, Inode *inode, int blockIndex, uint8_t *buffer) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];
    
    if (blockIndex < bound0) {
        diskRead((void*)buffer, sizeof(uint8_t), superBlock->blockSize, inode->pointer[blockIndex] * SECTOR_SIZE);
        return 0;
    }
    else if (blockIndex < bound1) {
        diskRead((void*)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
        diskRead((void*)buffer, sizeof(uint8_t), superBlock->blockSize, singlyPointerBuffer[blockIndex - bound0] * SECTOR_SIZE);
        return 0;
    }
    else {
        return -1;
    }
}

int readInode(SuperBlock *superBlock, Inode *destInode, int *inodeOffset, const char *destFilePath) {
    int i = 0;
    int j = 0;
    int ret = 0;
    int cond = 0;
    *inodeOffset = 0;
    uint8_t buffer[superBlock->blockSize];
    DirEntry *dirEntry = NULL;
    int count = 0;
    int size = 0;
    int blockCount = 0;

    if (destFilePath == NULL || destFilePath[count] == 0) {
        return -1;
    }
    ret = stringChr(destFilePath, '/', &size);
    if (ret == -1 || size != 0) {
        return -1;
    }
    count += 1;
    *inodeOffset = superBlock->inodeTable * SECTOR_SIZE;
    diskRead((void *)destInode, sizeof(Inode), 1, *inodeOffset);

    while (destFilePath[count] != 0) {
        ret = stringChr(destFilePath + count, '/', &size);
        if(ret == 0 && size == 0) {
            return -1;
        }
        if (ret == -1) {
            cond = 1;
        }
        else if (destInode->type == REGULAR_TYPE) {
            return -1;
        }
        blockCount = destInode->blockCount;
        for (i = 0; i < blockCount; i ++) {
            ret = readBlock(superBlock, destInode, i, buffer);
            if (ret == -1) {
                return -1;
            }
            dirEntry = (DirEntry *)buffer;
            for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j ++) {
                if (dirEntry[j].inode == 0) {
                    continue;
                }
                else if (stringCmp(dirEntry[j].name, destFilePath + count, size) == 0) {
                    *inodeOffset = superBlock->inodeTable * SECTOR_SIZE + (dirEntry[j].inode - 1) * sizeof(Inode);
                    diskRead((void *)destInode, sizeof(Inode), 1, *inodeOffset);
                    break;
                }
            }
            if (j < superBlock->blockSize / sizeof(DirEntry)) {
                break;
            }
        }
        if (i < blockCount) {
            if (cond == 0) {
                count += (size + 1);
            }
            else {
                return 0;
            }
        }
        else {
            return -1;
        }
    }
    return 0;
}

int freeInode (SuperBlock *superBlock,
               Inode *fatherInode,
               int fatherInodeOffset,
               Inode *destInode,
               int *destInodeOffset,
               const char *destFilename,
               int destFiletype)
{
    return 0;
}

int getDirEntry (SuperBlock *superBlock,
                 Inode *inode,
                 int dirIndex,
                 DirEntry *destDirEntry)
{
    int i = 0;
    int j = 0;
    int ret = 0;
    int dirCount = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];
    for (i = 0; i < inode->blockCount; i++) {
        ret = readBlock(superBlock, inode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j ++) {
            if (dirEntry[j].inode != 0) {
                if (dirCount == dirIndex)
                    break;
                else
                    dirCount ++;
            }
        }
        if (j < superBlock->blockSize / sizeof(DirEntry))
            break;
    }
    if (i == inode->blockCount)
        return -1;
    else {
        destDirEntry->inode = dirEntry[j].inode;
        stringCpy(dirEntry[j].name, destDirEntry->name, NAME_LENGTH);
        return 0;
    }
}

void ls(char *path, char *tmp, int *len){
    Inode inode;
    int inodeOffset = 0;
    int dirIndex = 0;
    DirEntry dirEntry;
    SuperBlock sb;
    int ret=0;
    ret = readSuperBlock(&sb);
    if(ret == -1){
        return;
    }
    ret = readInode(&sb, &inode, &inodeOffset, path);
    if(ret == -1){
        return;
    }
    if(inode.type != DIRECTORY_TYPE){
        return;
    }
    int i = 0;
    while (getDirEntry(&sb, &inode, dirIndex, &dirEntry) == 0) {
        dirIndex ++;
        int j = 0;
        while(dirEntry.name[j]!='\0'){
            tmp[i++] = dirEntry.name[j++];
        }
        tmp[i++]=' ';
    }
    tmp[i]='\n';
    *len = i+1;
}