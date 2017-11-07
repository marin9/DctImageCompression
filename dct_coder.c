#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define PI 3.141593
#define L 1
#define K 1

typedef unsigned char u8;

struct RGB{
    u8 *r;
    u8 *g;
    u8 *b;
};

struct YUV{
    short *y;
    short *u;
    short *v;
};

u8 matrixLum[8][8]={{16, 11, 10, 16, 24, 40, 51, 61},
                    {12, 12, 14, 19, 26, 58, 60, 55},
                    {14, 13, 16, 24, 40, 57, 69, 56},
                    {14, 17, 22, 29, 51, 87, 80, 62},
                    {18, 22, 37, 56, 68, 109, 103, 77},
                    {24, 35, 55, 64, 81, 104, 113, 92},
                    {49, 64, 78, 87, 103, 121, 120, 101},
                    {72, 92, 95, 98, 112, 100, 103, 99}};

u8 matrixCol[8][8]={{17, 18, 24, 47, 99, 99, 99, 99},
                    {18, 21, 26, 66, 99, 99, 99, 99},
                    {24, 26, 56, 99, 99, 99, 99, 99},
                    {47, 66, 99, 99, 99, 99, 99, 99},
                    {99, 99, 99, 99, 99, 99, 99, 99},
                    {99, 99, 99, 99, 99, 99, 99, 99},
                    {99, 99, 99, 99, 99, 99, 99, 99},
                    {99, 99, 99, 99, 99, 99, 99, 99}};


void CheckInput(int argc);
FILE* OpenImage(char *name, int *width, int *height, int *pixmax);
void ReadImage(FILE *f, int width, int height, struct RGB* rgb);
void seekToData(FILE *f);
void Rgb2Yuv(int width, int height, struct RGB *rgb, struct YUV *yuv);
void Dct2(int width, int height, struct YUV *yuv);
void Quantization(int width, int height, struct YUV *yuv);
void WriteToFile(int width, int height, struct YUV *yuv);


int main(int argc, char **argv){
    FILE *file;
    int width, height, pixmax;
    struct RGB rgb;
    struct YUV yuv;
    clock_t begin_t, rgbyuv_t, dct_t, q_t;

    CheckInput(argc);
    file=OpenImage(argv[1], &width, &height, &pixmax);
    printf("Resolution: %dx%d\n", width, height);

    ReadImage(file, width, height, &rgb);
    fclose(file);

    begin_t=rgbyuv_t=clock();
    printf("RGB to YUV...    ");
    Rgb2Yuv(width, height, &rgb, &yuv);
    printf("%f sec\n", (float)(clock()-rgbyuv_t)/CLOCKS_PER_SEC);

    dct_t=clock();
    printf("DCT2...          ");
    Dct2(width, height, &yuv);
    printf("%f sec\n", (float)(clock()-dct_t)/CLOCKS_PER_SEC);

    q_t=clock();
    printf("Quantization...  ");
    Quantization(width, height, &yuv);
    printf("%f sec\n", (float)(clock()-q_t)/CLOCKS_PER_SEC);

    clock_t end_time=clock();
    float time=(float)(end_time-begin_t)/CLOCKS_PER_SEC;
    printf("\nElsaped time: %f sec\n\n", time);

    WriteToFile(width, height, &yuv);
    return 0;
}


void CheckInput(int argc){
    if(argc!=2){
        printf("ERROR: Ilegal argument.\n");
        exit(1);
    }
}

FILE* OpenImage(char *name, int *width, int *height, int *pixmax){
    FILE *file=fopen(name, "rb");
    char buffer[32];

    if(file==NULL){
        printf("ERROR: File open error.\n");
        exit(2);
    }

	fread(buffer, sizeof(buffer), 1, file);
	if(buffer[0]!='P' || buffer[1]!='6'){
		printf("ERROR: Format is not P6.\n");
		exit(3);
	}

	int i=2, x=0;
	char tmpBuffer[10];
	while(buffer[i]!=' '){
		tmpBuffer[x]=buffer[i];
		++x;
		++i;
	}
	tmpBuffer[i]='\0';
	*width=atoi(tmpBuffer);

	++i;
	x=0;
	while(buffer[i]!='\n'){
		tmpBuffer[x]=buffer[i];
		++x;
		++i;
	}
	tmpBuffer[x]='\0';
	*height=atoi(tmpBuffer);

	++i;
	x=0;
	while(buffer[i]!='\n'){
		tmpBuffer[x]=buffer[i];
		++i;
		++x;
	}
	tmpBuffer[x]='\0';
	*pixmax=atoi(tmpBuffer);

	return file;
}

