// line demo
#include <stdio.h>
#include <stdlib.h>

enum cell_state{
	EMPTY,
	PLAYER,
	ENEMY,
	NEUTRAL
};

struct point{
	enum cell_state state;
	int player_n[7]; // 0 : rank 0, 6 : rank 6, 7 : dead
	int enemy_n[7];
};

struct line{
	struct point pts[19];
	int ranks[14];
	bool dead[14];
};

struct line l;

void init(){
	struct point * p;
	for (int i = 0; i < 19; i++){
		p = &(l.pts[i]);
		p->state = EMPTY;
		for (int j = 0; j < 7; j++){
			p->player_n[j] = 0;
			p->enemy_n[j] = 0;
		}
	}
	for (int c = 0; c < 14; c++){
		l.ranks[c] = 0;
		l.dead[c] =  false;
		for (int i = 0; i < 6; i++){
			p = &(l.pts[c+i]);
			p->player_n[0]++;
			p->enemy_n[0]++;
		}
	}
}

void put_player(int x){
	l.pts[x].state = PLAYER;
	int i, j, k;
	int st = x - 6;
	if(st < 0)
		st = 0;
	int ed = x + 6;
	if(ed > 18)
		ed = 18;
	int p_update[7];
	bool killed[13];
	for(j = 0; j <= 7; j++){
		p_update[j] = 0;
	}
	for(i = st; i <= ed; i++){
		// apply new chunk change, if any.
		killed[i-st] = false;
		if(i+5 <= ed){
			if(i == x - 6 || i == x + 1){
				// dead chunk (7+ connect)
				if(l.dead[i]){
					// already dead, do nothing
				}
				else{
					// is now dead
					killed[i-st] = true;
					l.dead[i] = true;
					p_update[l.ranks[i]]--;
				}
			}
			if(i >= x - 5 && i <= x){
				// rank + 1
				if(l.dead[i]){
					// dead, do nothing
				}
				else{
					p_update[l.ranks[i]]--;
					l.ranks[i]++;
					p_update[l.ranks[i]]++;
				}
			}	
		}

		// cancel old chunk change. if any.
		k = i - 6;
		if(k >= st){
			if(k == x - 6 || k == x + 1){
				if(killed[k - st]){
					p_update[l.ranks[k]]++;
				}
			}
			if(k >= x - 5 && k <= x){
				if(l.dead[k]){
				}
				else{
					p_update[l.ranks[k]]--;
					p_update[l.ranks[k]-1]++;
				}
			}
		}
		// apply update
		for(j = 0; j < 7; j++){
			l.pts[i].player_n[j] += p_update[j];
		}
	}
}

void put_enemy(int x){
	l.pts[x].state = ENEMY;
	int i, j, k;
	int st = x - 5;
	if(st < 0)
		st = 0;
	int ed = x + 5;
	if(ed > 18)
		ed = 18;
	int p_update[7];
	bool killed[11];
	for(j = 0; j <= 7; j++){
		p_update[j] = 0;
	}
	for(i = st; i <= ed; i++){
		// apply new chunk change, if any.
		killed[i-st] = false;
		if(i+5 <= ed){
			// dead chunks
			if(l.dead[i]){
				// already dead, do nothing
			}
			else{
				// is now dead
				killed[i-st] = true;
				l.dead[i] = true;
				p_update[l.ranks[i]]--;
			}
		}

		// cancel old chunk change. if any.
		k = i - 6;
		if(k >= st){
			if(killed[k - st]){
				p_update[l.ranks[k]]++;
			}
		}
		// apply update
		for(j = 0; j < 7; j++){
			l.pts[i].player_n[j] += p_update[j];
		}
	}
}

void disp(){
	struct point * p;
	for (int i = 0; i < 19; i++){
		p = &(l.pts[i]);
		if(p->state == EMPTY){
			printf(" . ");
		}
		if(p->state == PLAYER){
			printf(" p ");
		}
		if(p->state == ENEMY){
			printf(" e ");
		}
	}
	printf("\n\n");
	for (int j = 0; j < 7; j++){
		for (int i = 0; i < 19; i++){
			p = &(l.pts[i]);
			if(p->state == EMPTY)
				printf("%2d ", p->player_n[j]);
			else
				printf(" X ");
		}
		printf("\n");
	}
	printf("\n");
	for (int i = 0; i < 14; i++){
		if(l.dead[i]){
			printf(" D ");
		}
		else{
			printf("%2d ", l.ranks[i]);
		}
	}
	printf("\n");
}

int main(){
	printf("Line Example\n");
	init();
	put_enemy(10);
	put_player(13);
	disp();
	return 0;
}