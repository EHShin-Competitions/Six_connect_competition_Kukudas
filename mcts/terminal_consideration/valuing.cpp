// Samsung Go Tournament Form C (g++-4.8.3)

/*
[AI 코드 작성 방법]

1. char info[]의 배열 안에					"TeamName:자신의 팀명,Department:자신의 소속"					순서로 작성합니다.
( 주의 ) Teamname:과 Department:는 꼭 들어가야 합니다.
"자신의 팀명", "자신의 소속"을 수정해야 합니다.

2. 아래의 myturn() 함수 안에 자신만의 AI 코드를 작성합니다.

3. AI 파일을 테스트 하실 때는 "육목 알고리즘대회 툴"을 사용합니다.

4. 육목 알고리즘 대회 툴의 연습하기에서 바둑돌을 누른 후, 자신의 "팀명" 이 들어간 알고리즘을 추가하여 테스트 합니다.



[변수 및 함수]
myturn(int cnt) : 자신의 AI 코드를 작성하는 메인 함수 입니다.
int cnt (myturn()함수의 파라미터) : 돌을 몇 수 둬야하는지 정하는 변수, cnt가 1이면 육목 시작 시  한 번만  두는 상황(한 번), cnt가 2이면 그 이후 돌을 두는 상황(두 번)
int  x[0], y[0] : 자신이 둘 첫 번 째 돌의 x좌표 , y좌표가 저장되어야 합니다.
int  x[1], y[1] : 자신이 둘 두 번 째 돌의 x좌표 , y좌표가 저장되어야 합니다.
void domymove(int x[], int y[], cnt) : 둘 돌들의 좌표를 저장해서 출력


//int board[BOARD_SIZE][BOARD_SIZE]; 바둑판 현재상황 담고 있어 바로사용 가능함. 단, 원본데이터로 수정 절대금지
// 놓을수 없는 위치에 바둑돌을 놓으면 실격패 처리.

boolean ifFree(int x, int y) : 현재 [x,y]좌표에 바둑돌이 있는지 확인하는 함수 (없으면 true, 있으면 false)
int showBoard(int x, int y) : [x, y] 좌표에 무슨 돌이 존재하는지 보여주는 함수 (1 = 자신의 돌, 2 = 상대의 돌, 3 = 블럭킹)


<-------AI를 작성하실 때, 같은 이름의 함수 및 변수 사용을 권장하지 않습니다----->
*/

#include <stdio.h>

#include <time.h>

// "샘플코드[C]"  -> 자신의 팀명 (수정)
// "AI부서[C]"  -> 자신의 소속 (수정)
// 제출시 실행파일은 반드시 팀명으로 제출!


// remaining TODO's
//
//
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN_VALUE -1.0

#define MAX_BREADTH 10

#define C_PUCT 5

#define ZERO_ONE_LITTLE(n) ((n <= 1) ? n : (1+0.1*(n-1)))

int memcnt = 0;

char buf[128];

enum cell_state{
	EMPTY = 0,
	PLAYER,
	ENEMY,
	NEUTRAL
};

enum direction{
	LR = 0,
	UD,
	DD,
	DU
};

enum turn{
	PLAYER_FIRST = 0,
	PLAYER_SECOND,
	ENEMY_FIRST,
	ENEMY_SECOND
};

struct point{
	int row;
	int col;
	enum cell_state state;
	/* maybe 4 reference to associated line? (or value sources) */
	float value[4]; // function of n_player and n_enemy, idx: turn
	int n_player[4][7];
	int n_enemy[4][7];
	bool forbid_player; // no 7+ connection
	bool forbid_enemy;
	float prior_policy[4];
};

struct line{
	int length;
	enum direction dir;
	struct point * pts[19]; // 1D array of reference to associated points [row][col]
	int rank_player[14]; // array of size 'length - 5' or just null
	bool dead_player[14];
	int conv7_player[19 + 6]; // array of size 'length + 6' for storing [1111111] conv results

	int rank_enemy[14];
	bool dead_enemy[14];
	int conv7_enemy[19 + 6];

	/* later: may have to make a single value of line */
};

struct board{
	struct point pts[19][19];
	struct line LRlines[19]; // [0][:] ~ [18][:]
	struct line UDlines[19]; // [:][0] ~ [:][18]
	struct line DDlines[37]; // [18][0] ~ [0][18]
	struct line DUlines[37]; // [0][0] ~ [18][18]
	int empty_left;
};

struct stack_node{
	struct stack_node * below;
	struct board * saved_board;
};

struct dynamic_board{
	// board that can roll back moves
	struct board * current;
	int depth; // possible number of roll back (current stack size)
	struct stack_node * stack_top;
};

struct dynamic_board * dynamic_init(struct board * base);
void dynamic_put_player(struct dynamic_board * db, int i, int j);
void dynamic_put_enemy(struct dynamic_board * db, int i, int j);
void dynamic_rollback_one(struct dynamic_board * db);
void dynamic_push_current(struct dynamic_board * db);
void dynamic_free(struct dynamic_board * db);

float prior_board_value(struct board * b, enum turn turn);

void point_init(struct point * p, int row, int col){
	// initialize empty point
	for (int d = 0; d < 4; d++){
		for (int r = 0; r < 7; r++){
			p->n_player[d][r] = 0;
			p->n_enemy[d][r] = 0;
			p->state = EMPTY;
			for (int t = 0; t < 4; t++){
				p->value[t] = 0.0;
			}
			p->forbid_player = false;
			p->forbid_enemy = false;
			p->row = row;
			p->col = col;
		}
	}
}

void debug_log(char * buf){
	FILE * file = fopen("debug_log.txt", "a");
	fputs(buf, file);
	fclose(file);
}

void line_init(struct line * l, struct board * b, enum direction dir, int idx){

	if (dir == LR || dir == UD){
		l->length = 19;
	}
	else{

		// idx 0: length 1
		// idx 18: length 19
		// idx 36: length 1
		if (idx <= 18){
			l->length = idx + 1;

		}
		else{
			l->length = 37 - idx;
		}
	}

	l->dir = dir;

	if (dir == LR){
		for (int i = 0; i < 19; i++){
			l->pts[i] = &(b->pts[idx][i]);
		}
	}
	else if (dir == UD){
		for (int i = 0; i < 19; i++){
			l->pts[i] = &(b->pts[i][idx]);
		}
	}
	else if (dir == DD){

		int st_i, st_j;
		if (idx <= 18){
			st_i = 18 - idx;
			st_j = 0;
		}
		else{
			st_i = 0;
			st_j = idx - 18;
		}
		for (int i = 0; i < l->length; i++){
			l->pts[i] = &(b->pts[st_i + i][st_j + i]);
		}

	}
	else{
		int st_i, st_j;
		if (idx <= 18){
			st_i = idx;
			st_j = 0;
		}
		else{
			st_i = 18;
			st_j = idx - 18;
		}
		for (int i = 0; i < l->length; i++){
			l->pts[i] = &(b->pts[st_i - i][st_j + i]);
		}
	}

	if (l->length >= 6){
		for (int i = 0; i < l->length - 5; i++){
			l->rank_player[i] = 0;
			l->dead_player[i] = false;
			l->rank_enemy[i] = 0;
			l->dead_enemy[i] = false;
			for (int j = 0; j < 6; j++){
				l->pts[i + j]->n_player[dir][0]++;
				l->pts[i + j]->n_enemy[dir][0]++;
			}
		}
	}

	for (int i = 0; i < l->length + 6; i++){
		l->conv7_player[i] = 0;
		l->conv7_enemy[i] = 0;
	}
}

