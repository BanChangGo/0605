// loan_handler.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>
#include <math.h>
#include <sys/resource.h>

#define MAX_USERS 1000
#define MAX_LOANS 10000
#define SHM_NAME "/user_db_shm"


//연산용
volatile double dummy = 0.0;
void sim_load() {
    for (int i = 0; i < 100000; i++) dummy += sqrt(i);
    dummy = 0.0;
}

//측정
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

// 사용자 정보
typedef struct {
    int user;
    int identifier;
    int debt;
    int credit_rank;
} UserInfo;

// 공유 데이터베이스
typedef struct {
    UserInfo users[MAX_USERS + 1];
    int bank_funds;
} UserDB;

// 대출 요청 구조체
typedef struct {
    int amount;
    int user;
    int identifier;
    int dummy_val;
} LoanReq;

LoanReq loan_reqs[MAX_LOANS];
int loan_count = 0;

// DB 초기화
void init_user_db(UserDB *db) {
    db->bank_funds = 500000;
    for (int i = 1; i <= MAX_USERS; i++) {
        db->users[i].user = i;
        db->users[i].identifier = i;
        db->users[i].debt = 0;
        db->users[i].credit_rank = (i - 1) % 5 + 1;
    }
}

// 단일 대출 요청 처리
void handle_single_loan(LoanReq *req, UserDB *shared_db) {
    sim_load();
    if (req->user < 1 || req->user > MAX_USERS) return;

    UserInfo *info = &shared_db->users[req->user];
    if (info->identifier != req->identifier) {
        printf("대출 실패: 사용자 인증 실패 (%d번)\n", req->user);
        return;
    }

    if (shared_db->bank_funds >= req->amount) {
        info->debt += req->amount;
        shared_db->bank_funds -= req->amount;
        printf("대출 성공: 사용자 %d 금액 %d\n", req->user, req->amount);
    } else {
        printf("대출 실패: 은행 자금 부족\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <입력파일>\n", argv[0]);
        return 1;
    }

    // 공유 메모리 생성 및 초기화
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open 실패");
        return 1;
    }

    ftruncate(shm_fd, sizeof(UserDB));
    UserDB *shared_db = mmap(NULL, sizeof(UserDB), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_db == MAP_FAILED) {
        perror("mmap 실패");
        return 1;
    }

    init_user_db(shared_db); // 공유 메모리 초기화

    // 입력 파일 열기
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("파일 열기 실패");
        return 1;
    }

    // 대출 요청만 저장
    int type, amount, user, identifier, dummy_val;
    while (fscanf(fp, "%d", &type) == 1) {
        if (type == 2) {
            fscanf(fp, "%d %d %d %d", &amount, &user, &identifier, &dummy_val);
            if (loan_count < MAX_LOANS) {
                loan_reqs[loan_count++] = (LoanReq){amount, user, identifier, dummy_val};
            }
        } else {
            fscanf(fp, "%*[^\n]\n"); // 나머지 줄 무시
        }
    }
    fclose(fp);

    // fork하여 병렬 처리
    pid_t pid = fork();
    if (pid == 0) {
        // 자식: 짝수 인덱스
        for (int i = 0; i < loan_count; i += 2) {
            handle_single_loan(&loan_reqs[i], shared_db);
        }
        exit(0);
    }

    // 부모: 홀수 인덱스
    for (int i = 1; i < loan_count; i += 2) {
        handle_single_loan(&loan_reqs[i], shared_db);
    }
	
	
    wait(NULL);
	print_cpu_time();
    // 필요 시 공유 메모리 해제
    // shm_unlink(SHM_NAME);

    return 0;
}

