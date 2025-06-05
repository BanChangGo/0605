#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/resource.h>
#include <time.h>
#include <pthread.h>

#define MAX_USERS 1000
#define NUM_ATMS 1
#define MAX_TASKS 10000

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

typedef struct {
    int amount, user, account, password;
} ATMTask;

typedef struct {
    int amount, user, identifier;
} LoanTask;

typedef struct {
    int amount, user, account, password, receiver;
} TransferTask;

AccountDB acc_db;
UserDB loan_db;

ATMTask atm_tasks_0[MAX_TASKS], atm_tasks_1[MAX_TASKS], atm_tasks_2[MAX_TASKS];
LoanTask loan_tasks_0[MAX_TASKS], loan_tasks_1[MAX_TASKS], loan_tasks_2[MAX_TASKS];
TransferTask tr_tasks_0[MAX_TASKS], tr_tasks_1[MAX_TASKS], tr_tasks_2[MAX_TASKS];
int atm_cnt[3] = {0}, loan_cnt[3] = {0}, tr_cnt[3] = {0};

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

void sim_load() {
    volatile double dummy = 0.0;
    for (int i = 0; i < 10000000; i++) {
        dummy += sqrt(i);
    }
}

void atm_worker_line(int amount, int user, int account, int password) {
    if (user < 1 || user > MAX_USERS) return;
    sim_load();
    AccountInfo *info = &acc_db.accounts[user];
    if (info->account != account || info->password != password) return;

    if (amount >= 0) {
        info->card_balance += amount;
        acc_db.atm_funds[0] += amount;
    } else {
        int withdraw = -amount;
        if (withdraw <= info->card_balance && withdraw <= acc_db.atm_funds[0]) {
            info->card_balance -= withdraw;
            acc_db.atm_funds[0] -= withdraw;
        }
    }
}

void mobile_app_transfer(int amount, int name, int account, int password, int receiver) {
    if (name < 1 || name > MAX_USERS || receiver < 1 || receiver > MAX_USERS) return;
    sim_load();
    AccountInfo *sender = &acc_db.accounts[name];
    AccountInfo *recv   = &acc_db.accounts[receiver];
    int real_amount = abs(amount);
    if (sender->account != account || sender->password != password) return;
    if (sender->card_balance < real_amount) return;

    sender->card_balance -= real_amount;
    recv->card_balance   += real_amount;
}

void handle_single_loan(int user, int amount, int identifier) {
    if (user < 1 || user > MAX_USERS) return;
    sim_load();
    UserInfo *info = &loan_db.users[user];
    if (info->identifier != identifier) return;
    if (loan_db.bank_funds >= amount) {
        info->debt += amount;
        loan_db.bank_funds -= amount;
    }
}

void *worker_thread(void *arg) {
    int id = *(int*)arg;
    for (int i = 0; i < atm_cnt[id]; i++)
        atm_worker_line(atm_tasks_0[i + id * MAX_TASKS].amount, atm_tasks_0[i + id * MAX_TASKS].user,
                        atm_tasks_0[i + id * MAX_TASKS].account, atm_tasks_0[i + id * MAX_TASKS].password);

    for (int i = 0; i < loan_cnt[id]; i++)
        handle_single_loan(loan_tasks_0[i + id * MAX_TASKS].user, loan_tasks_0[i + id * MAX_TASKS].amount, loan_tasks_0[i + id * MAX_TASKS].identifier);

    for (int i = 0; i < tr_cnt[id]; i++)
        mobile_app_transfer(tr_tasks_0[i + id * MAX_TASKS].amount, tr_tasks_0[i + id * MAX_TASKS].user,
                            tr_tasks_0[i + id * MAX_TASKS].account, tr_tasks_0[i + id * MAX_TASKS].password,
                            tr_tasks_0[i + id * MAX_TASKS].receiver);
    return NULL;
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
        int mod;
        if (type == 1 && fscanf(fp, "%d %d %d %d", &amount, &user, &account, &password) == 4) {
            ATMTask task = {amount, user, account, password};
            mod = user % 3;
            if (mod == 0) atm_tasks_0[atm_cnt[0]++] = task;
            else if (mod == 1) atm_tasks_1[atm_cnt[1]++] = task;
            else atm_tasks_2[atm_cnt[2]++] = task;
        } else if (type == 2 && fscanf(fp, "%d %d %d", &amount, &user, &identifier) == 3) {
            LoanTask task = {amount, user, identifier};
            mod = user % 3;
            if (mod == 0) loan_tasks_0[loan_cnt[0]++] = task;
            else if (mod == 1) loan_tasks_1[loan_cnt[1]++] = task;
            else loan_tasks_2[loan_cnt[2]++] = task;
        } else if (type == 3 && fscanf(fp, "%d %d %d %d %d", &amount, &user, &account, &password, &receiver) == 5) {
            TransferTask task = {amount, user, account, password, receiver};
            mod = user % 3;
            if (mod == 0) tr_tasks_0[tr_cnt[0]++] = task;
            else if (mod == 1) tr_tasks_1[tr_cnt[1]++] = task;
            else tr_tasks_2[tr_cnt[2]++] = task;
        } else {
            fscanf(fp, "%*[^\n]");
        }
    }
    fclose(fp);

    pthread_t tids[2];
    int ids[2] = {1, 2};
    pthread_create(&tids[0], NULL, worker_thread, &ids[0]);
    pthread_create(&tids[1], NULL, worker_thread, &ids[1]);

    int id = 0;
    for (int i = 0; i < atm_cnt[id]; i++)
        atm_worker_line(atm_tasks_0[i].amount, atm_tasks_0[i].user, atm_tasks_0[i].account, atm_tasks_0[i].password);
    for (int i = 0; i < loan_cnt[id]; i++)
        handle_single_loan(loan_tasks_0[i].user, loan_tasks_0[i].amount, loan_tasks_0[i].identifier);
    for (int i = 0; i < tr_cnt[id]; i++)
        mobile_app_transfer(tr_tasks_0[i].amount, tr_tasks_0[i].user, tr_tasks_0[i].account, tr_tasks_0[i].password, tr_tasks_0[i].receiver);

    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double wall_sec = (end_time.tv_sec - start_time.tv_sec)
                    + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    print_cpu_time();
    printf("⏱ 전체 실행 시간 (Wall-clock): %.6f 초\n\n", wall_sec);
    return 0;
}

