/*
*  Based on code that is
*  Copyright (c) Rockbox authors.
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

struct bootloader_header {
    uint8_t magic[4];
    uint16_t head_len;
    uint16_t version;
    uint16_t unknown1;
    uint16_t build_year;
    uint8_t build_month;
    uint8_t build_day;
    uint8_t build_hour;
    uint8_t build_minute;
    uint8_t build_second;
    /* 104 (0x68) bytes */
    uint32_t chip;
} __attribute__ ((packed));

struct file_header {
    uint8_t unk1;
    uint32_t num;
    uint16_t uname[20];
    uint32_t offset;
    uint32_t len;
    uint32_t unk2;
} __attribute__ ((packed));

char *uni_to_onebyte_dumb(unsigned short str[])
{
    static char re[20];
    int i;
    for(i=0;i<20;i++)
	re[i]=str[i];
    return re;
}
/* scrambling/descrambling reverse engineered by AleMaxx */
static void rk_rc4(char *inpg, char *outpg, const int size)
{

char key[] = {
        0x7C, 0x4E, 0x03, 0x04,
        0x55, 0x05, 0x09, 0x07,
        0x2D, 0x2C, 0x7B, 0x38,
        0x17, 0x0D, 0x17, 0x11
};
        int i, i3, x, val, idx;

        char key1[0x100];
        char key2[0x100];

        for (i=0; i<0x100; i++) {
                key1[i] = i;
                key2[i] = key[i&0xf];
        }

        i3 = 0;
        for (i=0; i<0x100; i++) {
                x = key1[i];
                i3 = key1[i] + i3;
                i3 += key2[i];
                i3 &= 0xff;
                key1[i] = key1[i3];
                key1[i3] = x;
        }

        idx = 0;
        for (i=0; i<size; i++) {
                x = key1[(i+1) & 0xff];
                val = x;
                idx = (x + idx) & 0xff;
                key1[(i+1) & 0xff] = key1[idx];
                key1[idx] = (x & 0xff);
                val = (key1[(i+1)&0xff] + x) & 0xff;
                val = key1[val];
                outpg[i] = val ^ inpg[i];
        }
}

int decode_file(int fdin,struct file_header *fh)
{
    int fdout;
    int rlen,wlen;
    char bu[0x200],buo[0x200];
    
    fdout=open(uni_to_onebyte_dumb(fh->uname),O_WRONLY|O_CREAT,0664);
    lseek(fdin,fh->offset,SEEK_SET);
    rlen=fh->len;
    
    while(rlen){
	if(rlen>0x200)
	    wlen=0x200;
	else
	    wlen=rlen;
	read(fdin,bu,wlen);
	rk_rc4(bu,buo,wlen);
	write(fdout,buo,wlen);
	rlen-=wlen;
    }
    close(fdout);
}

int main(int argc,char *argv[])
{
    struct bootloader_header bh;
    struct file_header fh[4];
    int fdin;
    int i;

    if(argc<2)
	return 1;

    fdin=open(argv[1],O_RDONLY);
    read(fdin,&bh,sizeof(bh));

    printf("Header info:\n"
	"Magic:\t\t%c%c%c%c\n"
	"Header length:\t%X\n"
	"Version:\t%u\n"
	"Unknown:\t%08"PRIX32"\n"
	"Date:\t\t%u-%u-%u %u:%u:%u\n"
	"Chip:\t\t%08"PRIX32"\n"
	,bh.magic[0],bh.magic[1],bh.magic[2],bh.magic[3]
	,bh.head_len
	,bh.version
	,bh.unknown1
	,bh.build_year
	,bh.build_month
	,bh.build_day
	,bh.build_hour
	,bh.build_minute
	,bh.build_second
	,bh.chip
	);

    lseek(fdin,bh.head_len,SEEK_SET);

    read(fdin,&fh,sizeof(fh));

    for(i=0;i<4;i++){
	printf("\nFile info:\n"
	"Unknown1:\t\t%u\n"
	"Num:\t\t%u\n"
	"Name:\t\t%s\n"
	"Offset:\t\t%08"PRIX32"\n"
	"Length:\t\t%08"PRIX32"\n"
	"Unknown2:\t%08"PRIX32"\n"
	,fh[i].unk1
	,fh[i].num
	,uni_to_onebyte_dumb(fh[i].uname)
	,fh[i].offset
	,fh[i].len
	,fh[i].unk2
	);
	
	decode_file(fdin,&fh[i]);
    }

    close(fdin);

    return 0;
}
