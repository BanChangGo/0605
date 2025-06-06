#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/resource.h>
#include <time.h>

#define MAX_USERS 5000000
#define NUM_WORKERS 4
#define MAX_TASKS 10000
// ---------- 구조체 정의 ----------

typedef struct {
    int user;
    int identifier;
    int debt;
    int credit_rank;
} UserInfo;

typedef struct {
    UserInfo *users;
    int bank_funds;
} UserDB;

typedef struct {
    int amount;
    int user;
    int identifier;
} LoanTask;

// ---------- 전역 포인터 변수 ----------

UserDB *loan_db;
LoanTask loan_tasks[MAX_TASKS];
int loan_cnt = 0;

// ---------- 로딩 시뮬레이션 ----------

void database_sim_load() {
    volatile double dummy = 0.0;
    for (int i = 0; i < 1500; i++) {
        dummy += sqrt(i);
    }
}
// ---------- 초기화 ----------

void init_user_db() {
    loan_db = malloc(sizeof(UserDB));
    loan_db->users = malloc(sizeof(UserInfo) * (MAX_USERS + 1));
    loan_db->bank_funds = 2000000000;

    for (int i = 1; i <= MAX_USERS; i++) {
    	database_sim_load();
        loan_db->users[i].user = i;
        loan_db->users[i].identifier = i;
        loan_db->users[i].debt = 0;
        loan_db->users[i].credit_rank = (rand() % 5) + 1;
    }
}

// ---------- 로딩 시뮬레이션 ----------

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

void print_cpu_time() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double user_sec = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double sys_sec  = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
	printf("\n[Child] 📊 CPU 사용 시간\n");
    printf("\n[Child] 🧠 사용자 영역(user): %.6f 초\n", user_sec);
    printf("[Child] 🛠  커널 영역(system): %.6f 초\n", sys_sec);
    printf("[Child] 🕒 총합: %.6f 초\n", user_sec + sys_sec);
}

// ---------- 기능 처리 함수 ----------

void *worker_thread(void *arg) {
    int thread_id = *(int *)arg;
    free(arg);  // 동적 할당된 thread_id 해제

    for (int i = 0; i < loan_cnt; i++) {
        if (loan_tasks[i].user % NUM_WORKERS == thread_id) {
            handle_single_loan(loan_tasks[i].amount, loan_tasks[i].user, loan_tasks[i].identifier);
        }
    }
    return NULL;
}

void handle_single_loan(int user, int amount, int identifier) {
    if (user < 1 || user > MAX_USERS) {
        printf("대출 실패: 잘못된 사용자 번호 %d\n", user);
        return;
    }

    loan_sim_load();
    UserInfo *info = &loan_db->users[user];

    if (info->identifier != identifier) {
        printf("대출 실패: 사용자 인증 실패 (%d번)\n", user);
        return;
    }

    int credit = info->credit_rank;
    int max_loan = 0;

    switch (credit) {
        case 1: max_loan = 50000000; break;
        case 2: max_loan = 20000000; break;
        case 3: max_loan = 10000000; break;
        case 4: max_loan = 3000000;  break;
        case 5:
            printf("대출 거절: 사용자 %d (등급 5 → 대출 불가)\n\n", user);
            return;
        default:
            printf("대출 실패: 알 수 없는 등급 (%d번 사용자, 등급 %d)\n\n", user, credit);
            return;
    }

    if (amount > max_loan) {
        printf("요청 금액 %d원이 등급 %d의 최대 한도 %d원을 초과하여 조정\n",
               amount, credit, max_loan);
        amount = max_loan;
    }

    int user_debt_before = info->debt;
    int bank_before = loan_db->bank_funds;

    printf("대출 요청: 사용자 %d | 등급 %d | 최종 대출 금액: %d원\n",
           user, credit, amount);

    if (loan_db->bank_funds >= amount) {
        info->debt += amount;
        loan_db->bank_funds -= amount;
        printf("대출 승인\n");
        printf("은행 자금: %d원 → %d원\n", bank_before, loan_db->bank_funds);
        printf("사용자(%d번) 부채: %d원 → %d원\n\n", user, user_debt_before, info->debt);
    } else {
        printf("대출 실패: 은행 자금 부족 (요청 %d원, 보유 %d원)\n\n",
               amount, loan_db->bank_funds);
    }
}

// ---------- 워커 스레드 ----------
void *loan_worker(void *arg) {
    int thread_id = *(int *)arg;
    free(arg);

    for (int i = 0; i < loan_cnt; i++) {
        if (loan_tasks[i].user % NUM_WORKERS == thread_id) {
            handle_single_loan(loan_tasks[i].user, loan_tasks[i].amount, loan_tasks[i].identifier);
        }
    }
    return NULL;
}

// ---------- 메인 ----------

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <입력파일>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    srand(time(NULL));

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("파일 열기 실패");
        return 1;
    }

    init_user_db();

    int type, amount, user, account, password, receiver, identifier;
    while (fscanf(fp, "%d", &type) == 1) {
        if (type == 2) {
            if (fscanf(fp, "%d %d %d", &amount, &user, &identifier) == 3) {
                if (loan_cnt < MAX_TASKS) {
                    loan_tasks[loan_cnt++] = (LoanTask){amount, user, identifier};
                }
            }
        } else {
            char buf[256];
            fgets(buf, sizeof(buf), fp);
        }
    }
    fclose(fp);

    pthread_t threads[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&threads[i], NULL, loan_worker, arg);
    }

    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(threads[i], NULL);
    }


    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double wall_sec = (end_time.tv_sec - start_time.tv_sec)
                    + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    print_cpu_time();
    printf("⏱ 전체 실행 시간 (Wall-clock): %.6f 초\n\n", wall_sec);

    return 0;
}
