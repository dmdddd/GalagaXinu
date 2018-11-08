/* GALAGA */

// Includes
#include <conf.h>
#include <kernel.h>
#include <io.h>
#include <bios.h>
#include<stdio.h>
#include<stdlib.h>  // delay()
#include <dos.h>
#include <time.h>
// Global defines
#define ARROW_NUMBER 5
#define TARGET_NUMBER_PER_WAVE 8
#define NUMBER_OF_WAVES 5
#define TIME_BETWEEN_WAVES 80
#define ENEMY_SHOT '"'
// Movement patterns
#define BASIC_DESCENT 1
#define RIGHT_DIAGONAL 2
#define LEFT_DIAGONAL 3
#define SQUARE_MOVEMENT 4
#define STATIC_STATE 5
#define DESCEND_TO_3_AND_MOVE 6
#define DESCEND_TO_6_AND_MOVE 7
#define DESCEND_TO_9_AND_MOVE 8
// Enemy units' shapes
#define SHAPE1 1 // =0=
#define SHAPE2 2 // -0-
#define SHAPE3 3 // *V*
// Enemy unit's abilities
#define REGULAR 1 //Only moving
#define SHOOTING 2 //Moving and shooting at the player
#define CRASHING 3 //Leaving it's formation to try and hit the player
#define LASER 4 //Shooting a laser that catches the player
// Color defines
#define BLUE_TEXT 1        // foreground
#define GRAY_BACKGROUND 112 // background
#define WHITE_TEXT 7 // foreground
#define GREEN_TEXT 2 // foreground
#define LIGHT_BLUE_TEXT 3 // foreground
#define PRESS_ENTER_FONT 135
// #define GREEN_BACKGROUND 32 
#define ORANGE_BACKGROUND 96
#define ORANGE_BACKGROUND_GREEN_TEXT 98
#define ORANGE_BACKGROUND_LIGHT_BLUE_TEXT 99
// Sound defines
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349



// #define Cchord  16.35
#define Cchord  523.25
// #define Gchord  24.50
#define Gchord  783.99
// #define Dchord  20
#define Dchord  587.33
// #define Achord  27.50
#define Achord  880.00

// Game states
enum GAME_STATE {RUN, LOSE, WIN};
// Enemy's shooting states
enum SHOOTING_STATE {READY, SHOT, NONE};
/*------------------------------------------------------------------------
 * global variables
 *------------------------------------------------------------------------
 */ 
int arrow_shot=0;
int enemy_shot=0;
int enemy_killed=0;
extern struct intmap far *sys_imp;
extern int movement_counter;
extern int shot_movement_counter;
volatile int move_enemies_flag_speed_1 = 0; // fastest
volatile int move_enemies_flag_speed_2 = 0;
volatile int move_enemies_flag_speed_3 = 0;	// slowest
volatile int move_crashing_enemies_flag = 0;
volatile int move_shots_flag = 0;
volatile int in_welcome_screen = 1;
extern int entry_time_counter;
extern int shooting_counter;
int god_mode_on = 0;
int caught_in_laser = 0;
int release_laser_flag=0;
int sound_on = 1;
extern int on_laser_flag;
extern  int return_to_place;

int ship_in_laser_flag=0;

int volatile send_enemy_crashing = 0;
typedef struct position
{
  int x;
  int y;
} POSITION;
typedef struct enemy_unit
{
	POSITION enemy_position;
	int ability;
	int is_alive;
	enum SHOOTING_STATE shooting_state;
	POSITION shot_position;
	int shooting_frequency;
	int caught_player;
} ENEMY_UNIT;
typedef struct crashing_enemy
{
	ENEMY_UNIT enemy;
	POSITION target_location;
	int original_wave;
	int original_index;
} CRASHING_ENEMY_UNIT;
typedef struct wave
{
	ENEMY_UNIT enemy_units[TARGET_NUMBER_PER_WAVE];
	int shape;
	int entry_time;
	int movement_patern;
	int movement_counter;
	int appeared;		//wave appeared on the screen / did not appear yet
	int cleared;	//wave is cleared (destroyed)
	int movement_speed;	//dictates how fast the wave will move
} WAVE;
WAVE stage1_waves[TARGET_NUMBER_PER_WAVE];
WAVE stage2_waves[TARGET_NUMBER_PER_WAVE];
WAVE stage3_waves[TARGET_NUMBER_PER_WAVE];
WAVE *get_current_stage_waves();		// returns an array of waves for the current stage
CRASHING_ENEMY_UNIT crashing_enemy_units[NUMBER_OF_WAVES];

// Game states
enum GAME_STATE game_state;

POSITION player_ship_position = {80, 22};
POSITION target_pos[TARGET_NUMBER_PER_WAVE];
POSITION arrow_pos[ARROW_NUMBER];


unsigned char far *b800h;         //array[4000]
// int send_enemy_crashing;
int stage_number = 0;
int display_stage_number_flag = 0;
int player_score = 0;
int difficulty = 1;
int number_of_arrows = 0;

char display_draft[25][160];


char ch_arr[2048];
int front = -1;
int rear = -1;


int point_in_cycle;
int gcycle_length;
int gno_of_pids;


int receiver_pid;
int music_pid;
/*------------------------------------------------------------------------
 * function declarations
 *------------------------------------------------------------------------
 */
void music_process();
void print_player_ship();
void print_player_arrows();
void print_status_bar();
void clear_display_matrix();
void print_enemies();
void print_crashing_enemies();
void print_game_over();
void print_victory();
void user_hit_enemy_check();
void mark_waves_as_cleared();
void user_is_hit_check();
void advance_enemy_formations();
void advance_crashing_enemies_units();
void initialize_enemies();
void enemy_shooting_check();
extern SYSCALL sleept(int);
void sound_beep(int, int interval);
void end_game();
void play_ending_sound();
void ChangeSpeaker( int status );
void init_enemy_position_based_on_movement_pattern(WAVE*, int);
void advance_stage();
void show_welcome_screen();
void display_stage_number();
void start_crashing_into_player(ENEMY_UNIT*, int wave, int index);
void increase_difficulty();
void decrease_difficulty();
void clear_stage();
void advance_enemies_with_certain_speed(int speed);
void drag_with_laser(int enemy_pos);


INTPROC new_int9(int mdevno)
{
	char result = 0;
	int scan = 0;
	int ascii = 0;
	asm {
			MOV AH,1
			INT 16h
			JZ Skip1
			MOV AH,0
			INT 16h
			MOV BYTE PTR scan,AH
			MOV BYTE PTR ascii,AL
	} //asm
	switch(scan){
		case 75: {result = 'a'; break;}
		case 72: {result = 'w'; break;}
		case 77: {result = 'd'; break;}
		case 27: {result = ']'; break;}	// ]
		case 26: {result = '['; break;}	// [
		case 16: {result = 'q'; break;} //  Q
		case 46: {asm INT 27;} // CTRL+C (terminate xinu)
		case 34: {result = 'g'; break;} // G
		case 31: {result = 's'; break;} // S
		case 28: {result = 'e'; in_welcome_screen = 0; break;} // ENTER
		case 1: {result = 'x'; break;} // ESCAPE
		default: break;
		
	}

	send(receiver_pid, result); 
	
	Skip1:
} // new_int9

void set_new_int9_newisr()
{
  int i;
  for(i=0; i < 32; i++)
    if (sys_imp[i].ivec == 9)
    {
     sys_imp[i].newisr = new_int9;
     return;
    }


} // set_new_int9_newisr


/*------------------------------------------------------------------------
 *  prntr  --  print a character indefinitely
 *------------------------------------------------------------------------
 */
void displayer( void )
{
        while(1)
        {
                int i, j;
                char to_print;
                receive();        
                for(i=0; i < 25; i++)
                        for(j=0; j < 160; j++)
                        {
                                b800h[i*160+j]=display_draft[i][j];
                        }
        }
} // prntr