float inv_sq_sum(int n){
	float res = 0.0;
	for (int i = 1; i <= n; i++){
		res += 1 / (float(i)*float(i));
	}
	return res;
}

void compute_prior_policy(struct board * b){
	// sharpen (optional), normalize
	float sum[4];
	sum[0] = sum[1] = sum[2] = sum[3] = 0.01;
	struct point * p;
	for(int i = 0; i < 19; i++){
		for(int j = 0; j < 19; j++){
			p = &(b->pts[i][j]);
			if(p->state == EMPTY){
				if(!p->forbid_player){
					sum[0] += p->value[0];
					sum[1] += p->value[1];					
				}
				if(!b->pts[i][j].forbid_enemy){
					sum[2] += p->value[2];
					sum[3] += p->value[3];					
				}
			}
		}
	}
	for(int i = 0; i < 19; i++){
		for(int j = 0; j < 19; j++){
			p = &(b->pts[i][j]);
			p->prior_policy[0] = p->value[0]/sum[0];
			p->prior_policy[1] = p->value[1]/sum[1];
			p->prior_policy[2] = p->value[2]/sum[2];
			p->prior_policy[3] = p->value[3]/sum[3];
		}
	}
}


void reevaluate_point_m(float * r_m, float * r_y, float * first, float * second){
	// unprocessed 'policy' weight
	// make it 0 ~ 1

	// first : two stones
	if(r_m[4] > 0.5 || r_m[5] > 0.5){
		// certain win

		*first = 1.0;
	}
	else if(r_y[5] + r_y[4] > 0.5){
		// must block if no win
		*first = 0.9;
	}
	else{
		*first = -1.0; // temp
	}

	// second : one stone
	if(r_m[5] > 0.5){
		// certain win
		*second = 1.0;
	}
	else if(r_y[5] + r_y[4] > 0.5){
		// must block if no win
		*second = 0.9;
	}
	else{
		*second = -1.0; //temp
	}

	float pre_policy = 0.0; // > 0 => tanh => (0, 1)

	pre_policy += 0.002*r_m[0];
	pre_policy += 0.03*r_m[1];
	pre_policy += 0.1*r_m[2];
	pre_policy += 0.3*r_m[3];
	pre_policy += 0.5*r_m[4];
	// rank 5 will have max weight
	pre_policy += 0.002*r_y[0];
	pre_policy += 0.03*r_y[1];
	pre_policy += 0.1*r_y[2];
	pre_policy += 0.3*r_y[3];
	pre_policy += 0.5*r_y[4];

	float val = 0.5*tanh(pre_policy);
	if(*first < 0){
		*first = val;
	}
	if(*second < 0){
		*second = val;
	}
}

void reevaluate_point(struct point * p){
	float r_p[6];
	float r_e[6];
	for(int r = 0; r < 6; r++){
		r_p[r] = 0;
		r_e[r] = 0;
		for(int d = 0; d < 4; d++){
			r_p[r] += ZERO_ONE_LITTLE(p->n_player[d][r]);
			r_e[r] += ZERO_ONE_LITTLE(p->n_enemy[d][r]);
			// empty board center? : r_p[0] = 4*1.5 = 6
			// typical values 1.4
		}
	}
	
	reevaluate_point_m(r_p, r_e, &(p->value[PLAYER_FIRST]), &(p->value[PLAYER_SECOND]));
	reevaluate_point_m(r_e, r_p, &(p->value[ENEMY_FIRST]), &(p->value[ENEMY_SECOND]));
}

void reevaluate_point_old(struct point * p){
	// TODO: make heuristic value function

	// simple weighted sum?
	float total_value = 0.0;
	for (int d = 0; d < 4; d++){
		for (int r = 0; r < 7; r++){
			total_value += (r + 1)*(r + 1)*inv_sq_sum(p->n_player[d][r]);
			total_value += (r + 1)*(r)*inv_sq_sum(p->n_enemy[d][r]);
		}
	}

	total_value *= 0.01;

	if (total_value <= MIN_VALUE){
		printf("LOWER THAN MIN VALUE!!!\n");
	}

	// very naive heuristic : check certain wins

	for (int t = 0; t < 4; t++){
		p->value[t] = 0;
	}
	for (int r = 4; r <= 6; r++){
		for (int d = 0; d < 4; d++){
			if (p->n_player[d][r] > 0){
				//printf("certain player win!\n");
				p->value[PLAYER_FIRST] = 7; // certain win
				p->value[ENEMY_FIRST] = 5; // not win then lose
				p->value[ENEMY_SECOND] = 5;
				if (r >= 5){
					p->value[PLAYER_SECOND] = 7; // certain win
				}
			}
		}
	}
	for (int r = 4; r <= 6; r++){
		for (int d = 0; d < 4; d++){
			if (p->n_enemy[d][r] > 0){
				p->value[ENEMY_FIRST] = 7; // certain win
				p->value[PLAYER_FIRST] = 5; // not win then lose
				p->value[PLAYER_SECOND] = 5;
				if (r >= 5){
					p->value[ENEMY_SECOND] = 7; // certain win
				}
			}
		}
	}
	for (int t = 0; t < 4; t++){
		p->value[t] += total_value;
	}

}



void board_init(struct board * b){
	// intiialize empty board

	//init points
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			point_init(&(b->pts[i][j]), i, j);
		}
	}

	//init lines
	for (int i = 0; i < 19; i++){
		line_init(&(b->LRlines[i]), b, LR, i);
		line_init(&(b->UDlines[i]), b, UD, i);
	}
	for (int i = 0; i < 37; i++){
		line_init(&(b->DDlines[i]), b, DD, i);
		line_init(&(b->DUlines[i]), b, DU, i);
	}

	//evaluate points
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			reevaluate_point(&(b->pts[i][j]));
		}
	}

	b->empty_left = 19*19;
}


struct line * get_associated_line(struct board * b, int i, int j, enum direction dir, int * idx){
	struct line * l;
	if (dir == LR){
		*idx = j;
		l = &(b->LRlines[i]);
	}
	else if (dir == UD){
		*idx = i;
		l = &(b->UDlines[j]);
	}
	else if (dir == DD){
		if (i >= j){
			*idx = j;
		}
		else{
			*idx = i;
		}
		l = &(b->DDlines[18 + j - i]);
	}
	else{
		if (i + j <= 18){
			*idx = j;
		}
		else{
			*idx = 18 - i;
		}
		l = &(b->DUlines[i + j]);
	}
	return l;
}

void display_rank(struct board * b){
	printf("PLAYER\n");
	for (int r = 4; r <= 4; r++){
		printf("[RANK %d]\n", r);
		for (int d = 0; d < 4; d++){
			printf("direction %d:\n", d);
			for (int i = 0; i < 19; i++){
				for (int j = 0; j < 19; j++){
					printf("%d ", b->pts[i][j].n_player[d][r]);
				}
				printf("\n");
			}
		}
	}
	/*
	printf("ENEMY\n");
	for(int r = 0; r <= 1; r++){
	printf("[RANK %d]\n", r);
	for(int d = 0; d < 4; d++){
	printf("direction %d:\n", d);
	for(int i = 0; i < 19; i++){
	for(int j = 0; j < 19; j++){
	printf("%d ", b->pts[i][j].n_enemy[d][r]);
	}
	printf("\n");
	}
	}
	}
	*/
}

