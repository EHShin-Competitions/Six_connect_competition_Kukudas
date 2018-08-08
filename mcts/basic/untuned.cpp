// Samsung Go Tournament Form C (g++-4.8.3)

/*
[AI �ڵ� �ۼ� ���]

1. char info[]�� �迭 �ȿ�					"TeamName:�ڽ��� ����,Department:�ڽ��� �Ҽ�"					������ �ۼ��մϴ�.
( ���� ) Teamname:�� Department:�� �� ���� �մϴ�.
"�ڽ��� ����", "�ڽ��� �Ҽ�"�� �����ؾ� �մϴ�.

2. �Ʒ��� myturn() �Լ� �ȿ� �ڽŸ��� AI �ڵ带 �ۼ��մϴ�.

3. AI ������ �׽�Ʈ �Ͻ� ���� "���� �˰����ȸ ��"�� ����մϴ�.

4. ���� �˰��� ��ȸ ���� �����ϱ⿡�� �ٵϵ��� ���� ��, �ڽ��� "����" �� �� �˰����� �߰��Ͽ� �׽�Ʈ �մϴ�.



[���� �� �Լ�]
myturn(int cnt) : �ڽ��� AI �ڵ带 �ۼ��ϴ� ���� �Լ� �Դϴ�.
int cnt (myturn()�Լ��� �Ķ����) : ���� �� �� �־��ϴ��� ���ϴ� ����, cnt�� 1�̸� ���� ���� ��  �� ����  �δ� ��Ȳ(�� ��), cnt�� 2�̸� �� ���� ���� �δ� ��Ȳ(�� ��)
int  x[0], y[0] : �ڽ��� �� ù �� ° ���� x��ǥ , y��ǥ�� ����Ǿ�� �մϴ�.
int  x[1], y[1] : �ڽ��� �� �� �� ° ���� x��ǥ , y��ǥ�� ����Ǿ�� �մϴ�.
void domymove(int x[], int y[], cnt) : �� ������ ��ǥ�� �����ؼ� ���


//int board[BOARD_SIZE][BOARD_SIZE]; �ٵ��� �����Ȳ ��� �־� �ٷλ�� ������. ��, ���������ͷ� ���� �������
// ������ ���� ��ġ�� �ٵϵ��� ������ �ǰ��� ó��.

boolean ifFree(int x, int y) : ���� [x,y]��ǥ�� �ٵϵ��� �ִ��� Ȯ���ϴ� �Լ� (������ true, ������ false)
int showBoard(int x, int y) : [x, y] ��ǥ�� ���� ���� �����ϴ��� �����ִ� �Լ� (1 = �ڽ��� ��, 2 = ����� ��, 3 = ��ŷ)


<-------AI�� �ۼ��Ͻ� ��, ���� �̸��� �Լ� �� ���� ����� �������� �ʽ��ϴ�----->
*/

#include <stdio.h>

#include <time.h>

// "�����ڵ�[C]"  -> �ڽ��� ���� (����)
// "AI�μ�[C]"  -> �ڽ��� �Ҽ� (����)
// ����� ���������� �ݵ�� �������� ����!

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN_VALUE -1.0

#define MAX_BREADTH 10

#define C_PUCT 0.1

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

/*
*/

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
				printf("%5.2f ", p->value[PLAYER_FIRST]);
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
	for (int n = 0; n < 7; n++){
		l->conv7_player[pos + n]++;
		l->conv7_enemy[pos + n]++;
		if (l->conv7_player[pos + n] == 6){
			for (int i = 0; i < 7; i++){
				if (pos + n - i >= 0 && pos + n - i < l->length){
					forbid_point_player(b, l->pts[pos + n - i]);
				}
			}
		}
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


float prior_board_value(struct board * b, enum turn turn){
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
			//printf("P(%d, %d)=%f, QU=%f\n", i, j, e->P, QU);
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
				// TODO: make better prior
				prior = b->pts[i][j].value[turn];
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
				printf(" %3d:%4.1f", node->edges[i][j].N, node->edges[i][j].Q);
			}
		}
		printf("\n");
	}	
}

bool feq(float a, float b){
	return (a-b)<0.01 && (b-a)<0.01;
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
	
	if (cnt == 1){
		struct dynamic_board * db = dynamic_init(b);
		find_best_move(db, i1, j1, i2, j2, 5, 4, PLAYER_SECOND); // don't think too much
		dynamic_free(db);
		return;
	}

	struct mcts_tree tree;
	struct board saved_board;
	enum turn turn;
	memcpy(&saved_board, b, sizeof(struct board)); // save root board state

	struct mcts_node * node;
	struct mcts_edge * e;
	
	bool timeout = false;
	float added_value;

	tree.root = mcts_expand_node(b, PLAYER_FIRST, NULL);

	int time = 0;
	while(true){
		// new mcts step
		node = tree.root;
		turn = PLAYER_FIRST;

		// selection
		while(true){
			e = mcts_select_edge(node, turn);
			if(turn == PLAYER_FIRST || turn == PLAYER_SECOND){
				board_put_player(b, e->pos_i, e->pos_j);
			}
			else{
				board_put_enemy(b, e->pos_i, e->pos_j);
			}
			node = e->result_node;
			turn = (enum turn)((turn+1)%4);
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
		else{
			printf("should not reach here :/\n");
		}
		// back-up
		while(true){
			turn = (enum turn)(((int)turn+4-1)%4);
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
			e = e->upper_edge;
		}
		memcpy(b, &saved_board, sizeof(struct board)); // restore root board state

		time++;
		timeout = time >= 10000; //TODO: up to timelimit
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

}

int main(){

	struct board b;
	board_init(&b);


	display_board(&b);

	int i1, j1, i2, j2;
	mcts_find_best_move(&b, &i1, &j1, &i2, &j2, 2);
	printf("(%d, %d) and (%d, %d)\n", i1, j1, i2, j2);

	return 0;
}