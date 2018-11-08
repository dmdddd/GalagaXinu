/* clkint.c - clkint */

#include <conf.h>
#include <kernel.h>
#include <sleep.h>
#include <io.h>
#include <proc.h>

#define default_enemy_movement_speed 35
#define default_enemy_shot_speed 20
#define maximum_shooting_time_range 140


// Added varialbes 
volatile int movement_counter = default_enemy_movement_speed;
volatile int shot_movement_counter = default_enemy_shot_speed;  
extern  volatile int move_enemies_flag_speed_1;
extern  volatile int move_enemies_flag_speed_2;
extern  volatile int move_enemies_flag_speed_3;
extern  volatile int move_crashing_enemies_flag;
extern  volatile int move_shots_flag;
volatile int entry_time_counter = 0; // counts time from the beginning of the stage
volatile int shooting_counter = 0;
extern int stage_number;
extern int difficulty;
extern int sched_arr_pid[];
extern int sched_arr_int[];
volatile int laser_counter = 0;
extern int release_laser_flag;
extern int caught_in_laser;


volatile int on_laser=0;
volatile int laser_time=0;
volatile int on_laser_flag=0;
volatile int return_to_place;

extern int gcycle_length;
extern int point_in_cycle;
extern int gno_of_pids;

SYSCALL noresched_send(pid, msg)
int	pid;
int	msg;
{
	struct	pentry	*pptr;		/* receiver's proc. table addr.	*/
	int	ps;

	disable(ps);
	if (isbadpid(pid) || ( (pptr = &proctab[pid])->pstate == PRFREE)
	   || pptr->phasmsg != 0) {
		restore(ps);
		return(SYSERR);
	}
	pptr->pmsg = msg;		/* deposit message		*/
	pptr->phasmsg++;
	if (pptr->pstate == PRRECV) {	/* if receiver waits, start it	*/
		ready(pid);
	}
	restore(ps);
	return(OK);
} // noresched_send



/*------------------------------------------------------------------------
 *  clkint  --  clock service routine
 *  called at every clock tick and when starting the deferred clock
 *------------------------------------------------------------------------
 */
INTPROC clkint(mdevno)
int mdevno;				/* minor device number		*/
{
	int	i;
    int resched_flag;  
	tod++;
	resched_flag = 0;
	// ============================================================
	// ======= Counters =======
	++entry_time_counter;
	++shooting_counter;
	///////////////////////////////
	
		++on_laser;
		if(on_laser>190)// after 190 sec the laser is on
		{
			on_laser_flag=1;// laser on
			on_laser=0;
		}
		++laser_time;// how much time the lase is on 
		if(laser_time >290 && on_laser_flag ==1){
			on_laser_flag=0;
		 // turn off laser
			laser_time=0;
		}
	
	// ======= Enemies movement =======
	if (tod % movement_counter == 0)
		move_enemies_flag_speed_1 = 1; 
	
	if (tod % (movement_counter*2) == 0)
		move_enemies_flag_speed_2 = 1; 	
	
	if (tod % (movement_counter*3) == 0)
		move_enemies_flag_speed_3 = 1; 	
	
	if (tod % movement_counter/2 == 0)
		move_crashing_enemies_flag = 1; 
	
	// ======= Shots =======
	if(tod % shot_movement_counter == 0)
		move_shots_flag = 1;
	
	if(shooting_counter > maximum_shooting_time_range)
		shooting_counter = 0;
	
	// Increasing shooting speed in stage 3, every 10 seconds 2x
	if(stage_number == 3 && entry_time_counter % 182 == 0)
		if (shot_movement_counter>2)
			shot_movement_counter = shot_movement_counter/2;
		
	if(stage_number == 3 && entry_time_counter % 182 == 0)
		if (movement_counter>2)
			movement_counter = movement_counter/2;
	// ======= Laser =======
	
	if(caught_in_laser == 1)
		++laser_counter;
	if(laser_counter >=40)
	{
		release_laser_flag=1;
		laser_counter = 0;
		caught_in_laser=0;
	}
	// ============================================================
	if (slnempty)
		if ( (--*sltop) <= 0 )
                     {
                        resched_flag = 1;
			wakeup();
                     } /* if */

	if ( (--preempt) <= 0 )
             resched_flag = 1;

       point_in_cycle++;
       if (point_in_cycle == gcycle_length)
         point_in_cycle = 0;

       for(i=0; i < gno_of_pids; i++) 
       {
          if(point_in_cycle == sched_arr_int[i])
            {
             noresched_send(sched_arr_pid[i], 11);
             resched_flag = 1;
            } // if
       } // for

       if (resched_flag == 1)
 		resched();

} // clkint