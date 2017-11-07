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


void ReadFile(FILE *f, int *width, int *height, struct YUV* yuv);
void InvQuantization(int width, int height, struct YUV *yuv);
void InvDct2(int width, int height, struct YUV *yuv);
void Yuv2Rgb(int width, int height, struct YUV *yuv, struct RGB *rgb);
void WriteToImage(int width, int height, struct RGB *rgb);


int main(){
    FILE *file=NULL;
    int width, height;
    struct YUV yuv;
    struct RGB rgb;
    clock_t begin_t, rgbyuv_t, dct_t, q_t;

    ReadFile(file, &width, &height, &yuv);

    begin_t=q_t=clock();
    printf("Inverse Quantization... ");
    InvQuantization(width, height, &yuv);
    printf("%f sec\n", (float)(clock()-q_t)/CLOCKS_PER_SEC);

    dct_t=clock();
    printf("Inverse DCT2...         ");
    InvDct2(width, height, &yuv);
    printf("%f sec\n", (float)(clock()-dct_t)/CLOCKS_PER_SEC);

    rgbyuv_t=clock();
    printf("YUV to RGB...           ");
    Yuv2Rgb(width, height, &yuv, &rgb);
    printf("%f sec\n", (float)(clock()-rgbyuv_t)/CLOCKS_PER_SEC);

    clock_t end_time=clock();
    float time=(float)(end_time-begin_t)/CLOCKS_PER_SEC;
    printf("\nElsaped time: %f sec\n\n", time);

    WriteToImage(width, height, &rgb);
    return 0;
}



void ReadFile(FILE *f, int *width, int *height, struct YUV* yuv){
    f=fopen("out.txt", "rt");
    if(f==NULL){
        printf("ERROR: No file.\n");
        exit(1);
    }
    fscanf(f, "%d %d", width, height);
    int w=*width;
    int h=*height;

    yuv->y=(short*)malloc(w*h*sizeof(short));
    yuv->u=(short*)malloc(w*h*sizeof(short));
    yuv->v=(short*)malloc(w*h*sizeof(short));

    int n, a;
    short num;
    short zzblock[64];
    short block[64];

    for(n=0; n<w*h; n=n+64){
        for(a=0;a<64;++a){
            fscanf(f, "%hd", &num);
            zzblock[a]=num;
        }
        a=0;

        int i, j, m=8;
        for(i=0; i<m*2; i++){
            for(j=(i<m) ? 0 : i-m+1; j<=i&&j<m; j++){
                block[(i&1)? j*(m-1)+i : (i-j)*m+j]=zzblock[a++];
            }
        }

        for(a=0;a<64;++a){
            yuv->y[n+a]=block[a];
        }
    }

    for(n=0; n<w*h; n=n+64){
        for(a=0;a<64;++a){
            fscanf(f, "%hd", &num);
            zzblock[a]=num;
        }
        a=0;

        int i, j, m=8;
        for(i=0; i<m*2; i++){
            for(j=(i<m) ? 0 : i-m+1; j<=i&&j<m; j++){
                block[(i&1)? j*(m-1)+i : (i-j)*m+j]=zzblock[a++];
            }
        }

        for(a=0;a<64;++a){
            yuv->u[n+a]=block[a];
        }
    }

    for(n=0; n<w*h; n=n+64){
        for(a=0;a<64;++a){
            fscanf(f, "%hd", &num);
            zzblock[a]=num;
        }
        a=0;

        int i, j, m=8;
        for(i=0; i<m*2; i++){
            for(j=(i<m) ? 0 : i-m+1; j<=i&&j<m; j++){
                block[(i&1)? j*(m-1)+i : (i-j)*m+j]=zzblock[a++];
            }
        }

        for(a=0;a<64;++a){
            yuv->v[n+a]=block[a];
        }
    }


    /*
    int x;
    int num;
    for(x=0;x<w*h;++x){
        fscanf(f, "%d", &num);
        yuv->y[x]=(short)num;
    }

    for(x=0;x<w*h;++x){
        fscanf(f, "%d", &num);
        yuv->u[x]=(short)num;
    }

    for(x=0;x<w*h;++x){
        fscanf(f, "%d", &num);
        yuv->v[x]=(short)num;
    }*/
    fclose(f);
}

void InvQuantization(int width, int height, struct YUV *yuv){
    int x, y;
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            //blocks items iteration
            int bX, bY;
            for(bY=0;bY<8;++bY){
                for(bX=0;bX<8;++bX){
                    yuv->y[x+y*width + bX+bY*width]*=(matrixLum[bY][bX]*L);
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
                    yuv->u[x+y*width + bX+bY*width]*=(matrixCol[bY][bX]*K);
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
                    yuv->v[x+y*width + bX+bY*width]*=(matrixCol[bY][bX]*K);
                }
            }
        }
    }
}

