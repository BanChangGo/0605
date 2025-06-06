#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/resource.h>
#include <time.h>

#define MAX_USERS 5000000
#define NUM_ATMS 1

// ---------- 구조체 정의 ----------

typedef struct {
    int user;
    int account;
    int password;
    int card_balance;
} AccountInfo;

typedef struct {
    AccountInfo *accounts;
    int *atm_funds;
} AccountDB;

// ---------- 전역 포인터 변수 ----------

AccountDB *acc_db;


// ---------- 로딩 시뮬레이션 ----------

void database_sim_load() {
    volatile double dummy = 0.0;
    for (int i = 0; i < 1500; i++) {
        dummy += sqrt(i);
    }
}
// ---------- 초기화 ----------

void init_account_db() {
    acc_db = malloc(sizeof(AccountDB));
    acc_db->accounts = malloc(sizeof(AccountInfo) * (MAX_USERS + 1));
    acc_db->atm_funds = malloc(sizeof(int) * NUM_ATMS);
    acc_db->atm_funds[0] = 10000000;

    for (int i = 1; i <= MAX_USERS; i++) {
    	database_sim_load();
        acc_db->accounts[i].user = i;
        acc_db->accounts[i].account = i;
        acc_db->accounts[i].password = i;
        acc_db->accounts[i].card_balance = 10000000;
    }
}

// ---------- 로딩 시뮬레이션 ----------

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
    AccountInfo *info = &acc_db->accounts[user];

    if (info->account != account || info->password != password) {
        printf("ATM 인증 실패: 사용자 %d\n", user);
        return;
    }

    int user_before = info->card_balance;
    int atm_before = acc_db->atm_funds[0];

    if (amount >= 0) {
        // 입금 처리
        info->card_balance += amount;
        acc_db->atm_funds[0] += amount;

        printf("ATM 입금 성공: 사용자(%d번) | 금액: %d원\n", user, amount);
        printf("사용자(%d번) 잔액: %d원 → %d원\n", user, user_before, info->card_balance);
        printf("ATM 자금: %d원 → %d원\n\n", atm_before, acc_db->atm_funds[0]);
    } else {
        // 출금 처리
        int withdraw = -amount;

        if (withdraw <= info->card_balance && withdraw <= acc_db->atm_funds[0]) {
            // 출금 성공
            info->card_balance -= withdraw;
            acc_db->atm_funds[0] -= withdraw;

            printf("ATM 출금 성공: 사용자(%d번) | 금액: %d원\n", user, withdraw);
            printf("사용자(%d번) 잔액: %d원 → %d원\n", user, user_before, info->card_balance);
            printf("ATM 자금: %d원 → %d원\n\n", atm_before, acc_db->atm_funds[0]);
        } else {
            // 출금 실패: 사유별 메시지
            printf("ATM 출금 실패: 사용자(%d번) | 요청: %d원\n", user, withdraw);
            if (withdraw > info->card_balance) {
                printf("사용자(%d번) 잔액 부족: 보유 %d원\n", user, info->card_balance);
            }
            if (withdraw > acc_db->atm_funds[0]) {
                printf("ATM 자금 부족: 기기 보유 %d원\n", acc_db->atm_funds[0]);
            }
            printf("\n");
        }
    }
}



void mobile_app_transfer(int amount, int name, int account, int password, int receiver) {
    if (name < 1 || name > MAX_USERS || receiver < 1 || receiver > MAX_USERS) {
        printf("송금 실패: 잘못된 사용자 번호 (송금자 %d, 수신자 %d)\n", name, receiver);
        return;
    }

    sim_load();
    AccountInfo *sender = &acc_db->accounts[name];
    AccountInfo *recv   = &acc_db->accounts[receiver];
    int real_amount = abs(amount);

    if (sender->account != account || sender->password != password) {
        printf("모바일 송금 실패: 계좌번호 또는 비밀번호 불일치 (송금자 %d번)\n\n", name);
        return;
    }

    if (sender->card_balance < real_amount) {
        printf("모바일 송금 실패: 잔액 부족 (송금자 %d번, 필요: %d, 보유: %d)\n\n",
               name, real_amount, sender->card_balance);
        return;
    }

    // 전 잔액 저장
    int sender_before = sender->card_balance;
    int receiver_before = recv->card_balance;

    // 송금 수행
    sender->card_balance -= real_amount;
    recv->card_balance   += real_amount;

    // 출력
    printf("모바일 송금 성공: %d번 → %d번 | 금액: %d원\n", name, receiver, real_amount);
    printf("송금자(%d번) 잔액: %d원 → %d원\n", name, sender_before, sender->card_balance);
    printf("수신자(%d번) 잔액: %d원 → %d원\n\n", receiver, receiver_before, recv->card_balance);
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
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork 실패");
        return 1;
    }
    else if (pid == 0) {
        execl("./loanchild", "loanchild",  filename, NULL);
    	perror("exec 실패");  // exec 실패 시에만 실행됨
    	exit(1);
    }else {
        
    
    init_account_db();


    int type, amount, user, account, password, receiver, identifier;
    while (fscanf(fp, "%d", &type) == 1) {
        if (type == 1) {
            fscanf(fp, "%d %d %d %d", &amount, &user, &account, &password);
            atm_worker_line(amount, user, account, password);
        } else if (type == 3) {
            fscanf(fp, "%d %d %d %d %d", &amount, &user, &account, &password, &receiver);
            mobile_app_transfer(amount, user, account, password, receiver);
        } else {
            char buf[256];
            fgets(buf, sizeof(buf), fp);
        }
    }
    print_memory_usage("👶 이전 부모 프로세스");
    fclose(fp);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double wall_sec = (end_time.tv_sec - start_time.tv_sec)
                    + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    print_cpu_time();
    print_memory_usage("👶 부모 프로세스");
    printf("⏱ 전체 실행 시간 (Wall-clock): %.6f 초\n\n", wall_sec);

    return 0;
    }
}