void display_value(struct board * b){
	struct point * p;
	printf("PLAYER_FIRST VALUE\n");
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			p = &(b->pts[i][j]);
			if (p->state == PLAYER){
				printf("  P   ");
			}
			else if (p->state == ENEMY){
				printf("  E   ");
			}
			else if (p->state == NEUTRAL){
				printf("  N   ");
			}
			else if (p->forbid_player){
				printf("  x   ");
			}
			else{
				printf("%5.2f ", p->value[PLAYER_SECOND]);
			}
		}
		printf("\n");
	}
}


void display_board_transpose(struct board * b){
	struct point * p;
	printf("BOARD\n");
	for (int j = 0; j < 19; j++){
		for (int i = 0; i < 19; i++){
			p = &(b->pts[i][j]);
			if (p->state == PLAYER){
				printf("  P ");
			}
			else if (p->state == ENEMY){
				printf("  E ");
			}
			else if (p->state == NEUTRAL){
				printf("  N ");
			}
			else if (p->forbid_player){
				printf("  x ");
			}
			else{
				printf("  . ");
			}
		}
		printf("\n");
	}	
}

void display_board(struct board * b){
	struct point * p;
	printf("BOARD\n");
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			p = &(b->pts[i][j]);
			if (p->state == PLAYER){
				printf("  P ");
			}
			else if (p->state == ENEMY){
				printf("  E ");
			}
			else if (p->state == NEUTRAL){
				printf("  N ");
			}
			else if (p->forbid_player){
				printf("  x ");
			}
			else{
				printf("  . ");
			}
		}
		printf("\n");
	}	
}

void mark_point(struct board * b, int i, int j){
	int idx;
	struct line * l;
	for (int d = 0; d < 4; d++){
		l = get_associated_line(b, i, j, (enum direction)d, &idx);
		l->pts[idx]->n_player[d][0] = 9;
	}
}


void forbid_in_line_player(struct line * l, int pos){
	// pretty much same as enemy stone for player

	int st, ed;
	int p_update[7];
	bool p_killed[11];
	for (int r = 0; r < 7; r++){
		p_update[r] = 0;
	}

	// player rank, n update
	st = pos - 5;
	if (st < 0){
		st = 0;
	}
	ed = pos + 5;
	if (ed > l->length - 1){
		ed = l->length - 1;
	}
	for (int i = st; i <= ed; i++){
		p_killed[i - st] = false;
		if (i + 5 <= ed){
			// dead chunks
			if (l->dead_player[i]){
				// already dead. do nothing
			}
			else{
				// is now dead
				p_killed[i - st] = true;
				l->dead_player[i] = true;
				p_update[l->rank_player[i]]--;
			}
		}
		// cancel old chunk change if any.
		int k = i - 6;
		if (k >= st){
			if (p_killed[k - st]){
				p_update[l->rank_player[k]]++;
			}
		}
		// apply update
		for (int r = 0; r < 7; r++){
			l->pts[i]->n_player[l->dir][r] += p_update[r];
		}
	}
}


void forbid_in_line_enemy(struct line * l, int pos){
	// pretty much same as player stone for enemy

	int st, ed;
	int e_update[7];
	bool e_killed[11];
	for (int r = 0; r < 7; r++){
		e_update[r] = 0;
	}

	// enemy rank, n update
	st = pos - 5;
	if (st < 0){
		st = 0;
	}
	ed = pos + 5;
	if (ed > l->length - 1){
		ed = l->length - 1;
	}
	for (int i = st; i <= ed; i++){
		e_killed[i - st] = false;
		if (i + 5 <= ed){
			// dead chunks
			if (l->dead_enemy[i]){
				// already dead. do nothing
			}
			else{
				// is now dead
				e_killed[i - st] = true;
				l->dead_enemy[i] = true;
				e_update[l->rank_enemy[i]]--;
			}
		}
		// cancel old chunk change if any.
		int k = i - 6;
		if (k >= st){
			if (e_killed[k - st]){
				e_update[l->rank_enemy[k]]++;
			}
		}
		// apply update
		for (int r = 0; r < 7; r++){
			l->pts[i]->n_enemy[l->dir][r] += e_update[r];
		}
	}
}


void forbid_point_player(struct board * b, struct point * p){
	struct line * l;
	int idx;
	if (p->state != EMPTY || p->forbid_player){
		return;
	}
	p->forbid_player = true;
	for (int d = 0; d < 4; d++){
		l = get_associated_line(b, p->row, p->col, (enum direction)d, &idx);
		if (l->length >= 6){
			forbid_in_line_player(l, idx);
		}
	}
}

void forbid_point_enemy(struct board * b, struct point * p){
	struct line * l;
	int idx;
	if (p->state != EMPTY || p->forbid_enemy){
		return;
	}
	p->forbid_enemy = true;
	for (int d = 0; d < 4; d++){
		l = get_associated_line(b, p->row, p->col, (enum direction)d, &idx);
		if (l->length >= 6){
			forbid_in_line_enemy(l, idx);
		}
	}
}


void line_put_player(struct board * b, struct line * l, int pos){
	int st, ed;

	int p_update[7];
	bool p_killed[13];
	int e_update[7];
	bool e_killed[11];
	for (int r = 0; r < 7; r++){
		p_update[r] = 0;
		e_update[r] = 0;
	}

	// enemy rank, n update
	st = pos - 5;
	if (st < 0){
		st = 0;
	}
	ed = pos + 5;
	if (ed > l->length - 1){
		ed = l->length - 1;
	}
	for (int i = st; i <= ed; i++){
		e_killed[i - st] = false;
		if (i + 5 <= ed){
			// dead chunks
			if (l->dead_enemy[i]){
				// already dead. do nothing
			}
			else{
				// is now dead
				e_killed[i - st] = true;
				l->dead_enemy[i] = true;
				e_update[l->rank_enemy[i]]--;
			}
		}
		// cancel old chunk change if any.
		int k = i - 6;
		if (k >= st){
			if (e_killed[k - st]){
				e_update[l->rank_enemy[k]]++;
			}
		}
		// apply update
		for (int r = 0; r < 7; r++){
			l->pts[i]->n_enemy[l->dir][r] += e_update[r];
		}
	}

	// player rank, n update
	st = pos - 6;
	if (st < 0){
		st = 0;
	}
	ed = pos + 6;
	if (ed > l->length - 1){
		ed = l->length - 1;
	}
	for (int i = st; i <= ed; i++){
		p_killed[i - st] = false;
		if (i + 5 <= ed){
			if (i == pos - 6 || i == pos + 1){
				// dead chunk (7+ connect)
				if (l->dead_player[i]){
					// already dead. do nothing
				}
				else{
					// now dead
					p_killed[i - st] = true;
					l->dead_player[i] = true;
					p_update[l->rank_player[i]]--;
				}
			}
			if (i >= pos - 5 && i <= pos){
				// rank + 1
				if (l->dead_player[i]){
					// dead, do nothing
				}
				else{
					p_update[l->rank_player[i]]--;
					l->rank_player[i]++;
					p_update[l->rank_player[i]]++;
				}
			}
		}
		// cancel old chunk change if any
		int k = i - 6;
		if (k >= st){
			if (k == pos - 6 || k == pos + 1){
				if (p_killed[k - st]){
					p_update[l->rank_player[k]]++;
				}
			}
			if (k >= pos - 5 && k <= pos){
				if (l->dead_player[k]){
				}
				else{
					p_update[l->rank_player[k]]--;
					p_update[l->rank_player[k] - 1]++;
				}
			}
		}
		// apply update
		for (int r = 0; r < 7; r++){
			l->pts[i]->n_player[l->dir][r] += p_update[r];
		}

		reevaluate_point(l->pts[i]);
	}

	for (int n = 0; n < 7; n++){
		l->conv7_player[pos + n]++;
		if (l->conv7_player[pos + n] == 6){
			for (int i = 0; i < 7; i++){
				if (pos + n - i >= 0 && pos + n - i < l->length){
					forbid_point_player(b, l->pts[pos + n - i]);
				}
			}
		}
	}
}