void ReadImage(FILE *f, int width, int height, struct RGB* rgb){
    rgb->r=(u8*)malloc(width*height*sizeof(u8));
    rgb->g=(u8*)malloc(width*height*sizeof(u8));
    rgb->b=(u8*)malloc(width*height*sizeof(u8));

    char buffer[width*3];
    seekToData(f);

    int y, x;
    for(y=0;y<height;++y){
        if(!fread(buffer, sizeof(buffer), 1, f)){
            printf("ERROR: Read file problem: y=%d, x=%d\n", y, x);
            exit(4);
        }

        for(x=0;x<width;++x){
            rgb->r[y*width+x]=buffer[x*3+0];
            rgb->g[y*width+x]=buffer[x*3+1];
            rgb->b[y*width+x]=buffer[x*3+2];
        }
    }
}

void seekToData(FILE *f){
    char buffer[128];
    fseek(f, 0, SEEK_SET);
    fread(buffer, sizeof(buffer), 1, f);

    int x=0;
    int nl_num=0;
    while(nl_num<3){
        if(buffer[x]=='\n') ++nl_num;
        ++x;
    }
    fseek(f, x, SEEK_SET);
}

void Rgb2Yuv(int width, int height, struct RGB *rgb, struct YUV *yuv){
    yuv->y=(short*)malloc(width*height*sizeof(short));
    yuv->u=(short*)malloc(width*height*sizeof(short));
    yuv->v=(short*)malloc(width*height*sizeof(short));

    int i;
    for(i=0;i<width*height;++i){
        short r=rgb->r[i];
        short g=rgb->g[i];
        short b=rgb->b[i];

        yuv->y[i]=(short)round(r*0.299+0.587*g+0.114*b-128);
        yuv->u[i]=(short)round(-0.1687*r-0.3313*g+0.5*b);
        yuv->v[i]=(short)round(0.5*r-0.4187*g-0.0813*b);

        //yuv->y[i]=(r/4)+(g/2)+(b/8)-128;
        //yuv->u[i]=-(r/8)-(g/3)+(b/2);
        //yuv->v[i]=(r/2)-(g/2);
    }
}

void Dct2(int width, int height, struct YUV *yuv){
    //blocks iteration
    int x, y;
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            short newBlock[8][8];
            //blocks items iteration
            int bX, bY;
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    float value=0;
                    int i, j;

                    //DCT iteration
                    for(j=0;j<8;++j){
                        for(i=0;i<8;++i){
                            float data_ij=yuv->y[x+y*width + i+j*width];

                            float cu=1, cv=1;
                            if(bX==0) cu=0.707;
                            if(bY==0) cv=0.707;
                            value+=0.25*cu*cv*data_ij*cos((2.0*i+1)*bX*PI/16)*cos((2.0*j+1)*bY*PI/16);
                        }
                    }
                    newBlock[bY][bX]=roundf(value);
                }
            }
            //new block to yuv
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    yuv->y[x+y*width + bX+bY*width]=newBlock[bY][bX];
                }
            }
        }
    }


    //blocks iteration
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            short newBlock[8][8];
            //blocks items iteration
            int bX, bY;
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    float value=0;
                    int i, j;

                    //DCT iteration
                    for(j=0;j<8;++j){
                        for(i=0;i<8;++i){
                            float data_ij=yuv->u[x+y*width + i+j*width];

                            float cu=1, cv=1;
                            if(bX==0) cu=0.707;
                            if(bY==0) cv=0.707;
                            value+=0.25*cu*cv*data_ij*cos((2.0*i+1)*bX*PI/16)*cos((2.0*j+1)*bY*PI/16);
                        }
                    }
                    newBlock[bY][bX]=roundf(value);
                }
            }
            //new block to yuv
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    yuv->u[x+y*width + bX+bY*width]=newBlock[bY][bX];
                }
            }
        }
    }


    //blocks iteration
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            short newBlock[8][8];
            //blocks items iteration
            int bX, bY;
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    float value=0;
                    int i, j;

                    //DCT iteration
                    for(j=0;j<8;++j){
                        for(i=0;i<8;++i){
                            float data_ij=yuv->v[x+y*width + i+j*width];

                            float cu=1, cv=1;
                            if(bX==0) cu=0.707;
                            if(bY==0) cv=0.707;
                            value+=0.25*cu*cv*data_ij*cos((2.0*i+1)*bX*PI/16)*cos((2.0*j+1)*bY*PI/16);
                        }
                    }
                    newBlock[bY][bX]=roundf(value);
                }
            }
            //new block to yuv
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    yuv->v[x+y*width + bX+bY*width]=newBlock[bY][bX];
                }
            }
        }
    }
}

