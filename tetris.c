#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <pthread.h>

enum block_type{
	O,J,L,I,Z,S,T
};
int block_type_num=7;
enum block_type type_arr[3]={};

HANDLE hand;
int cur_x=0,cur_y=0;
int score=0;

#define BLOCK_QUEUE 3
#define FIRST_SPEED 1000
#define LEVEL_SPEED 100
#define HEIGHT 18
#define WIDTH 18
#define block_h 4
#define block_w 4

char title_TETRIS[HEIGHT+1][WIDTH+1]={
"                  ",
"                  ",
"     ooo ooo      ",
"      o  o        ",
"      o  ooo      ",
"      o  o        ",
"      o  ooo      ",
"                  ",      
"ooo ooo  ooo  ooo ",
" o  o  o  o  o    ",    
" o  ooo   o   ooo ", 
" o  o o   o      o",
" o  o  o ooo  ooo ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  "
};

char title_BG[HEIGHT+1][WIDTH+1]={
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo", 
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo",
"oooooooooooooooooo"
};

int surface[HEIGHT][WIDTH]={0};

int BlockO[block_h][block_w]={
{0,0,0,0},
{0,1,1,0},
{0,1,1,0},
{0,0,0,0},
};
int BlockJ[block_h][block_w]={
{0,0,1,0},
{0,0,1,0},
{0,1,1,0},
{0,0,0,0},
};
int BlockL[block_h][block_w]={
{0,1,0,0},
{0,1,0,0},
{0,1,1,0},
{0,0,0,0},
};
int BlockI[block_h][block_w]={
{0,1,0,0},
{0,1,0,0},
{0,1,0,0},
{0,1,0,0},
};
int BlockZ[block_h][block_w]={
{0,0,0,0},
{1,1,0,0},
{0,1,1,0},
{0,0,0,0},
};
int BlockS[block_h][block_w]={
{0,0,0,0},
{0,1,1,0},
{1,1,0,0},
{0,0,0,0},
};
int BlockT[block_h][block_w]={
{0,0,0,0},
{1,1,1,0},
{0,1,0,0},
{0,0,0,0},
};

int Block_blank[block_h][block_w]={
{0,0,0,0},
{0,0,0,0},
{0,0,0,0},
{0,0,0,0},	
};

int hold_frame[block_h+2][block_w+2]={
{0,1,1,1,1,1},
{2,0,0,0,0,2},
{2,0,0,0,0,2},
{2,0,0,0,0,2},
{2,0,0,0,0,2},
{0,1,1,1,1,1},
};
int hold_frame_x=WIDTH+block_w+3,hold_frame_y=4;
int block_hold[block_w][block_h]={0};
int temp_block[block_w][block_h]={0};
int hold_chance=1;

int block_cur[block_h][block_w]={0};
int next_block[BLOCK_QUEUE][block_h][block_w]={0};

int quit=0;
int speed=FIRST_SPEED;
int level=1;
int clear_number=0;
int music_switch=1;
int bgm_switch=2;
UINT_PTR timerId,countId;

static void count_timer(int t){
	KillTimer(NULL,countId);
	countId=SetTimer(NULL,0,t,NULL);
}

static void close_count_timer(){
	KillTimer(NULL,countId);
}

void CALLBACK TimerProc(HWND hWnd,UINT nMsg,UINT nTimerid,DWORD dwTime);

static void set_timer(int t){
	KillTimer(NULL,timerId);
	timerId=SetTimer(NULL,0,t,NULL);
}

static void close_timer(){
	KillTimer(NULL,timerId);
}

void gotoxy(int x,int y){
	COORD loc;
	loc.X=x;
	loc.Y=y;
	SetConsoleCursorPosition(hand,loc);
}