void board_put_player(struct board * b, int i, int j){
	int idx;
	struct line * l;

	if (b->pts[i][j].state != EMPTY){
		printf("Cell Not Empty! (%d, %d)\n", i, j);
		return;
	}
	if (b->pts[i][j].forbid_player){
		printf("Cell Forbidden!\n");
		return;
	}
	b->pts[i][j].state = PLAYER;
	b->empty_left--;
	// TODO : may have to pop out from ordered list of empty points

	for (int d = 0; d < 4; d++){
		l = get_associated_line(b, i, j, (enum direction)d, &idx);
		if (l->length >= 6){
			line_put_player(b, l, idx);
		}
	}
}

void line_put_enemy(struct board * b, struct line * l, int pos){
	int st, ed;

	int e_update[7];
	bool e_killed[13];
	int p_update[7];
	bool p_killed[11];
	for (int r = 0; r < 7; r++){
		e_update[r] = 0;
		p_update[r] = 0;
	}

	// player rank, n update
	st = pos - 5;
	if (st < 0){
		st = 0;
	}
	ed = pos + 5;
	if (ed > l->length - 1){
		ed = l->length - 1;
	}
	for (int i = st; i <= ed; i++){
		p_killed[i - st] = false;
		if (i + 5 <= ed){
			// dead chunks
			if (l->dead_player[i]){
				// already dead. do nothing
			}
			else{
				// is now dead
				p_killed[i - st] = true;
				l->dead_player[i] = true;
				p_update[l->rank_player[i]]--;
			}
		}
		// cancel old chunk change if any.
		int k = i - 6;
		if (k >= st){
			if (p_killed[k - st]){
				p_update[l->rank_player[k]]++;
			}
		}
		// apply update
		for (int r = 0; r < 7; r++){
			l->pts[i]->n_player[l->dir][r] += p_update[r];
		}
	}

	// enemy rank, n update
	st = pos - 6;
	if (st < 0){
		st = 0;
	}
	ed = pos + 6;
	if (ed > l->length - 1){
		ed = l->length - 1;
	}
	for (int i = st; i <= ed; i++){
		e_killed[i - st] = false;
		if (i + 5 <= ed){
			if (i == pos - 6 || i == pos + 1){
				// dead chunk (7+ connect)
				if (l->dead_enemy[i]){
					// already dead. do nothing
				}
				else{
					// now dead
					e_killed[i - st] = true;
					l->dead_enemy[i] = true;
					e_update[l->rank_enemy[i]]--;
				}
			}
			if (i >= pos - 5 && i <= pos){
				// rank + 1
				if (l->dead_enemy[i]){
					// dead, do nothing
				}
				else{
					e_update[l->rank_enemy[i]]--;
					l->rank_enemy[i]++;
					e_update[l->rank_enemy[i]]++;
				}
			}
		}
		// cancel old chunk change if any
		int k = i - 6;
		if (k >= st){
			if (k == pos - 6 || k == pos + 1){
				if (e_killed[k - st]){
					e_update[l->rank_enemy[k]]++;
				}
			}
			if (k >= pos - 5 && k <= pos){
				if (l->dead_enemy[k]){
				}
				else{
					e_update[l->rank_enemy[k]]--;
					e_update[l->rank_enemy[k] - 1]++;
				}
			}
		}
		// apply update
		for (int r = 0; r < 7; r++){
			l->pts[i]->n_enemy[l->dir][r] += e_update[r];
		}
		reevaluate_point(l->pts[i]);
	}

	for (int n = 0; n < 7; n++){
		l->conv7_enemy[pos + n]++;
		if (l->conv7_enemy[pos + n] == 6){
			for (int i = 0; i < 7; i++){
				if (pos + n - i >= 0 && pos + n - i < l->length){
					forbid_point_enemy(b, l->pts[pos + n - i]);
				}
			}
		}
	}

}

void board_put_enemy(struct board * b, int i, int j){
	int idx;
	struct line * l;

	if (b->pts[i][j].state != EMPTY){
		printf("Cell Not Empty!\n");
		return;
	}
	if (b->pts[i][j].forbid_enemy){
		printf("Cell Forbidden!\n");
		return;
	}
	b->empty_left--;
	b->pts[i][j].state = ENEMY;
	// TODO : may have to pop out from ordered list of empty points

	for (int d = 0; d < 4; d++){
		l = get_associated_line(b, i, j, (enum direction)d, &idx);
		if (l->length >= 6){
			line_put_enemy(b, l, idx);
		}
	}
}





void line_put_neutral(struct board * b, struct line * l, int pos){

	int st, ed;

	int p_update[7];
	bool p_killed[13];
	int e_update[7];
	bool e_killed[13];
	for (int r = 0; r < 7; r++){
		p_update[r] = 0;
		e_update[r] = 0;
	}

	// player rank, n update
	st = pos - 6;
	if (st < 0){
		st = 0;
	}
	ed = pos + 6;
	if (ed > l->length - 1){
		ed = l->length - 1;
	}
	for (int i = st; i <= ed; i++){
		p_killed[i - st] = false;
		if (i + 5 <= ed){
			if (i == pos - 6 || i == pos + 1){
				// dead chunk (7+ connect)
				if (l->dead_player[i]){
					// already dead. do nothing
				}
				else{
					// now dead
					p_killed[i - st] = true;
					l->dead_player[i] = true;
					p_update[l->rank_player[i]]--;
				}
			}
			if (i >= pos - 5 && i <= pos){
				// rank + 1
				if (l->dead_player[i]){
					// dead, do nothing
				}
				else{
					p_update[l->rank_player[i]]--;
					l->rank_player[i]++;
					p_update[l->rank_player[i]]++;
				}
			}
		}
		// cancel old chunk change if any
		int k = i - 6;
		if (k >= st){
			if (k == pos - 6 || k == pos + 1){
				if (p_killed[k - st]){
					p_update[l->rank_player[k]]++;
				}
			}
			if (k >= pos - 5 && k <= pos){
				if (l->dead_player[k]){
				}
				else{
					p_update[l->rank_player[k]]--;
					p_update[l->rank_player[k] - 1]++;
				}
			}
		}
		// apply update
		for (int r = 0; r < 7; r++){
			l->pts[i]->n_player[l->dir][r] += p_update[r];
		}
		reevaluate_point(l->pts[i]);
	}



	// enemy rank, n update
	st = pos - 6;
	if (st < 0){
		st = 0;
	}
	ed = pos + 6;
	if (ed > l->length - 1){
		ed = l->length - 1;
	}
	for (int i = st; i <= ed; i++){
		e_killed[i - st] = false;
		if (i + 5 <= ed){
			if (i == pos - 6 || i == pos + 1){
				// dead chunk (7+ connect)
				if (l->dead_enemy[i]){
					// already dead. do nothing
				}
				else{
					// now dead
					e_killed[i - st] = true;
					l->dead_enemy[i] = true;
					e_update[l->rank_enemy[i]]--;
				}
			}
			if (i >= pos - 5 && i <= pos){
				// rank + 1
				if (l->dead_enemy[i]){
					// dead, do nothing
				}
				else{
					e_update[l->rank_enemy[i]]--;
					l->rank_enemy[i]++;
					e_update[l->rank_enemy[i]]++;
				}
			}
		}
		// cancel old chunk change if any
		int k = i - 6;
		if (k >= st){
			if (k == pos - 6 || k == pos + 1){
				if (e_killed[k - st]){
					e_update[l->rank_enemy[k]]++;
				}
			}
			if (k >= pos - 5 && k <= pos){
				if (l->dead_enemy[k]){
				}
				else{
					e_update[l->rank_enemy[k]]--;
					e_update[l->rank_enemy[k] - 1]++;
				}
			}
		}
		// apply update
		for (int r = 0; r < 7; r++){
			l->pts[i]->n_enemy[l->dir][r] += e_update[r];
		}
		reevaluate_point(l->pts[i]);
	}

	for (int n = 0; n < 7; n++){
		l->conv7_player[pos + n]++;
		if (l->conv7_player[pos + n] == 6){
			for (int i = 0; i < 7; i++){
				if (pos + n - i >= 0 && pos + n - i < l->length){
					forbid_point_player(b, l->pts[pos + n - i]);
				}
			}
		}
	}
	for (int n = 0; n < 7; n++){
		l->conv7_enemy[pos + n]++;
		if (l->conv7_enemy[pos + n] == 6){
			for (int i = 0; i < 7; i++){
				if (pos + n - i >= 0 && pos + n - i < l->length){
					forbid_point_enemy(b, l->pts[pos + n - i]);
				}
			}
		}
	}

}