void receiver()
{
  while(1)
  {
    char temp;
    temp = receive();
    rear++;
    ch_arr[rear] = temp;
    if (front == -1)
       front = 0;
    //getc(CONSOLE);
  } // while


} //  receiver

void updateter()
{


        int i,j;          
        int target_disp = 80/TARGET_NUMBER_PER_WAVE;
        char ch;
		
		initialize_enemies();
		
		game_state = RUN;


        target_pos[0].x = 3;
        target_pos[0].y = 1; 

        // Clearing arrows
        for(i=0; i < ARROW_NUMBER; i++)
           arrow_pos[i].x =  arrow_pos[i].y = -1;
		
		show_welcome_screen();
		while(in_welcome_screen == 1);
		advance_stage();

        // main loop:
        while(1) {
			receive();
			while(front != -1)
			{
				ch = ch_arr[front];
				if(front != rear)
					front++;
				else
					front = rear = -1;
				
				switch (ch){
					case 'a': case 'A': {if (player_ship_position.x >= 4 && caught_in_laser ==0) player_ship_position.x-=2; break;}
					case 'd': case 'D': {if (player_ship_position.x <= 154 && caught_in_laser ==0) player_ship_position.x+=2; break;}
					case ']': {if (difficulty < 3) increase_difficulty(); break;}
					case '[': {if (difficulty > 1) decrease_difficulty(); break;}
					case 'e': {in_welcome_screen = 0; break;}
					case 's': {sound_on = 1 - sound_on; break;}
					case 'g': {god_mode_on = 1 - god_mode_on; break;}
					case 'q': {if (in_welcome_screen == 0) clear_stage(); break;}
					case 'w': case 'W': 
					{
						if (number_of_arrows < ARROW_NUMBER && caught_in_laser ==0)                        // We can shoot another arrow
							for (i=0; i< ARROW_NUMBER; i++)                                // Find an empty slot for the arrow
								if (arrow_pos[i].x == -1){                                // If empty slot found: add arrow
									arrow_pos[i].x = player_ship_position.x;
									arrow_pos[i].y = 23;
									number_of_arrows++;
									// if (sound_on) sound_beep(2800, 1);
									if (sound_on) {arrow_shot=1; send(music_pid, 'a');}
									break;
								} // if
						break;
					}
					case 'x':
					{
						asm{
							XOR AH,AH
							MOV AL,13h
							INT 10h
						}
						end_game(); 
					}
				}// switch				
					
			} // while(front != -1)

			 ch = 0;
			 
			// Our code
			
			if(release_laser_flag == 1 && caught_in_laser == 1){
				release_laser_flag = 0;
				caught_in_laser = 0;
			}
			
			if(player_ship_position.y!=22 && caught_in_laser!=1 && on_laser_flag ==0)
			player_ship_position.y=22;
		 
			clear_display_matrix();
			print_player_ship();
			print_player_arrows();
			print_enemies();
			print_crashing_enemies();
			print_status_bar();
			enemy_shooting_check();
			if (god_mode_on == 0)
				user_is_hit_check();
			user_hit_enemy_check();
			mark_waves_as_cleared();
			
			// if we have advanced a stage, show a stage number of the screen
			display_stage_number();
			
			if(game_state == LOSE){
				print_game_over();
				break;
			}
			
			if(game_state == WIN){
				print_victory();
				break;
			}

	} // while(1)
} // updater 

void music_process(){
	ChangeSpeaker( 1 );
	while (1){
		receive();
		if (game_state == LOSE)
		{
			play_ending_sound();
			break;
		}
		if (in_welcome_screen == 1)
		{
			sound_beep(Cchord, 4);
			sound_beep(Gchord, 4);
			sound_beep(Dchord, 4);
			sound_beep(Achord, 4);
			// break;
		}
		if(arrow_shot){
			arrow_shot = 0;
			sound_beep(2800, 2);
		}
		if(enemy_shot){
			enemy_shot = 0;
			sound_beep(4000, 2);
		}
		if(enemy_killed){
			enemy_killed = 0;
			sound_beep(4000, 2);
		}
	}
	ChangeSpeaker( 0 );
}

int sched_arr_pid[5] = {-1};
int sched_arr_int[5] = {-1};