void Music(char c[],int n){
	switch(n)
	{
		case 2:
			PlaySound(TEXT(c),NULL,SND_FILENAME | SND_ASYNC | SND_LOOP);
			break;
		
		case 1:
			PlaySound(TEXT(c),NULL,SND_FILENAME | SND_ASYNC);
			break;
			
		case 0:
			PlaySound(NULL,NULL,SND_FILENAME);
			break;
	}
}

void BGM(char c[],int n){
	char File[100];
	if(n==1||n==2)	strcpy(File,"play ");
	else if(n==0)	strcpy(File,"close ");
	strcat(File,c);
	if(n==2)		strcat(File," repeat");
	mciSendString(File,NULL,0,NULL);
}



void printxy(char *str,int x,int y){
	gotoxy(x,y);
	printf("%s",str);
}

void print_score(){
	printxy("           ",WIDTH+2,HEIGHT-2);
	gotoxy(WIDTH+2,HEIGHT-2);
	printf("Score:%d",score);
}

void print_frame(){
	int i,j;
	for(i=0;i<HEIGHT;i++)printxy("|",WIDTH,i);
	for(i=0;i<WIDTH;i++)printxy("-",i,HEIGHT);
	printxy(" HOLD:",hold_frame_x,hold_frame_y-1);
	for(i=0;i<block_w+2;i++){
		for(j=0;j<block_h+2;j++){
			if(hold_frame[i][j]==1)printxy("|",i+hold_frame_x,j+hold_frame_y);
			if(hold_frame[i][j]==2)printxy("_",i+hold_frame_x,j+hold_frame_y);
		}
	}
}

void print_surface(){
	int x,y;
	for(x=0;x<WIDTH;x++){
		for(y=0;y<HEIGHT;y++){
			int row=y;
			int col=x;
			if(surface[row][col]==0){
				printxy(" ",x,y);
			}
			else {
				printxy("O",x,y);
			}
		}
	}	
}

void copy_block(int block_s[][block_w],int block_d[][block_w]){
	int w,h;
	for(w=0;w<block_w;w++){
		for(h=0;h<block_h;h++){
			block_d[h][w]=block_s[h][w];
		}
	}
}

void print_rule(){
	printxy("上鍵旋轉，左右鍵移動，下鍵加速，空白鍵移到底部，Z hold/交換方塊",0,HEIGHT+1);
	printxy("P暫停，R重新開始，Q退出，B開啟/關閉音樂，M開啟/關閉音效",0,HEIGHT+2);
	printxy("每消除10列難度增加一級(下落速度加快)，直至等級10",0,HEIGHT+3);
}

void print_level(){
	int i;
	gotoxy(WIDTH+2,HEIGHT-4);
	printf("Level:%d",level);
	gotoxy(WIDTH+2,HEIGHT-3);
	printf("[");
	if(clear_number<=10){
		for(i=0;i<clear_number;i++)printf("■");
		for(i=clear_number;i<10;i++)printf("□"); 
	}
	else{
		for(i=0;i<10;i++)printf("■");
	}
	printf("]");
}

void print_sound(){
	printxy("        ",WIDTH+7,0);
	printxy("        ",WIDTH+7,1);
	printxy("音樂:",WIDTH+7,0);
	if(bgm_switch==0)printf("OFF");
	else printf("ON");
	printxy("音效:",WIDTH+7,1);
	if(music_switch==0)printf("OFF");
	else printf("ON");
}

