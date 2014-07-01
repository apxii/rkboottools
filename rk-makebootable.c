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
#include <string.h>

struct bootinfo {
	uint32_t flag;
	uint32_t zero1;
	uint32_t zero2;
	uint16_t unk1;
	uint16_t unk2;
	uint8_t zeros [0x1EA];
	uint16_t stage1length;
	uint16_t stage2length;
	uint16_t zero;
} __attribute__((packed));

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

static int rc4_file(int fdin,int fdout,int len)
{
	int rlen,wlen,restlen;
	char bu[0x200],buo[0x200];

	rlen=lseek(fdin,0,SEEK_END);
	lseek(fdin,0,SEEK_SET);
	restlen=len;

	while(rlen){
		if(rlen>0x200)
			wlen=0x200;
		else
			wlen=rlen;
		memset(bu,0,0x200);
		read(fdin,bu,wlen);
		rk_rc4(bu,buo,0x200);
		write(fdout,buo,0x200);
		rlen-=wlen;
		restlen--;
	}

	while(restlen--){
		memset(buo,0,0x200);
		write(fdout,buo,0x200);
	}
	return 0;
}

int main(int argc,char *argv[])
{
	int fd1,fd2,fdout;
	size_t flen;
	struct bootinfo bi;
	char buf[0x200];

	if(argc<3){
		printf("Rockchip SD Boot image maker\nUsage:\n %s <Stage1> <Stage2> <Output>\n",argv[0]);
		return 1;
	}

	memset(&bi,0,sizeof(bi));

	bi.flag=0x0FF0AA55;	/* Boot flag */
	bi.unk1=4;		/* Unknown value == 4 */
	bi.unk2=4;		/* Unknown value == 4 */

	fdout=open(argv[3],O_WRONLY|O_CREAT,0664);
	if(fdout==-1){
		perror("Can't open Output file");
		return 1;
	}

	fd1=open(argv[1],O_RDONLY);
	if(fd1==-1){
		perror("Can't open Stage1 file");
		return 1;
	}
	fd2=open(argv[2],O_RDONLY);
	if(fd2==-1){
		perror("Can't open Stage2 file");
		return 1;
	}

	flen=lseek(fd1,0,SEEK_END);
	bi.stage1length=flen/0x200;
	if(flen%0x200)
		bi.stage1length++;

	/* Align Stage1 length to be a multiple of 4 sectors, otherwise it won't boot. */
	bi.stage1length=(bi.stage1length+3)&~3;

	flen=lseek(fd2,0,SEEK_END);
	bi.stage2length=flen/0x200;
	if(flen%0x200)
		bi.stage2length++;

	bi.stage2length+=bi.stage1length+1;

	rk_rc4((char *)&bi,buf,0x200);
	write(fdout,buf,sizeof(bi));

	/*
	   Pad 3*0x200 bytes so stage1 code will be placed at 0x8800 on SD card
	 */
	memset(buf,0,0x200);
	write(fdout,buf,0x200);
	write(fdout,buf,0x200);
	write(fdout,buf,0x200);

	rc4_file(fd1,fdout,bi.stage1length);

	rc4_file(fd2,fdout,bi.stage2length);

	close(fdout);
	close(fd1);
	close(fd2);

	printf("RK boot image written,\n Stage1 at 0x8800, length=0x%04X\n Stage2 at 0x%04X, length=0x%04X\n",
		bi.stage1length,bi.stage1length*0x200+0x8800,bi.stage2length);

	return 0;
}
