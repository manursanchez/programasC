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

//Procedimiento con información del error, su origen, proceso donde se genera y causa.
void error(char *origenError, int numeroProceso){
	printf("%s. Proceso: %i. Información del sistema: %s\n", origenError, numeroProceso, strerror(errno));
	exit(-1);
}

int main(int argc, char *argv[])
{
	//Declaraciones	
	char mensaje[SIZE];
	int fichero, sem1, *vc1, longitud, proceso, shmId, contador;
	
	/*shmId: identificador del espacio de memoria compartida
	sem1: identificador del semáforo*/

	key_t llaveSM; //Llave para identificacion de Semaforo y Memoria compartida
	pid_t pid3; //Almacena el número de pid del proceso 3
	
	union semun{
		int val;
		struct semin_ds *buf;
		ushort* array;
	}arg; 

	struct sembuf opSem = {0,1,0};//Estructura para operaciones de bloqueo/desbloqueo en el semáforo. Inicializamos a 1.
	
	proceso=2;
	
	//Apertura fichero FIFO.
	fichero=open("fichero1", O_RDWR|O_NONBLOCK);
	if(fichero<0) error("Error en la apertura de fichero1.\n",proceso);

	//Lectura del mensaje almacenado en fichero FIFO 'fichero1'
	if ((longitud=read(fichero,mensaje,SIZE))<0) error("Error al leer el fichero.\n",proceso);
	printf("El proceso %d (PID=%d, Ej2) lee el mensaje: '%s', del fichero FIFO 'fichero1'.\n",proceso, getpid(), mensaje);
		
	// Creación de llave para semáforo y memoria compartida
	llaveSM=ftok("fichero1",'B');
	if (llaveSM==(key_t)-1)
	{
		error("No se ha podido crear la llave.\n",proceso);
	}	

	//Creación de espacio de memoria compartida
	if ((shmId=shmget(llaveSM,SIZE,0666|IPC_CREAT))<0) error("Error al generar el identificador de la zona de memoria compartida asociada a la llave.\n",proceso);
	if((vc1=(int *)shmat(shmId,0,0))<0) error("No se ha podido almacenar la dirección a la que está unido el segmento de memoria compartida.\n", proceso);
	
	// Creación de semáforo 
	if ((sem1=semget(llaveSM,1,0666|IPC_CREAT))<0) error("No se ha podido crear el conjunto de semáforos.\n",proceso);
	
	//Iniciacilización del semáforo
	arg.val=0;
	if((semctl(sem1,0,SETVAL,arg))==-1) error("No se puede inicializar el semáforo.\n", proceso);
	semop(sem1,&opSem,1);//Bloqueado acceso a Proceso 3

	//Bifurcación nuevo proceso P3
	if ((pid3=fork())==-1) error("No se ha podido crear el proceso 3\n", proceso);
		
	if (pid3==0) //Estamos en el proceso 3
	{
		proceso=3;				
		execv("./ej3",NULL);	
	}
	else //Proceso 2
	{
		sleep(1);
		printf("El proceso 2 (PID=%d, Ej2) coloca el mensaje: '%s', en un espacio de memoria compartido\n", getpid(), mensaje);
		//Metemos el mensaje en el espacio de memoria compartida		
		vc1[0]=strlen(mensaje);		
		for (contador=0;contador<strlen(mensaje);contador++) vc1[contador+1]=(int) mensaje[contador];
		
		//Como ya hemos escrito el mensaje en memoria, el semáforo se abre para permitir acceso a Proceso 3
		opSem.sem_op=-1;
		semop(sem1,&opSem,1);
		
		//Proceso 2 esperando ser asesinado
		printf("El proceso 2 (PID=%d, Ej2) queda a la espera de ser matado.\n",getpid());
		pause();
	}
	return(0);
}