SYSCALL schedule(int no_of_pids, int cycle_length, int pid1, ...)
{
  int i;
  int ps;
  int *iptr;

  disable(ps);

  gcycle_length = cycle_length;
  point_in_cycle = 0;
  gno_of_pids = no_of_pids;

  iptr = &pid1;
  for(i=0; i < no_of_pids; i++)
  {
    sched_arr_pid[i] = *iptr;
    iptr++;
    sched_arr_int[i] = *iptr;
    iptr++;
  } // for
  restore(ps);
} // schedule 
//===========================================================================================================================================
// Functions
//===========================================================================================================================================
void initiate_display_mode()
{
        asm{
                        PUSH AX
                        MOV AH,0
                        MOV AL,03
                        INT 10h
                        
                        MOV AX, 0B800h
                        MOV WORD PTR b800h+2, AX
                        MOV WORD PTR b800h, 0
                        
                        POP AX
                }
}
void print_player_ship(){
        display_draft[player_ship_position.y][player_ship_position.x] = '^';
        display_draft[player_ship_position.y][player_ship_position.x+1] = BLUE_TEXT;
        display_draft[player_ship_position.y+1][player_ship_position.x-2] = '\\';
        display_draft[player_ship_position.y+1][player_ship_position.x-2+1] = BLUE_TEXT;
        display_draft[player_ship_position.y+1][player_ship_position.x] = '#';
        display_draft[player_ship_position.y+1][player_ship_position.x+1] = BLUE_TEXT;
        display_draft[player_ship_position.y+1][player_ship_position.x+2] = '/';
        display_draft[player_ship_position.y+1][player_ship_position.x+2+1] = BLUE_TEXT;
        display_draft[player_ship_position.y+2][player_ship_position.x] = '#';
        display_draft[player_ship_position.y+2][player_ship_position.x+1] = BLUE_TEXT;
}
void print_player_arrows(){
        int i;
        // deleting arrows that got to the status bar
        for(i=0; i < ARROW_NUMBER; i++)
                   if (arrow_pos[i].y == 1){
                                arrow_pos[i].x = arrow_pos[i].y = -1;
                                number_of_arrows--;
                   }


        // printing arrows
        for(i=0; i < ARROW_NUMBER; i++)
                   if (arrow_pos[i].x != -1)
                   {
                         if (arrow_pos[i].y > 1){
                           arrow_pos[i].y--;
                           display_draft[arrow_pos[i].y][arrow_pos[i].x] = '^';
                           display_draft[arrow_pos[i].y][arrow_pos[i].x+1] = WHITE_TEXT;
                           display_draft[arrow_pos[i].y+1][arrow_pos[i].x] = '|';
                           display_draft[arrow_pos[i].y+1][arrow_pos[i].x+1] = WHITE_TEXT;
                         }
                         // else {
                                // arrow_pos[i].x = arrow_pos[i].y = -1;
                                // number_of_arrows--;
                         // }


                   } // if
}
void clear_display_matrix(){
        int i, j;
        // clearing the display_draft matrix to start updating it again
        for(i=0; i < 25; i++)
                for(j=0; j < 160; j++){
                        if(j%2 == 0)
                                display_draft[i][j]=' ';
                        else
                                display_draft[i][j]=0;
                }
}
void print_enemies(){
	int i, j, k, enemy_x, enemy_y, to_print;
	WAVE *current_stage_waves = get_current_stage_waves();
	to_print = 0;
	// Move enemy units if needed:
	if (move_crashing_enemies_flag == 1){
		move_crashing_enemies_flag = 0;
		advance_crashing_enemies_units();
	}
	if (move_enemies_flag_speed_1 == 1 || move_enemies_flag_speed_2 == 1 || move_enemies_flag_speed_3 == 1){
		advance_enemy_formations();
	}	
	
	// print the targets of every wave
	for(i =0; i<NUMBER_OF_WAVES; i++)
	{
		if(current_stage_waves[i].cleared != 1 && current_stage_waves[i].entry_time <= entry_time_counter){
			current_stage_waves[i].appeared = 1;		// The wave should be showing on the screen, we will start advancing it
			for(j =0; j<TARGET_NUMBER_PER_WAVE; j++){
				enemy_x = current_stage_waves[i].enemy_units[j].enemy_position.x;
				enemy_y = current_stage_waves[i].enemy_units[j].enemy_position.y;
				if(enemy_y >=1 && enemy_y <= 24 && enemy_x > -1 && enemy_x < 159)
				{
					//PRINT LASER
						if (current_stage_waves[i].enemy_units[j].ability == LASER)
						{
							k = 1;
							if(on_laser_flag==1){
							while ((enemy_y + k)<=24)
							{
								if(display_draft[enemy_y+k][enemy_x] == '=' || display_draft[enemy_y+k][enemy_x] == '-' || display_draft[enemy_y+k][enemy_x] == '*' || display_draft[enemy_y+k][enemy_x] == '0' || display_draft[enemy_y+k][enemy_x] == 'V' || display_draft[enemy_y+k][enemy_x] == '\\' || display_draft[enemy_y+k][enemy_x] == '/')
									display_draft[enemy_y+k][enemy_x+1]=ORANGE_BACKGROUND_GREEN_TEXT;
								else if(display_draft[enemy_y+k][enemy_x] == '=' || display_draft[enemy_y+k][enemy_x] == ENEMY_SHOT)
									display_draft[enemy_y+k][enemy_x+1]=ORANGE_BACKGROUND_LIGHT_BLUE_TEXT;
								else
									display_draft[enemy_y+k][enemy_x+1]=ORANGE_BACKGROUND;
								k++;
							}
							}
						}
					switch(current_stage_waves[i].shape){
					case SHAPE1:
						display_draft[enemy_y][enemy_x-2]='=';
						if (display_draft[enemy_y][enemy_x-1] == ORANGE_BACKGROUND)
							display_draft[enemy_y][enemy_x-1]=ORANGE_BACKGROUND_GREEN_TEXT;
						else
							display_draft[enemy_y][enemy_x-1]=GREEN_TEXT;
						display_draft[enemy_y][enemy_x]='0';
						if (display_draft[enemy_y][enemy_x+1] == ORANGE_BACKGROUND)
							display_draft[enemy_y][enemy_x+1]=ORANGE_BACKGROUND_GREEN_TEXT;
						else
							display_draft[enemy_y][enemy_x+1]=GREEN_TEXT;
						display_draft[enemy_y][enemy_x+2]='=';
						if (display_draft[enemy_y][enemy_x+3] == ORANGE_BACKGROUND)
							display_draft[enemy_y][enemy_x+3]=ORANGE_BACKGROUND_GREEN_TEXT;
						else
							display_draft[enemy_y][enemy_x+3]=GREEN_TEXT;
					break;
					case SHAPE2:
						display_draft[enemy_y][enemy_x-2]='-';
						if (display_draft[enemy_y][enemy_x-1] == ORANGE_BACKGROUND)
							display_draft[enemy_y][enemy_x-1]=ORANGE_BACKGROUND_GREEN_TEXT;
						else
							display_draft[enemy_y][enemy_x-1]=GREEN_TEXT;
						display_draft[enemy_y][enemy_x]='0';
						if (display_draft[enemy_y][enemy_x+1] == ORANGE_BACKGROUND)
							display_draft[enemy_y][enemy_x+1]=ORANGE_BACKGROUND_GREEN_TEXT;
						else
							display_draft[enemy_y][enemy_x+1]=GREEN_TEXT;
						display_draft[enemy_y][enemy_x+2]='-';
						if (display_draft[enemy_y][enemy_x+3] == ORANGE_BACKGROUND)
							display_draft[enemy_y][enemy_x+3]=ORANGE_BACKGROUND_GREEN_TEXT;
						else
							display_draft[enemy_y][enemy_x+3]=GREEN_TEXT;
					break;
					case SHAPE3:
						display_draft[enemy_y][enemy_x-2]='*';
						if (display_draft[enemy_y][enemy_x-1] == ORANGE_BACKGROUND)
							display_draft[enemy_y][enemy_x-1]=ORANGE_BACKGROUND_GREEN_TEXT;
						else
							display_draft[enemy_y][enemy_x-1]=GREEN_TEXT;
						display_draft[enemy_y][enemy_x]='V';
						if (display_draft[enemy_y][enemy_x+1] == ORANGE_BACKGROUND)
							display_draft[enemy_y][enemy_x+1]=ORANGE_BACKGROUND_GREEN_TEXT;
						else
							display_draft[enemy_y][enemy_x+1]=GREEN_TEXT;
						display_draft[enemy_y][enemy_x+2]='*';
						if (display_draft[enemy_y][enemy_x+3] == ORANGE_BACKGROUND)
							display_draft[enemy_y][enemy_x+3]=ORANGE_BACKGROUND_GREEN_TEXT;
						else
							display_draft[enemy_y][enemy_x+3]=GREEN_TEXT;
					break;
					}	
				}
			}
		}
	}
}
void print_crashing_enemies(){
	int i, enemy_x, enemy_y;
	for(i=0; i<NUMBER_OF_WAVES; i++){
		enemy_x = crashing_enemy_units[i].enemy.enemy_position.x;
		enemy_y = crashing_enemy_units[i].enemy.enemy_position.y;
		if(enemy_x >= 2 && enemy_y >= 1 && enemy_x < 159 && enemy_y <= 24){
			display_draft[enemy_y][enemy_x-2]='\\';
			display_draft[enemy_y][enemy_x-1]=GREEN_TEXT;
			display_draft[enemy_y][enemy_x]='V';
			display_draft[enemy_y][enemy_x+1]=GREEN_TEXT;
			display_draft[enemy_y][enemy_x+2]='/';
			display_draft[enemy_y][enemy_x+3]=GREEN_TEXT;
		}
	}
}
void print_status_bar(){
        int i, score;
		char stage_tag[7] = {'S', 't', 'a', 'g', 'e', ':', ' '};
		char score_tag[12] = {'S', 'c', 'o', 'r', 'e', ':', ' ', '0', '0', '0', '0', '0'};
		char difficulty_tag[12] = {'D', 'i', 'f', 'f', 'i', 'c', 'u', 'l', 't', 'y', ':', ' '};
		char mute_tag[6] = {'-', 'M', 'u', 't', 'e', '-'};
        char digit;
        for(i=0; i<160; i+=2){
                display_draft[0][i] = ' ';
                display_draft[0][i+1] = GRAY_BACKGROUND;
        }
		for(i=0; i<7; i++)		// Printing "Stage: "
                display_draft[0][2*i] = stage_tag[i];
			
        if (stage_number == 1)
                display_draft[0][14] = '1';
        else if (stage_number == 2)
                display_draft[0][14] = '2';
        else
                display_draft[0][14] = '3';

        for(i=0; i<12; i++)// Printing "Score: 00000"
                display_draft[0][50 + 2*i] = score_tag[i];
			
        // breaking down the score to digits and printing it
        score = player_score; // local variable, we don't want to change the score
        i = 0;
        while (score >0){
                digit = score % 10;
                display_draft[0][72-i] = digit+'0';
                score /= 10;
                i+=2;
        }
        for(i=0; i<12; i++)		// Printing "Difficulty: "
                display_draft[0][134 + 2*i] = difficulty_tag[i];
				
        if(difficulty == 1)
                display_draft[0][158] = '1';
        else if (difficulty == 2)
                display_draft[0][158] = '2';
        else
                display_draft[0][158] = '3';
        
		if (god_mode_on)
		{
			display_draft[0][96] = '-';
			display_draft[0][98] = 'G';
			display_draft[0][100] = '-';
		}
		if (sound_on == 0)
		{
			for(i=0; i<6; i++)// Printing "Score: 00000"
                display_draft[0][110 + 2*i] = mute_tag[i];
		}
}

