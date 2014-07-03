#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

/* scrambling/descrambling reverse engineered by AleMaxx */
static void decode_page(char *inpg, char *outpg, const int size)
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

int decode_file(int fdin,int fdout)
{
	int rlen,wlen;
	char bu[0x200],buo[0x200];

	rlen=lseek(fdin,0,SEEK_END);
	lseek(fdin,0,SEEK_SET);

	while(rlen){
		if(rlen>0x200)
			wlen=0x200;
		else
			wlen=rlen;
		read(fdin,bu,wlen);
		decode_page(bu,buo,wlen);
		write(fdout,buo,wlen);
		rlen-=wlen;
	}
}

int main(int argc,char *argv[])
{
	int fdin,fdout;

	if(argc<2)
		return 1;

	fdin=open(argv[1],O_RDONLY);
	fdout=open(argv[2],O_CREAT|O_WRONLY,00664);

	decode_file(fdin,fdout);

	close(fdin);
	close(fdout);

	return 0;
}
