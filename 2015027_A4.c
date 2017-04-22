#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#define PROC_LIMIT 50
#define QUEUE_LIMIT 10

//Please add config file in given format only

typedef struct procx{

	int pid;
	int pstart;
	int pend;
	int used_time;
	int timeleft;
	int priority;
}proc;

typedef struct configx
{
	int queue_count;
	int time_slice_arr[QUEUE_LIMIT];
	int boost_time;
	int proc_count;
	proc proc_arr[PROC_LIMIT];
}config;

proc mlfq[QUEUE_LIMIT][PROC_LIMIT+1];

int mlfqsz[QUEUE_LIMIT][3]; //front end count
pthread_mutex_t flaglock;
pthread_cond_t job_condition;
pthread_mutex_t job_lock;
config * current_config=NULL;

int currtime=0;

int jobsdone =0;

int totaljobs=0;

int qcounter = 0;

int next_job_time;

proc * joblist;

int boostflag=0;

int ans[PROC_LIMIT];

void  init_mlfq()
{
	for(int i=0;i<QUEUE_LIMIT;i++)
	{
		mlfqsz[i][0] = 0; //front
		mlfqsz[i][1] = -1; //rear
		mlfqsz[i][2] = 0; //item count
	}
}

proc * peek(int i) {
   return &mlfq[i][mlfqsz[i][0]];
}

bool isEmpty(int i) {
   return mlfqsz[i][2] == 0;
}

bool isFull(int i) {
   return mlfqsz[i][2] == PROC_LIMIT;
}

int size(int i) {
   return mlfqsz[i][2];
}  

void insert(proc * data, int i) {

   if(!isFull(i)) {
	
      if(mlfqsz[i][1] == PROC_LIMIT-1) {
         mlfqsz[i][1] = -1;            
      }       

      mlfq[i][++mlfqsz[i][1]] = *data;
      mlfqsz[i][2]++;
   }
}

proc * removeData(int i) {
   proc * data = &(mlfq[i][((mlfqsz[i][0])++)]);
	
   if(mlfqsz[i][0] == PROC_LIMIT) {
      mlfqsz[i][0] = 0;
   }
   mlfqsz[i][2]--;
   return data;  
}

void readconfig(const char * filename)
{

	FILE * fp;
	int i;
	fp = fopen(filename,"r");
	if(fp==NULL)
	{
		printf("read error\n");
		exit(-1);
	}
	current_config = (config *)malloc(sizeof(config));
	fscanf(fp,"%d",&(current_config->queue_count));
	for(i=0;i<(current_config->queue_count);i++)
	{
		fscanf(fp,"%d",&((current_config->time_slice_arr[i])));
	}
	fscanf(fp,"%d%d",&(current_config->boost_time),&(current_config->proc_count));
	for(i=0;i<(current_config->proc_count);i++)
	{
		int temp;
		fscanf(fp,"%d %d",&((current_config->proc_arr[i]).pstart),&temp);
		(current_config->proc_arr[i]).pend= ((current_config->proc_arr[i]).pstart) + temp;
		current_config->proc_arr[i].timeleft=temp;
		current_config->proc_arr[i].used_time=0;
		current_config->proc_arr[i].priority = 0;
		current_config->proc_arr[i].pid = i;
	}
	fclose(fp);
	return;
}

void printconfig()
{
	//printf("%d\n%d\n%d\n",current_config->queue_count,current_config->boost_time,current_config->proc_count);
	int pc = current_config->proc_count;
	int i;
	for(i=0;i<pc;i++)
	{
		//printf("%d %d\n",(current_config->proc_arr[i]).pstart,(current_config->proc_arr[i]).pend);
	}

	for(i=0;i<current_config->queue_count;i++)
	{
		//printf("%d \n",current_config->time_slice_arr[i]);
	}

}



proc * nextproc()
{
	int qsz  = current_config->queue_count;
	proc * job = NULL;
	for(int i=0;i<qsz;i++)
	{
		if(size(i)>0)
		{
			job = peek(i);
			break;
		}
	}
	return job;
}

void boostpriority()
{
	//printf("priority boost function:\n");
	for(int i=1;i<current_config->queue_count;i++)
	{
		for(int j=0;j<mlfqsz[i][2];j++)
		{
		    if (mlfqsz[i][0] == - 1)
		    {
		    	printf("\n");
		    }
		    else
		    {
		        for (j = mlfqsz[i][0]; j <= mlfqsz[i][1]; j++)
		        {
		        	proc * pp = removeData(i);
					//printf("removed job = %d from queue = %d old sz = %d new size = %d\n",pp->pid,pp->priority,os,mlfqsz[pp->priority][2]);
					if(pp!=NULL)
					{
						pp->priority = 0;
						pp->used_time=0;
						insert(pp,0);
						//printf("sz of 0 q = %d\n",mlfqsz[0][2]);

					}
			    }
			}
		}
	}
}