void print_menu(){
	int i,j,len,up,up_x=WIDTH-1,up_y=0,down,down_x=0,down_y=HEIGHT-1,left,left_x=0,left_y=0,right,right_x=WIDTH-1,right_y=HEIGHT-1;
	for(i=0;i<HEIGHT;i++)printxy("|",WIDTH,i);
	for(i=0;i<WIDTH;i++)printxy("-",i,HEIGHT);
	for(i=0;i<HEIGHT;i++){
		for(j=0;j<WIDTH;j++){
			printxy("o",i,j);
		}
	}
	
	for(i=0;i<18;i++){
		for(j=0;j<18;j++){
			if(title_TETRIS[j][i]=='o')printxy("o",i,j+2);
		}
	}
	
	for(len=HEIGHT-1;len>0;len-=2){
		for(left=left_y,i=0;i<len;left++,i++){
			if(!(title_BG[left][left_x]==title_TETRIS[left][left_x]&&title_TETRIS[left][left_x]=='o')){
				printxy(" ",left_x,left);
			}
			Sleep(20);
		}
		left_y++;
		left_x++;
		for(down=down_x,i=0;i<len;down++,i++){
			if(!(title_BG[down_y][down]==title_TETRIS[down_y][down]&&title_TETRIS[down_y][down]=='o')){
				printxy(" ",down,down_y);
			}
			Sleep(20);
		}
		down_x++;
		down_y--;
		for(right=right_y,i=0;i<len;right--,i++){
			if(!(title_BG[right][right_x]==title_TETRIS[right][right_x]&&title_TETRIS[right][right_x]=='o')){
				printxy(" ",right_x,right);
			}
			Sleep(20);
		}
		right_y--;
		right_x--;
		for(up=up_x,i=0;i<len;up--,i++){
			if(!(title_BG[up_y][up]==title_TETRIS[up_y][up]&&title_TETRIS[up_y][up]=='o')){
				printxy(" ",up,up_y);
			}
			Sleep(20);
		}
		up_y++;
		up_x--;
	}
	printxy("[PRESS TO START]",WIDTH/2-8,HEIGHT-3);
	gotoxy(0,HEIGHT+1);
}



void erase_shadow(int block[][block_w],int x,int y){
	int shadow_y=y;
	int w,h;
	while(isCollision(cur_x,shadow_y+1)==0)shadow_y++;
	for(w=0;w<block_w;w++){
		for(h=0;h<block_h;h++){
			if(block[h][w]==1){
				printxy(" ",cur_x+w,shadow_y+h);
			}
		}
	}
}

void erase_block(int block[][block_w],int x,int y){
	int w,h;
	
	for(w=0;w<block_w;w++){
		for(h=0;h<block_h;h++){
			if(block[h][w]==1){
				printxy(" ",x+w,y+h);
			}
		}
	}
}

void land_block(){
	int w,h;
	for(w=0;w<block_w;w++){
		for(h=0;h<block_h;h++){
			if(block_cur[h][w]==1){
			surface[cur_y+h][cur_x+w]=1;
			}
		}
	}
}

int get_block_x_fill(int row){
	int is_fill=0;
	int col;
	for(col=0;col<block_w;col++){
		if(block_cur[row][col]==1){
			is_fill=1;
		}
	}
	return is_fill;
}

int get_block_y_fill(int col){
	int is_fill=0;
	int row;
	for(row=0;row<block_h;row++){
		if(block_cur[row][col]==1){
			is_fill=1;
		}
	}
	return is_fill;
}

int get_block_left_right_margin(int *left_margin,int *right_margin){
int is_fill,i;
	for(i=0;i<block_w/2;i++){
		is_fill=get_block_y_fill(i);
		if(is_fill==0){
			*left_margin+=1;
		}
	}
	for(i=block_w-1;i>=block_w/2;i--){
		is_fill=get_block_y_fill(i);
		if(is_fill==0){
			*right_margin+=1;
		}
	}
}

void get_block_top_bottom_margin(int *top_margin,int *bottom_margin){
int is_fill,i;
	for(i=0;i<block_h/2;i++){
		is_fill=get_block_x_fill(i);
		if(is_fill==0){
			*top_margin+=1;
		}
	}
	for(i=block_h-1;i>=block_h/2;i--){
		is_fill=get_block_x_fill(i);
		if(is_fill==0){
			*bottom_margin+=1;
		}
	}
}

