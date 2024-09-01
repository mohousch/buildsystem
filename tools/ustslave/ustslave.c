#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>

#define ST_IOCTL_BASE      'l'
#define STCOP_GRANT      _IOR(ST_IOCTL_BASE, 0, unsigned int)
#define STCOP_RESET      _IOR(ST_IOCTL_BASE, 1, unsigned int)
#define STCOP_START             STCOP_GRANT

#define BUF_SIZE 4096

#define MAX_SECTIONS 100
#define MAX_NAMELEN 40

#define SEC_LOAD 0x01
#define SEC_NOT_LOAD 0x08

/* fixme: Dagobert: we should include <linux/stm/coprocessor.h>
* here but I dont understand the automatic generated makefile here
* ->so use the hacky version here ...
* (Schischu): Actually this makes it easier to implement a stm22 stm23 detection,
* so leave it.
*/
typedef struct
{
	char           name[16];      /* coprocessor name                 */
	u_int          flags;         /* control flags                    */
	/* Coprocessor region:                                            */
	unsigned long   ram_start;    /*   Host effective address         */
	u_int           ram_size;     /*   region size (in bytes)         */
	unsigned long   cp_ram_start; /*   coprocessor effective address  */

} cop_properties_t;

#define STCOP_GET_PROPERTIES   _IOR(ST_IOCTL_BASE, 4, cop_properties_t*)

typedef struct
{

	unsigned int ID;
	char         Name[MAX_NAMELEN];
	unsigned int DestinationAddress;
	unsigned int SourceAddress;
	unsigned int Size;
	unsigned int Alignment;

	unsigned int Flags;
	unsigned int DontKnow2;
} tSecIndex;

tSecIndex IndexTable[MAX_SECTIONS];
int IndexCounter = 0;
int IDCounter = -1;

unsigned int getKernelVersion()
{
	unsigned int version = 24;
	struct utsname name;
	uname(&name);

	if (!strncmp(name.release, "2.6.17", 6))
		version = 22;
	else if (!strncmp(name.release, "2.6.23", 6))
		version = 23;
	else // 2.6.32
		version = 24;

	printf("ustslave: Kernel Version: %d\n", version);

	return version;
}


int writeToSlave(int cpuf, int fd, off_t DestinationAddress, unsigned int SourceAddress, unsigned int Size)
{
	unsigned char *BUFFER = malloc(Size);
	int err;

	err = lseek(fd, SourceAddress, SEEK_SET);
	if (err < 0)
	{
		printf("error seeking SourceAddress\n");
		return 1;
	}

	err = read(fd, BUFFER, Size);
	if (err != Size)
	{
		printf("error read fd\n");
		return 1;
	}

	err = lseek(cpuf, DestinationAddress, SEEK_SET);

	printf("seeking to %x\n", (int)DestinationAddress);

	if (err < 0)
	{
		printf("error seeking copo addi (addi = %x)\n", (int)DestinationAddress);
		return 1;
	}

	err = write(cpuf, BUFFER, Size);
	if (err != Size)
	{
		printf("error write cpuf\n");
		return 1;
	}

	free(BUFFER);

	return 0;
}

int sectionToSlave(int cpuf, int fd, unsigned int *EntryPoint)
{
	int i = 0, err = 0;
	int BootSection = -2;
	int LastSection = -2;
	unsigned long ramStart = 0;
	unsigned char kernelVersion = getKernelVersion();

	if (kernelVersion == 23 || kernelVersion == 24)
	{
		cop_properties_t cop;

		err = ioctl(cpuf, STCOP_GET_PROPERTIES, &cop);
		if (err < 0)
		{
			printf("error ioctl STCOP_GET_PROPERTIES\n");
			return 1;
		}
		printf("base_address 0x%.8lx\n", cop.cp_ram_start);
		ramStart = cop.cp_ram_start;
	}

	for (i = 0; i < IndexCounter; i++)
	{
		if (IndexTable[i].Size > 0 && (IndexTable[i].Flags & (SEC_LOAD == SEC_LOAD)))
		{
			if (0 == strncmp(".boot", IndexTable[i].Name, 5))
			{
				/* defer the loading of the (relocatable) .boot section until we know where to
				 * relocate it to.
				 */
				BootSection = i;

				continue;
			}

			err = writeToSlave(cpuf, fd, IndexTable[i].DestinationAddress - ramStart,
					   IndexTable[i].SourceAddress, IndexTable[i].Size);
			if (err != 0) return 1;

			LastSection = i;
		}
	}

	if (BootSection != -2)
	{
		//Add relocated .boot
		unsigned int Alignment = 8;

		unsigned int DestinationAddress = (IndexTable[LastSection].DestinationAddress + IndexTable[LastSection].Size
						   + (1 << Alignment)) & ~((1 << Alignment) - 1);

		err = writeToSlave(cpuf, fd, DestinationAddress - ramStart,
				   IndexTable[BootSection].SourceAddress, IndexTable[BootSection].Size);
		if (err != 0) return 1;

		*EntryPoint = DestinationAddress;
	}
	else
	{
		//We allready have the EntryPoint
	}

	return 0;
}