void user_hit_enemy_check()
{
	int i,j,k,center_x,center_y, right_wing_x, left_wing_x, right_wing_y, left_wing_y;
	WAVE *current_stage;
	
	current_stage = get_current_stage_waves();
	
	center_x = -1;
	center_y = -1; 
	right_wing_x = -1; 
	left_wing_x = -1; 
	right_wing_y = -1; 
	left_wing_y = -1;
	
	for(i = 0; i < ARROW_NUMBER; i++)
	{
		if(arrow_pos[i].y > 1) {
			if(display_draft[arrow_pos[i].y][arrow_pos[i].x] == '0' || display_draft[arrow_pos[i].y][arrow_pos[i].x] == 'V')
			{
				center_x = arrow_pos[i].x;
				center_y = arrow_pos[i].y;
				arrow_pos[i].x = -1;
			}
			if(display_draft[arrow_pos[i].y-1][arrow_pos[i].x] == '0' || display_draft[arrow_pos[i].y-1][arrow_pos[i].x] == 'V')
			{
				center_x = arrow_pos[i].x;
				center_y = arrow_pos[i].y-1;
				arrow_pos[i].x = -1;
			}
			if(display_draft[arrow_pos[i].y][arrow_pos[i].x+2] == '0' || display_draft[arrow_pos[i].y][arrow_pos[i].x+2] == 'V')
			{
				right_wing_x = arrow_pos[i].x+2;
				right_wing_y = arrow_pos[i].y;
				arrow_pos[i].x = -1;
			}
			if(display_draft[arrow_pos[i].y-1][arrow_pos[i].x+2] == '0' || display_draft[arrow_pos[i].y-1][arrow_pos[i].x+2] == 'V')
			{
				right_wing_x = arrow_pos[i].x+2;
				right_wing_y = arrow_pos[i].y-1;
				arrow_pos[i].x = -1;
			}
				if(display_draft[arrow_pos[i].y][arrow_pos[i].x-2] == '0' || display_draft[arrow_pos[i].y][arrow_pos[i].x-2] == 'V')
			{
				left_wing_x = arrow_pos[i].x-2;
				left_wing_y = arrow_pos[i].y;
				arrow_pos[i].x = -1;
			}
			if(display_draft[arrow_pos[i].y-1][arrow_pos[i].x-2] == '0' || display_draft[arrow_pos[i].y-1][arrow_pos[i].x-2] == 'V')
			{
				left_wing_x = arrow_pos[i].x-2;
				left_wing_y = arrow_pos[i].y-1;
				arrow_pos[i].x = -1;
			}
		}	
	}
	
	
	if((center_x != -1 && center_y != -1) || (right_wing_x != -1 && right_wing_y != -1) || (left_wing_x != -1 && left_wing_y != -1)) //Arrow and target intersect
	{
		for(i = 0; i < NUMBER_OF_WAVES; i++)
			for(j = 0; j < TARGET_NUMBER_PER_WAVE; j++)
				if(current_stage[i].enemy_units[j].is_alive == 1)
				{
					if((current_stage[i].enemy_units[j].enemy_position.x == center_x && current_stage[i].enemy_units[j].enemy_position.y == center_y) || (current_stage[i].enemy_units[j].enemy_position.x == right_wing_x && current_stage[i].enemy_units[j].enemy_position.y == right_wing_y) || (current_stage[i].enemy_units[j].enemy_position.x == left_wing_x && current_stage[i].enemy_units[j].enemy_position.y == left_wing_y))
					{
								current_stage[i].enemy_units[j].enemy_position.x = -1;
								current_stage[i].enemy_units[j].enemy_position.y = -1;
								current_stage[i].enemy_units[j].is_alive = 0;
					}
				}
	 	for(i = 0; i < NUMBER_OF_WAVES; i++)		
			if(crashing_enemy_units[i].original_index != -1 && ((crashing_enemy_units[i].enemy.enemy_position.x == center_x && crashing_enemy_units[i].enemy.enemy_position.y == center_y) || (crashing_enemy_units[i].enemy.enemy_position.x == right_wing_x && crashing_enemy_units[i].enemy.enemy_position.y == right_wing_y) || (crashing_enemy_units[i].enemy.enemy_position.x == left_wing_x && crashing_enemy_units[i].enemy.enemy_position.y == left_wing_y)))
			{
				j = crashing_enemy_units[i].original_wave;
				k = crashing_enemy_units[i].original_index;
				current_stage[j].enemy_units[k].is_alive = 0;
				crashing_enemy_units[i].enemy.enemy_position.x = -1;
				crashing_enemy_units[i].enemy.enemy_position.y = -1;				
			} 
		player_score += 100;
		// if (sound_on) sound_beep(1000, 1);
		if (sound_on) {enemy_killed = 1; send(music_pid, 'k');}
		number_of_arrows--;
	}
}
// Goes over the waves of the current stage and marks the cleared ones as "cleared"
void mark_waves_as_cleared()
{
	int i, j, allCleared = 1, waveDestroyed = 1, waveOutside = 1;
	WAVE *current_stage_waves;
	
	current_stage_waves = get_current_stage_waves();
	
	for(i = 0; i < NUMBER_OF_WAVES; i++)
		if(current_stage_waves[i].cleared != 1)		//if current wave is not cleared
		{
			allCleared = 0;
			for(j = 0; j < TARGET_NUMBER_PER_WAVE; j++)
			{
				if(current_stage_waves[i].enemy_units[j].enemy_position.x != -1 && current_stage_waves[i].enemy_units[j].enemy_position.y != -1)
					waveOutside = 0;
				if(current_stage_waves[i].enemy_units[j].is_alive == 1)
					waveDestroyed = 0;
				
			}
				
			if(waveDestroyed == 1 || waveOutside == 1)
			{
				current_stage_waves[i].cleared = 1;
				if(waveDestroyed == 1)
					player_score += 1000;
			}
		}
	
	if(allCleared == 1)
		advance_stage();
	
	if(stage_number > 3)
		game_state = WIN;
}