int isCollision(int x,int y){
	
	int left_margin=0,right_margin=0,top_margin=0,bottom_margin=0;
	get_block_top_bottom_margin(&top_margin,&bottom_margin);
	get_block_left_right_margin(&left_margin,&right_margin);
	if(y+block_h>HEIGHT+bottom_margin||x<0-left_margin||x+block_w>WIDTH+right_margin||y<0-top_margin)return 1;
	
	int w,h;
	for(w=0;w<block_w;w++){
		for(h=0;h<block_h;h++){
			if(block_cur[h][w]==1){
				if(surface[y+h][x+w]==1){
					return 1;
				}
			}
		}
	}
	
	return 0;
}

void print_shadow(int block[][block_w],int x,int y){
	int shadow_y=y;
	while(isCollision(cur_x,shadow_y+1)==0)shadow_y++;
	int w,h;
	for(w=0;w<block_w;w++){
		for(h=0;h<block_h;h++){
			if(block[h][w]==1){
				printxy("I",cur_x+w,shadow_y+h);
			}
		}
	}
}

void print_block(int block[][block_w],int x,int y){
	int w,h;
	for(w=0;w<block_w;w++){
		for(h=0;h<block_h;h++){
			if(block[h][w]==1){
				printxy("O",x+w,y+h);
			}
		}
	}
}

void print_next_block(){
	int i,j;
	for(i=WIDTH+2,j=0;j<HEIGHT-4;j++)printxy("    ",i,j);
	for(i=BLOCK_QUEUE-1,j=0;i>=0;i--,j+=5)print_block(next_block[i],WIDTH+2,j);
}

int isSame(enum block_type type,enum block_type arr[]){
	int i;
	for(i=0;i<BLOCK_QUEUE;i++){
		if(type==arr[i])return 1;
	}
	return 0;
}

void make_new_block(){
	int i,len;
	enum block_type type;
	while(isSame(type,type_arr)==1){
		type=(rand()%block_type_num);
	}
	copy_block(next_block[BLOCK_QUEUE-1],block_cur);
	for(i=BLOCK_QUEUE-1;i>=0;i--){
		copy_block(next_block[i-1],next_block[i]);
		type_arr[i]=type_arr[i-1];
	}
	type_arr[0]=type;
	switch(type){
		case O:
			copy_block(BlockO,next_block[0]);
			break;
		case J:
			copy_block(BlockJ,next_block[0]);
			break;
		case L:
			copy_block(BlockL,next_block[0]);
			break;
		case I:
			copy_block(BlockI,next_block[0]);
			break;
		case Z:
			copy_block(BlockZ,next_block[0]);
			break;
		case S:
			copy_block(BlockS,next_block[0]);
			break;
		case T:
			copy_block(BlockT,next_block[0]);
			break;
	}
	cur_x=(WIDTH-block_w)/2;
	if(isCollision(cur_x,-1)==0){
		cur_y=-1;
	}
	else{
		cur_y=0;
	}
	print_next_block();
}

void PRINT_INIT(){
	print_next_block();
	print_sound();
	print_rule();
	print_level();
	print_score();
	print_frame();
	print_surface();
}

void clear_one_line(int h){
	int w,row;
	for(row=h;row>1;row--){
		for(w=0;w<WIDTH;w++){
			surface[row][w]=surface[row-1][w];
		}
	}
	for(w=0;w<WIDTH;w++){
		surface[0][w]=0;
	}
	Music("clear.wav",music_switch);
}

void complete(time_t start){
	time_t stop=time(NULL);
	int i,j;
	Music("complete.wav",music_switch);
	for(j=-1;j<2;j++){
		for(i=0;i<WIDTH;i++){
			printxy(" ",i,HEIGHT/2+j);
		}
	}
	printxy("[COMPLETE!]",WIDTH/2-5,HEIGHT/2-1);
	Sleep(2000);
	int second=(int)difftime(stop,start),minute=0,hour=0;
	while(second>=60){
		second-=60;
		minute++;
	}
	while(minute>=60){
		minute-=60;
		hour++;
	}
	printxy("time:",WIDTH/2-2,HEIGHT/2);
	Sleep(1700);
	gotoxy(WIDTH/2-5,HEIGHT/2+1);
	printf("[%.2d:%.2d:%.2d]",hour,minute,second);
	Sleep(2500);
	gotoxy(WIDTH+2,HEIGHT-1);
	system("pause");
	BGM("bgm.mp3",bgm_switch);
	printxy("                    ",WIDTH+2,HEIGHT-1);
	print_surface();
}

