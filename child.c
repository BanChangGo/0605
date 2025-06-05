#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/resource.h>

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

void sim_load() {
    volatile double dummy = 0.0;  
    for (int i = 0; i < 10000000; i++) {
        dummy += sqrt(i);  // 무거운 연산
    }
    dummy = 0.0;  
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
	sim_load();
	
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
    print_cpu_time();
    return 0;
}