WAVE *get_current_stage_waves()
{	
	WAVE *current_stage;
		switch(stage_number)
	{
		case 1:
			current_stage = stage1_waves;
		break;
		case 2:
			current_stage = stage2_waves;
		break;
		case 3:
			current_stage = stage3_waves;
		break;
	}
	
	return current_stage;
}
void advance_crashing_enemies_units(){
	int i, j;
	int enemy_x, enemy_y, target_x, target_y;
	for(i=0; i < NUMBER_OF_WAVES; i++){
		enemy_x = crashing_enemy_units[i].enemy.enemy_position.x;
		enemy_y = crashing_enemy_units[i].enemy.enemy_position.y;
		target_x = crashing_enemy_units[i].target_location.x;
		target_y = crashing_enemy_units[i].target_location.y;
		if (enemy_x != -1){	// enemy is alive
			if(enemy_x < target_x && enemy_y < target_y){		//left of the target
				crashing_enemy_units[i].enemy.enemy_position.x += 2;
				crashing_enemy_units[i].enemy.enemy_position.y ++;
			}
			else if(enemy_x > target_x && enemy_y < target_y){	// right of the target
				crashing_enemy_units[i].enemy.enemy_position.x -= 2;
				crashing_enemy_units[i].enemy.enemy_position.y ++;
			}
			else if(enemy_x == target_x){						// in front of the target
				crashing_enemy_units[i].enemy.enemy_position.y ++;
			}
			else if(enemy_y == target_y && enemy_x < target_x ){	//same height, from the left
				crashing_enemy_units[i].enemy.enemy_position.x += 2;
				crashing_enemy_units[i].enemy.enemy_position.y ++;
			}
			else if(enemy_y == target_y && enemy_x > target_x ){	//same height, from the right
				crashing_enemy_units[i].enemy.enemy_position.x -= 2;
				crashing_enemy_units[i].enemy.enemy_position.y ++;
			}
			else if((enemy_y == target_y || enemy_y == target_y+1 || enemy_y == target_y+2) && enemy_x == target_x ){	// if enemy was hit, destroy the crushing enemy
				crashing_enemy_units[i].enemy.enemy_position.x = -1;
				crashing_enemy_units[i].enemy.enemy_position.y = -1;
			}
			// else if (enemy_y > target_y)
				// crashing_enemy_units[i].enemy.enemy_position.y ++;
			else{
				crashing_enemy_units[i].enemy.enemy_position.x = -1;
				crashing_enemy_units[i].enemy.enemy_position.y = -1;
			}
		}
	}
}
void advance_enemy_formations(){
	// Advancing only the enemies whose flag is on
	if (move_enemies_flag_speed_1 == 1){
		move_enemies_flag_speed_1 = 0;				// Resetting the movement flag
		advance_enemies_with_certain_speed(1);
		
	}
	if (move_enemies_flag_speed_2 == 1){
		move_enemies_flag_speed_2 = 0;				// Resetting the movement flag
		advance_enemies_with_certain_speed(2);
		
	}
	if (move_enemies_flag_speed_3 == 1){
		move_enemies_flag_speed_3 = 0;				// Resetting the movement flag
		advance_enemies_with_certain_speed(3);
		
	}
}
void advance_enemies_with_certain_speed(int given_speed){
	int i, j, k;
	int enemy_x, enemy_y;
	WAVE *current_stage_waves = get_current_stage_waves();
	int num_of_wave, num_of_enemy, added_crashing_unit = 0;
	int to_crash = 0;
	
	srand(time(NULL));
	
	for(i=0; i < NUMBER_OF_WAVES; i++){
	   if (current_stage_waves[i].cleared != 1 && current_stage_waves[i].appeared == 1 && current_stage_waves[i].movement_speed == given_speed){

		   // Let the crushing enemies leave their formation and start moving
		   for(k=0; k<TARGET_NUMBER_PER_WAVE; k++){
			   if(current_stage_waves[i].enemy_units[k].ability == CRASHING && current_stage_waves[i].enemy_units[k].enemy_position.x != -1){
				   // if the enemy is set to CRASHING, it will move it to the crashing list
					   start_crashing_into_player(&current_stage_waves[i].enemy_units[k], i, k);
					   current_stage_waves[i].enemy_units[k].enemy_position.x = -1;	// means it started crushing
					   current_stage_waves[i].enemy_units[k].enemy_position.y = -1;	// means it started crushing
					   break;

			   }
		   }
		   // Advance enemies based on their movement pattern
			switch(current_stage_waves[i].movement_patern){
				case BASIC_DESCENT:
					for(j=0; j< TARGET_NUMBER_PER_WAVE ; j++){
						enemy_x = current_stage_waves[i].enemy_units[j].enemy_position.x;
						enemy_y = current_stage_waves[i].enemy_units[j].enemy_position.y;
						if(enemy_x != -1 && enemy_y < 28){ // 28 creates move space between same waves
							current_stage_waves[i].enemy_units[j].enemy_position.y++;
						}
						else if(enemy_x != -1){	//left the screen but alive -> come back to the beginning
							current_stage_waves[i].enemy_units[j].enemy_position.y = 0;
						}
					}
					break;
					
				case RIGHT_DIAGONAL:
					for(j=0; j< TARGET_NUMBER_PER_WAVE ; j++){
						enemy_x = current_stage_waves[i].enemy_units[j].enemy_position.x;
						enemy_y = current_stage_waves[i].enemy_units[j].enemy_position.y;
						if(enemy_x != -1 && enemy_y < 25) // is alive and should be on the screen
						{
							current_stage_waves[i].enemy_units[j].enemy_position.x-=4;
							current_stage_waves[i].enemy_units[j].enemy_position.y++;
						}
						else if(enemy_x != -1){	// They are gone, x = y = -1
							// current_stage_waves[i].enemy_units[j].enemy_position.x = current_stage_waves[i].enemy_units[j].enemy_position.y = -1;
							current_stage_waves[i].enemy_units[j].enemy_position.x = 142+4*j;
							current_stage_waves[i].enemy_units[j].enemy_position.y = 0;
						}
					}
					break;
					
				case LEFT_DIAGONAL:
					for(j=0; j< TARGET_NUMBER_PER_WAVE ; j++){
						enemy_x = current_stage_waves[i].enemy_units[j].enemy_position.x;
						enemy_y = current_stage_waves[i].enemy_units[j].enemy_position.y;
						if(enemy_x != -1 && enemy_y < 25)
						{
							current_stage_waves[i].enemy_units[j].enemy_position.x+=4;
							current_stage_waves[i].enemy_units[j].enemy_position.y++;
						}
						else if(enemy_x != -1){	// They are gone, x = y = -1
							// current_stage_waves[i].enemy_units[j].enemy_position.x = current_stage_waves[i].enemy_units[j].enemy_position.y = -1;
							current_stage_waves[i].enemy_units[j].enemy_position.x = 10-4*j;
							current_stage_waves[i].enemy_units[j].enemy_position.y = 0;
						}
					}
					break;
					
				case STATIC_STATE:
					for(j=0; j< TARGET_NUMBER_PER_WAVE ; j++){
						enemy_x = current_stage_waves[i].enemy_units[j].enemy_position.x;
						enemy_y = current_stage_waves[i].enemy_units[j].enemy_position.y;
						if(enemy_x != -1 && enemy_y <= 14){
							current_stage_waves[i].enemy_units[j].enemy_position.y++;
						}
					}
					break;
					
				case DESCEND_TO_3_AND_MOVE:
					for(j=0; j< TARGET_NUMBER_PER_WAVE ; j++){
						enemy_x = current_stage_waves[i].enemy_units[j].enemy_position.x;
						enemy_y = current_stage_waves[i].enemy_units[j].enemy_position.y;
						if(enemy_x != -1 && enemy_y < 3){
							current_stage_waves[i].enemy_units[j].enemy_position.y++;
						}
						else if (enemy_x != -1 && current_stage_waves[i].movement_counter == 1){
								current_stage_waves[i].enemy_units[j].enemy_position.x+=2;
						}
						else if (enemy_x != -1 && current_stage_waves[i].movement_counter == 0){
								current_stage_waves[i].enemy_units[j].enemy_position.x-=2;
						}
					}
					// update to the whole wave, not for a specific enemy
					if (current_stage_waves[i].movement_counter == 1)
						current_stage_waves[i].movement_counter--;
					else
						current_stage_waves[i].movement_counter++;
					break;
					
				case DESCEND_TO_6_AND_MOVE:
					for(j=0; j< TARGET_NUMBER_PER_WAVE ; j++){
						enemy_x = current_stage_waves[i].enemy_units[j].enemy_position.x;
						enemy_y = current_stage_waves[i].enemy_units[j].enemy_position.y;
						if(enemy_x != -1 && enemy_y < 6){
							current_stage_waves[i].enemy_units[j].enemy_position.y++;
						}
						else if (enemy_x != -1 && current_stage_waves[i].movement_counter == 1){
								current_stage_waves[i].enemy_units[j].enemy_position.x-=2;
						}
						else if (enemy_x != -1 && current_stage_waves[i].movement_counter == 0){
								current_stage_waves[i].enemy_units[j].enemy_position.x+=2;
						}
					}
					// update to the whole wave, not for a specific enemy
					if (current_stage_waves[i].movement_counter == 1)
						current_stage_waves[i].movement_counter--;
					else
						current_stage_waves[i].movement_counter++;
					break;
					
				case DESCEND_TO_9_AND_MOVE:
					for(j=0; j< TARGET_NUMBER_PER_WAVE ; j++){
						enemy_x = current_stage_waves[i].enemy_units[j].enemy_position.x;
						enemy_y = current_stage_waves[i].enemy_units[j].enemy_position.y;
						if(enemy_x != -1 && enemy_y < 9){
							current_stage_waves[i].enemy_units[j].enemy_position.y++;
						}
						else if (enemy_x != -1 && current_stage_waves[i].movement_counter == 1){
								current_stage_waves[i].enemy_units[j].enemy_position.x+=2;
						}
						else if (enemy_x != -1 && current_stage_waves[i].movement_counter == 0){
								current_stage_waves[i].enemy_units[j].enemy_position.x-=2;
						}
					}
					if (current_stage_waves[i].movement_counter == 1)
						current_stage_waves[i].movement_counter--;
					else
						current_stage_waves[i].movement_counter++;
					break;
					
				case SQUARE_MOVEMENT:
					for(j=0; j< TARGET_NUMBER_PER_WAVE ; j++){
						enemy_x = current_stage_waves[i].enemy_units[j].enemy_position.x;
						enemy_y = current_stage_waves[i].enemy_units[j].enemy_position.y;
						if(enemy_x != -1 && enemy_y < 5){	//descend
							current_stage_waves[i].enemy_units[j].enemy_position.y++;
						}
						else if (enemy_x != -1 && enemy_x < 138 && enemy_y == 5){	//move right
								current_stage_waves[i].enemy_units[j].enemy_position.x+=2;
						}
						else if (enemy_x != -1 && enemy_x == 138 && enemy_y < 16){	//move down
								current_stage_waves[i].enemy_units[j].enemy_position.y++;
						}
						else if (enemy_x != -1 && enemy_y == 16 && enemy_x > 20){	//move left
								current_stage_waves[i].enemy_units[j].enemy_position.x-=2;
						}
						else if (enemy_x != -1 && enemy_x == 20 && enemy_y > 5){	//move up
								current_stage_waves[i].enemy_units[j].enemy_position.y--;
						}
					}
					break;
			}
		}
	}
	
}
void initialize_enemies(){
	int i, j;
	int target_disp = 160/TARGET_NUMBER_PER_WAVE;
	// ========== STAGE 1 ==========
	// Creating waves
	for (i=0; i<NUMBER_OF_WAVES; i++){
		WAVE wave;
		wave.movement_counter = 1;
		wave.cleared = 0;
		wave.appeared = 0;
		wave.entry_time = i*TIME_BETWEEN_WAVES;
		crashing_enemy_units[i].original_index = -1;
		// Choose movement pattern for each wave
		switch(i){
			case 1: wave.shape = SHAPE2; wave.movement_patern = RIGHT_DIAGONAL; wave.movement_speed = 1; break;
			case 3: wave.shape = SHAPE3; wave.movement_patern = LEFT_DIAGONAL; wave.movement_speed = 1; break;
			case 4: wave.shape = SHAPE2; wave.movement_patern = STATIC_STATE; wave.movement_speed = 3; break;
			default: wave.shape = SHAPE1; wave.movement_patern = BASIC_DESCENT; wave.movement_speed = 2; break;
		}
		stage1_waves[i] = wave;
	}
	// Initialize enemy position based on the wave's movement pattern
	init_enemy_position_based_on_movement_pattern(stage1_waves, 1);
	// ========== STAGE 2 ==========
	for (i=0; i<NUMBER_OF_WAVES; i++){
		WAVE wave;
		wave.movement_counter = 1;
		wave.cleared = 0;
		wave.appeared = 0;
		wave.entry_time = i*TIME_BETWEEN_WAVES;
		// Choose movement pattern for each wave
		switch(i){
			case 1: wave.shape = SHAPE1; wave.movement_patern = DESCEND_TO_9_AND_MOVE; wave.movement_speed = 1; break;
			case 2: wave.shape = SHAPE2; wave.movement_patern = DESCEND_TO_6_AND_MOVE; wave.movement_speed = 3; break;
			case 3: wave.shape = SHAPE1; wave.movement_patern = LEFT_DIAGONAL; wave.movement_speed = 1; break;
			case 4: wave.shape = SHAPE2; wave.movement_patern = STATIC_STATE; wave.movement_speed = 1; break;
			default: wave.shape = SHAPE3; wave.movement_patern = BASIC_DESCENT; wave.movement_speed = 3; break;
			
		}
		stage2_waves[i] = wave;
	}
	// Initialize enemy position based on the wave's movement pattern
	init_enemy_position_based_on_movement_pattern(stage2_waves, 2);
	// // ========== STAGE 3 ==========
	for (i=0; i<NUMBER_OF_WAVES; i++){
		WAVE wave;
		wave.movement_counter = 1;
		wave.cleared = 0;
		wave.appeared = 0;
		wave.entry_time = i*TIME_BETWEEN_WAVES;
		// Choose movement pattern for each wave
		switch(i){
			case 1: wave.shape = SHAPE2; wave.movement_patern = DESCEND_TO_9_AND_MOVE; wave.movement_speed = 2; break;
			case 2: wave.shape = SHAPE1; wave.movement_patern = SQUARE_MOVEMENT; wave.movement_speed = 1; break;
			case 3: wave.shape = SHAPE2; wave.movement_patern = DESCEND_TO_6_AND_MOVE; wave.movement_speed = 2; break;
			case 4: wave.shape = SHAPE3; wave.movement_patern = DESCEND_TO_3_AND_MOVE; wave.movement_speed = 3; break;
			default: wave.shape = SHAPE1; wave.movement_patern = STATIC_STATE; wave.movement_speed = 3; break;
		}
		stage3_waves[i] = wave;
	}
	// Initialize enemy position based on the wave's movement pattern
	init_enemy_position_based_on_movement_pattern(stage3_waves, 3);
}