void print_mlfq()
{
	printf("\n-----------------MLFQ------------------------\n");
	int end = current_config->queue_count;
	for(int i=0;i<end;i++)
	{
		int j;
	    if (mlfqsz[i][0] == - 1)
	    {
	    	printf("\n");
	    }
	    else
	    {
	        for (j = mlfqsz[i][0]; j <= mlfqsz[i][1]; j++)
	            printf("%d ", mlfq[i][j].pid);
	    }
		printf("\n----------------------------------------\n");
	}
	//	printf("\n-------------------------------------------\n");
}



void * worker(void * t)
{


    printf("worker thread woken up...\n");
    while(1)
    {
 	   	if( current_config->boost_time!=0 && currtime!=joblist[0].pstart && (currtime - joblist[0].pstart)%current_config->boost_time==0 && jobsdone<totaljobs && boostflag==1)
    	{
    		printf("boosting priority\n");
    		boostpriority();
    		pthread_mutex_lock(&flaglock);
    		boostflag=0;
    		pthread_mutex_unlock(&flaglock);
    		pthread_exit(NULL);
    	}
    	print_mlfq();
    	proc * job = nextproc();
    	if(job==NULL)
    	{
    		//printf("Nothing to do...Going in fututre\n");
    		if(qcounter>=totaljobs)
    		{
    			printf("Finished everything\n");
    			pthread_exit(NULL);
    		}
    		else
    		{
    			//printf("Was idle\n");
 		   		currtime = joblist[qcounter].pstart;
    		}
    	}
    	else
    	{	
    		printf("found job %d to do\n",job->pid);
    		job->used_time++;
    		job->timeleft--;//CHECK initialize timeleft
  			currtime++;
  			if( current_config->boost_time!=0 && currtime!=joblist[0].pstart && (currtime - joblist[0].pstart)%current_config->boost_time==0 && jobsdone<totaljobs)
  			{
  				pthread_mutex_lock(&flaglock);
  				boostflag=1;
  				pthread_mutex_unlock(&flaglock);
  			}
  			//printf("job %d used time = %d timeleft = %d current time = %d\n",job->pid,job->used_time,job->timeleft,currtime);
  			int quanta = current_config->time_slice_arr[job->priority];
  			printf("Quanta max = %d\n",quanta);
  			if(job->timeleft<=0)
  			{
  				printf("Job %d done currtime = %d \n",job->pid,currtime);
  				ans[job->pid]= currtime - job->pstart;
  				jobsdone++;
  				//printf("jobsdone = %d\n",jobsdone);
  				removeData(job->priority);
  			}
  			else
  			{
				if(job->used_time>=quanta)
	  			{
	  				printf("used time more than allowed");
	  				job->used_time=0;
	  				if(job->priority==current_config->queue_count-1)
	  				{
	  					removeData(job->priority);
	  					insert(job,job->priority);
	  					printf(" sent to same queue but at end\n");

	  				}
	  				else
	  				{
	  					removeData(job->priority);
	  					(job->priority)++;
	  					insert(job,job->priority);
	  					printf(" sent to lower queue\n");
	  					printf("job new priority= %d\n",job->priority);

	  				}
	  			}
    		}

  		}
  		if( qcounter!=totaljobs && joblist[qcounter].pstart<=currtime)
  		{
  			//pthread_cond_wait(&job_condition, &job_lock);
  			//printf("exiting worker thread\n");	
  			pthread_exit(NULL);
  		}

    }

}



int main()
{
	if (pthread_mutex_init(&flaglock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
	init_mlfq();
	readconfig("mlfq.config");
	totaljobs = current_config->proc_count;
    joblist = current_config->proc_arr;
    qcounter= 0;
//produce a worker thread here	
    boostflag=0;
    pthread_t wthread;
    printf("New job = %d came and got inserted in mlfq\n",joblist[qcounter].pid);
    insert(&joblist[qcounter],0);
    currtime = joblist[qcounter].pstart;
    printf("clock started from time t = %d\n",currtime );
    qcounter++;
   	pthread_create(&wthread,NULL,worker, NULL);
   	pthread_join(wthread,NULL);
    while(jobsdone<totaljobs)
    {
    	if(qcounter>=totaljobs)
    	{
    		pthread_create(&wthread,NULL,worker, NULL);
        	pthread_join(wthread,NULL);
 	   	}
    	else
    	{
	        if(joblist[qcounter].pstart > currtime) //Can schedule other threads
	        {
	        	printf("signal and wait for next thread\n");
				pthread_create(&wthread,NULL,worker, NULL);
	        	pthread_join(wthread,NULL);
	        }
	        else //new job arrrived
	        {
	        	if(qcounter<totaljobs)
	        	{
					printf("Inserting new job = %d at time %d \n",joblist[qcounter].pid,currtime);
		        	insert(&joblist[qcounter],0);
		        	qcounter++;
		        	//printf("next job index now= %d\n",qcounter);
		        	pthread_create(&wthread,NULL,worker, NULL);
		        	pthread_join(wthread,NULL);
	        	}
	        	else
	        	{
	        		pthread_create(&wthread,NULL,worker, NULL);
		        	pthread_join(wthread,NULL);	
	        	}

	        }
	    }

    }
    printf("=====================================T.T============================================\n");
    for(int i=0;i<current_config->proc_count;i++)
    {
    	printf("pid = %d tt = %d\n",i,ans[i]);
    }

	return 0;
}
