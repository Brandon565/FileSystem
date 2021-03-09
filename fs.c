
// return the index of data block where offset of @fd is
int offsetBlock(int fd) {
	// find start data block for file
	uint16_t prevBlock = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp(openFiles[fd].name, root[i].fileName)) {
			prevBlock = root[i].dataBlockBegin;
			break;
		}
	}

	// if no blocks assigned, offset is not within a block, return -1
	if (prevBlock == FAT_EOC || prevBlock == 0) {
		return -1;
	}

	int count = 0;

	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE * sizeof(uint16_t));
	// read the fat block where prevBlock value is
	block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
	// next block is the values at prevBlock location in fat
	int nextBlock = fatBuf[prevBlock%FAT_MAX_SIZE];
	count += BLOCK_MAX_BYTES;

	// Count tracks the bytes we have already passed in the file, each block is 4096.
	// Once we have passed the offset, the loop terminates and returns the last block
	// where offset must have been since we just passed it.
	while(count <= openFiles[fd].offset) {
    	// if there isnt a next data block
		if (nextBlock == FAT_EOC) {
			return -1;
		}

		// read the fat block where nextBlock index is
		block_read(1 + floor(nextBlock/FAT_MAX_SIZE), fatBuf);

		// prevBlock = nextBlock , nextBlock = value at nextBlock in fat
		prevBlock = nextBlock;
		nextBlock = fatBuf[nextBlock%FAT_MAX_SIZE];
		// add 4096 because we passed another block
		count += BLOCK_MAX_BYTES;
	}

	if (prevBlock == FAT_EOC || prevBlock == 0) {
		return -1;
	}

	return prevBlock;

}

// Purpose: set empty fat spot to FAT_EOC and return its index
// if no fat left empty return -1
int find_fat_block()
{
	int count = 0;
	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE* sizeof(uint16_t));

	// outer loop for each fat block
	// inner loop through each fat entry
	for (int i = 0; i < 1 + floor(mounted->dataBlockAmt/FAT_MAX_SIZE); i++) {
		block_read(1 + i, fatBuf);
		for (int j = 0; j < FAT_MAX_SIZE; j++) {
			count++;
			if (fatBuf[j] == 0) {
				// if the fat spot is empty, set to fat eoc, write, return index
				fatBuf[j] = FAT_EOC;
				block_write(1 + i, fatBuf);
				return  i * FAT_MAX_SIZE + j;
			}
			if (count == mounted->dataBlockAmt) {
				// ran out of blocks
				return -1;
			}
		}
	}

	return -1;
}