void level_up(time_t start){
	time_t stop;
	int i,j;
	if(level==10){
		stop=time(NULL);
		BGM("bgm.mp3",0);
		complete(start);
	}
	if(level!=10){
		Music("levelup.wav",music_switch);
		Sleep(100);
	}
	if(level<10){
		close_timer();
		speed-=LEVEL_SPEED;
		set_timer(speed);	
	}
	level++;
	clear_number-=10;
}

void check_line(time_t start){
	int total_line=0;
	int clear[4]={0},len=0,h,temp_score=0,pre_clear_number=clear_number;
	int front_temp_score,after_temp_score,front_temp,after_temp;
	for(h=0;h<HEIGHT;h++){
		if(is_line_fill(h)==1){
			clear[len]=h;
			len++;
			total_line++;
			clear_number++;
		}
	}
	for(h=0;h<len;h++){
		clear_one_line(clear[h]);
		if(level==10&&clear_number>=10){
			int temp_score=pow(2,total_line-1)*100;
		
			front_temp=10-pre_clear_number;
			after_temp=total_line-front_temp;
		
			front_temp_score=pow(2,front_temp-1)*100;
			after_temp_score=temp_score-front_temp_score;
		
			score+=front_temp_score;
			print_score();
			print_level();
			print_surface();
			level_up(start);
			score+=after_temp_score;
			total_line=0;
		}
		else{
			if(total_line>0)score+=pow(2,total_line-1)*100;
			total_line=0;
		}
		if(clear_number>=10){
			level_up(start);
		}
	}
	print_surface();
	print_level();
	print_score();
}

int move_block_down(time_t start){
	
	if(isCollision(cur_x,cur_y+1)==0){
		erase_block(block_cur,cur_x,cur_y);
		erase_shadow(block_cur,cur_x,cur_y);
		cur_y++;
		print_shadow(block_cur,cur_x,cur_y);
		print_block(block_cur,cur_x,cur_y);
	}
	else{
		land_block();
		check_line(start);
		make_new_block();
		if(isCollision(cur_x,cur_y)==1){
			return 1;
		}
		hold_chance=1;
		print_shadow(block_cur,cur_x,cur_y);
		print_block(block_cur,cur_x,cur_y);
	}
}

int is_line_fill(int h){
	int w;
	for(w=0;w<WIDTH;w++){
		if(surface[h][w]==0){
			return 0;
		}
	}
	return 1;
}

void restart(){
	int w,h,i;
	hold_chance=1;
	speed=FIRST_SPEED;
	level=1;
	clear_number=0;
	cur_x=0;
	cur_y=0;
	score=0;
	close_timer();
	set_timer(speed);
	for(i=0;i<BLOCK_QUEUE;i++){
		for(w=0;w<WIDTH;w++){
			for(h=0;h<HEIGHT;h++){
				next_block[i][w][h]=0;
			}
		}
	}
	erase_block(block_hold,hold_frame_x+1,hold_frame_y+1);
	for(w=0;w<block_w;w++){
		for(h=0;h<block_h;h++){
			block_hold[w][h]=0;
		}
	}
	for(w=0;w<BLOCK_QUEUE;w++){
		type_arr[w]='\0';
	}
	for(w=0;w<=BLOCK_QUEUE+1;w++)make_new_block();
	print_block(block_cur,cur_x,cur_y);
	for(w=0;w<WIDTH;w++){
		for(h=0;h<HEIGHT;h++){
			surface[w][h]=0;
		}
	}
	PRINT_INIT();
}

