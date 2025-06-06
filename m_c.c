// 멀티스레드 기반 대출 처리 프로그램 (스레드 수 변경 가능)

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

#define MAX_USERS 1000
#define MAX_REQUESTS 10000
#define THREAD_COUNT 1  // 여기서 2, 3, 4로 변경하며 실험

typedef struct {
    int user;
    int identifier;
    int debt;
    int credit_rank;
} UserInfo;

typedef struct {
    UserInfo users[MAX_USERS + 1];
    int bank_funds;
    pthread_mutex_t lock;
} UserDB;

UserDB user_db;

void init_user_db() {
    user_db.bank_funds = 500000;
    pthread_mutex_init(&user_db.lock, NULL);
    for (int i = 1; i <= MAX_USERS; i++) {
        user_db.users[i].user = i;
        user_db.users[i].identifier = i;
        user_db.users[i].debt = 0;
        user_db.users[i].credit_rank = (i - 1) % 5 + 1;
    }
}

void print_memory_usage(const char *label) {
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) {
        perror("메모리 상태 조회 실패");
        return;
    }
    char line[256];
    printf("\n📦 [%s] 메모리 사용량:\n", label);
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "VmHWM:", 6) == 0) {
            printf("  %s", line);
        }
    }
    fclose(fp);
}

void print_cpu_time(double wall_sec) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double user_sec = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double sys_sec  = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
    double total_cpu_time = user_sec + sys_sec;
    printf("\n📊 CPU 사용 시간\n");
    printf("  🧠 사용자 영역(user): %.6f 초\n", user_sec);
    printf("  🛠  커널 영역(system): %.6f 초\n", sys_sec);
    printf("  🕒 총합: %.6f 초\n\n", total_cpu_time);
    if (wall_sec > 0) {
        double cpu_usage_percent = (total_cpu_time / wall_sec) * 100.0;
        printf("  ⚙️  CPU 사용률: %.2f %%\n", cpu_usage_percent);
    }
}

void loan_sim_load() {
    volatile unsigned long long dummy = 0;
    int base_user = 12345;
    for (int i = 1; i <= 40; i++) {
        unsigned long long result = 1;
        unsigned long long base = base_user + i;
        for (unsigned long long e = 0; e < 50000; e++) {
            result = (result * base) % 1000000007;
        }
        dummy += result;
    }
    for (int i = 0; i < 10000; i++) {
        dummy += (dummy * 31 + 17) % 1234567;
    }
    volatile double interest = 1.05;
    for (int i = 0; i < 10000; i++) interest *= 1.00001;
}

int get_limit_by_credit_rank(int rank) {
    switch(rank) {
        case 1: return 100000;
        case 2: return 70000;
        case 3: return 50000;
        case 4: return 30000;
        case 5: return 10000;
        default: return 0;
    }
}

typedef struct {
    int amount, name, identifier;
} LoanRequest;

LoanRequest requests[MAX_REQUESTS];
int request_count = 0;

typedef struct {
    int start, end;
} ThreadArg;

void* loan_worker(void *arg) {
    ThreadArg *range = (ThreadArg *)arg;
    for (int i = range->start; i < range->end; i++) {
        LoanRequest *r = &requests[i];
        loan_sim_load();

        UserInfo *user = &user_db.users[r->name];
        if (user->identifier != r->identifier) {
            fprintf(stderr, "[상담원] 인증 실패: 사용자 %d\n", r->name);
            continue;
        }

        int limit = get_limit_by_credit_rank(user->credit_rank);

        pthread_mutex_lock(&user_db.lock);
        if (user->debt + r->amount > limit) {
            fprintf(stderr, "[상담원] 대출 거절: 초과 요청 (%d + %d > %d)\n",
                    user->debt, r->amount, limit);
        } else if (user_db.bank_funds < r->amount) {
            fprintf(stderr, "[상담원] 대출 거절: 은행 자금 부족\n");
        } else {
            user->debt += r->amount;
            user_db.bank_funds -= r->amount;
            fprintf(stderr,
                    "[상담원] 대출 승인: 사용자 %d | 금액: %d | 부채: %d | 남은 은행 자금: %d\n",
                    r->name, r->amount, user->debt, user_db.bank_funds);
        }
        pthread_mutex_unlock(&user_db.lock);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <입력파일>\n", argv[0]);
        return 1;
    }

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("파일 열기 실패");
        return 1;
    }

    init_user_db();
    int type, amount, name, identifier, dummy;
    while (fscanf(fp, "%d", &type) == 1) {
        if (type == 2) {
            fscanf(fp, "%d %d %d %d", &amount, &name, &identifier, &dummy);
            requests[request_count++] = (LoanRequest){amount, name, identifier};
        } else {
            fscanf(fp, "%*[^\n]"); fscanf(fp, "%*c");
        }
    }
    fclose(fp);

    pthread_t threads[THREAD_COUNT];
    ThreadArg args[THREAD_COUNT];
    int chunk = (request_count + THREAD_COUNT - 1) / THREAD_COUNT;

    for (int i = 0; i < THREAD_COUNT; i++) {
        args[i].start = i * chunk;
        args[i].end = (i + 1) * chunk;
        if (args[i].end > request_count) args[i].end = request_count;
        pthread_create(&threads[i], NULL, loan_worker, &args[i]);
    }
    for (int i = 0; i < THREAD_COUNT; i++) {
        print_memory_usage("👶 자식 프로세스 (대출) %d", i);
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double wall_sec = (end_time.tv_sec - start_time.tv_sec)
                    + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    print_cpu_time(wall_sec);
    print_memory_usage("👶 자식 프로세스 (대출)");
    printf("⏱ 전체 실행 시간 (Wall-clock): %.6f 초\n\n", wall_sec);
    return 0;
}