void enemy_shooting_check()
{
	int i,j,x,y;
	WAVE *current_stage;
	
	current_stage = get_current_stage_waves();
	
	for(i = 0; i < NUMBER_OF_WAVES; i++)
		for(j = 0; j < TARGET_NUMBER_PER_WAVE; j++)
		{
			if(current_stage[i].appeared == 1 && current_stage[i].enemy_units[j].ability == SHOOTING)
			{
				x = current_stage[i].enemy_units[j].enemy_position.x;
				y = current_stage[i].enemy_units[j].enemy_position.y;

				if(current_stage[i].enemy_units[j].shooting_state == READY && shooting_counter > current_stage[i].enemy_units[j].shooting_frequency && y > 2 && y+1 < 25)
				{
					if (sound_on) {enemy_shot = 1; send(music_pid, 'e');}
					display_draft[y+1][x] = ENEMY_SHOT;
					display_draft[y+1][x+1] = LIGHT_BLUE_TEXT;
					current_stage[i].enemy_units[j].shot_position.x = x;
					current_stage[i].enemy_units[j].shot_position.y = y+1;
					current_stage[i].enemy_units[j].shooting_state = SHOT;
				}
				else if (current_stage[i].enemy_units[j].shooting_state == SHOT)
				{	
					if(move_shots_flag == 1)
						current_stage[i].enemy_units[j].shot_position.y++;

					if(current_stage[i].enemy_units[j].shot_position.y < 25)
					{
						display_draft[current_stage[i].enemy_units[j].shot_position.y][current_stage[i].enemy_units[j].shot_position.x] = ENEMY_SHOT;
						if(display_draft[current_stage[i].enemy_units[j].shot_position.y][current_stage[i].enemy_units[j].shot_position.x+1] == ORANGE_BACKGROUND)
							display_draft[current_stage[i].enemy_units[j].shot_position.y][current_stage[i].enemy_units[j].shot_position.x+1] = ORANGE_BACKGROUND_LIGHT_BLUE_TEXT;
						else
							display_draft[current_stage[i].enemy_units[j].shot_position.y][current_stage[i].enemy_units[j].shot_position.x+1] = LIGHT_BLUE_TEXT;
					}
					else
						current_stage[i].enemy_units[j].shooting_state = READY;
				}
			}	
			if(current_stage[i].appeared == 1 && current_stage[i].enemy_units[j].ability == LASER){
			  ship_in_laser_flag=0;
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+1) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1;   drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+2) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+3) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+4) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1;  drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+5) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1;  drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+6) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+7) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+8) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1 ; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+9) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1 ; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x &&on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+10) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1 ; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
				if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+11) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+12) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+13) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1 ; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+14) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1;  drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+15) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y); }
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+16) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1;  drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+17) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1;  drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+18) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y); }
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+19) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1;  drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+20) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1;  drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+21) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y); }
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 && (current_stage[i].enemy_units[j].enemy_position.y+22) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1;  drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
			 	if(current_stage[i].enemy_units[j].enemy_position.x == player_ship_position.x && on_laser_flag==1 &&(current_stage[i].enemy_units[j].enemy_position.y+23) == player_ship_position.y) {current_stage[i].enemy_units[j].caught_player = 1; caught_in_laser = 1; drag_with_laser(current_stage[i].enemy_units[j].enemy_position.y);}
		
			}	
				 
		}
			
	move_shots_flag = 0;
}

