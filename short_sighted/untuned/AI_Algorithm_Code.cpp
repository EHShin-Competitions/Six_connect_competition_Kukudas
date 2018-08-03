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

#include <Windows.h>
#include <time.h>
#include "Connect6Algo.h"

// "�����ڵ�[C]"  -> �ڽ��� ���� (����)
// "AI�μ�[C]"  -> �ڽ��� �Ҽ� (����)
// ����� ���������� �ݵ�� �������� ����!
char info[] = { "TeamName:��ũ�ٽ�,Department:KAIST" };

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
	int x;
	int y;
	enum cell_state state;
	/* maybe 4 reference to associated line? (or value sources) */
	float value; // function of n_player and n_enemy
	int n_player[4][7];
	int n_enemy[4][7];
};

struct line{
	int length;
	enum direction dir;
	struct point * pts[19]; // 1D array of reference to associated points [row][col]
	int rank_player[14]; // array of size 'length - 5' or just null
	bool dead_player[14];

	int rank_enemy[14];
	bool dead_enemy[14];

	/* later: may have to make a single value of line */
};


struct board{
	struct point pts[19][19];
	struct line LRlines[19]; // [0][:] ~ [18][:]
	struct line UDlines[19]; // [:][0] ~ [:][18]
	struct line DDlines[37]; // [18][0] ~ [0][18]
	struct line DUlines[37]; // [0][0] ~ [18][18]
};


void point_init(struct point * p){
	// initialize empty point
	for (int d = 0; d < 4; d++){
		for (int r = 0; r < 7; r++){
			p->n_player[d][r] = 0;
			p->n_enemy[d][r] = 0;
			p->state = EMPTY;
			p->value = 0.0;
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
	// TODO: take care of 7+ connect foul somewhere. (not just value 0 (dead), make it very bad!!!)

	// simple weighted sum?
	float total_value = 0.0;
	for (int d = 0; d < 4; d++){
		for (int r = 0; r < 7; r++){
			total_value += (r + 1)*(r + 1)*inv_sq_sum(p->n_player[d][r]);
			total_value += (r + 1)*(r)*inv_sq_sum(p->n_enemy[d][r]);
		}
	}
	p->value = total_value;
	if (total_value <= MIN_VALUE){
		printf("LOWER THAN MIN VALUE!!!\n");
	}
}


void board_init(struct board * b){
	// intiialize empty board

	//init points
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			point_init(&(b->pts[i][j]));
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
	printf("VALUE\n");
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			printf("%5.2f ", b->pts[i][j].value);
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


void line_put_player(struct line * l, int pos){
	// TODO : care neutral points <- maybe dual board was right??
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
}


void board_put_player(struct board * b, int i, int j){
	int idx;
	struct line * l;

	if (b->pts[i][j].state != EMPTY){
		printf("Cell Not Empty!\n");
		return;
	}
	b->pts[i][j].state = PLAYER;
	// TODO : may have to pop out from ordered list of empty points

	for (int d = 0; d < 4; d++){
		l = get_associated_line(b, i, j, (enum direction)d, &idx);
		if (l->length >= 6){
			line_put_player(l, idx);
		}
	}
}

void line_put_enemy(struct line * l, int pos){
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
}

void board_put_enemy(struct board * b, int i, int j){
	int idx;
	struct line * l;

	if (b->pts[i][j].state != EMPTY){
		printf("Cell Not Empty!\n");
		return;
	}
	b->pts[i][j].state = ENEMY;
	// TODO : may have to pop out from ordered list of empty points

	for (int d = 0; d < 4; d++){
		l = get_associated_line(b, i, j, (enum direction)d, &idx);
		if (l->length >= 6){
			line_put_enemy(l, idx);
		}
	}
}

void find_max_value(struct board * b, int * bi, int * bj){
	int best_i = -1, best_j = -1;
	float best_value = MIN_VALUE;
	for (int i = 0; i < 19; i++){
		for (int j = 0; j < 19; j++){
			if (b->pts[i][j].state == EMPTY){
				if (b->pts[i][j].value > best_value){
					best_i = i;
					best_j = j;
					best_value = b->pts[i][j].value;
				}
			}
		}
	}
	*bi = best_i;
	*bj = best_j;
}


// ------------------- END NEW CODE ---------------------
struct board main_board;

void myturn(int cnt) {

	int x[2], y[2];

	// �� �κп��� �˰��� ���α׷�(AI)�� �ۼ��Ͻʽÿ�. �⺻ ������ �ڵ带 ���� �Ǵ� �����ϰ� ������ �ڵ带 ����Ͻø� �˴ϴ�.
	// ���� Sample code�� AI�� Random���� ���� ���� Algorithm�� �ۼ��Ǿ� �ֽ��ϴ�.

	/*
	srand((unsigned)time(NULL));

	for (int i = 0; i < cnt; i++) {
		do {
			x[i] = rand() % width;
			y[i] = rand() % height;
			if (terminateAI) return;
		} while (!isFree(x[i], y[i]));

		if (x[1] == x[0] && y[1] == y[0]) i--;
	}
	*/

	if (!initialized){
		initialized = true;
		board_init(&main_board);
		//TODO: Neutral!!!
	}

	int opCnt;
	if (cnt != 1){
		// I'm not first
		get_op_move(x, y, &opCnt);
		for (int n = 0; n < opCnt; n++){
			board_put_enemy(&main_board, y[n], x[n]);
		}
	}

	int by, bx;
	for (int n = 0; n < cnt; n++){
		find_max_value(&main_board, &by, &bx);
		board_put_player(&main_board, by, bx);
		x[n] = bx;
		y[n] = by;
	}

	// �� �κп��� �ڽ��� ���� ���� ����Ͻʽÿ�.
	// �ʼ� �Լ� : domymove(x�迭,y�迭,�迭ũ��)
	// ���⼭ �迭ũ��(cnt)�� myturn()�� �Ķ���� cnt�� �״�� �־���մϴ�.
	domymove(x, y, cnt);
}