void board_put_neutral(struct board * b, int i, int j){
	int idx;
	struct line * l;

	if (b->pts[i][j].state != EMPTY){
		printf("Cell Not Empty!\n");
		return;
	}
	b->empty_left--;
	b->pts[i][j].state = NEUTRAL;
	// TODO : may have to pop out from ordered list of empty points

	for (int d = 0; d < 4; d++){
		l = get_associated_line(b, i, j, (enum direction)d, &idx);
		if (l->length >= 6){
			line_put_neutral(b, l, idx);
		}
	}
}

void find_max_position(struct board * b, int * bi, int * bj, enum turn turn){
	int best_i = -1, best_j = -1;
	float best_value = MIN_VALUE;
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			if (b->pts[i][j].state == EMPTY && (!(b->pts[i][j].forbid_player))){
				if (b->pts[i][j].value[turn] > best_value){
					best_i = i;
					best_j = j;
					best_value = b->pts[i][j].value[turn];
				}
			}
		}
	}
	*bi = best_i;
	*bj = best_j;
}

void fix_references(struct board * b){
	// only used for initializing dynamic current board
	long long ofs; // NOTE: careful abt system dependency
	ofs = ((char *)(&(b->pts[0][0])) - (char *)(b->LRlines[0].pts[0]));
	for (int idx = 0; idx < 19; idx++){
		for (int pos = 0; pos < 19; pos++){
			b->LRlines[idx].pts[pos] = (struct point *)((char *)(b->LRlines[idx].pts[pos]) + ofs);
			b->UDlines[idx].pts[pos] = (struct point *)((char *)(b->UDlines[idx].pts[pos]) + ofs);
		}
	}
	for (int idx = 0; idx < 37; idx++){
		for (int pos = 0; pos < 19; pos++){
			b->DDlines[idx].pts[pos] = (struct point *)((char *)(b->DDlines[idx].pts[pos]) + ofs);
			b->DUlines[idx].pts[pos] = (struct point *)((char *)(b->DUlines[idx].pts[pos]) + ofs);
		}
	}
}

struct dynamic_board * dynamic_init(struct board * base){
	struct dynamic_board * db = (struct dynamic_board *)malloc(sizeof(struct dynamic_board));
	db->depth = 0;
	db->stack_top = NULL;
	db->current = (struct board *)malloc(sizeof(struct board));
	memcpy(db->current, base, sizeof(struct board));
	fix_references(db->current);
	return db;
}

void dynamic_push_current(struct dynamic_board * db){
	/* TODO: guarantee memory limit */
	struct stack_node * sn;
	sn = (struct stack_node *)malloc(sizeof(struct stack_node));
	sn->saved_board = (struct board *)malloc(sizeof(struct board)); // NOTE: l->pts refers to current board
	memcpy(sn->saved_board, db->current, sizeof(struct board));
	sn->below = db->stack_top;
	db->stack_top = sn;
	db->depth++;
}

void dynamic_rollback_one(struct dynamic_board * db){
	struct stack_node * next_top;
	if (db->depth == 0){
		printf("Cannot rollback further!\n");
	}
	memcpy(db->current, db->stack_top->saved_board, sizeof(struct board));
	next_top = db->stack_top->below;
	free(db->stack_top->saved_board);
	free(db->stack_top);
	db->stack_top = next_top;
	db->depth--;
}

void dynamic_put_player(struct dynamic_board * db, int i, int j){
	dynamic_push_current(db);
	board_put_player(db->current, i, j);
}

void dynamic_put_enemy(struct dynamic_board * db, int i, int j){
	dynamic_push_current(db);
	board_put_enemy(db->current, i, j);
}

void dynamic_free(struct dynamic_board * db){
	struct stack_node * sn;
	struct stack_node * next_sn;
	for (sn = db->stack_top; sn != NULL; sn = next_sn){
		next_sn = sn->below;
		free(sn->saved_board);
		free(sn);
	}
	free(db->current);
	free(db);
}




