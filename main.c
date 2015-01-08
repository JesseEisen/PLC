#include <stdio.h>
#include <stdlib.h>
#include "fx-serial.h"
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include "common.h"
#include "message_header.pb-c.h"
#include "getdata.pb-c.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSIZE  1024
#define MAXSLEEP 128
struct fx_serial *ss;
time_t the_time;
FILE   *fp;
int		fd;
float avge = 0; //count light's average

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

MessageHeader mh;
SensorInfo   si;
SensorT		st;

void *buf_si;
void *buf_mh;
void *buf_st;

char buf[BUFSIZE];

size_t si_length;
size_t st_length;
size_t mh_length;

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

void single_sensor(float temperature,int id)
{
	st.roomid = id;
	st.temperature = temperature;

	size_t st_length = sensor_t__get_packed_size(&st);
	buf_st = malloc(st_length);
	sensor_t__pack(&st,buf_st);
}

void sensordata_pack(float T, float H, float Co2)
{
	si.roomid = 1;
	si.temperature = T;
	si.humidity = H;
	si.co2 = Co2;
	si.has_light = 1;
	si.light = avge;

	si_length = sensor_info__get_packed_size(&si);
	buf_si = malloc(si_length);
	sensor_info__pack(&si,buf_si);

}

void split_array(int len)
{
	float T_average,Co2_average;
	T_average =(room_info[2]+room_info[3]+room_info[4]+room_info[5]+room_info[6])/4;
	Co2_average =(room_info[8] + room_info[9])/2;
	
	sensordata_pack(T_average,room_info[7],Co2_average);

}


void cleanup_array(int len,int id)
{
	float Co2_average;

	Co2_average = (room_info[len-1] + room_info[len-2]) /2;
	
	si.roomid = id;
	si.temperature = room_info[0];
	si.humidity = room_info[1];
	si.co2 = Co2_average;
	si.has_light = 1;
	si.light = 0;
	
	si_length = sensor_info__get_packed_size(&si);
	buf_si = malloc(si_length);
	sensor_info__pack(&si,buf_si);


}

void makeup_data(int id,int len)
{
	
	switch(id)
	{
		case 1:
			split_array(len); //use to split array and use different proto to send
			break;
		case 2:
		case 3:
			cleanup_array(len,id);
			break;
	}
}

void init_msgheader(int id)
{
	mh.message_id = 1;
	mh.has_message_type = 1;
	mh.message_type = MESSAGE_HEADER__TYPE__TEXT;
	mh.has_session = 1;
	mh.session = 2;
	mh.has_version = 1;
	mh.version = 1;
	mh.has_room_tag = 1;
	
	switch(id)
	{
		case 1:
			mh.room_tag = 1;
			break;
		case 2:
			mh.room_tag = 2;
			break;
	 }

    mh_length = message_header__get_packed_size(&mh);
	buf_mh = malloc(mh_length);
	message_header__pack(&mh,buf_mh);
}



void Send_data(int id)
{
	int len,ret;
	
	init_msgheader(id);
	if(id == 1) //1 mean the sensor is full
	{
		snprintf(buf,sizeof(buf),"%s%d%s%s\n","MUSHROOM",si_length,(char *)buf_mh,(char *)buf_si);
		free(buf_si);
	}else if(id == 2) //mean the sensor is single
	{
		snprintf(buf,sizeof(buf),"%s%d%s%s\n","MUSHROOM",st_length,(char *)buf_mh,(char *)buf_st);
		free(buf_st);
	}  

	free(buf_mh);
	
	ret = send(fd,buf,strlen(buf),0);
	if(ret < 0)
	{
		fprintf(stderr,"Error: send buf error\n");
		exit(1);
	} 

}


void Get_data(void)
{
	int  len;
	int i;

	(void)time(&the_time);
	
	fp = fopen("sensordata.txt","a+");
	if(fp == NULL)
	{
		fprintf(stderr,"Open file error\n");
		exit(1);
 	}
	
	fputs(ctime(&the_time),fp);

	fputs("Room1 Light:\n",fp);
	len = sizeof(Sensor_light_Room1)/sizeof(Sensor_light_Room1[0]);
	Calc_data(Sensor_light_Room1,len);
	Write_into_file(room_info,len);
	for(i = 0; i < len; i++)
	{
		avge += room_info[i];
	}
	avge /= len;
	
	fputs("Room1:\n",fp);
	len = sizeof(Sensor_TrainRoom1)/sizeof(Sensor_TrainRoom1[0]);
	Calc_data(Sensor_TrainRoom1,len);
	Write_into_file(room_info,len);
	makeup_data(1,len);	
	
	//send the first two single sensor room
	single_sensor(room_info[0],4); //room_id is 4
	Send_data(2);
	single_sensor(room_info[1],5); //room_id is 5
	Send_data(2);

	Send_data(1); //here used to send room1 data

	fputs("Room2:\n",fp);
	len = sizeof(Sensor_TrainRoom2)/sizeof(Sensor_TrainRoom2[0]);
	Calc_data(Sensor_TrainRoom2,len);
	Write_into_file(room_info,len);
	makeup_data(2,len);
	Send_data(1); //room 2 data

	fputs("Room3:\n",fp);
	len = sizeof(Sensor_TrainRoom3)/sizeof(Sensor_TrainRoom3[0]);
	Calc_data(Sensor_TrainRoom3,len);
	Write_into_file(room_info,len);
	makeup_data(3,len);
	Send_data(1); //room 3 data
 
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

int connect_retry(int sockfd, const struct sockaddr *addr,socklen_t alen)
{
	int nesc;

	for(nesc = 1; nesc <= MAXSLEEP; nesc <<= 1)
	{
		if(connect(sockfd, addr,alen) == 0)
			return(0);
		if(nesc <= MAXSLEEP/2)
			sleep(nesc);
	}


	return -1;
}

int main(int argc, const char *argv[])
{
#define OPEN 1
#define CLOSE 0
	pthread_t show_data,wait_controller;
	int err,ret;
	
	ss = fx_serial_start();
	fd = client_init(7000,argv[1]);

	message_header__init(&mh);
	sensor_info__init(&si);
	sensor_t__init(&st);
	
	ret = connect_retry(fd,(struct sockaddr *)&clientaddr,sizeof(clientaddr));

	if(ret == -1)
	{
		printf("cannot connect to server\n");
	 	exit(1);
	}

	sleep(1);
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
