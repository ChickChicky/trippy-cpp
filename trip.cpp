#include <stdio.h>
#include <cmath>
#include <time.h>
#include <thread>
#include <unistd.h>

using namespace std;

#define WIDTH  500L
#define HEIGHT 500L

#define CENTERX 0.0L
#define CENTERY 0.0L
#define SCALEX  10.0L
#define SCALEY  10.0L

#define N_FRAMES 240L
#define N_THREADS 10L

#define WIDTH_D   (PREC)WIDTH
#define HEIGHT_D  (PREC)HEIGHT
#define CENTERX_D (PREC)CENTERX
#define CENTERY_D (PREC)CENTERY
#define SCALEX_D  (PREC)SCALEX
#define SCALEY_D  (PREC)SCALEY

#define IMAGE_SIZE (3*WIDTH*HEIGHT)

#define PREC long double

PREC randd() {
    timespec ts;
    clock_gettime(1,&ts);
    unsigned int t = ts.tv_nsec;
    return ((PREC)rand_r(&t)/RAND_MAX);
}

PREC shader(PREC x, PREC y, PREC n, PREC m) {
    return sin(n * (PREC)M_PI * x) * sin(m * (PREC)M_PI  * y) - sin(m * (PREC)M_PI  * x) * sin(n * (long double)M_PI  * y);
}

void generate_image(PREC nr, PREC mr, PREC ng, PREC mg, PREC nb, PREC mb, unsigned char* output) {
    for (PREC rx = -WIDTH_D/2.0; rx < WIDTH_D/2; rx += 1.0)
    for (PREC ry = -WIDTH_D/2.0; ry < WIDTH_D/2; ry += 1.0) {
        int imagex = round(WIDTH_D / 2.0 + rx);
        int imagey = round(HEIGHT_D / 2.0 + ry);
        int imagei = 3*(imagex+imagey*WIDTH);
        const PREC x = rx/SCALEX + CENTERX;
        const PREC y = ry/SCALEY + CENTERY;
        /*output[imagei]   = 128.0L + shader(x,y,nr,mr)*64.0L;
        output[imagei+1] = 128.0L + shader(x,y,ng,mg)*64.0L;
        output[imagei+2] = 128.0L + shader(x,y,nb,mb)*64.0L;*/
        output[imagei]   = max(0.L,shader(x,y,nr,mr)*128.0L);
        output[imagei+1] = max(0.L,shader(x,y,ng,mg)*128.0L);
        output[imagei+2] = max(0.L,shader(x,y,nb,mb)*128.0L);
    }
}

struct FrameData {
    PREC nr;
    PREC mr;
    PREC ng;
    PREC mg;
    PREC nb;
    PREC mb;
    unsigned char* image;
};

void generate_image_wrapper(size_t n, FrameData** data, size_t* fcount, size_t* tcount) {
    for (size_t i = 0; i < n; i++) {
        FrameData* frame = data[i];
        if (!frame) continue;
        generate_image(frame->nr,frame->mr,frame->ng,frame->mg,frame->nb,frame->mb,frame->image);
        if (fcount) *fcount += 1;
    }
    if (tcount) *tcount += 1;
}

int main(int argc, const char** argv) {
    PREC base_nr = randd()*2.0L;
    PREC base_mr = randd()*2.0L;
    PREC base_ng = randd()*2.0L;
    PREC base_mg = randd()*2.0L;
    PREC base_nb = randd()*2.0L;
    PREC base_mb = randd()*2.0L;

    PREC nr = base_nr;
    PREC mr = base_mr;
    PREC ng = base_ng;
    PREC mg = base_mg;
    PREC nb = base_nb;
    PREC mb = base_mb;

    unsigned char* video = new unsigned char[N_FRAMES*IMAGE_SIZE];
    
    size_t framez = ((float)N_FRAMES/(float)N_THREADS);
    FrameData*** data = new FrameData**[N_THREADS];
    for (size_t t = 0; t < N_THREADS; t++)
        data[t] = new FrameData*[framez];

    for (size_t frame = 0; frame < N_FRAMES*.75; frame++) {
        data[frame%N_THREADS][frame/N_THREADS] = new FrameData{nr,mr,ng,mg,nb,mb,video+frame*IMAGE_SIZE};
        mr += randd()*.01L;
        mg += randd()*.01L;
        mb += randd()*.01L;
    }

    PREC final_mr = mr;
    PREC final_mg = mg;
    PREC final_mb = mb;

    for (size_t frame = N_FRAMES*.75; frame < N_FRAMES; frame++) {
        mr += (base_mr-final_mr)/(N_FRAMES*.25);
        mg += (base_mg-final_mg)/(N_FRAMES*.25);
        mb += (base_mb-final_mb)/(N_FRAMES*.25);
        data[frame%N_THREADS][frame/N_THREADS] = new FrameData{nr,mr,ng,mg,nb,mb,video+frame*IMAGE_SIZE};
    }

    thread* threads = new thread[N_THREADS];
    size_t fcount = 0;
    size_t tcount = 0;

    for (size_t i = 0; i < N_THREADS; i++)
        threads[i] = thread(generate_image_wrapper,framez,data[i],&fcount,&tcount);

    while (fcount < N_FRAMES) {
        usleep(100000);
        printf("\x1b[G%ld/%ld frames (%.1f%%) (%ld threads)\x1b[K",fcount,N_FRAMES,100.f*(float)fcount/(float)N_FRAMES,N_THREADS-tcount);
        fflush(stdout);
    }
    printf("\n");

    char* cmd = new char[1024];

    printf("Generating video...\n");
    sprintf(cmd,"ffmpeg -y -f rawvideo -video_size %ldx%ld -pix_fmt rgb24 -r 30 -i pipe:0 output.gif > /dev/null",WIDTH,HEIGHT);
    // sprintf(cmd,"hexdump");

    FILE* ffmpeg = popen(cmd,"w");
    if (!ffmpeg) {
        printf("Failed to invoke ffmpeg.\n");
        return 1;
    }

    fwrite(video,1,N_FRAMES*IMAGE_SIZE,ffmpeg);
    fflush(ffmpeg);
    fclose(ffmpeg);

    printf("Done !\n");

    return 0;
}

