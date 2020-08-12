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
#define ESCRITURA 1
#define LECTURA 0

//Procedimiento con información del error, su origen, proceso y causa
void error(char *origenError, int numeroProceso){
	printf("%s. Proceso: %i. Información del sistema: %s\n", origenError, numeroProceso, strerror(errno));
	exit(-1);
}

int main( int argc, char **argv )
{
	
	//Declaraciones  	
	int tuberia[2], tamanoMensaje, proceso, longitud, contador, bytesMensaje,c;
  	char mensaje[SIZE], m[SIZE];
  	int fichero;
	int msqid; //indicador de la cola de mensajes asociado a la llave

	/************************** INSTRUCCIONES PARA LAS ESTADÍSTICAS DE CPU **********************************/

	//Estructuras que almacenarán información estadística de los tiempos de ejecución empleados por el proceso.
	struct tms infTemp1,infTemp2; 
	//Variables para almacenar los tiempos de inicio y fin.
	clock_t tiempo1, tiempo2; 
	//Rellenamos la estructura infTemp1 con la informacion estadística de los tiempos de ejecución iniciales 	
	tiempo1=times(&infTemp1);
	
	/*******************************************************************************************************/

  	// estructura para la cola de mensajes
	struct 
	{
		long idMensaje;
		int pid_3;//Contiene el PID del proceso 3
	}colaDeMensajes;
	
	key_t llave;
	pid_t pid2, pid3;

	
  	pipe(tuberia);//creamos la tuberia
	if((pid2=fork())==-1) error("Imposible crear proceso 2\n",1);
 
	if  (pid2 != 0) // Estamos en el proceso 1
  	{ // padre
		proceso=1;
    		close( tuberia[LECTURA] ); /* cerramos el lado de lectura del pipe */
		printf("Introduce el mensaje: ");	
		gets(mensaje);
		strcat(mensaje,".");
		if(write(tuberia[ESCRITURA], mensaje, (strlen(mensaje)))<0) error("No se puede escribir en la tuberia sin nombre\n",proceso);
		printf("El proceso %d (PID=%d, Ej1) transmite mensaje '%s' al proceso 2 por una tubería sin nombre.\n", proceso, getpid(),mensaje); 	
				
		//Creación de cola de mensajes
		llave=ftok("ej1",'A');
		if ((msqid=msgget(llave,0666|IPC_CREAT))<0) error("No se puede crear la cola de mensajes\n",proceso);
		
		//Estas líneas esperan el mensaje que ha de llegar por la cola desde el proceso 3
		bytesMensaje=0;		
		while (bytesMensaje<=0){
			bytesMensaje=msgrcv(msqid,(struct msgbuf *)&colaDeMensajes,sizeof(colaDeMensajes),1,0);
			if (bytesMensaje<0) error("Ha habido problemas con la recepción de mensajes\n",proceso);	
		}
		pid3=colaDeMensajes.pid_3;
		printf("El proceso %d (PID=%d, Ej1) ha recibido por cola de mensajes el PID %i del proceso 3.\n", proceso, getpid(), pid3);		

	
		//Señales para matar procesos y finalizar
		printf("El proceso %d (PID=%d, Ej1) Envía señales para matar los procesos 2 y 3...\n", proceso, getpid());
		if (kill(pid3,SIGKILL)<0) error("No se puede eliminar proceso 3.\n", proceso);
		if (kill(pid2,SIGKILL)<0) error("No se puede eliminar proceso 2.\n", proceso);
		
		//Eliminación del fichero FIFO
		printf("El proceso %d (PID=%d, Ej1) elimina 'fichero1'.\n",proceso, getpid());
		if (unlink("fichero1")<0) error("No se puede eliminar fichero1",proceso);
		
		//Eliminación de la cola de mensajes
		msgctl(msqid,IPC_RMID,0);

		/**************************** Cálculos para las estadísticas de uso de CPU ***********************/

		tiempo2=times(&infTemp2); // Rellenamos la segunda estructura para el cálculo de inicio y fin.			
		printf("\nTiempo total de uso de la CPU: %g segundos\n", (float)(tiempo2-tiempo1)/CLOCKS_PER_SEC);	
		printf("Tiempo de uso de la CPU en modo usuario: %g segundos\n",(float)(infTemp2.tms_utime-infTemp1.tms_utime)/CLOCKS_PER_SEC);
		printf("Tiempo de uso de la CPU en modo núcleo: %g segundos\n",(float)(infTemp2.tms_stime-infTemp1.tms_stime)/CLOCKS_PER_SEC);

  	}
  	else // Estamos en el proceso 2
  	{ // hijo
		proceso=2;    		
		close( tuberia[ESCRITURA] ); /* cerramos el lado de escritura del pipe */
	
		//leemos el mensaje de la tuberia sin nombre, mientras no haya datos, no lee nada
    		/*tamanoMensaje=*/read( tuberia[LECTURA], mensaje, SIZE );
		c=0;		
		while (mensaje[c]!='.')
		{
			m[c]=mensaje[c];
			printf("%c\n", m[c]);
			c++;
		}
		
		printf("El proceso %d (PID=%d, Ej1) recibe mensaje '%s' del proceso 1 por una tubería sin nombre.\n", proceso, getpid(), mensaje);

		//Creando fichero de tuberia con nombre "fichero1"
		if(mknod("fichero1", S_IFIFO|0666,0)<0) error("No se puede crear el fichero\n",proceso);

		//apertura del fichero
		fichero=open("fichero1",O_RDWR|O_NONBLOCK); 
		if (fichero<0) error("No se ha podido abrir el fichero1\n",proceso);
			
		//Se escribe mensaje en el fichero		
		printf("El proceso %d (PID=%d, Ej1) coloca el mensaje: '%s', en un fichero FIFO llamado 'fichero1'.\n", proceso, getpid(), mensaje);	
		if (longitud=write(fichero,mensaje,strlen(mensaje))<0) error("Error al escribir en fichero1\n",proceso);
		execv("./ej2",NULL);
		/**********************************************************************************/
  	}
  	return(0);
}