void drag_with_laser(int enemy_pos)
{
	if(caught_in_laser==1 &&ship_in_laser_flag==0 && player_ship_position.y !=10){
		player_ship_position.y=enemy_pos+8;
		ship_in_laser_flag=1;
	}
	 
	
	
}
void init_enemy_position_based_on_movement_pattern(WAVE* array_of_waves, int stage){
	int i, j, shoot_1, shoot_2, crashing_enemy_index, laser_enemy_index;
	int target_disp = 160/TARGET_NUMBER_PER_WAVE;
	
	srand(time(NULL));
	
			// Initialize enemy position based on the wave's movement pattern
	for(i=0; i<NUMBER_OF_WAVES; i++){
		shoot_1 = rand() % 8;
		shoot_2 = shoot_1;
		while(shoot_2 == shoot_1)
			shoot_1 = rand() % 8;
		
		crashing_enemy_index = shoot_2;
		while(crashing_enemy_index == shoot_1 || crashing_enemy_index == shoot_2)
			crashing_enemy_index = rand() % 8;
		
		laser_enemy_index = shoot_1;
while(laser_enemy_index == shoot_1 || laser_enemy_index == shoot_2 || laser_enemy_index == crashing_enemy_index)
			laser_enemy_index = rand() % 8;
		
		
		for (j=0; j<TARGET_NUMBER_PER_WAVE; j++){
			ENEMY_UNIT enemy;
			enemy.is_alive = 1;
			enemy.caught_player = 0;
			enemy.shooting_state = NONE;
			enemy.shot_position.x = -1;
			enemy.shot_position.y = -1;
			
			switch(array_of_waves[i].movement_patern)
			{
				case RIGHT_DIAGONAL:
				enemy.enemy_position.x = 140+10*j;
				enemy.enemy_position.y = 0-2*j;
				enemy.ability = REGULAR;
				break;
				case LEFT_DIAGONAL:
				enemy.enemy_position.x = 20-10*j;
				enemy.enemy_position.y = 0-2*j;
				enemy.ability = REGULAR;
				break;
				case STATIC_STATE:
				enemy.enemy_position.x = 4 + j*target_disp+4;
				enemy.enemy_position.y = 0;
				enemy.ability = REGULAR;
				break;
				case DESCEND_TO_3_AND_MOVE:
				enemy.enemy_position.x = 4 + j*target_disp+4;
				enemy.enemy_position.y = 0;
				enemy.ability = REGULAR;
				break;
				case DESCEND_TO_6_AND_MOVE:
				enemy.enemy_position.x = 4 + j*target_disp+4;
				enemy.enemy_position.y = 0;
				enemy.ability = REGULAR;
				break;
				case DESCEND_TO_9_AND_MOVE:
				enemy.enemy_position.x = 4 + j*target_disp+4;
				enemy.enemy_position.y = 0;
				enemy.ability = REGULAR;
				break;
				case SQUARE_MOVEMENT:
				enemy.enemy_position.x = 42 +j*target_disp/2+4;
				enemy.enemy_position.y = 0;
				enemy.ability = REGULAR;
				break;
				default:
				enemy.enemy_position.x = 4 + j*target_disp+4;
				enemy.enemy_position.y = 0;
				enemy.ability = REGULAR;
			}
			//Second and before the last enemies have shooting ability
			if(j == shoot_1 || j == shoot_2)
			{
				enemy.ability = SHOOTING;
				enemy.shooting_state = READY;
				enemy.shooting_frequency = rand() % 120;
			}
			if ((stage == 2 || stage == 3) && (array_of_waves[i].movement_patern != LEFT_DIAGONAL && array_of_waves[i].movement_patern != RIGHT_DIAGONAL && j==laser_enemy_index))
				enemy.ability = LASER;
			// if(stage == 3 && j == crashing_enemy_index)
			if(stage == 3 && j == crashing_enemy_index)
				enemy.ability = CRASHING;
			
			array_of_waves[i].enemy_units[j] = enemy;
		}
	}
}
void user_is_hit_check(){
	switch(display_draft[player_ship_position.y][player_ship_position.x]){
		case ENEMY_SHOT: case '0': case 'V': case '=': case '-': case '*': game_state = LOSE;
	}
	switch(display_draft[player_ship_position.y+1][player_ship_position.x]){
		case ENEMY_SHOT: case '0': case 'V': case '=': case '-': case '*': game_state = LOSE;
	}
	switch(display_draft[player_ship_position.y+1][player_ship_position.x-2]){
		case ENEMY_SHOT: case '0': case 'V': case '=': case '-': case '*': game_state = LOSE;
	}
	switch(display_draft[player_ship_position.y+1][player_ship_position.x+2]){
		case ENEMY_SHOT: case '0': case 'V': case '=': case '-': case '*': game_state = LOSE;
	}
	switch(display_draft[player_ship_position.y+2][player_ship_position.x]){
		case ENEMY_SHOT: case '0': case 'V': case '=': case '-': case '*': game_state = LOSE;
	}
}
 
 void sound_beep(int h, int interval) {
	unsigned divisor = 1193180L / h;			 
	   ChangeSpeaker( 1 );
   //        outportb( 0x43, 0xB6 );
        asm {
          PUSH AX
          MOV AL,0B6h
          OUT 43h,AL
          POP AX
        } // asm


     //       outportb( 0x42, divisor & 0xFF ) ;
        asm {
          PUSH AX
          MOV AX,divisor
          AND AX,0FFh
          OUT 42h,AL
          POP AX
        } // asm


     //        outportb( 0x42, divisor >> 8 ) ;

        asm {
          PUSH AX
          MOV AX,divisor
          MOV AL,AH
          OUT 42h,AL
          POP AX
        } // asm
		sleept(interval);
		ChangeSpeaker(0);
}

void ChangeSpeaker( int status ){
  int portval;
//   portval = inportb( 0x61 );

      portval = 0;
   asm {
        PUSH AX
        MOV AL,61h
        MOV byte ptr portval,AL
        POP AX
       }

    if ( status==1 )
     portval |= 0x03;
      else
       portval &=~ 0x03;
        // outportb( 0x61, portval );
        asm {
          PUSH AX
          MOV AX,portval
          OUT 61h,AL
          POP AX
        } // asm
	 
} 
void end_game()
{
	printf("\n\n\n\n\n\n\n\tEnd Of Game");
	printf("\n\n\n\tTotal score - ");
	printf("%d",player_score);
	play_ending_sound();
}


void play_ending_sound()
{
	sound_beep(1000, 2);
	sound_beep(2000, 2);
	sound_beep(3000, 2);
	sound_beep(4000, 2);
	sound_beep(5000, 2);
	sound_beep(1000, 2);
	sound_beep(2000, 2);
	sound_beep(3000, 2);
	sound_beep(4000, 2);
	sound_beep(5000, 2);
}

