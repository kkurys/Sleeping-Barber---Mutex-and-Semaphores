#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

/* BY KAMIL KURYŚ & MACIEJ OWERCZUK
   SLEEPING BARBER PROBLEM USING ONLY MUTEX AND SEMAPHORES
   BIAŁYSTOK, 2016 */

sem_t customerReady, cutting;
pthread_mutex_t accessChairs = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t barberCutting = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gettingCut = PTHREAD_MUTEX_INITIALIZER;
int numberOfWaitingClients = 0;
int numberOfChairs;
int totalNumberOfClients = 0;
int numberOfResignedClients = 0;
int IsDebug = 0;
volatile int currentlyServedClient;

typedef struct Pos_Struct
{
	int id;
	sem_t turn;
	sem_t wasCut;
	struct Pos_Struct *next;
}Position;

// fryzjer strzyze przez wylosowany czas i informuje o skonczeniu strzyzenia klienta
void DoCut(Position *newClient)
{
	pthread_mutex_lock(&
	int i = rand()%4;
	sleep(i);
// sygnalizujemy koniec strzyzenia
	sem_post(&newClient->wasCut);

}
void GetCut(Position *newClient)
{
//czekamy na skończenie strzyzenia
	sem_wait(&newClient->wasCut);

}
// barberQueue - kolejka w poczekalni; resignedClients - lista zrezygnowanych klientow
Position *barberQueue = NULL;
Position *resignedClients = NULL;
// wypisywanie kolejki
void PrintQueue(Position *start)
{
	Position *tmp = NULL;
	if (start == NULL) 
	{	
		return;
	}
	else 
	{
		tmp = start;
		while (tmp != NULL)
		{
			printf("%d ", tmp->id);
			tmp = tmp->next;
		}
	}
	printf("\n");
}
// dodawanie klienta do kolejki
Position* AddNewClientToQueue(Position **start, int id)
{
	Position *new = malloc(sizeof(Position));
	new->id = id;
	new->next = NULL;
	sem_init(&new->turn, 0, 0);
	sem_init(&new->wasCut, 0, 0);
	if (*start == NULL)
	{
		(*start) = new;
	}
	else 
	{
		Position *tmp = *start;
		while (tmp->next != NULL)
		{
			tmp = tmp->next;
		}
		tmp->next = new;
	}
	return new;
}
// wpuszczanie klienta do strzyzenia czyli "pobieranie" go z poczekalni
Position *AllowClientIn()
{
	Position *tmp = barberQueue;
	barberQueue = barberQueue->next;
	return tmp;

}
// metoda do wypisywania list przy parametrze -debug
void PrintDebug()
{
	printf("Waiting: ");	
	PrintQueue(barberQueue);
	printf("Resigned: ");	
	PrintQueue(resignedClients);
	printf("\n\n");
}
// metoda do wypisywania podstawowych informacji: liczbie zrezygnowanych klientow, stanie poczekalni oraz aktualnie obslugiwanym kliencie
void PrintStats()
{
	if (currentlyServedClient != 0)
	{
		printf("Res: %d wRoom: %d/%d In: %d\n", numberOfResignedClients, numberOfWaitingClients, numberOfChairs, currentlyServedClient);
	}
	else
	{
		printf("Res: %d wRoom: %d/%d In: -\n", numberOfResignedClients, numberOfWaitingClients, numberOfChairs);
	}
	if (IsDebug)
	{
		PrintDebug();
	}
}


void Customer(void *tid)
{
// klient wchodzi do poczekalni i zmienia stan liczby wolnych krzesel w poczekalni, wiec musi zablokowac do nich dostep
	pthread_mutex_lock(&accessChairs);
// kolejnym klientom nadawane jest ID rowne liczbie wszystkich klientow (pierwszy - 1, drugi - 2, itd..)
	totalNumberOfClients++;
	int id = totalNumberOfClients;
// jezeli sa miejsca w poczekalni
	if (numberOfWaitingClients < numberOfChairs)
	{
// to doliczamy kolejnego ktory czeka i dokladamy go do listy i wypisujemy zmiany
		numberOfWaitingClients++;
		Position *newClient = AddNewClientToQueue(&barberQueue, totalNumberOfClients);
		printf("New client got into lobby!\n");		
		PrintStats();
// informujemy, ze czeka klient
		sem_post(&customerReady);
// zmiany w stanie krzesel juz sie skonczyly wiec mozemy odblokowac dostep
		pthread_mutex_unlock(&accessChairs);
// czeka na swoja kolej
		sem_wait(&newClient->turn);

// czekamy na skonczenie strzyzenia
		GetCut(newClient);
	//	sem_wait(&newClient->wasCut);
	}
	else 
	{
// dodajemy zrezygnowanego klienta do listy, zliczamy i wypisujemy zmiany
		AddNewClientToQueue(&resignedClients, totalNumberOfClients);
		numberOfResignedClients++;
		printf("New client couldn't get into lobby!\n");
		PrintStats();

// rowniez mozemy odblokowac stan krzesel
		pthread_mutex_unlock(&accessChairs);
	}
}
void Barber()
{
	while(1)
	{
// fryzjer czeka na klienta - jak nie ma zadnego to spi: zasada dzialania semafory
		sem_wait(&customerReady);

// wzywajac klienta do siebie zmieni stan krzeselek w poczekalni wiec musi zablokowac dostep
		pthread_mutex_lock(&accessChairs);

// wzywa klienta, stan poczekalni sie zmienia, wypisujemy zmiany i odblokowujemy dostep
		numberOfWaitingClients--;
		Position *client = AllowClientIn(barberQueue);
		currentlyServedClient = client->id;

		printf("Next client for the cut!\n");
		PrintStats();

// konczymy zmieniac stan poczekalni
		pthread_mutex_unlock(&accessChairs);

// wzywa nastepnego klienta	
		sem_post(&client->turn);
		DoCut(client);
// okres oczekiwania na koniec strzyzenia
	//	sem_wait(&client->wasCut);
		currentlyServedClient = 0;
	}				
}

int main(int argc, char *argv[])
{
	int status = 0;
	pthread_t id1, id2;
	srand(time(NULL));
	if (argc < 2)
	{
		printf("N chairs expected!\n");
		exit(-1);
	}
	numberOfChairs = atoi(argv[1]);
	if (argc == 3)
	{
		if ((strncmp(argv[2], "-debug", 6) == 0))
		{
			IsDebug = 1;
		}
	}
	sem_init(&customerReady, 0, 0);
	sem_init(&cutting, 0, 0);
	status = pthread_create(&id1, NULL, (void*)Barber, NULL);
	if (status!=0)
	{
		printf("Barber thread couldn't start!'\n");
		exit(status);
	}
	while (1)
	{
		int k = rand()%3;
		sleep(k);
		status = pthread_create(&id2, NULL, (void*)Customer, (void*)id2);
		if (status != 0)
		{
			printf("Customer thread couldn't start!\n");
			exit(status);
		}
	}

	pthread_join(id1, NULL);
	return 0;
}
