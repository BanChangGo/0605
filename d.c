#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/resource.h>
#include <pthread.h>

#define MAX_USERS 1000
#define SHM_NAME "/account_db_shm"

typedef struct {
    int user;
    int identifier;
    int account;
    int password;
    int card_balance;
} AccountInfo;

typedef struct {
    AccountInfo accounts[MAX_USERS + 1];
    int bank_funds;
    int atm_funds;
} AccountDB;

volatile double dummy = 0.0;
void sim_load() {
    for (int i = 0; i < 100000; i++) dummy += sqrt(i);
    dummy = 0.0;
}

void print_cpu_time() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double user_sec = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double sys_sec  = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
    printf("\n\U0001F4CA CPU 사용 시간\n");
    printf("  \U0001F9E0 사용자 영역(user): %.6f 초\n", user_sec);
    printf("  \U0001F6E0  커널 영역(system): %.6f 초\n", sys_sec);
    printf("  \U0001F553 총합: %.6f 초\n\n", user_sec + sys_sec);
}  

void init_account_db(AccountDB *db) {
    db->bank_funds = 500000;
    db->atm_funds = 1000000;
    for (int i = 1; i <= MAX_USERS; i++) {
        db->accounts[i].user = i;
        db->accounts[i].identifier = i;
        db->accounts[i].account = i;
        db->accounts[i].password = i;
        db->accounts[i].card_balance = 100000;
    }
}

typedef struct {
    FILE *fp;
    AccountDB *db;
    int parity; // 0: even, 1: odd
} ThreadArg;

void *handle_atm_thread(void *arg) {
    ThreadArg *targ = (ThreadArg *)arg;
    int type, amount, user, account, password;
    rewind(targ->fp);
    while (fscanf(targ->fp, "%d", &type) == 1) {
        if (type == 1) {
            fscanf(targ->fp, "%d %d %d %d", &amount, &user, &account, &password);
            if (user % 2 != targ->parity) continue;
            sim_load();
            if (user < 1 || user > MAX_USERS) continue;
            AccountInfo *info = &targ->db->accounts[user];
            if (info->account != account || info->password != password) {
                printf("ATM 인증 실패: 사용자 %d\n", user);
                continue;
            }
            if (amount >= 0) {
                info->card_balance += amount;
                targ->db->atm_funds += amount;
                printf("ATM 입금: 사용자 %d 금액 %d원\n", user, amount);
            } else {
                int withdraw = -amount;
                if (withdraw <= info->card_balance && withdraw <= targ->db->atm_funds) {
                    info->card_balance -= withdraw;
                    targ->db->atm_funds -= withdraw;
                    printf("ATM 출금: 사용자 %d 금액 %d원\n", user, withdraw);
                } else {
                    printf("ATM 출금 실패: 사용자 %d 잔액 부족\n", user);
                }
            }
        } else {
            fscanf(targ->fp, "%*[^\"\n]"); fscanf(targ->fp, "%*c"); // skip line
        }
    }
    return NULL;
}

void *handle_mobile_thread(void *arg) {
    ThreadArg *targ = (ThreadArg *)arg;
    int type, amount, sender, account, password, receiver;
    rewind(targ->fp);
    while (fscanf(targ->fp, "%d", &type) == 1) {
        if (type == 3) {
            fscanf(targ->fp, "%d %d %d %d %d", &amount, &sender, &account, &password, &receiver);
            if (sender % 2 != targ->parity) continue;
            sim_load();
            if (sender < 1 || sender > MAX_USERS || receiver < 1 || receiver > MAX_USERS) continue;
            AccountInfo *s = &targ->db->accounts[sender];
            AccountInfo *r = &targ->db->accounts[receiver];
            if (s->account != account || s->password != password) {
                printf("송금 실패: 계좌번호 또는 비밀번호 불일치 (송금자 %d번)\n\n", sender);
                continue;
            }
            if (s->card_balance < amount) {
                printf("송금 실패: 잔액 부족 (송금자 %d번, 필요: %d, 보유: %d)\n\n",
                       sender, amount, s->card_balance);
                continue;
            }
            s->card_balance -= amount;
            r->card_balance += amount;
            printf("송금 성공: %d번 → %d번, 금액: %d\n", sender, receiver, amount);
            printf("송금자 남은 잔액: %d\n", s->card_balance);
            printf("수신자 새로운 잔액: %d\n\n", r->card_balance);
        } else {
            fscanf(targ->fp, "%*[^\"\n]"); fscanf(targ->fp, "%*c"); // skip line
        }
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

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(AccountDB));
    AccountDB *shared_db = mmap(NULL, sizeof(AccountDB), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    init_account_db(shared_db);

    pid_t atm_pid = fork();
    if (atm_pid == 0) {
        FILE *fp = fopen(argv[1], "r");
        if (!fp) exit(1);
        pthread_t t1, t2;
        ThreadArg arg1 = {fp, shared_db, 0};
        ThreadArg arg2 = {fp, shared_db, 1};
        pthread_create(&t1, NULL, handle_atm_thread, &arg1);
        pthread_create(&t2, NULL, handle_atm_thread, &arg2);
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        fclose(fp);
        exit(0);
    }

    pid_t transfer_pid = fork();
    if (transfer_pid == 0) {
        FILE *fp = fopen(argv[1], "r");
        if (!fp) exit(1);
        pthread_t t1, t2;
        ThreadArg arg1 = {fp, shared_db, 0};
        ThreadArg arg2 = {fp, shared_db, 1};
        pthread_create(&t1, NULL, handle_mobile_thread, &arg1);
        pthread_create(&t2, NULL, handle_mobile_thread, &arg2);
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        fclose(fp);
        exit(0);
    }

    pid_t loan_pid = fork();
    if (loan_pid == 0) {
        execl("./c_1_child", "c_1_child", argv[1], NULL);
        perror("exec 실패");
        exit(1);
    }

    wait(NULL); wait(NULL); wait(NULL);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double wall_sec = (end_time.tv_sec - start_time.tv_sec)
                    + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    print_cpu_time();
    printf("\u23F1 전체 실행 시간 (Wall-clock): %.6f 초\n\n", wall_sec);
    shm_unlink(SHM_NAME);
    return 0;
}