bool has_rank_ge_n_player(struct board * b, int n){
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			for (int d = 0; d < 4; d++){
				for (int r = n; r <= 6; r++){
					if (b->pts[i][j].n_player[d][r] > 0){
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool has_rank_ge_n_enemy(struct board * b, int n){
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			for (int d = 0; d < 4; d++){
				for (int r = n; r <= 6; r++){
					if (b->pts[i][j].n_enemy[d][r] > 0){
						return true;
					}
				}
			}
		}
	}
	return false;
}

float prior_board_value_old(struct board * b, enum turn turn){
	// naive winning probability {-1.2 ~ (-1 ~ 1) ~ 1.2} estimation for player;
	// TODO: local recompututation after single action (consider forbid 7)
	// TODO: make good heuristic

	// check for definite wins
	if (turn == PLAYER_FIRST){
		if (has_rank_ge_n_player(b, 4)){
			return 1.2;
		}
	}
	if (turn == PLAYER_SECOND){
		if (has_rank_ge_n_player(b, 5)){
			return 1.2;
		}
	}
	if (turn == ENEMY_FIRST){
		if (has_rank_ge_n_enemy(b, 4)){
			return -1.2;
		}
	}
	if (turn == ENEMY_SECOND){
		if (has_rank_ge_n_enemy(b, 5)){
			return -1.2;
		}
	}

	// estimation
	float res = 0.0;
	if (has_rank_ge_n_player(b, 3)){
		res += 0.1;
	}
	if (has_rank_ge_n_enemy(b, 3)){
		res -= 0.1;
	}
	if (has_rank_ge_n_player(b, 4)){
		res += 0.1;
	}
	if (has_rank_ge_n_enemy(b, 4)){
		res -= 0.1;
	}
	return res;
}


int find_max_n_pts(struct board * b, int n, int * is, int * js, enum turn turn){
	// TODO: optimize with list? (statically allocated in struct board / or handle rollback and freeing)
	int found = 0;
	bool newly_found;
	struct point * p;

	int best_i;
	int best_j;
	bool already_found;
	while (found < n){
		newly_found = false;
		float best_value = MIN_VALUE; // not a winning probablility (may be valuable for both player and enemy)
		for (int i = 0; i < 19; i++){
			for (int j = 0; j < 19; j++){
				p = &(b->pts[i][j]);
				if (p->state != EMPTY){
					continue;
				}
				if (turn == PLAYER_FIRST || turn == PLAYER_SECOND){
					if (p->forbid_player){
						continue;
					}
				}
				if (turn == ENEMY_FIRST || turn == ENEMY_SECOND){
					if (p->forbid_enemy){
						continue;
					}
				}
				if (p->value[turn] > best_value){
					already_found = false;
					for (int k = 0; k < found; k++){
						if (is[k] == i && js[k] == j){
							already_found = true;
							break;
						}
					}
					if (already_found){
						continue;
					}
					newly_found = true;
					best_value = p->value[turn];
					best_i = i;
					best_j = j;
				}
			}
		}
		if (newly_found){
			is[found] = best_i;
			js[found] = best_j;
			found++;
		}
		else{
			// not enough empty points
			return found;
		}
	}
	return found;
}

float find_best_move(struct dynamic_board * db, int * i1, int * j1, int * i2, int * j2, const int breadth, const int depth, enum turn turn){
	// return estimated winning probability with search
	if (depth == 0){
		return prior_board_value(db->current, turn);
	}
	int is[MAX_BREADTH];
	int js[MAX_BREADTH];
	int found;
	found = find_max_n_pts(db->current, breadth, is, js, turn);

	// (i2, j2) is best move at lext level
	int best_i;
	int best_j;
	int best_next_i;
	int best_next_j;
	int i3, j3, i4, j4;

	float best_win_prob;
	float wp;

	if (turn == PLAYER_FIRST || turn == PLAYER_SECOND){
		best_win_prob = -1.3;
		for (int n = 0; n < found; n++){
			dynamic_put_player(db, is[n], js[n]);
			wp = find_best_move(db, &i3, &j3, &i4, &j4, breadth, depth - 1, (enum turn)((turn + 1) % 4));
			if (wp > best_win_prob){
				best_win_prob = wp;
				best_i = is[n];
				best_j = js[n];
				best_next_i = i3;
				best_next_j = j3;
			}
			dynamic_rollback_one(db);
		}
	}
	if (turn == ENEMY_FIRST || turn == ENEMY_SECOND){
		best_win_prob = 1.3;
		for (int n = 0; n < found; n++){
			dynamic_put_enemy(db, is[n], js[n]);
			wp = find_best_move(db, &i3, &j3, &i4, &j4, breadth, depth - 1, (enum turn)((turn + 1) % 4));
			if (wp < best_win_prob){
				best_win_prob = wp;
				best_i = is[n];
				best_j = js[n];
				best_next_i = i3;
				best_next_j = j3;
			}
			dynamic_rollback_one(db);
		}
	}
	*i1 = best_i;
	*j1 = best_j;
	*i2 = best_next_i;
	*j2 = best_next_j;

	return best_win_prob;
}

void fixed_tree_search(struct board * b, int * i1, int * j1, int * i2, int * j2, int cnt){
	// minimax tree search. top 5 pts, depth 4 search (evaluate 625 board states)
	int breadth = 5;
	int depth = 4;
	struct dynamic_board * db = dynamic_init(b);
	if (cnt == 1){
		find_best_move(db, i1, j1, i2, j2, breadth, depth, PLAYER_SECOND);
	}
	if (cnt == 2){
		find_best_move(db, i1, j1, i2, j2, breadth, depth, PLAYER_FIRST);
	}
	dynamic_free(db);
}

enum terminal_info{
	NOT_TERMINAL,
	PLAYER_WIN,
	ENEMY_WIN,
	TIE,
	UNVERIFIED
};

struct mcts_edge{
	bool valid;
	int pos_i;
	int pos_j;
	int N; // visit count
	float W; // total action value
	float Q; // mean action value, wrt turn of this edge
	float P; // prior probability
	struct mcts_node * result_node;
	struct mcts_edge * upper_edge;
	enum terminal_info terminal;
};

struct mcts_node{
	float v;
	struct mcts_edge edges[19][19]; // array of all possible moves
};

struct mcts_tree{
	struct mcts_node * root;
};

struct mcts_edge * mcts_select_edge(struct mcts_node * node, enum turn turn){
	struct mcts_edge * e;
	struct mcts_edge * best_edge = NULL;
	float best_QU = -1.3 + 0;
	float QU;
	int total_n = 1; // because everything becomes 0 if this is 0 
	for(int i = 0; i < 19; i++){
		for(int j = 0; j < 19; j++){
			e = &(node->edges[i][j]);
			if(!e->valid){
				continue;
			}
			total_n += e->N;
		}
	}

	for(int i = 0; i < 19; i++){
		for(int j = 0; j < 19; j++){
			e = &(node->edges[i][j]);
			if(!e->valid){
				continue;
			}
			QU = e->Q + (C_PUCT*e->P*sqrt(total_n)/(1 + e->N));
			if(QU > best_QU){
				best_QU = QU;
				best_edge = e;
			}
		}
	}
	//printf("best QU: %f\n", best_QU);

	return best_edge;
}

void mcts_init_edge(struct mcts_edge * e, int i, int j, float prior, struct mcts_edge * upper){
	e->valid = true;
	e->upper_edge = upper;
	e->pos_i = i;
	e->pos_j = j;
	e->terminal = UNVERIFIED;
	e->N = 0;
	e->W = 0;
	e->Q = 0;
	e->P = prior;
	e->result_node = NULL; // not yet expanded
}

struct mcts_node * mcts_expand_node(struct board * b, enum turn turn, struct mcts_edge * upper){
	// for root, turn = PLAYER_FIRST
	struct mcts_node * node = (struct mcts_node *)malloc(sizeof(struct mcts_node));
	if(node == NULL){
		printf("malloc fail!\n");
	}
	memcnt++;
	float prior;
	bool forbidden;
	compute_prior_policy(b);
	for(int i = 0; i < 19; i++){
		for(int j = 0; j < 19; j++){
			if(turn == PLAYER_FIRST || turn == PLAYER_SECOND){
				forbidden = b->pts[i][j].forbid_player;
			}
			else{
				forbidden = b->pts[i][j].forbid_enemy;
			}
			if(b->pts[i][j].state != EMPTY || forbidden){
				node->edges[i][j].valid = false;
			}
			else{
				prior = b->pts[i][j].value[turn];
				//printf("%.4f", prior)
				mcts_init_edge(&(node->edges[i][j]), i, j, prior, upper);
			}
		}
	}
	return node;
}

void mcts_display(struct board * b, mcts_node * node){
	struct point * p;
	printf("BOARD\n");
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			p = &(b->pts[i][j]);
			if (p->state == PLAYER){
				printf("     ");
				printf("  P ");
			}
			else if (p->state == ENEMY){
				printf("     ");
				printf("  E ");
			}
			else if (p->state == NEUTRAL){
				printf("     ");
				printf("  N ");
			}
			else if (p->forbid_player){
				printf("     ");
				printf("  x ");
			}
			else{
				printf(" %3d:%4.2f", node->edges[i][j].N, 100*node->edges[i][j].P);
				//printf(" %3d:%4.2f", node->edges[i][j].N, 100*node->edges[i][j].Q);
			}
		}
		printf("\n");
	}	
}