void game_over(time_t start){
	time_t stop = time(NULL);
	close_timer();
	BGM("bgm.mp3",0);
	Music("gameover.wav",music_switch);
	int i,j;
	for(j=-1;j<2;j++){
		for(i=0;i<WIDTH;i++){
			printxy(" ",i,HEIGHT/2+j);
		}
	}
	printxy("[GAME OVER]",WIDTH/2-5,HEIGHT/2-1);
	int second=(int)difftime(stop,start),minute=0,hour=0;
	while(second>=60){
		second-=60;
		minute++;
	}
	while(minute>=60){
		minute-=60;
		hour++;
	}
	printxy("time:",WIDTH/2-2,HEIGHT/2);
	gotoxy(WIDTH/2-5,HEIGHT/2+1);
	printf("[%.2d:%.2d:%.2d]",hour,minute,second);
	gotoxy(0,HEIGHT+5);
	Sleep(1000);
	printf("[R]重新開始/[Q]退出\n");
	char ch;
	while(ch!='r'&&ch!='R'&&ch!='q'&&ch!='Q'){
		ch=getch();
	}
	if(ch=='r'||ch=='R'){
		quit=0;
		BGM("bgm.mp3",bgm_switch);
		restart();
		gotoxy(0,HEIGHT+5);
		printf("                   ");
	}
	if(ch=='q'||ch=='Q'){
		quit=1;
	}
}

void setCursorVisable(int v){ 
	CONSOLE_CURSOR_INFO cursor_info = {100,v};
	SetConsoleCursorInfo(hand,&cursor_info);
}

void hold(){
	
	int i,j,blank=1;
	for(i=0;i<block_w;i++){
		for(j=0;j<block_h;j++){
			if(block_hold[i][j]==1)blank=0;
		}
	}
	erase_block(block_hold,hold_frame_x+1,hold_frame_y+1);
	if(blank==1){
		copy_block(block_cur,block_hold);
		make_new_block();
	}
	else{
		copy_block(block_hold,temp_block);
		copy_block(block_cur,block_hold);
		copy_block(temp_block,block_cur);
	}
	cur_x=(WIDTH-block_w)/2;
	if(isCollision(cur_x,-1)==0){
		cur_y=-1;
	}
	else{
		cur_y=0;
	}
	print_surface();
	print_block(block_hold,hold_frame_x+1,hold_frame_y+1);
}

void rotate_block(){
	int temp[block_h][block_w]={0};
	copy_block(block_cur,temp);
	int w,h;
	for(w=0;w<block_w;w++){
		for(h=0;h<block_h;h++){
			block_cur[h][w]=temp[block_w-1-w][h];
		}
	}
	
}

