#include "fs.h"
#include <cmath>

int INE5412_FS::fs_format()
{
	// TODO: Se o disco já foi montado não pode formatar, retorna falha
	/* if (mounted) {
		return 0;
	} */

	union fs_block block;

	block.super.magic = FS_MAGIC;
	block.super.nblocks = disk->size();
	// 10% dos blocos são reservados para inodes arredondado para cima 
	block.super.ninodeblocks = ceil(block.super.nblocks * 0.1);
	block.super.ninodes = block.super.ninodeblocks * INODES_PER_BLOCK;

	// Escreve o superbloco
	disk->write(0, block.data);

	// Zera o resto do disco
	for (int i = 1; i < block.super.nblocks; i++) {
		for (int j = 0; j < POINTERS_PER_BLOCK; j++) {
			block.pointers[j] = 0;
		}
	}

	return 1;
}

void INE5412_FS::fs_debug()
{
	union fs_block block;

	disk->read(0, block.data);

	cout << "superblock:\n";
	cout << "    " << (block.super.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
 	cout << "    " << block.super.nblocks << " blocks\n";
	cout << "    " << block.super.ninodeblocks << " inode blocks\n";
	cout << "    " << block.super.ninodes << " inodes\n";

	for (int i = 1; i <= block.super.ninodeblocks; i++) {
		disk->read(i, block.data);

		for (int j = 0; j < INODES_PER_BLOCK; j++) {
			if (block.inode[j].isvalid) {
				cout << "inode " << j << ":\n";
				cout << "    size: " << block.inode[j].size << " bytes\n";
				cout << "    direct blocks:";
				for (int k = 0; k < POINTERS_PER_INODE; k++) {
					if (block.inode[j].direct[k] != 0) {
						cout << " " << block.inode[j].direct[k];
					}
				}
				cout << "\n";
				cout << "    indirect block: " << block.inode[j].indirect << "\n";

				if (block.inode[j].indirect != 0) {
					disk->read(block.inode[j].indirect, block.data);
					cout << "    indirect data blocks:";
					for (int k = 0; k < POINTERS_PER_BLOCK; k++) {
						if (block.pointers[k] != 0) {
							cout << " " << block.pointers[k];
						}
					}
					cout << "\n";
				}
			}
		}
	}
}

int INE5412_FS::fs_mount()
{
	/* if (mounted) {
		return 0;
	}
 */
	union fs_block block;

	disk->read(0, block.data);

	if (block.super.magic != FS_MAGIC) return 0;
	if (block.super.nblocks != disk->size()) return 0;
	if (block.super.ninodeblocks != ceil(block.super.nblocks * 0.1)) return 0;
	if (block.super.ninodes != block.super.ninodeblocks * INODES_PER_BLOCK) return 0;
	cout << "before bitmap\n";
	
	std::vector<int> bitmap = std::vector<int>(block.super.nblocks, 0);
	cout << "after bitmap\n";
	cout << bitmap.at(0);
	cout << "after bitmap[0]\n";
	fs_superblock super = block.super;

	// Marca os blocos ocupados pelos inodes como ocupados
	for (int i = 1; i <= super.ninodeblocks; i++) {
		disk->read(i, block.data);
		cout << "inode block " << i << ":\n";
		for (int j = 0; j < INODES_PER_BLOCK; j++) {
			if (block.inode[j].isvalid) {
				bitmap[i] = 1;

				for (int k = 0; k < POINTERS_PER_INODE; k++) {
					if (block.inode[j].direct[k] >= super.nblocks) {
						return 0;
					} else if (block.inode[j].direct[k] > 0) {
						bitmap[block.inode[j].direct[k]] = 1;
					}
				}

				if (block.inode[j].indirect >= super.nblocks) {
					return 0;
				} else if (block.inode[j].indirect > 0) {
					bitmap[block.inode[j].indirect] = 1;

					union fs_block indirect_block;
					disk->read(block.inode[j].indirect, indirect_block.data);

					for (int k = 0; k < POINTERS_PER_BLOCK; k++) {
						if (indirect_block.pointers[k] >= super.nblocks) {
							return 0;
						} else if (indirect_block.pointers[k] > 0) {
							bitmap[indirect_block.pointers[k]] = 1;
						}
					}
				}
			}
		}
	}

	mounted = true;

	return 1;
}

int INE5412_FS::fs_create()
{
	return 0;
}

int INE5412_FS::fs_delete(int inumber)
{
	return 0;
}

int INE5412_FS::fs_getsize(int inumber)
{
	return -1;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	return 0;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	return 0;
}