int printTable()
{
	int i = 0;

	for (i = 0; i < IndexCounter; i++)
	{
		if (IndexTable[i].Size > 0 && (IndexTable[i].Flags & (SEC_LOAD == SEC_LOAD)))
		{
			printf("%2d: %30s 0x%08X(- 0x%08X) 0x%08X(- 0x%08X) 0x%08X(%6u) 2**%d  "
			       "0x%04X 0x%04X\n",
			       IndexTable[i].ID,
			       IndexTable[i].Name,
			       IndexTable[i].DestinationAddress,
			       IndexTable[i].DestinationAddress + IndexTable[i].Size,
			       IndexTable[i].SourceAddress,
			       IndexTable[i].SourceAddress + IndexTable[i].Size,
			       IndexTable[i].Size,
			       IndexTable[i].Size,
			       IndexTable[i].Alignment == 0x02 ? 1 :
			       IndexTable[i].Alignment == 0x04 ? 2 :
			       IndexTable[i].Alignment == 0x08 ? 3 :
			       IndexTable[i].Alignment == 0x10 ? 4 :
			       IndexTable[i].Alignment == 0x20 ? 5 :
			       IndexTable[i].Alignment == 0x40 ? 6 :
			       IndexTable[i].Alignment == 0x80 ? 7 :
			       IndexTable[i].Alignment == 0x100 ? 8 :
			       IndexTable[i].Alignment,

			       IndexTable[i].Flags,
			       IndexTable[i].DontKnow2
			      );
		}
	}

	return 0;
}

int addIndex(unsigned int DestinationAddress, unsigned int SourceAddress, unsigned int Size, unsigned int Alignment,
	     unsigned int Flags, unsigned int DontKnow2)
{
	/*printf("%2d: 0x%08X 0x%08X 0x%08X(%u) 2**%d\n",
		IDCounter, DestinationAddress, SourceAddress, Size, Size,
		Alignment==0x02?1:
		Alignment==0x04?2:
		Alignment==0x08?3:
		Alignment==0x10?4:
		Alignment==0x20?5:
		Alignment==0x40?6:
		Alignment==0x80?7:
		Alignment==0x100?8:
		Alignment);*/

	IndexTable[IndexCounter].ID                 = IDCounter++;
	IndexTable[IndexCounter].DestinationAddress = DestinationAddress;
	IndexTable[IndexCounter].SourceAddress      = SourceAddress;
	IndexTable[IndexCounter].Size               = Size;
	IndexTable[IndexCounter].Alignment          = Alignment;

	IndexTable[IndexCounter].Flags              = Flags;
	IndexTable[IndexCounter].DontKnow2          = DontKnow2;

	IndexCounter++;

	return 0;
}

int readDescription(int fd, unsigned int Address, unsigned int Size)
{
	int SectionIndex = 0, err = 0;
	int Position = 1;
	unsigned char buf[BUF_SIZE];

	err = lseek(fd, Address, SEEK_SET);
	if (err < 0)
	{
		printf("error lseek Descritpion\n");
		return 1;
	}

	err = read(fd, &buf, Size);
	if (err != Size)
	{
		printf("error read Descritpion\n");
		return 1;
	}

	while (Position < Size)
	{
		int i = 0;

		for (; buf[Position] != 0x00;)
		{

			IndexTable[SectionIndex].Name[i++] = buf[Position++];
		}
		Position++;

		IndexTable[SectionIndex].Name[i++] = 0x00;

		//printf("%2d : %s\n", IndexTable[SectionIndex].ID, IndexTable[SectionIndex].Name);

		SectionIndex++;
	}

	return 0;
}