int key_control(){
	time_t start = time(NULL),stop;
	int GAMEOVER=0,temp_music_switch=-1,pause_bgm_switch,i,j;
	char ch;
	MSG msg,msg_count;
	while(1){
		int temp_ori_cur_x,temp_ori_cur_y;
		setCursorVisable(0);
		if(PeekMessage(&msg,NULL,WM_TIMER,WM_TIMER,PM_REMOVE)!=0){
			GAMEOVER=move_block_down(start);
			if(GAMEOVER==1){
				game_over(start);
				if(quit==1){
					gotoxy(0,HEIGHT+6);
					return 0;
				}
			}
		}
		if(kbhit()!=0){
			ch=getch();
			if(ch=='z'||ch=='Z'){
				if(hold_chance==1){
					hold();
					hold_chance=0;
				}
			}
			if(ch=='b'||ch=='B'){
				bgm_switch--;
				bgm_switch*=-1;
				bgm_switch++;
				BGM("bgm.mp3",bgm_switch);
				print_sound();
			}
			if(ch=='m'){
				music_switch+=temp_music_switch;
				temp_music_switch*=-1;
				print_sound();
			}
			if(ch=='p'){
				pause_bgm_switch=bgm_switch;
				BGM("bgm.mp3",0);
				printxy("[PAUSE]",WIDTH/2-3,HEIGHT/2);
				gotoxy(WIDTH+2,HEIGHT-1);
				system("pause");
				BGM("bgm.mp3",pause_bgm_switch);
				print_surface();
			}
			if(ch=='q'||ch=='Q'){
				gotoxy(0,HEIGHT+6);
				return 0;
			}
			if(ch=='r'||ch=='R'){
				restart();
			}
			
			switch (ch){
				case 72:	
					Music("bottom.wav",music_switch);
					erase_shadow(block_cur,cur_x,cur_y);
					erase_block(block_cur,cur_x,cur_y);
					rotate_block();
					if(isCollision(cur_x,cur_y)==1){
						temp_ori_cur_x=cur_x;
						for(i=0;isCollision(cur_x,cur_y)==1&&i<2;i++){
							cur_x--;
						}
					}
					if(isCollision(cur_x,cur_y)==1){
						cur_x=temp_ori_cur_x;
						for(i=0;isCollision(cur_x,cur_y)==1&&i<2;i++){
							cur_x++;
						}
					}
					if(isCollision(cur_x,cur_y)==1){
						cur_x=temp_ori_cur_x;
						temp_ori_cur_y=cur_y;
						if(isCollision(cur_x,cur_y+1)==0){
							cur_y++;
						}
						if(isCollision(cur_x,cur_y+1)==0&&isCollision(cur_x,cur_y)==1){
							cur_y++;
						}
					}	
					if(isCollision(cur_x,cur_y)==1){
						cur_x=temp_ori_cur_x;
						cur_y=temp_ori_cur_y;
						rotate_block();
						rotate_block();
						rotate_block();
					}
					print_shadow(block_cur,cur_x,cur_y);
					print_block(block_cur,cur_x,cur_y);
					break;
				case 80:
					if(isCollision(cur_x,cur_y+1)==0){
						Music("bottom.wav",music_switch);
						erase_shadow(block_cur,cur_x,cur_y);
						erase_block(block_cur,cur_x,cur_y);
						cur_y++;
						print_shadow(block_cur,cur_x,cur_y);
						print_block(block_cur,cur_x,cur_y);
					}
					break;
				case 75:
					if(isCollision(cur_x-1,cur_y)==0){
						Music("bottom.wav",music_switch);
						erase_shadow(block_cur,cur_x,cur_y);
						erase_block(block_cur,cur_x,cur_y);
						cur_x--;
						print_shadow(block_cur,cur_x,cur_y);
						print_block(block_cur,cur_x,cur_y);
					}
					break;
				case 77:
					if(isCollision(cur_x+1,cur_y)==0){
						Music("bottom.wav",music_switch);
						erase_shadow(block_cur,cur_x,cur_y);
						erase_block(block_cur,cur_x,cur_y);
						cur_x++;
						print_shadow(block_cur,cur_x,cur_y);
						print_block(block_cur,cur_x,cur_y);
					}
					break;
				case 32:
					Music("bottom.wav",music_switch);
					erase_block(block_cur,cur_x,cur_y);
					while(isCollision(cur_x,cur_y+1)==0)cur_y++;
					print_block(block_cur,cur_x,cur_y);
					break;
			}
		}
	}
}


int main() {
	
	srand(time(NULL));
	hand = GetStdHandle(STD_OUTPUT_HANDLE);
	setCursorVisable(0);
	
	int i,a,b;
	for(i=0;i<=BLOCK_QUEUE;i++)make_new_block();
	PRINT_INIT();
	BGM("bgm.mp3",bgm_switch);
	print_block(block_cur,cur_x,cur_y);
	print_shadow(block_cur,cur_x,cur_y);
	set_timer(speed);
	key_control();
	
	return 0;
}
