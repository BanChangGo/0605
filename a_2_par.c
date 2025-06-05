#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>
#include <sys/resource.h>

#define MAX_USERS 1000
#define NUM_ATMS 1

// 사용자 계좌 정보 구조체
typedef struct {
    int user;
    int account;
    int password;
    int card_balance;
} AccountInfo;

// 전체 은행 DB 구조체
typedef struct {
    AccountInfo accounts[MAX_USERS + 1];
    int atm_funds[NUM_ATMS];
} AccountDB;

AccountDB acc_db;  // 전역 선언

// 계좌 DB 초기화 함수
void init_account_db() {
    acc_db.atm_funds[0] = 5000000; // ATM 자금 초기화
    for (int i = 1; i <= MAX_USERS; i++) {
        acc_db.accounts[i].user = i;
        acc_db.accounts[i].account = i;
        acc_db.accounts[i].password = i;
        acc_db.accounts[i].card_balance = rand() % 50000000 + 1000000; // 1백만~5천만 랜덤
    }
}
void sim_load() {
    volatile unsigned long long dummy = 0;
    int base_user = 12345;

    int outer_loop = 20;          // 20번 반복
    unsigned long long exponent = 50000;  // 내부 반복 5만번

    for (int i = 1; i <= outer_loop; i++) {
        unsigned long long result = 1;
        unsigned long long base = (unsigned long long)(base_user + i);
        unsigned long long mod = 1000000007;

        for (unsigned long long e = 0; e < exponent; e++) {
            result = (result * base) % mod;
        }
        dummy += result;
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

void atm_worker_line(int amount, int user, int account, int password) {
    if (user < 1 || user > MAX_USERS) {
        printf("ATM 처리 실패: 잘못된 사용자 번호 %d\n", user);
        return;
    }

    sim_load();
    AccountInfo *info = &acc_db.accounts[user];

    if (info->account != account || info->password != password) {
        printf("ATM 인증 실패: 사용자 %d\n", user);
        return;
    }

    if (amount >= 0) {
        info->card_balance += amount;
        acc_db.atm_funds[0] += amount;
        printf("ATM 입금: 사용자 %d 금액 %d원\n", user, amount);
    } else {
        int withdraw = -amount;
        if (withdraw <= info->card_balance && withdraw <= acc_db.atm_funds[0]) {
            info->card_balance -= withdraw;
            acc_db.atm_funds[0] -= withdraw;
            printf("ATM 출금: 사용자 %d 금액 %d원\n", user, withdraw);
        } else {
            printf("ATM 출금 실패: 사용자 %d 잔액 부족\n", user);
        }
    }
}

void mobile_app_transfer(int amount, int name, int account, int password, int receiver) {
    if (name < 1 || name > MAX_USERS || receiver < 1 || receiver > MAX_USERS) {
        printf("송금 실패: 잘못된 사용자 번호 (송금자 %d, 수신자 %d)\n", name, receiver);
        return;
    }

    sim_load();
    AccountInfo *sender = &acc_db.accounts[name];
    AccountInfo *recv   = &acc_db.accounts[receiver];
    int real_amount = abs(amount);

    if (sender->account != account || sender->password != password) {
        printf("송금 실패: 계좌번호 또는 비밀번호 불일치 (송금자 %d번)\n\n", name);
        return;
    }

    if (sender->card_balance < real_amount) {
        printf("송금 실패: 잔액 부족 (송금자 %d번, 필요: %d, 보유: %d)\n\n",
               name, real_amount, sender->card_balance);
        return;
    }

    sender->card_balance -= real_amount;
    recv->card_balance   += real_amount;

    printf("송금 성공: %d번 → %d번, 금액: %d\n", name, receiver, real_amount);
    printf("송금자 남은 잔액: %d\n", sender->card_balance);
    printf("수신자 새로운 잔액: %d\n\n", recv->card_balance);
}

// 메인 함수
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <입력파일>\n", argv[0]);
        return 1;
    }
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    srand(time(NULL));
    init_account_db();

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("파일 열기 실패");
        return 1;
    }

    // 자식 프로세스: loan_handler 실행 (대출 전용)
    pid_t pid = fork();
    if (pid == 0) {
        execl("./a_2_child", "a_2_child", argv[1], NULL);
        perror("exec 실패");
        exit(1);
    }

    // 부모 프로세스: ATM(1), 모바일 송금(3)만 처리
    int type, amount, user, account, password, receiver;
    while (fscanf(fp, "%d", &type) == 1) {
        if (type == 1) {
            fscanf(fp, "%d %d %d %d", &amount, &user, &account, &password);
            atm_worker_line(amount, user, account, password);
        } else if (type == 3) {
            fscanf(fp, "%d %d %d %d %d", &amount, &user, &account, &password, &receiver);
            mobile_app_transfer(amount, user, account, password, receiver);
        } else {
			continue;
		}
    }

    fclose(fp);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double wall_sec = (end_time.tv_sec - start_time.tv_sec)
                    + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    print_cpu_time();
    printf("⏱ 전체 실행 시간 (Wall-clock): %.6f 초\n\n", wall_sec);
    wait(NULL); // 자식 종료 대기
    return 0;
}