void mcts_display_transpose(struct board * b, mcts_node * node){
	struct point * p;
	printf("BOARD\n");
	for (int j = 0; j < 19; j++){
		for (int i = 0; i < 19; i++){
			p = &(b->pts[i][j]);
			if (p->state == PLAYER){
				printf("     ");
				printf("  P ");
			}
			else if (p->state == ENEMY){
				printf("     ");
				printf("  E ");
			}
			else if (p->state == NEUTRAL){
				printf("     ");
				printf("  N ");
			}
			else if (p->forbid_player){
				printf("     ");
				printf("  x ");
			}
			else{
				printf(" %3d:%4.2f", node->edges[i][j].N, node->edges[i][j].P);
				//printf(" %3d:%2.2f", node->edges[i][j].N, node->edges[i][j].Q);
			}
		}
		printf("\n");
	}	
}

bool feq(float a, float b){
	return (a-b)<0.01 && (b-a)<0.01;
}

void mcts_free(struct mcts_node * node){
	struct mcts_edge * e;
	if (node == NULL){
		return;
	}
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			e = &(node->edges[i][j]);
			if (e->valid){
				mcts_free(e->result_node);
			}
		}
	}
	free(node);
}

bool mcts_check_tree(struct mcts_node * node, int depth, float * total_W, int * total_N, int * ilog, int * jlog){
	struct mcts_edge * e;
	int total_n;
	float total_w;
	int part_n;
	float part_w;
	
	if(node == NULL){
		*total_W = 0;
		*total_N = 0;
		return true;
	}

	total_n = 1;
	total_w = node->v;
	for(int i = 0; i < 19; i++){
		for(int j = 0; j < 19; j++){
			e = &(node->edges[i][j]);
			if(e->valid){
				total_n += e->N;
				total_w += e->W;
				ilog[depth] = i;
				jlog[depth] = j;
				if(!mcts_check_tree(e->result_node, depth + 1, &part_w, &part_n, ilog, jlog)){
					return false;
				}
				if(part_n != e->N && e->terminal == NOT_TERMINAL){
					printf("part_n (%d) VS e->N (%d) at depth %d\n", part_n, e->N, depth);
					for(int c = 0; c <= depth; c++){
						printf("(%d, %d) ", ilog[c], jlog[c]);
					}
					printf("\n");
					return false;
				}
			}
		}
	}
	*total_W = total_w;
	*total_N = total_n;
	return true;
}


void mcts_find_best_move(struct board * b, int * i1, int * j1, int * i2, int * j2, int cnt){
	
	struct mcts_tree tree;
	struct board saved_board;
	enum turn turn;
	memcpy(&saved_board, b, sizeof(struct board)); // save root board state

	struct mcts_node * node;
	struct mcts_edge * e;
	
	bool timeout = false;
	float added_value;
	enum turn init_turn;
	if(cnt == 1){
		init_turn = PLAYER_SECOND;
	}
	else{
		init_turn = PLAYER_FIRST;
	}

	tree.root = mcts_expand_node(b, init_turn, NULL);
	int depth = 0;
	int time = 0;
	struct mcts_edge * sel_e;
	while(true){
		// new mcts step
		node = tree.root;
		turn = init_turn;
		// selection
		while(true){
			sel_e = mcts_select_edge(node, turn);
			if(sel_e == NULL){
				if(b->empty_left == 0){
					// tie
					e->terminal = TIE;
				}
				else if(turn == PLAYER_FIRST || turn == PLAYER_SECOND){
					// player lost state!
					e->terminal = ENEMY_WIN;
				}
				else{
					// enemy lost state!
					e->terminal = PLAYER_WIN;
				}
				break;
				// no depth/turn increase
			}
			else{
				e = sel_e;
			}
			if(turn == PLAYER_FIRST || turn == PLAYER_SECOND){
				board_put_player(b, e->pos_i, e->pos_j);
			}
			else{
				board_put_enemy(b, e->pos_i, e->pos_j);
			}
			node = e->result_node;
			turn = (enum turn)((turn+1)%4);
			depth++;
			if(node == NULL){
				break;
			}
		}

		// expansion
		if(e->terminal == UNVERIFIED){
			if(has_rank_ge_n_player(b, 6)){
				e->terminal = PLAYER_WIN;
			}
			else if(has_rank_ge_n_enemy(b, 6)){
				e->terminal = ENEMY_WIN;
			}
			else{
				e->terminal = NOT_TERMINAL;
			}
		}

		if(e->terminal == NOT_TERMINAL){
			e->result_node = mcts_expand_node(b, turn, e); // child of 'e' is move of 'turn'	
			added_value = prior_board_value(b, turn); // (-1 ~ 1) wrt player
		}
		else if(e->terminal == PLAYER_WIN){
			added_value = 1.3;
		}
		else if(e->terminal == ENEMY_WIN){
			added_value = -1.3;
		}
		else if(e->terminal == TIE){
			added_value = 0.0;
		}
		else{
			printf("should not reach here :/\n");
		}
		struct mcts_edge * pre_e; //DB
		// back-up
		while(true){
			turn = (enum turn)(((int)turn+4-1)%4);
			depth--;
			if(turn == PLAYER_FIRST || turn == PLAYER_SECOND){
				e->W += added_value;
			}
			else{
				e->W -= added_value;
			}
			e->N ++;
			e->Q = e->W / e->N;
			if(e->upper_edge == NULL){
				break;
			}
			pre_e = e;
			e = e->upper_edge;
		}
		memcpy(b, &saved_board, sizeof(struct board)); // restore root board state

		time++;
		timeout = time >= 100; //TODO: up to timelimit
		if(timeout){
			break;
		}
	}


	// get 2 actions with maximum visit counts
	int max_n;
	max_n = 0;
	node = tree.root;
	for(int i = 0; i < 19; i++){
		for(int j = 0; j < 19; j++){
			e = &(node->edges[i][j]);
			if(e->valid){
				if(e->N > max_n){
					max_n = e->N;
					*i1 = i;
					*j1 = j;
				}
			}
		}
	}
	//mcts_display_transpose(b, tree.root);
	if(cnt == 1){
		mcts_free(tree.root);
		return;
	}
	// careful becoming terminal (null ptr) at first stone
	max_n = 0;
	node = node->edges[*i1][*j1].result_node;
	if(node == NULL){
		// game already over
		board_put_player(b, *i1, *j1);
		find_max_position(b, i2, j2, PLAYER_SECOND);
		memcpy(b, &saved_board, sizeof(struct board));
	}
	else{
		for(int i = 0; i < 19; i++){
			for(int j = 0; j < 19; j++){
				e = &(node->edges[i][j]);
				if(e->valid){
					if(e->N > max_n){
						max_n = e->N;
						*i2 = i;
						*j2 = j;
					}
				}
			}
		}		
	}
	//mcts_display_transpose(b, tree.root->edges[*i1][*j1].result_node);
	mcts_free(tree.root);
	return;
}


