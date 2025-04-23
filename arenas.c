#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include <mach/mach.h>
#include <sys/mman.h>

static void print_stats(char const *heading)
{
    printf("-------- %s ---------\n", heading);

    struct task_basic_info bi;
    mach_msg_type_number_t bi_count = TASK_BASIC_INFO_COUNT;

    kern_return_t status = task_info(mach_task_self(), 
                                     TASK_BASIC_INFO, 
                                     (task_info_t) &bi, 
                                     &bi_count);
    if (status != KERN_SUCCESS) 
    {
        fprintf(stderr, "Failed to read task info.\n");
        return;
    }

    printf("Suspend count: %d\n", bi.suspend_count);
    printf("Virtual size: %lu (bytes)\n", bi.virtual_size);
    printf("Resident size: %lu (bytes)\n", bi.resident_size);

    float user_time = bi.user_time.seconds + 1e-6 * bi.user_time.microseconds;
    float system_time = bi.system_time.seconds + 1e-6 * bi.system_time.microseconds;
    printf("User time: %.3f (s)\n", user_time);
    printf("System time: %.3f (s)\n", system_time);
}

int main(void)
{
    int status;

    print_stats("mem info at startup");

    uint64_t size = (uint64_t) 64 * 1024 * 1024 * 1024; // 64G
    char *block = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, 0, 0);
    if (block == MAP_FAILED)
    {
        fprintf(stderr, "mmap errno: %d\n", errno);
        return 1;
    }

    print_stats("mmap - before traversal");

    uint64_t gig = (uint64_t) 1024 * 1024 * 1024;
    for (size_t i = 0; i < gig; i += 4096)
    {
        block[i] = 1;
    }

    print_stats("mmap - after traversal");

    uint64_t halfgig = gig / 2;
    status = madvise(block + halfgig, halfgig, MADV_DONTNEED);
    if (status == -1)
    {
        fprintf(stderr, "madvise don't need %d", errno);
        return 1;
    }

    print_stats("mmap - don't need last half gig");

    status = madvise(block + halfgig, halfgig, MADV_FREE);
    if (status == -1)
    {
        fprintf(stderr, "madvise free %d", errno);
        return 1;
    }

    print_stats("mmap - free last half gig");

    return 0;
}
