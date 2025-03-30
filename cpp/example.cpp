
/*
 * This is just an example, showing how the Disk class can be used.
 * Use this as a reference, but you'll need to write your own code.
 */

#include <iostream>
#include <cstring>
#include "Disk.h"

using namespace std;

#define DISK_FILE	"disk.img"
#define BLOCK_SIZE	512

int main() {

	Disk * d;
	char block[BLOCK_SIZE];

	// Instantiate a new Disk, backed by the user-supplied file
	d = new Disk(DISK_FILE, BLOCK_SIZE);

	// Write A's to the 0th block on the disk
	memset(block, 'A', BLOCK_SIZE);
	d->writeBlock(0, block);

	// Zero out our temporary memory block, and then read
	// the first block back off the disk.  It should be the
	// data we previously wrote (all A's)
	memset(block, '\0', BLOCK_SIZE);
	d->readBlock(0, block);

	// Print that data out, showing it worked
	cout.write(block, BLOCK_SIZE);

	// Print statistics showing block read/writes
	// Should be one read, one write
	d->printStats();

	delete d;

	return 0;

}
