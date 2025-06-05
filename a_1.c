#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/resource.h>
#include <time.h>
#define MAX_USERS 2000
#define NUM_ATMS 1

// ---------- 구조체 정의 ----------

typedef struct {
    int user;
    int account;
    int password;
    int card_balance;
} AccountInfo;

typedef struct {
    AccountInfo accounts[MAX_USERS + 1];
    int atm_funds[NUM_ATMS];
} AccountDB;

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

// ---------- 전역 변수 ----------

AccountDB acc_db;
UserDB loan_db;

// ---------- 초기화 ----------

void init_account_db() {
    acc_db.atm_funds[0] = 5000000;
    for (int i = 1; i <= MAX_USERS; i++) {
        acc_db.accounts[i].user = i;
        acc_db.accounts[i].account = i;
        acc_db.accounts[i].password = i;
        acc_db.accounts[i].card_balance = rand() % 50000000 + 1000000;
    }
}

void init_user_db() {
    loan_db.bank_funds = 500000;
    for (int i = 1; i <= MAX_USERS; i++) {
        loan_db.users[i].user = i;
        loan_db.users[i].identifier = i;
        loan_db.users[i].debt = 0;
        loan_db.users[i].credit_rank = (i - 1) % 5 + 1;
    }
}


// sim_load: 간단한 암호 해독 시뮬레이션
void sim_load() {
    volatile unsigned long long dummy = 0;
    int base_user = 12345; // 임의의 고정 사용자 ID
    
    for (int i = 1; i <= 1000; i++) {
        unsigned long long result = 1;
        unsigned long long base = (unsigned long long)(base_user * i);
        unsigned long long exponent = 20;
        unsigned long long mod = 1000000007;
        
        // 모듈러 지수 반복 계산 (빠른 제곱법 없이 단순 반복)
        for (unsigned long long e = 0; e < exponent; e++) {
            result = (result * base) % mod;
        }
        dummy += result;
    }
}


// loan_sim_load: 대출 처리 시뮬레이션 (sim_load + 추가 연산)
void loan_sim_load() {
    volatile unsigned long long dummy = 0;
    int base_user = 12345; // 임의의 고정 사용자 ID

    // sim_load의 2/3 연산만 수행
    for (int i = 1; i <= 667; i++) {
        unsigned long long result = 1;
        unsigned long long base = (unsigned long long)(base_user * i);
        unsigned long long exponent = 20;
        unsigned long long mod = 1000000007;
        
        for (unsigned long long e = 0; e < exponent; e++) {
            result = (result * base) % mod;
        }
        dummy += result;
    }
    
    // 신용조회 모듈
    for (int i = 0; i < 20000; i++) {
        dummy += (dummy * 31 + 17) % 1234567;
    }
    
    // 이자 계산 모듈 (단순 부동소수점 반복)
    volatile double interest = 1.05;
    for (int i = 0; i < 20000; i++) {
        interest = interest * 1.00001;
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
    printf("  🕒 총합: %.6f 초\n", user_sec + sys_sec);
}

// ---------- 기능 처리 함수 ----------

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

void handle_single_loan(int user, int amount, int identifier) {
    if (user < 1 || user > MAX_USERS) {
        printf("대출 실패: 잘못된 사용자 번호 %d\n", user);
        return;
    }

    loan_sim_load();
    UserInfo *info = &loan_db.users[user];
    if (info->identifier != identifier) {
        printf("대출 실패: 사용자 인증 실패 (%d번)\n", user);
        return;
    }

    if (loan_db.bank_funds >= amount) {
        info->debt += amount;
        loan_db.bank_funds -= amount;
        printf("대출 성공: 사용자 %d 금액 %d\n", user, amount);
    } else {
        printf("대출 실패: 은행 자금 부족\n");
    }
}


// ---------- 메인 ----------

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <입력파일>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    srand(time(NULL));
    init_account_db();
    init_user_db();

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("파일 열기 실패");
        return 1;
    }

    int type, amount, user, account, password, receiver, identifier;
    while (fscanf(fp, "%d", &type) == 1) {
        if (type == 1) {
            fscanf(fp, "%d %d %d %d", &amount, &user, &account, &password);
            atm_worker_line(amount, user, account, password);
        } else if (type == 2) {
            fscanf(fp, "%d %d %d", &amount, &user, &identifier);
            handle_single_loan(user, amount, identifier);
        } else if (type == 3) {
            fscanf(fp, "%d %d %d %d %d", &amount, &user, &account, &password, &receiver);
            mobile_app_transfer(amount, user, account, password, receiver);
        } else {
            char buf[256];
            fgets(buf, sizeof(buf), fp);
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

