#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/resource.h>
#include <time.h>
#define MAX_USERS 1000

typedef struct {
    int user;
    int identifier; 
    int debt;
    int credit_rank;
} UserInfo;

typedef struct {
    UserInfo users[MAX_USERS + 1];
    int bank_funds;
} UserDB;

UserDB user_db;

void init_user_db() {
    user_db.bank_funds = 500000;

    for (int i = 1; i <= MAX_USERS; i++) {
        user_db.users[i].user = i;
        user_db.users[i].identifier = i;
        user_db.users[i].debt = 0;
        user_db.users[i].credit_rank = (i - 1) % 5 + 1;  // 1~5 순환
    }
}

void print_cpu_time() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    double user_sec = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double sys_sec  = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;

    printf("\n📊 CPU 사용 시간\n");
    printf("  🧠 사용자 영역(user): %.6f 초\n", user_sec);
    printf("  🛠  커널 영역(system): %.6f 초\n", sys_sec);
    printf("  🕒 총합: %.6f 초\n\n", user_sec + sys_sec);
}  




void loan_sim_load() {
    volatile unsigned long long dummy = 0;
    int base_user = 12345;

    int outer_loop = 40;          // sim_load의 2배 (40번)
    unsigned long long exponent = 50000;  // 내부 반복 5만번

    // 모듈러 지수 반복 (2배 연산)
    for (int i = 1; i <= outer_loop; i++) {
        unsigned long long result = 1;
        unsigned long long base = (unsigned long long)(base_user + i);
        unsigned long long mod = 1000000007;

        for (unsigned long long e = 0; e < exponent; e++) {
            result = (result * base) % mod;
        }
        dummy += result;
    }

    // 추가 계산 모듈
    for (int i = 0; i < 10000; i++) {
        dummy += (dummy * 31 + 17) % 1234567;
    }

    volatile double interest = 1.05;
    for (int i = 0; i < 10000; i++) {
        interest *= 1.00001;
    }
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
        if (type != 2) {
            continue;
        }
	loan_sim_load();
	
        fscanf(fp, "%d %d %d %d", &amount, &name, &identifier, &dummy);  // dummy = password
        UserInfo *user = &user_db.users[name];

        if (user->identifier != identifier) {
            fprintf(stderr, "[상담원] 인증 실패: 사용자 %d\n", name);
            continue;
        }

        int limit = get_limit_by_credit_rank(user->credit_rank);
        if (user->debt + amount > limit) {
            fprintf(stderr, "[상담원] 대출 거절: 초과 요청 (%d + %d > %d)\n",
                    user->debt, amount, limit);
        } else if (user_db.bank_funds < amount) {
            fprintf(stderr, "[상담원] 대출 거절: 은행 자금 부족\n");
        } else {
            user->debt += amount;
            user_db.bank_funds -= amount;

            fprintf(stderr,
                    "[상담원] 대출 승인: 사용자 %d | 금액: %d | 부채: %d | 남은 은행 자금: %d\n",
                    name, amount, user->debt, user_db.bank_funds);
        }
    }
	
    fclose(fp);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double wall_sec = (end_time.tv_sec - start_time.tv_sec)
                    + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    print_cpu_time();
    printf("⏱ 전체 실행 시간 (Wall-clock): %.6f 초\n\n", wall_sec);
    return 0;
}