int fs_write(int fd, void *buf, size_t count)
{
	// used to calculate file size change at end
	int sizeInc = openFiles[fd].offset;

	// BEGIN ERROR HANDLING
	if(openFiles[fd].offset == -1){
		return -1;
	}

	if (buf == NULL ||  openFile == 0 || fd > 31 || fdArray[fd] == 0) {
		return -1;
	}

	if ((int)count == 0) {
		return 0;
	}
	// END ERROR HANDLING

	// find the index of file in root directory
	int rootIndex = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (!strcmp(root[i].fileName, openFiles[fd].name)) {
			rootIndex = i;
			break;
		}
	}


	uint16_t * fatBuf = (uint16_t* )malloc(FAT_MAX_SIZE * sizeof(uint16_t));
	int prevBlock = root[rootIndex].dataBlockBegin;

	int bytesWrote = 0;
	// curr block = index of data block for current offset
	int currBlock = offsetBlock(fd);


	// if there is no data block assigned
	if (currBlock == -1) {
		currBlock = find_fat_block();

		// if there are no free fat spots, cannot write, return 0
		if (currBlock == -1) {
			return bytesWrote;
		}

		if (root[rootIndex].dataBlockBegin == FAT_EOC) {
			// if this is the first data block of file, update root dir
			root[rootIndex].dataBlockBegin = currBlock;
			block_write(mounted->rootDirectory, root);
		} else {
			block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
			// nextBlock = values at prevBlock index in fat
			int nextBlock = fatBuf[prevBlock%FAT_MAX_SIZE];

			// while we are not at the last location in file's fat chain
			while(fatBuf[prevBlock%FAT_MAX_SIZE] != 0 && fatBuf[prevBlock%FAT_MAX_SIZE] != FAT_EOC) {
				// continue moving forward through fat chain
				block_read(1 + floor(nextBlock/FAT_MAX_SIZE), fatBuf);
				prevBlock = nextBlock;
				nextBlock = fatBuf[nextBlock%FAT_MAX_SIZE];
			}

			// set the previous last location to point to the new data block we allocated
			fatBuf[prevBlock%FAT_MAX_SIZE] = currBlock;
			block_write(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
		}
		// set fat buf to be where currBlock is, last link in chain
		prevBlock = currBlock;
		block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
	}

	// currOffset = offset within a block, rather than full file
	int currOffset = openFiles[fd].offset % BLOCK_MAX_BYTES;
	uint8_t * blockBuf = (uint8_t* )malloc(BLOCK_MAX_BYTES* sizeof(uint8_t));

	while (bytesWrote < (int)count) {
		// read  the block to write next
		block_read(currBlock + mounted->dataBlock, blockBuf);

		if ((int) count >= bytesWrote + BLOCK_MAX_BYTES - currOffset) {
			// if we can write the entirety of the remaining block
			// Memcpy: write to blockBuf starting at offset,
			// 	from buf at the next byte we havent used yet,
			// 	for 4096 bytes - currOffset
			memcpy(blockBuf + currOffset, buf + bytesWrote, BLOCK_MAX_BYTES - currOffset);
			bytesWrote += BLOCK_MAX_BYTES - currOffset;
			// move the offset to the beginning of the next block
			openFiles[fd].offset += BLOCK_MAX_BYTES - currOffset;
		} else {
			// if this is the last block to write
			// Memcpy: write to blockBuf starting at offset,
			// 	from buf at the next byte we havent used yet,
			// 	for bytesToWrite - bytesAlreadyWritten
			memcpy(blockBuf + currOffset, buf + bytesWrote, count - bytesWrote);
			openFiles[fd].offset += count - bytesWrote;
			bytesWrote += count - bytesWrote;
		}

		// actually write the block
		block_write(currBlock + mounted->dataBlock, blockBuf);

		// find the next data block
		currBlock = offsetBlock(fd);

		// if there is no data block assigned
		if (currBlock == -1 && bytesWrote < (int)count) {
			currBlock = find_fat_block();
			block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);

			// if there are no free fat spots, cannot write
			if (currBlock == -1) {
				// size increase = bytesWrote - (original file size - original offset)
				sizeInc = bytesWrote - (openFiles[fd].fileSize - sizeInc);
				if (sizeInc > 0) {
					// if the size of the file increased, change root and fd
					// THIS SHOULD HAVE BEEN bytesWrote > sizeInc for both lines below
					root[rootIndex].fileSize += bytesWrote;
					block_write(mounted->rootDirectory, root);
					openFiles[fd].fileSize += bytesWrote;
				}
				return bytesWrote;
			}

			// add currBlock pointer to previous FAT_EOC spot for file
			fatBuf[prevBlock%FAT_MAX_SIZE] = currBlock;
			block_write(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
			prevBlock = currBlock;
			block_read(1 + floor(prevBlock/FAT_MAX_SIZE), fatBuf);
		}

		// offset into next block in loop is 0, only first loop has offset mid block
		currOffset = 0;
	}

	// all the same shit as 20 lines above
	sizeInc = bytesWrote - (openFiles[fd].fileSize - sizeInc);
	if (sizeInc > 0) {
		root[rootIndex].fileSize += bytesWrote;
		block_write(mounted->rootDirectory, root);
		openFiles[fd].fileSize += bytesWrote;
	}

	return bytesWrote;
}