void InvDct2(int width, int height, struct YUV *yuv){
    //blocks iteration
    int x, y;
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            short newBlock[8][8];
            //blocks items iteration
            int i, j;
            for(j=0;j<8;++j){
                for(i=0;i<8;++i){
                    float value=0;
                    int u, v;

                    //DCT iteration
                    for(v=0;v<8;++v){
                        for(u=0;u<8;++u){
                            float data_ij=yuv->y[x+y*width + u+v*width];

                            float cu=1, cv=1;
                            if(u==0) cu=0.707;
                            if(v==0) cv=0.707;
                            value+=0.25*cu*cv*data_ij*cos((2.0*i+1)*u*PI/16)*cos((2.0*j+1)*v*PI/16);
                        }
                    }
                    newBlock[j][i]=roundf(value);
                }
            }
            //new block to yuv
            for(j=0;j<8;++j){
                for(i=0;i<8;++i){
                    yuv->y[x+y*width + i+j*width]=newBlock[j][i];
                }
            }
        }
    }



    //blocks iteration
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            short newBlock[8][8];
            //blocks items iteration
            int i, j;
            for(j=0;j<8;++j){
                for(i=0;i<8;++i){
                    float value=0;
                    int u, v;

                    //DCT iteration
                    for(v=0;v<8;++v){
                        for(u=0;u<8;++u){
                            float data_ij=yuv->u[x+y*width + u+v*width];

                            float cu=1, cv=1;
                            if(u==0) cu=0.707;
                            if(v==0) cv=0.707;
                            value+=0.25*cu*cv*data_ij*cos((2.0*i+1)*u*PI/16)*cos((2.0*j+1)*v*PI/16);
                        }
                    }
                    newBlock[j][i]=roundf(value);
                }
            }
            //new block to yuv
            for(j=0;j<8;++j){
                for(i=0;i<8;++i){
                    yuv->u[x+y*width + i+j*width]=newBlock[j][i];
                }
            }
        }
    }



    //blocks iteration
    for(y=0; y<height; y=y+8){
        for(x=0; x<width; x=x+8){

            short newBlock[8][8];
            //blocks items iteration
            int i, j;
            for(j=0;j<8;++j){
                for(i=0;i<8;++i){
                    float value=0;
                    int u, v;

                    //DCT iteration
                    for(v=0;v<8;++v){
                        for(u=0;u<8;++u){
                            float data_ij=yuv->v[x+y*width + u+v*width];

                            float cu=1, cv=1;
                            if(u==0) cu=0.707;
                            if(v==0) cv=0.707;
                            value+=0.25*cu*cv*data_ij*cos((2.0*i+1)*u*PI/16)*cos((2.0*j+1)*v*PI/16);
                        }
                    }
                    newBlock[j][i]=roundf(value);
                }
            }
            //new block to yuv
            for(j=0;j<8;++j){
                for(i=0;i<8;++i){
                    yuv->v[x+y*width + i+j*width]=newBlock[j][i];
                }
            }
        }
    }

}

void Yuv2Rgb(int width, int height, struct YUV *yuv, struct RGB *rgb){
    rgb->r=(u8*)malloc(width*height*sizeof(u8));
    rgb->g=(u8*)malloc(width*height*sizeof(u8));
    rgb->b=(u8*)malloc(width*height*sizeof(u8));

    int x;
    for(x=0;x<width*height;++x){
        short y=yuv->y[x];
        short u=yuv->u[x];
        short v=yuv->v[x];

        float r=round(y+128+1.402*v);
        float g=round(y+128-0.3441*u-0.7141*v);
        float b=round(y+128+1.772*u);
        if(r<0) r=0;
        if(r>255) r=255;
        if(g<0) g=0;
        if(g>255) g=255;
        if(b<0) b=0;
        if(b>255) b=255;

        rgb->r[x]=(u8)r;
        rgb->g[x]=(u8)g;
        rgb->b[x]=(u8)b;
    }
}

void WriteToImage(int width, int height, struct RGB *rgb){
    FILE *f=fopen("out.ppm", "wb");
    if(f==NULL){
        printf("ERROR: File write fail.\n");
        exit(2);
    }

    char header[64];
    sprintf(header, "P6\n%d %d\n%d\n", width, height, 255);
    fwrite(header, strlen(header), 1, f);

    int x;
    for(x=0;x<width*height;++x){
        u8 pixel[3];
        pixel[0]=rgb->r[x];
        pixel[1]=rgb->g[x];
        pixel[2]=rgb->b[x];
        fwrite(pixel, sizeof(pixel), 1, f);
    }
    fclose(f);
}


