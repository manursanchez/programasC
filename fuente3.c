#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/times.h>
#define SIZE 512 

//Procedimiento con información del error, su origen, proceso y causa
void error(char *origenError, int numeroProceso){
	printf("%s. Proceso: %i. Información del sistema: %s\n", origenError, numeroProceso, strerror(errno));
	exit(-1);
}

int main(int argc, char *argv[])
{
	int llaveSM, sem1, memoriaId, *vc1, contador, pid3, proceso, longitud;
	char mensaje[SIZE];	
	
	struct // estructura para la cola de mensajes
	{
		long idMensaje;
		int pid_3;
	}colaDeMensajes;	

	proceso=3;
		
	llaveSM=ftok("fichero1",'B');

	//Semáforo
	if((sem1=semget(llaveSM,1,0666))<0) error("Error en la asignación de semáforo",proceso);		
	
	//Abrimos espacio de memoria compartida
	if((memoriaId=shmget(llaveSM,SIZE,0666))<0) error("Error en la creación de la zona de memoria compartida\n",proceso);
	if((vc1=(int *)shmat(memoriaId,0,0))<0) error("Error al almacenar la dirección a la que está unida el segmento de memoria compartida\n",proceso);
	
	//Esperamos a que el proceso 2, cambie el semáforo para leer de la memoria.
	while(semctl(sem1,0,GETVAL,0)>0); 

	//Ha cambiado el semáforo y da acceso al proceso 3
	longitud=vc1[0];
	for (contador=0;contador<longitud;contador++) mensaje[contador]=(char) vc1[contador+1];
	printf("El proceso %d (PID=%d, Ej3) lee de la memoria compartida el mensaje: '%s'.\n", proceso, getpid(), mensaje);
	
	//Desasociamos el puntero del espacio de memoria compartida
	shmdt((char *)vc1);
	//Eliminamos la zona de memoria compartida
	shmctl(memoriaId,IPC_RMID,0);

	//Eliminamos el semáforo
	semctl(sem1,0,IPC_RMID);

	//Enviamos PID al proceso 1 por la cola de mensajes
	pid3=getpid();
	colaDeMensajes.idMensaje=1;
	colaDeMensajes.pid_3=pid3;
	printf("El proceso %d (PID=%d, Ej3) envía al proceso 1 su PID por cola de mensajes.\n", proceso, getpid());


	//Abrimos la cola para el envío
	int llaveCola=ftok("ej1",'A');
	int msqid;
	if (llaveCola == (key_t)-1)
	{
		error("Error al obtener llave para cola mensajes\n",proceso);
	}
	
	if((msqid=msgget(llaveCola,0666|IPC_CREAT))<0) error("Error al obtener identificador para cola mensajes\n",proceso);
	
	if(msgsnd(msqid,(struct msgbuf *)&colaDeMensajes,sizeof(colaDeMensajes),0)<0) error("No se puede enviar PID por cola de mensajes\n",proceso);
	
		
	//Pausamos el sistema a la espera de ser asesinado.	
	printf("El proceso %d (PID=%d, Ej3) queda a la espera de ser matado.", proceso, getpid());
	pause();
	return (0);
}

