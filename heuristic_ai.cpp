// Only position valuing, no tree search
#include <stdio.h>
#include <stdlib.h>


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

	if(dir == LR || dir == UD){
		l->length = 19;
	}
	else{

		// idx 0: length 1
		// idx 18: length 19
		// idx 36: length 1
		if(idx <= 18){
			l->length = idx + 1;

		}
		else{
			l->length = 37 - idx;
		}
	}

	l->dir = dir;

	if(dir == LR){
		for(int i = 0; i < 19; i++){
			l->pts[i] = &(b->pts[idx][i]);
		}
	}
	else if(dir == UD){
		for(int i = 0; i < 19; i++){
			l->pts[i] = &(b->pts[i][idx]);
		}
	}
	else if(dir == DD){

		int st_i, st_j;
		if(idx <= 18){
			st_i = 18 - idx;
			st_j = 0;
		}
		else{
			st_i = 0;
			st_j = idx - 18;
		}
		for(int i = 0; i < l->length; i++){
			l->pts[i] = &(b->pts[st_i+i][st_j+i]);
		}

	}
	else{
		int st_i, st_j;
		if(idx <= 18){
			st_i = idx;
			st_j = 0;
		}
		else{
			st_i = 18;
			st_j = idx-18;
		}
		for(int i = 0; i < l->length; i++){
			l->pts[i] = &(b->pts[st_i-i][st_j+i]);
		}
	}

	if(l->length >= 6){

		for(int i = 0; i < l->length - 5; i++){
			l->rank_player[i] = 0;
			l->dead_player[i] = false;
			l->rank_enemy[i] = 0;
			l->dead_enemy[i] = false;			
			for(int j = 0; j < 6; j++){
				l->pts[i+j]->n_player[dir][0]++;
				l->pts[i+j]->n_enemy[dir][0]++;
			}
		}
	}
}


void board_init(struct board * b){
	// intiialize empty board

	//init points
	for(int i = 0; i < 19; i++){
		for(int j = 0; j < 19; j++){
			point_init(&(b->pts[i][j]));
		}
	}
	//init lines
	for(int i = 0; i < 19; i++){

		line_init(&(b->LRlines[i]), b, LR, i);
		line_init(&(b->UDlines[i]), b, UD, i);
	}	


	for(int i = 0; i < 37; i++){

		line_init(&(b->DDlines[i]), b, DD, i);

		line_init(&(b->DUlines[i]), b, DU, i);

	}

}


int main(){
	struct board b;
	b.DDlines[18].length = 0;
	printf("&b = [%p]\n", &b);
	printf("linep = [%p]\n", &(b.DDlines[18]));
	printf("size: %d\n", (int) sizeof(struct board));
	board_init(&b);
	printf("Hello AI!\n");
	return 0;
}
