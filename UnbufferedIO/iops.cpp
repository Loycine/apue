#include<fcntl.h>
#include<unistd.h>
#include<cstdio>
#include<errno.h>

#include <cstdlib>
#include <ctime>
#include<sys/types.h>
#include<sys/stat.h>

#include<thread>
#include<mutex>


class FIOPS{
protected:
    int iops_total = 0;
    std::mutex m;

public:
    const char* file_path = "/mnt/cephfs/iotest"; //ceph file
    //const char* file_path = "./iotest"; //local disk file

    const int block_size = 4096;
    const double runtime = 20.0;
    const int threads_num = 64;

    int worker_job(){
        int fd = open(file_path, O_RDONLY | O_DIRECT);
        if(fd == -1)
        {
            printf("open file error\n");
            return -1;
        }

        void* buf;
        int ret = 0;
        ret = posix_memalign((void **)&buf, 512, block_size);

        int file_size;
        file_size = lseek(fd, 0, SEEK_END);
        if(file_size == -1) {
            printf("lseek file error\n");
            return -1;
        }

        srand(time(NULL));

        int iops_local = 0;
        int start, end;
        start = time(NULL);

        while(true) {
            int start_pos = rand()%(file_size - block_size);
            ret = lseek(fd, start_pos/block_size*block_size, SEEK_SET);
            if(ret == -1) {
                printf("lseek file error\n");
                return -1;
            }
            
            ret = read(fd, (void *)buf, block_size);
            if(ret == -1) {
                printf("read file error\n");
                return -1;
            }
            ++ iops_local;
            end = time(NULL);

            double time = (double)(end - start);
            if(time > runtime) {
                m.lock();
                iops_total += iops_local;
                m.unlock();
                break;
            }
        }
        return 0;
    }

    int run() {
        printf("The test file: %s\n", file_path);

        std::thread* threads[threads_num];
        for(int i=0; i<threads_num; i++)
        {
            threads[i] = new std::thread([this]{worker_job();});
        }

        for(int i=0; i<threads_num; i++)
        {
            threads[i]->join();
            delete threads[i];
        }

        printf("iops: %lf\n", (double)iops_total / (double)runtime);
    }
};


int main()
{
    FIOPS fiops;
    fiops.run();
}  