// line count of existence of rank 2~5
void rank_line_count(struct board * b, int * p_chunks, int * e_chunks){
	struct line * l;
	for(int r = 2; r < 6; r++){
		p_chunks[r] = 0;
		e_chunks[r] = 0;

		//LRlines
		for(int n = 0; n < 19; n++){
			l = &(b->LRlines[n]);
			for(int p = 0; p < l->length - 5; p++){
				if(!l->dead_player[p] && l->rank_player[p] == r){
					p_chunks[r]++;
					break;
				}
			}
			for(int p = 0; p < l->length - 5; p++){
				if(!l->dead_enemy[p] && l->rank_enemy[p] == r){
					e_chunks[r]++;
					break;
				}
			}
		}

		//UDlines
		for(int n = 0; n < 19; n++){
			l = &(b->UDlines[n]);
			for(int p = 0; p < l->length - 5; p++){
				if(!l->dead_player[p] && l->rank_player[p] == r){
					p_chunks[r]++;
					break;
				}
			}
			for(int p = 0; p < l->length - 5; p++){
				if(!l->dead_enemy[p] && l->rank_enemy[p] == r){
					e_chunks[r]++;
					break;
				}
			}
		}

		//DDlines
		for(int n = 0; n < 37; n++){
			l = &(b->DDlines[n]);
			for(int p = 0; p < l->length - 5; p++){
				if(!l->dead_player[p] && l->rank_player[p] == r){
					p_chunks[r]++;
					break;
				}
			}
			for(int p = 0; p < l->length - 5; p++){
				if(!l->dead_enemy[p] && l->rank_enemy[p] == r){
					e_chunks[r]++;
					break;
				}
			}
		}

		//LRlines
		for(int n = 0; n < 37; n++){
			l = &(b->DUlines[n]);
			for(int p = 0; p < l->length - 5; p++){
				if(!l->dead_player[p] && l->rank_player[p] == r){
					p_chunks[r]++;
					break;
				}
			}
			for(int p = 0; p < l->length - 5; p++){
				if(!l->dead_enemy[p] && l->rank_enemy[p] == r){
					e_chunks[r]++;
					break;
				}
			}
		}
	}
}

float prior_board_value_m(int * rlc_m, int * rlc_y, bool oneStone){
	float pre_value = 0.0;
	if(oneStone){

		// definite win
		if(rlc_m[5] > 0){
			return 1.0; 
		}

		// lose unless defense at intersection
		if(rlc_y[4] + rlc_y[5] >= 2){
			pre_value -= 0.5*(rlc_y[4] + rlc_y[5] - 1);
		}
	}
	else{

		// definite win
		if(rlc_m[4] + rlc_m[5] > 0){
			return 1.0; 
		}

		// multiple threats
		if(rlc_y[4] + rlc_y[5] >= 2){
			pre_value -= 0.4*(rlc_y[4] + rlc_y[5] - 1);
		}
	}

	pre_value += 0.03*rlc_m[4];
	pre_value += 0.05*rlc_m[3];
	pre_value += 0.02*rlc_m[2];

	pre_value -= 0.03*rlc_y[4];
	pre_value -= 0.05*rlc_y[3];
	pre_value -= 0.02*rlc_y[2];

	//printf("%f ", pre_value);

	return tanh(pre_value);
}

// PLAYER and ENEMY will be symmetric
float prior_board_value(struct board * b, enum turn turn){
	
	int rlc_p[7]; // only rank 2 ~ rank 5 is valid
	int rlc_e[7];
	rank_line_count(b, rlc_p, rlc_e);

	float pre_value = 0.0;
	if (turn == PLAYER_SECOND){
		return prior_board_value_m(rlc_p, rlc_e, true);
	}
	else if (turn == PLAYER_FIRST){
		return prior_board_value_m(rlc_p, rlc_e, false);
	}
	else if (turn == ENEMY_SECOND){
		return -prior_board_value_m(rlc_e, rlc_p, true);
	}
	else{
		return -prior_board_value_m(rlc_e, rlc_p, false);
	}
}

int self_play_test(){
	struct board p_board, e_board;
	board_init(&p_board);
	board_init(&e_board);
	int t = 0;
	bool first = true;
	int cnt;
	int i1, j1, i2, j2;
	while(p_board.empty_left > 0){
		if(t == 0){
			cnt = 1;
		}
		else{
			cnt = 2;
		}
		display_board_transpose(&p_board);
		if((t%2 == 0) == true){
			printf("PLAYER's turn, board value: %f\n", prior_board_value(&p_board, PLAYER_FIRST));
			mcts_find_best_move(&p_board, &i1, &j1, &i2, &j2, cnt);
			board_put_player(&p_board, i1, j1);
			board_put_enemy(&e_board, i1, j1);
			if(cnt == 2){
				board_put_player(&p_board, i2, j2);
				board_put_enemy(&e_board, i2, j2);
			}
		}
		else{
			printf("ENEMY's turn, board value: %f\n", prior_board_value(&p_board, ENEMY_FIRST));
			mcts_find_best_move(&e_board, &i1, &j1, &i2, &j2, cnt);
			board_put_player(&e_board, i1, j1);
			board_put_enemy(&p_board, i1, j1);
			if(cnt == 2){
				board_put_player(&e_board, i2, j2);
				board_put_enemy(&p_board, i2, j2);
			}
		}
		t++;
		getchar();
	}
}

void debug_set_board_transpose(struct board * b){
	char *s[19];

	char player_color = 'w';
	/*
	s[0] =  "...................";
	s[1] =  "...................";
	s[2] =  "...................";
	s[3] =  "...................";
	s[4] =  "...................";
	s[5] =  "...................";
	s[6] =  "...................";
	s[7] =  "...................";
	s[8] =  "...................";
	s[9] =  "...................";
	s[10] = "...................";
	s[11] = "...................";
	s[12] = "...................";
	s[13] = "...................";
	s[14] = "...................";
	s[15] = "...................";
	s[16] = "...................";
	s[17] = "...................";
	s[18] = "...................";
	*/

	s[0] =  "...................";
	s[1] =  "...................";
	s[2] =  "...................";
	s[3] =  "...................";
	s[4] =  "...................";
	s[5] =  "...................";
	s[6] =  "..........b........";
	s[7] =  ".........www.......";
	s[8] =  ".......bwwwbw......";
	s[9] =  ".......w.bbbbw.....";
	s[10] = "......w..bb........";
	s[11] = ".....b..b..........";
	s[12] = "...................";
	s[13] = "...................";
	s[14] = "...................";
	s[15] = "...................";
	s[16] = "...................";
	s[17] = "...................";
	s[18] = "...................";

	char c;
	for(int j = 0; j < 19; j++){
		for(int i = 0; i < 19; i++){
			c = s[j][i];

			if(c == '.'){
				// do nothing
			}
			else if(c == 'n'){
				board_put_neutral(b, i, j);
			}
			else if(c == player_color){
				board_put_player(b, i, j);
			}
			else{
				board_put_enemy(b, i, j);
			}
			
		}
	}
}

int main(){
/*
	struct board b;
	board_init(&b);
	debug_set_board_transpose(&b);
	display_board_transpose(&b);
	int i1, j1, i2, j2;
	mcts_find_best_move(&b, &i1, &j1, &i2, &j2, 2);
	printf("bestmove: (%d, %d) and (%d, %d)\n", i1, j1, i2, j2);
	*/
	self_play_test();
	return 0;
}