void Quantization(int width, int height, struct YUV *yuv){
    //blocks iteration
    int x, y;
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            //blocks items iteration
            int bX, bY;
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    yuv->y[x+y*width + bX+bY*width]/=(matrixLum[bY][bX]*L);
                }
            }
        }
    }

    //blocks iteration
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            //blocks items iteration
            int bX, bY;
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    yuv->u[x+y*width + bX+bY*width]/=(matrixCol[bY][bX]*K);
                }
            }
        }
    }

    //blocks iteration
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            //blocks items iteration
            int bX, bY;
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    yuv->v[x+y*width + bX+bY*width]/=(matrixCol[bY][bX]*K);
                }
            }
        }
    }
}

void WriteToFile(int width, int height, struct YUV *yuv){
    FILE *file=fopen("out.txt", "wt");
    if(file==NULL){
        printf("ERROR: File write fail.\n");
        exit(10);
    }

    fprintf(file, "%d %d\n", width, height);

    int n, a;
    short block[64];
    short zzblock[64];

    for(n=0;n<width*height;n=n+64){
        for(a=0;a<64;++a){
            block[a]=yuv->y[n+a];
        }
        a=0;

        int i, j, m=8;
        for(i=0; i<m*2; i++){
            for(j=(i<m) ? 0 : i-m+1; j<=i&&j<m; j++){
                zzblock[a++]=block[(i&1)? j*(m-1)+i : (i-j)*m+j ];
            }
        }

        for(a=0;a<64;++a){
            fprintf(file, "%hd ", zzblock[a]);
        }
    }
    fprintf(file, "\n\n");


    for(n=0;n<width*height;n=n+64){
        for(a=0;a<64;++a){
            block[a]=yuv->u[n+a];
        }
        a=0;

        int i, j, m=8;
        for(i=0; i<m*2; i++){
            for(j=(i<m) ? 0 : i-m+1; j<=i&&j<m; j++){
                zzblock[a++]=block[(i&1)? j*(m-1)+i : (i-j)*m+j ];
            }
        }

        for(a=0;a<64;++a){
            fprintf(file, "%hd ", zzblock[a]);
        }
    }
    fprintf(file, "\n\n");


    for(n=0;n<width*height;n=n+64){
        for(a=0;a<64;++a){
            block[a]=yuv->v[n+a];
        }
        a=0;

        int i, j, m=8;
        for(i=0; i<m*2; i++){
            for(j=(i<m) ? 0 : i-m+1; j<=i&&j<m; j++){
                zzblock[a++]=block[(i&1)? j*(m-1)+i : (i-j)*m+j ];
            }
        }

        for(a=0;a<64;++a){
            fprintf(file, "%hd ", zzblock[a]);
        }
    }
    fprintf(file, "\n\n");

    /*int n;
    for(n=0;n<width*height;++n){
        fprintf(file, "%d ", yuv->y[n]);
    }
    fprintf(file, "\n\n");

    for(n=0;n<width*height;++n){
        fprintf(file, "%d ", yuv->u[n]);
    }
    fprintf(file, "\n\n");

    for(n=0;n<width*height;++n){
        fprintf(file, "%d ", yuv->v[n]);
    }
    fprintf(file, "\n\n");*/

    fclose(file);
}