int loadElf(int cpuf, int fd, unsigned int *entry_p, unsigned int *stack_p, int verbose)
{
	unsigned char buf[BUF_SIZE];
	int ReadBytes = 0, err = 0;

	//EntryPoint
	err = lseek(fd, 0x18, SEEK_SET);
	if (err < 0)
	{
		printf("error lseek(0x18) loadElf\n");
		return 1;
	}

	err = read(fd, &buf, 4);
	if (err != 4)
	{
		printf("error read(4) loadElf\n");
		return 1;
	}

	*entry_p = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
	//printf("EntryPoint is 0x%08X\n", *entry_p);

	//seek to the table address field
	err = lseek(fd, 0x20, SEEK_SET);
	if (err < 0)
	{
		printf("error lseek(0x18) loadElf\n");
		return 1;
	}

	err = read(fd, &buf, 4);
	if (err != 4)
	{
		printf("error read(4) loadElf\n");
		return 1;
	}

	unsigned int TableAddress = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
	//printf("TableAddress is 0x%08X\n", TableAddress);

	err = lseek(fd, TableAddress, SEEK_SET);
	if (err < 0)
	{
		printf("error lseek(TableAdress) loadElf\n");
		return 1;
	}

	while ((ReadBytes = read(fd, &buf, 10 * sizeof(int))) == (10 * sizeof(int)))
	{

//		unsigned int IncreasingNumber   = buf[0]  | buf[1]<<8  | buf[2]<<16  | buf[3]<<24;
		unsigned int Flags              = buf[4]  | buf[5] << 8  | buf[6] << 16  | buf[7] << 24;
		unsigned int DontKnow2          = buf[8]  | buf[9] << 8  | buf[10] << 16 | buf[11] << 24;

		unsigned int DestinationAddress = buf[12] | buf[13] << 8 | buf[14] << 16 | buf[15] << 24;
		unsigned int SourceAddress      = buf[16] | buf[17] << 8 | buf[18] << 16 | buf[19] << 24;
		unsigned int Size               = buf[20] | buf[21] << 8 | buf[22] << 16 | buf[23] << 24;

//		unsigned int DontKnow3          = buf[24] | buf[25]<<8 | buf[26]<<16 | buf[27]<<24;
//		unsigned int DontKnow4          = buf[28] | buf[29]<<8 | buf[30]<<16 | buf[31]<<24;

		unsigned int Alignment          = buf[32] | buf[33] << 8 | buf[34] << 16 | buf[35] << 24;

//		unsigned int DontKnow5          = buf[36] | buf[37]<<8 | buf[38]<<16 | buf[39]<<24;

		if (DestinationAddress == 0x00 && SourceAddress != 0x00)
		{
			//Source Address is address of description
			err = readDescription(fd, SourceAddress, Size);
			if (err != 0) return 1;
			break; //Exit While
		}
		else
		{
			//Add Index to Table
			addIndex(DestinationAddress, SourceAddress, Size, Alignment, Flags, DontKnow2);
		}
	}
	if (ReadBytes != 10 * sizeof(int))
	{
		printf("error ReadBytes\n");
		return 1;
	}

	//printTable();

	err = sectionToSlave(cpuf, fd, entry_p);
	if (err != 0) return 1;

	//printf("start address = 0x%08X\n", *entry_p);

	return 0;
}

int copLoadFile(int cpuf, char *infile, unsigned int *entry_p, unsigned int *stack_p, int verbose)
{
	int   inf;
	char *sfx;
//	int pipe;
//	unsigned char header[4];

	printf("%s (file %s)\n", __FUNCTION__, infile);

	if ((inf = open(infile, O_RDONLY))  < 0)
	{
		printf("[%d] cannot open input file %s\n", errno, infile);
		return (1);
	}

	if ((sfx = strrchr(infile, '.')))
	{
		sfx++;
		if (strcmp(sfx, "elf") == 0)
			return (loadElf(cpuf, inf, entry_p, stack_p, verbose));
	}
	return 1;
}

/* ------------------------------------------------------------------------
**  copRun:
**  Prerequisite: the application image has already been loaded into
**                coprocessor RAM.
*/
int copRun(int cpuf, unsigned long entry_p, int verbose)
{
	//printf("<DBG>\tstart execution...\n");

	if (ioctl(cpuf, STCOP_START, entry_p) < 0)
	{
		printf("[%d] while triggering coprocessor start!\n", errno);
		return (1);
	}

	if (verbose)
		printf("Coprocessor running! (from 0x%lx)\n", entry_p);
	return (0);
}


int main(int argc, char *argv[])
{
	int cpuf = -1;
	int res;
	unsigned int entry_p, stack_p;
	/*
	* Open the coprocessor device
	*/
	if ((cpuf = open(argv[1] /* /dev/st231-0 and -1*/, O_RDWR))  < 0)
	{
		printf("cannot open %s device (errno = %d)\n", argv[1], errno);
		exit(1);
	}

	/*
	* Execute the command
	*/
	res = copLoadFile(cpuf, argv[2], &entry_p, &stack_p, 0);
	if (res == 0)
		res = copRun(cpuf, entry_p, 0);

	return res;
}
