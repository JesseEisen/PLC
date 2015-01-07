#include <stdio.h>
#include <stdlib.h>
#include "fx-serial.h"
#include <string.h>
#include <pthread.h>
#include "common.h"

struct fx_serial *ss;
time_t the_time;
FILE   *fp;
int		fd;

//room controller id 
int Ctrl_TrainRoom1[] = {56,57,58,59,60,61,62,63,80};
int Ctrl_TrainRoom2[] = {64,65,66,67,68,69,70,71,81};
int Ctrl_TrainRoom3[] = {72,73,74,75,76,77,78,79,82};
int Sterilization_Ctrl[]= {50,51,52,53,54,55};

//room sensor id
int Sensor_TrainRoom1[] = {1,2,3,4,5,6,7,8,9,10};
int Sensor_TrainRoom2[] = {11,12,13,14};
int Sensor_TrainRoom3[] = {15,16,17,18};
int Sensor_light_Room1[] = {19,20,21,22};

float room_info[10]; //save the room info


float T1_trans(int id)
{	
	int data;
	float ret;

	sensor_get(ss,id,&data);
	ret = (float)data * 120/1000;

	return ret;

}

float H_trans(int id)
{
	int data;
	float ret;

	sensor_get(ss,id,&data);
	ret = (float)data / 10;

	return ret;
}

float Co2_trans(int id)
{
	int data;
	float ret;

	sensor_get(ss, id,&data);
	ret = (float)data * 10;

	return ret;
}

float L_trans(int id)
{
	int data;
	float ret;

	sensor_get(ss,id,&data);
	ret = (float)data * 10;

	return ret;
}

float T2_trans(int id)
{
	int data;
	float ret;

	sensor_get(ss,id,&data);
	ret = ((float)data - 30000/110)*110 / 1000;

	return ret;
}

void Calc_data(int room[],int len)
{
	int i;
	float data;

	for(i = 0; i< len; i++)
	 {
		switch(room[i])
		{
			case 1:
				data = T1_trans(room[i]);
				break;
			case 8:
			case 12:
			case 16:
				data = H_trans(room[i]);
				break;
			case 9:
			case 10:
			case 13:
			case 14:
			case 17:
			case 18:
				data = Co2_trans(room[i]);
				break;
			case 19:
			case 20:
			case 21:
			case 22:
				data = L_trans(room[i]);
				break;
			default:
				data = T2_trans(room[i]);
				break;
		} 

		room_info[i] = data;
	}
}

void Write_into_file(float room[],int len)
{
	int i;

	for(i = 0; i< len; i++)
	{
		fprintf(fp,"%.2f\t",room[i]);
	}

	fputs("\n",fp);
}


void makeupdata(int id,int len)
{
	
	switch(id)
	{
		case 1:
			split_array(len); //use to split array and use different proto to send
			break;
		case 2:
					
			
	}
}



void Get_data(void)
{
	int  len;
	(void)time(&the_time);
	
	fp = fopen("sensordata.txt","a+");
	if(fp == NULL)
	{
		fprintf(stderr,"Open file error\n");
		exit(1);
 	}
	
	fputs(ctime(&the_time),fp);

	fputs("Room1:\n",fp);
	len = sizeof(Sensor_TrainRoom1)/sizeof(Sensor_TrainRoom1[0]);
	Calc_data(Sensor_TrainRoom1,len);
	Write_into_file(room_info,len);
	makeup_data(1,len);	
	
	fputs("Room2:\n",fp);
	len = sizeof(Sensor_TrainRoom2)/sizeof(Sensor_TrainRoom2[0]);
	Calc_data(Sensor_TrainRoom2,len);
	Write_into_file(room_info,len);
	makeup_data(2,len);

	fputs("Room3:\n",fp);
	len = sizeof(Sensor_TrainRoom3)/sizeof(Sensor_TrainRoom3[0]);
	Calc_data(Sensor_TrainRoom3,len);
	Write_into_file(room_info,len);

	fputs("Room1 Light:\n",fp);
	len = sizeof(Sensor_light_Room1)/sizeof(Sensor_light_Room1[0]);
	Calc_data(Sensor_light_Room1,len);
	Write_into_file(room_info,len);
	

	fputs("\n",fp);
	fclose(fp);
	
}


void *Sensor_data(void *arg)
{
	while(1)
	{
		Get_data();
		sleep(5); //sleep 5 mins
	 }
}


void *Control_controller(void *arg)
{
//	while(1)
//	{
//		printf("waiting for control\n");
//		sleep(3);
//	}

}

int main(int argc, const char *argv[])
{
#define OPEN 1
#define CLOSE 0
	pthread_t show_data,wait_controller;
	int err;
	ss = fx_serial_start();
	fd = client_init();

	err = pthread_create(&show_data,NULL,Sensor_data,NULL);
	if(err != 0)
	{
		fprintf(stderr,"cannot create the thread\n");
		exit(1);
	}

	err = pthread_create(&wait_controller,NULL,Control_controller,NULL);
	if(err != 0)
	{
		fprintf(stderr,"cannot create the thread\n");
		exit(1);
	}

	pthread_join(show_data, NULL);
	pthread_join(wait_controller, NULL);


	while(1) getchar();
	fx_serial_stop(ss);
	return 0;
}
