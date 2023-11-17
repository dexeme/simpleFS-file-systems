#include "fs.h"
#include <cmath>

int INE5412_FS::fs_format()
{
	// TODO: Se o disco já foi montado não pode formatar, retorna falha
	if (mounted) {
		return 0;
	}

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

// A saída não está sendo que nem a do professor, mas não consigo encontrar erro no código!
void INE5412_FS::fs_debug()
{
	union fs_block block;

	disk->read(0, block.data);

	cout << "superblock:\n";
	cout << "    " << (block.super.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
 	cout << "    " << block.super.nblocks << " blocks\n";
	cout << "    " << block.super.ninodeblocks << " inode blocks\n";
	cout << "    " << block.super.ninodes << " inodes\n";

	fs_superblock super = block.super;

	int inode_num = 0;
	for (int i = 1; i <= super.ninodeblocks; i++) {
		disk->read(i, block.data);

		for (int j = 0; j < INODES_PER_BLOCK; j++) {
			if (block.inode[j].isvalid == 1) {
				cout << "inode " << inode_num << ":\n";
				cout << "    size: " << block.inode[j].size << " bytes\n";
				cout << "    direct blocks:";
				for (int k = 0; k < POINTERS_PER_INODE; k++) {
					if (block.inode[j].direct[k] != 0) {
						cout << " " << block.inode[j].direct[k];
					}
				}
				cout << "\n";
				if (block.inode[j].indirect != 0) {
				
					union fs_block indirect_block;

					cout << "    indirect block: " << block.inode[j].indirect << "\n";

					disk->read(block.inode[j].indirect, indirect_block.data);
					cout << "    indirect data blocks:";
					for (int k = 0; k < POINTERS_PER_BLOCK; k++) {
						if (indirect_block.pointers[k] != 0) {
							cout << " " << indirect_block.pointers[k];
						}
					}
					cout << "\n";
				}
			}

			inode_num++;
		}
	}
}

int INE5412_FS::fs_mount()
{
	if (mounted) {
		return 0;
	}

	union fs_block block;

	disk->read(0, block.data);

	if (block.super.magic != FS_MAGIC) return 0;
	if (block.super.nblocks != disk->size()) return 0;
	//if (block.super.ninodeblocks != ceil(block.super.nblocks * 0.1)) return 0; Isso daqui não funciona pros discos 20 e 200 pois eles não seguem a regra dos 10%
	if (block.super.ninodes != block.super.ninodeblocks * INODES_PER_BLOCK) return 0;

	bitmap.resize(block.super.nblocks);

	fs_superblock super = block.super;

	// Marca os blocos ocupados pelos inodes como ocupados
	for (int i = 1; i <= super.ninodeblocks; i++) {
		disk->read(i, block.data);
		for (int j = 0; j < INODES_PER_BLOCK; j++) {
			if (block.inode[j].isvalid == 1) {
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

	// Print bitmap
	for (int i = 0; i < super.nblocks; i++) {
		cout << bitmap[i];
	}
	cout << "\n";

	mounted = true;

	return 1;
}

int INE5412_FS::fs_create()
{
	// Cria um novo inodo de comprimento zero. Em caso de sucesso, retorna o inúmero (positivo). Em
    // caso de falha, retorna zero. (Note que isto implica que zero não pode ser um inúmero válido.)

	if (!mounted) {
		return 0;
	}

	union fs_block block;

	disk->read(0, block.data);

	fs_superblock super = block.super;

	for (int i = 1; i <= super.ninodeblocks; i++) {
		disk->read(i, block.data);

		for (int j = 0; j < INODES_PER_BLOCK; j++) {
			if (block.inode[j].isvalid == 0) {
				block.inode[j].isvalid = 1;
				block.inode[j].size = 0;

				disk->write(i, block.data);

				return (i - 1) * INODES_PER_BLOCK + j + 1;
			}
		}
	}

	return 0;
}

int INE5412_FS::load_inode(int inumber, fs_inode *inode)
{
	if (!mounted) {
		return 0;
	}

	union fs_block block;

	disk->read(0, block.data);

	fs_superblock super = block.super;

	if (inumber < 1 || inumber > super.ninodes) return 0;

	int inode_block = ceil(inumber / INODES_PER_BLOCK) + 1;
	int inode_index = (inumber - 1) % INODES_PER_BLOCK;

	disk->read(inode_block, block.data);

	if (block.inode[inode_index].isvalid == 0) return 0;

	*inode = block.inode[inode_index];

	return 1;
}

int INE5412_FS::save_inode(int inumber, fs_inode *inode) {
	if (!mounted) {
		return 0;
	}

	union fs_block block;

	disk->read(0, block.data);

	fs_superblock super = block.super;

	if (inumber < 1 || inumber > super.ninodes) return 0;

	int inode_block = ceil(inumber / INODES_PER_BLOCK) + 1;
	int inode_index = (inumber - 1) % INODES_PER_BLOCK;

	disk->read(inode_block, block.data);

	if (block.inode[inode_index].isvalid == 0) return 0;

	block.inode[inode_index] = *inode;

	disk->write(inode_block, block.data);

	return 1;
}

int INE5412_FS::fs_delete(int inumber)
{
	// Deleta o inodo indicado pelo inúmero. Libera todo o dado e blocos indiretos atribuı́dos a este
    // inodo e os retorna ao mapa de blocos livres. Em caso de sucesso, retorna um. Em caso de falha, retorna
    // 0.
	
	if (!mounted) {
		return 0;
	}

	union fs_block block;

	disk->read(0, block.data);

	fs_inode inode;

	if (!load_inode(inumber, &inode)) return 0;

	inode.isvalid = 0;
	inode.size = 0;

	for (int i = 0; i < POINTERS_PER_INODE; i++) {
		bitmap[inode.direct[i]] = 0;
		inode.direct[i] = 0;
	}

	if (inode.indirect > 0) {
		bitmap[inode.indirect] = 0;
		inode.indirect = 0;

		union fs_block indirect_block;
		disk->read(inode.indirect, indirect_block.data);

		for (int i = 0; i < POINTERS_PER_BLOCK; i++) {
			bitmap[indirect_block.pointers[i]] = 0;
			indirect_block.pointers[i] = 0;
		}
	}

	save_inode(inumber, &inode);

	return 1;	
}

int INE5412_FS::fs_getsize(int inumber)
{
	// Retorna o tamanho lógico do inodo especificado, em bytes. Note que zero é um tamanho lógico
	// válido para um inodo! Em caso de falha, retorna -1.

	if (!mounted) {
		return -1;
	}

	union fs_block block;

	disk->read(0, block.data);

	fs_inode inode;

	if (!load_inode(inumber, &inode)) return -1;

	return inode.size;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	return 0;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	return 0;
}
