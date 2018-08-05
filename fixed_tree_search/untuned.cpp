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

// ------------------ NEW CODE ----------------------
#include <stdlib.h>
#define MIN_VALUE -1.0

bool initialized = false;

enum cell_state{
	EMPTY,
	PLAYER,
	ENEMY,
	NEUTRAL
};

enum direction{
	LR,
	UD,
	DD,
	DU
};


struct point{
	int row;
	int col;
	enum cell_state state;
	/* maybe 4 reference to associated line? (or value sources) */
	float value_player; // function of n_player and n_enemy
	float value_enemy;
	int n_player[4][7];
	int n_enemy[4][7];
	bool forbid_player; // no 7+ connection
	bool forbid_enemy;
};

struct line{
	int length;
	enum direction dir;
	struct point * pts[19]; // 1D array of reference to associated points [row][col]
	int rank_player[14]; // array of size 'length - 5' or just null
	bool dead_player[14];
	int conv7_player[19+6]; // array of size 'length + 6' for storing [1111111] conv results

	int rank_enemy[14];
	bool dead_enemy[14];
	int conv7_enemy[19+6];

	/* later: may have to make a single value of line */
};


struct board{
	struct point pts[19][19];
	struct line LRlines[19]; // [0][:] ~ [18][:]
	struct line UDlines[19]; // [:][0] ~ [:][18]
	struct line DDlines[37]; // [18][0] ~ [0][18]
	struct line DUlines[37]; // [0][0] ~ [18][18]
};

struct stack_node{
	struct stack_node * below;
	struct board * saved_board;
};

struct dynamic_board{
	// board that can roll back moves
	struct board base_board; // starting point
	int depth; // possible number of roll back (current stack size + 1)
	struct stack_top;
};

struct dynamic_board * dynamic_init(struct board * base);
void dynamic_put_player(struct dynamic_board * db, int i, int j);
void dynamic_put_enemy(struct dynamic_board * db, int i, int j);
void dynamic_rollback_one(struct dynamic_board * db);
void dynamic_free(struct dynamic_board * db);

void point_init(struct point * p, int row, int col){
	// initialize empty point
	for (int d = 0; d < 4; d++){
		for (int r = 0; r < 7; r++){
			p->n_player[d][r] = 0;
			p->n_enemy[d][r] = 0;
			p->state = EMPTY;
			p->value_player = 0.0;
			p->value_enemy = 0.0;
			p->forbid_player = false;
			p->forbid_enemy = false;
			p->row = row;
			p->col = col;
		}
	}
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


void reevaluate_point(struct point * p){
	// TODO: make heuristic value function

	// simple weighted sum?
	float total_value = 0.0;
	for (int d = 0; d < 4; d++){
		for (int r = 0; r < 7; r++){
			total_value += (r + 1)*(r + 1)*inv_sq_sum(p->n_player[d][r]);
			total_value += (r + 1)*(r)*inv_sq_sum(p->n_enemy[d][r]);
		}
	}


	if (total_value <= MIN_VALUE){
		printf("LOWER THAN MIN VALUE!!!\n");
	}

	p->value_player = 0.1 * total_value;
	p->value_enemy = 0.1 * total_value;
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

}


struct line * get_associated_line(struct board * b, int i, int j, enum direction dir, int * idx){
	if (dir == LR){
		*idx = j;
		return &(b->LRlines[i]);
	}
	else if (dir == UD){
		*idx = i;
		return &(b->UDlines[j]);
	}
	else if (dir == DD){
		if (i >= j){
			*idx = j;
		}
		else{
			*idx = i;
		}
		return &(b->DDlines[18 + j - i]);
	}
	else{
		if (i + j <= 18){
			*idx = j;
		}
		else{
			*idx = 18 - i;
		}
		return &(b->DUlines[i + j]);
	}
}

void display_rank(struct board * b){
	printf("PLAYER\n");
	for (int r = 0; r <= 1; r++){
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
	printf("PLAYER VALUE\n");
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			p = &(b->pts[i][j]);
			if(p->state == PLAYER){
				printf("  P   ");
			}
			else if(p->state == ENEMY){
				printf("  E   ");
			}
			else if(p->state == NEUTRAL){
				printf("  N   ");
			}
			else if(p->forbid_player){
				printf("  x   ");
			}
			else{
				printf("%5.2f ", p->value_player);
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
	if(p->state != EMPTY || p->forbid_player){
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
	if(p->state != EMPTY || p->forbid_enemy){
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

	for(int n = 0; n < 7; n++){
		l->conv7_player[pos + n]++;
		if(l->conv7_player[pos + n] == 6){
			for(int i = 0; i < 7; i++){
				if(pos + n - i >= 0 && pos + n - i < l->length){
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
		printf("Cell Not Empty!\n");
		return;
	}
	if (b->pts[i][j].forbid_player){
		printf("Cell Forbidden!\n");
		return;
	}
	b->pts[i][j].state = PLAYER;
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

	for(int n = 0; n < 7; n++){
		l->conv7_enemy[pos + n]++;
		if(l->conv7_enemy[pos + n] == 6){
			for(int i = 0; i < 7; i++){
				if(pos + n - i >= 0 && pos + n - i < l->length){
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
	bool e_killed[11];
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


	// care connect seven for both player and enemy
	for(int n = 0; n < 7; n++){
		l->conv7_player[pos + n]++;
		l->conv7_enemy[pos + n]++;
		if(l->conv7_player[pos + n] == 6){
			for(int i = 0; i < 7; i++){
				if(pos + n - i >= 0 && pos + n - i < l->length){
					forbid_point_player(b, l->pts[pos + n - i]);				
				}
			}
		}
		if(l->conv7_enemy[pos + n] == 6){
			for(int i = 0; i < 7; i++){
				if(pos + n - i >= 0 && pos + n - i < l->length){
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
	b->pts[i][j].state = NEUTRAL;
	// TODO : may have to pop out from ordered list of empty points

	for (int d = 0; d < 4; d++){
		l = get_associated_line(b, i, j, (enum direction)d, &idx);
		if (l->length >= 6){
			line_put_neutral(b, l, idx);
		}
	}	
}

void find_max_value_player(struct board * b, int * bi, int * bj){
	int best_i = -1, best_j = -1;
	float best_value = MIN_VALUE;
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			if (b->pts[i][j].state == EMPTY && (!(b->pts[i][j].forbid_player))){
				if (b->pts[i][j].value_player > best_value){
					best_i = i;
					best_j = j;
					best_value = b->pts[i][j].value_player;
				}
			}
		}
	}
	*bi = best_i;
	*bj = best_j;
}


int main(){

	struct board b;
	board_init(&b);
	board_put_player(&b, 9, 13);
	board_put_player(&b, 9, 14);
	board_put_neutral(&b, 9, 15);
	board_put_player(&b, 9, 16);
	board_put_player(&b, 9, 17);
	board_put_player(&b, 9, 18);
	display_value(&b);
	display_rank(&b);
	return 0;
}