void print_game_over(){
	char game_over[9] = {'G', 'A', 'M', 'E', ' ', 'O', 'V', 'E', 'R'};
	char score_tag[12] = {'S', 'c', 'o', 'r', 'e', ':', ' ', '0', '0', '0', '0', '0'};
	int i, digit, score;
	clear_display_matrix();

	for (i=0; i<9; i++){
		display_draft[12][72+i*2] = game_over[i];
		display_draft[12][72+i*2+1] = WHITE_TEXT;
	}	
	for(i=0; i<12; i++){// Printing "Score: 00000"
                display_draft[13][70+i*2] = score_tag[i];
				display_draft[13][70+i*2+1] = WHITE_TEXT;
		}
        // breaking down the score to digits and printing it
        score = player_score; // local variable, we don't want to change the score
        i = 0;
        while (score >0){
                digit = score % 10;
                display_draft[13][92-i] = digit+'0';
                display_draft[13][92-i+1] = WHITE_TEXT;
                score /= 10;
                i+=2;
        }
		
		game_state = LOSE;
		send(music_pid, 'o');

}
void print_victory()
{
	char victory_tag[7] = {'V', 'I', 'C', 'T', 'O', 'R', 'Y'};
	char score_tag[12] = {'S', 'c', 'o', 'r', 'e', ':', ' ', '0', '0', '0', '0', '0'};
	int i, digit, score;
	clear_display_matrix();
	for (i=0; i<7; i++){
		display_draft[12][74+i*2] = victory_tag[i];
		display_draft[12][74+i*2+1] = WHITE_TEXT;
	}	
        for(i=0; i<12; i++){// Printing "Score: 00000"
                display_draft[13][68+i*2] = score_tag[i];
				display_draft[13][68+i*2+1] = WHITE_TEXT;
		}
        // breaking down the score to digits and printing it
        score = player_score; // local variable, we don't want to change the score
        i = 0;
        while (score >0){
                digit = score % 10;
                display_draft[13][90-i] = digit+'0';
                display_draft[13][90-i+1] = WHITE_TEXT;
                score /= 10;
                i+=2;
        }
}
void advance_stage(){
	// char stage_tag[7] = {'S', 't', 'a', 'g', 'e', ':', ' '};
	int i;
	stage_number++;				// we have advanced a stage
	entry_time_counter = 0;		// resetting entry counter for the next stage's waves
	for (i=0; i< NUMBER_OF_WAVES; i++){
		crashing_enemy_units[i].enemy.enemy_position.x=crashing_enemy_units[i].enemy.enemy_position.y=-1;
	}
	display_stage_number_flag = 1;
		
}
void show_welcome_screen(){
	int i, j;
	char galaga_logo[6][50] = {
		{' ',' ','_','_','_','_','_',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','_',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','_','_','_','_','_',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
			{' ','/',' ','_','_','_','_','|',' ',' ',' ','/','\\',' ',' ',' ','|',' ','|',' ',' ',' ',' ',' ',' ',' ',' ','/','\\',' ',' ',' ','/',' ','_','_','_','_','|',' ',' ',' ','/','\\',' ',' ',' ',' '},
				{'|',' ','|',' ',' ','_','_',' ',' ',' ','/',' ',' ','\\',' ',' ','|',' ','|',' ',' ',' ',' ',' ',' ',' ','/',' ',' ','\\',' ','|',' ','|',' ',' ','_','_',' ',' ',' ','/',' ',' ','\\',' ',' ',' '},
					{'|',' ','|',' ','|','_',' ','|',' ','/',' ','/','\\',' ','\\',' ','|',' ','|',' ',' ',' ',' ',' ',' ','/',' ','/','\\',' ','\\','|',' ','|',' ','|','_',' ','|',' ','/',' ','/','\\',' ','\\',' ',' '},
						{'|',' ','|','_','_','|',' ','|','/',' ','_','_','_','_',' ','\\','|',' ','|','_','_','_','_',' ','/',' ','_','_','_','_',' ','\\',' ','|','_','_','|',' ','|','/',' ','_','_','_','_',' ','\\',' '},
							{' ','\\','_','_','_','_','_','/','_','/',' ',' ',' ',' ','\\','_','\\','_','_','_','_','_','_','/','_','/',' ',' ',' ',' ','\\','_','\\','_','_','_','_','_','/','_','/',' ',' ',' ',' ','\\','_','\\'}
};
	char press_enter[11] = {'P', 'R', 'E', 'S', 'S', ' ', 'E', 'N', 'T', 'E', 'R'};
		for (i=0; i<11; i++){
			display_draft[20][70+i*2] = press_enter[i];
			display_draft[20][70+i*2+1] = PRESS_ENTER_FONT;
		}
		for(i=0; i<6; i++)
			for(j=0; j<100; j+=2){
				display_draft[2+i][30 + j] = galaga_logo[i][j/2];
				display_draft[2+i][30 + j+1] = WHITE_TEXT;
			}
}

void display_stage_number(){
	char stage_tag[6] = {'S', 'T', 'A', 'G', 'E', ' '};
	int i;
	if (display_stage_number_flag > 0){
		if(stage_number < 4){
			for (i=0; i<7; i++){
				display_draft[13][74+i*2] = stage_tag[i];
				display_draft[13][74+i*2+1] = WHITE_TEXT;
			}
		}
		if (stage_number == 1){
			display_draft[13][86] = '1';
			display_draft[13][86+1] = WHITE_TEXT;
		}
		if (stage_number == 2){
			display_draft[13][86] = '2';
			display_draft[13][86+1] = WHITE_TEXT;
		}
		else if (stage_number == 3){
			display_draft[13][86] = '3';
			display_draft[13][86+1] = WHITE_TEXT;
		}
		display_stage_number_flag++;
		if (display_stage_number_flag == 50)
			display_stage_number_flag = 0;
	}
}
void start_crashing_into_player(ENEMY_UNIT *enemy, int wave, int index){
	int i=0;
	CRASHING_ENEMY_UNIT crashing_unit;
	crashing_unit.enemy = *enemy;
	crashing_unit.target_location = player_ship_position ;
	crashing_unit.original_wave = wave;		// Which wave it came from
	crashing_unit.original_index = index;		// Which wave it came from
	while(crashing_enemy_units[i].enemy.enemy_position.x !=- 1 && i< TARGET_NUMBER_PER_WAVE)
		i++;
	crashing_enemy_units[i] = crashing_unit;
}
void increase_difficulty(){
	movement_counter -= 15;
	shot_movement_counter -= 9;
	difficulty++;
}
void decrease_difficulty(){
	movement_counter += 15;
	shot_movement_counter += 9;
	difficulty--;
}
void clear_stage(){
	int i, j;
	WAVE *current_stage_waves;
	current_stage_waves = get_current_stage_waves();

	for(i = 0; i < NUMBER_OF_WAVES; i++)
		for(j = 0; j < TARGET_NUMBER_PER_WAVE; j++)
			current_stage_waves[i].enemy_units[j].enemy_position.x = current_stage_waves[i].enemy_units[j].enemy_position.y = -1;
			
	for(i = 0; i < NUMBER_OF_WAVES; i++)
		crashing_enemy_units[i].enemy.enemy_position.x = crashing_enemy_units[i].enemy.enemy_position.y = -1;	
}
//===========================================================================================================================================
// xmain
//===========================================================================================================================================
void xmain()
{
        int uppid, dispid, recvpid, musicpid;
		
        initiate_display_mode();
                
        resume( dispid = create(displayer, INITSTK, INITPRIO, "DISPLAYER", 0) );
        resume( recvpid = create(receiver, INITSTK, INITPRIO+3, "RECIVEVER", 0) );
        resume( uppid = create(updateter, INITSTK, INITPRIO, "UPDATER", 0) );
        resume( music_pid = create(music_process, INITSTK, INITPRIO, "MUSIC PLAYER", 0) );
        receiver_pid =recvpid;  
        set_new_int9_newisr();
    // schedule(2,1, dispid, 0,  uppid, 0);
    schedule(3,1, dispid, 0,  uppid, 0, music_pid, 0);
} // xmain