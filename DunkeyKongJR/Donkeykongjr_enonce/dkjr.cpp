#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <SDL/SDL.h>
#include "./presentation/presentation.h"

// MACRO 

#define VIDE 0
#define DKJR 1
#define CROCO 2
#define CORBEAU 3
#define CLE 4

#define AUCUN_EVENEMENT 0

#define LIBRE_BAS	1
#define LIANE_BAS	2
#define DOUBLE_LIANE_BAS 3
#define LIBRE_HAUT	4
#define LIANE_HAUT	5

// FONCTIONS RELATIVE AU THREADS

void* FctThreadEvenements(void *);
void* FctThreadCle(void *);
void* FctThreadDK(void *);
void* FctThreadDKJr(void *);
void* FctThreadScore(void *);
void* FctThreadEnnemis(void *);
void* FctThreadCorbeau(void *);
void* FctThreadCroco(void *);

// FONCTION RELATIVES A LA GRILLE

void initGrilleJeu();
void setGrilleJeu(int l, int c, int type = VIDE, pthread_t tid = 0);
void afficherGrilleJeu();

// HANDLER DE SIGNAUX

void HandlerSIGUSR1(int);
void HandlerSIGUSR2(int);
void HandlerSIGALRM(int);
void HandlerSIGINT(int);
void HandlerSIGQUIT(int);
void HandlerSIGCHLD(int);
void HandlerSIGHUP(int);

// DESTRUCTEUR DE LA VARIABLE SPÉCIFIQUE

void DestructeurVS(void *p);

// VARIABLE THREADS

pthread_t threadCle;
pthread_t threadDK;
pthread_t threadDKJr;
pthread_t threadEvenements;
pthread_t threadScore;
pthread_t threadEnnemis;

pthread_cond_t condDK;
pthread_cond_t condScore;


// VARIABLE MUTEX

pthread_mutex_t mutexGrilleJeu;
pthread_mutex_t mutexDK;
pthread_mutex_t mutexEvenement;
pthread_mutex_t mutexScore;

// VARIABLE SPECIFIQUE

pthread_key_t keySpec;

// VARIABLE NORMALE

bool MAJDK = false;
int  score = 0;
bool MAJScore = true;
int  delaiEnnemis = 4000;
int  positionDKJr = 1;
int  evenement = AUCUN_EVENEMENT;
int etatDKJr;

// STRUCTURE 

typedef struct
{
  int type;
  pthread_t tid;
} S_CASE;

S_CASE grilleJeu[4][8];

typedef struct
{
  bool haut;
  int position;
} S_CROCO;



// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{

	// variable 

	int nbVie = 3;

	//initialisation de la fenêtre
	ouvrirFenetreGraphique();
	initGrilleJeu();

	// initialisation des MUTEX
	pthread_mutex_init(&mutexGrilleJeu,NULL);
	pthread_mutex_init(&mutexEvenement,NULL);

	// initialisation des conditions 

	pthread_cond_init(&condDK, NULL);


	// Armement des signaux
	struct sigaction A;

	A.sa_handler = HandlerSIGQUIT;
	A.sa_flags = 0; 
	sigemptyset(&A.sa_mask);
	sigaction(SIGQUIT,&A,NULL);

	// Masquage des signaux
	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask,SIGINT);
	sigprocmask(SIG_SETMASK,&mask,NULL);

	// création des threads
	pthread_create(&threadCle,NULL,FctThreadCle,0);
	pthread_create(&threadEvenements,NULL,FctThreadEvenements,(void *)threadDKJr);
	pthread_create(&threadDK,NULL,FctThreadDK,0);
	

	for(int i = 1; i<=nbVie; i++)
	{
		pthread_create(&threadDKJr,NULL,FctThreadDKJr,0);
		pthread_join(threadDKJr, NULL);
		afficherEchec(i);
	}

}

// -------------------------------------

void initGrilleJeu()
{
  int i, j;   
  
  pthread_mutex_lock(&mutexGrilleJeu);

  for(i = 0; i < 4; i++)
    for(j = 0; j < 7; j++)
      setGrilleJeu(i, j);

  pthread_mutex_unlock(&mutexGrilleJeu);
}

// -------------------------------------

void setGrilleJeu(int l, int c, int type, pthread_t tid)
{
  grilleJeu[l][c].type = type;
  grilleJeu[l][c].tid = tid;
}

// -------------------------------------

void afficherGrilleJeu()
{   
   for(int j = 0; j < 4; j++)
   {
       for(int k = 0; k < 8; k++)
          printf("%d  ", grilleJeu[j][k].type);
       printf("\n");
   }

   printf("\n");   
}


// THREAD GERANT LA CLE 

void * FctThreadCle(void *param)
{
	struct timespec temps = {1,200000000}; // structure qui permet d'attendre 0.7 sec
	int i = 0; // i --> type de la cle à afficher
	int typeop = 0; // permet de savoir l'opération à réaliser

	while(1)
	{
		if(typeop == 0)
		{
			i++;
		}
		else i--;

		pthread_mutex_lock(&mutexGrilleJeu);
		switch(i)
		{
			case 1:
				afficherCle(1);
				setGrilleJeu(0, 1, CLE, pthread_self());
				break;

			case 2: 
				afficherCle(2);
				setGrilleJeu(0, 1, VIDE, pthread_self());
				break;

			case 3:
				afficherCle(3);
				setGrilleJeu(0, 1, VIDE, pthread_self());
				break;

			case 4:
				afficherCle(4);
				setGrilleJeu(0, 1, VIDE, pthread_self());
				break;
		}
		pthread_mutex_unlock(&mutexGrilleJeu);
		afficherGrilleJeu();
		nanosleep(&temps,NULL);
		effacerCarres(3,12,3,3);

		if(i == 4)
		{
			typeop = 1;
		}
		else
		{
			if(i == 1)
				typeop = 0;
		}
	}
	
}

// THREAD GERANT LES EVENEMENTS

void * FctThreadEvenements(void *param)
{
	struct timespec temps = {0,100000000};
	int evt;
	while (1)
	{
		pthread_mutex_lock(&mutexEvenement);
		evenement = AUCUN_EVENEMENT;
		pthread_mutex_unlock(&mutexEvenement);

		evt = lireEvenement();

		if(evt == SDL_QUIT)
			exit(0);
		else
			pthread_mutex_lock(&mutexEvenement);
				evenement = evt;
			pthread_mutex_unlock(&mutexEvenement);

			pthread_kill(threadDKJr,SIGQUIT);

		nanosleep(&temps,NULL);
	}
}

// THREAD GERANT DKJR 

void * FctThreadDKJr(void *param)
{
	// VARIABLE 
	bool on = true;
	struct timespec temps = {1,400000000};
	struct timespec fail = {0,500000000};
	// MASQUE
	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask,SIGQUIT);
	sigprocmask(SIG_SETMASK,&mask,NULL);

	pthread_mutex_lock(&mutexGrilleJeu);
	setGrilleJeu(3,1,DKJR);
	afficherDKJr(11, 9, 1);
	positionDKJr = 1;
	etatDKJr = LIBRE_BAS;

	pthread_mutex_unlock(&mutexGrilleJeu);

	

	while(on)
	{
		pause();

		pthread_mutex_lock(&mutexEvenement);
		pthread_mutex_lock(&mutexGrilleJeu);
		printf("etatDKJr : %d\n",etatDKJr);
		switch(etatDKJr)
		{
			case LIBRE_BAS:
				switch(evenement)
				{
					case SDLK_LEFT:
						if(positionDKJr>1)
						{

							setGrilleJeu(3, positionDKJr);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
							positionDKJr--;
							setGrilleJeu(3, positionDKJr, DKJR);
							afficherGrilleJeu();
							afficherDKJr(11, (positionDKJr * 2) + 7,((positionDKJr - 1) % 4) + 1);
						}
						break;

					case SDLK_RIGHT:
					
						if(positionDKJr < 7)
						{
							setGrilleJeu(3, positionDKJr);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
							positionDKJr++;
							setGrilleJeu(3, positionDKJr, DKJR); 
							afficherGrilleJeu();
							afficherDKJr(11, (positionDKJr * 2) + 7,((positionDKJr - 1) % 4) + 1);
						}
						break;

					case SDLK_UP:
						if (positionDKJr == 7 )
						{
							setGrilleJeu(3,positionDKJr);
							effacerCarres(11,(positionDKJr * 2) + 7 ,2,2);
							setGrilleJeu(2,positionDKJr,DKJR);
							afficherGrilleJeu();
						
							afficherDKJr(10, (positionDKJr * 2) + 7,5);
							etatDKJr = DOUBLE_LIANE_BAS;	
						}
						else
						{

								if(positionDKJr == 1 or positionDKJr == 5)
								{
									setGrilleJeu(3,positionDKJr);
									effacerCarres(11,(positionDKJr * 2) + 7 ,2,2);
									setGrilleJeu(2,positionDKJr,DKJR);
									afficherGrilleJeu();
									afficherDKJr(10, (positionDKJr * 2) + 7,7);
									etatDKJr = LIANE_BAS;
								}
								else
								{
									setGrilleJeu(3,positionDKJr);
									effacerCarres(11,(positionDKJr * 2) + 7 ,2,2);
									setGrilleJeu(2,positionDKJr,DKJR);
									afficherGrilleJeu();
									afficherDKJr(10, (positionDKJr * 2) + 7,8);

									pthread_mutex_unlock(&mutexGrilleJeu);
									nanosleep(&temps,NULL);
									pthread_mutex_lock(&mutexGrilleJeu);

									setGrilleJeu(2,positionDKJr);
									effacerCarres(10,(positionDKJr * 2) + 7 ,2,2);
									setGrilleJeu(3,positionDKJr,DKJR);
									afficherGrilleJeu();
									afficherDKJr(11, (positionDKJr * 2) + 7,((positionDKJr - 1) % 4) + 1);
								}
								
						}
						break;
				}
				break;

				case DOUBLE_LIANE_BAS:
					switch(evenement)
					{

						case SDLK_UP: 
							setGrilleJeu(2,positionDKJr);
							effacerCarres(10,(positionDKJr * 2) + 7 ,2,2);
							setGrilleJeu(1,positionDKJr,DKJR);
							afficherGrilleJeu();
							afficherDKJr(7, (positionDKJr * 2) + 7,7);
							etatDKJr = LIBRE_HAUT;
							break;

						case SDLK_DOWN:
							setGrilleJeu(2,positionDKJr);
							effacerCarres(10,(positionDKJr * 2) + 7 ,2,2);
							setGrilleJeu(3,positionDKJr,DKJR);
							afficherGrilleJeu();
							afficherDKJr(11, (positionDKJr * 2) + 7,((positionDKJr - 1) % 4) + 1);
							etatDKJr = LIBRE_BAS;
							break;
					}

					break;

				case LIBRE_HAUT:
					switch(evenement)
					{

						case SDLK_DOWN:
							if(positionDKJr == 7)
							{
								setGrilleJeu(1,positionDKJr);
								effacerCarres(7,(positionDKJr * 2) + 7 ,2,2);
								setGrilleJeu(2,positionDKJr,DKJR);
								afficherGrilleJeu();
								afficherDKJr(10, (positionDKJr * 2) + 7,7);
								etatDKJr = DOUBLE_LIANE_BAS;	
							}
							break;

						case SDLK_LEFT:

							if(positionDKJr>=3)
							{
								setGrilleJeu(1,positionDKJr);
								effacerCarres(7,(positionDKJr * 2) + 7 ,2,2);
								positionDKJr--;
								setGrilleJeu(1,positionDKJr,DKJR);
								afficherGrilleJeu();
								afficherDKJr(7, (positionDKJr * 2) + 7,((positionDKJr - 1) % 4) + 1);

								if(positionDKJr == 2 and grilleJeu[0][1].type != CLE)
								{
									

									setGrilleJeu(1,positionDKJr);
									effacerCarres(7,(positionDKJr * 2) + 7 ,2,2);
									afficherDKJr(6, (positionDKJr * 2) + 7,8);

									pthread_mutex_unlock(&mutexGrilleJeu);
									nanosleep(&fail,NULL);
									pthread_mutex_lock(&mutexGrilleJeu);

									effacerCarres(6,(positionDKJr * 2) + 7 ,2,2);
									afficherDKJr(6,(positionDKJr * 2) + 7 ,12);
									

									pthread_mutex_unlock(&mutexGrilleJeu);
									nanosleep(&fail,NULL);
									pthread_mutex_lock(&mutexGrilleJeu);

									effacerCarres(6,(positionDKJr * 2) + 7 ,2,2);
									afficherDKJr(11,7,13);

									pthread_mutex_unlock(&mutexGrilleJeu);
									nanosleep(&fail,NULL);
									pthread_mutex_lock(&mutexGrilleJeu);

									effacerCarres(11,7,2,2);

									pthread_mutex_unlock(&mutexEvenement);
									pthread_mutex_unlock(&mutexGrilleJeu);
									pthread_exit(0);
								}
								else
								{
									if(positionDKJr == 2 and grilleJeu[0][1].type == CLE)
									{	

										setGrilleJeu(1,positionDKJr);
										effacerCarres(7,(positionDKJr * 2) + 7 ,2,2);
										afficherDKJr(6, (positionDKJr * 2) + 7,8);
										effacerCarres(6,(positionDKJr * 2) + 7 ,2,2);

										afficherDKJr(5,12,9);
										

										pthread_mutex_unlock(&mutexGrilleJeu);
										nanosleep(&fail,NULL);
										pthread_mutex_lock(&mutexGrilleJeu);
										effacerCarres(5,12,3,2);
										afficherDKJr(3,11,10);
										nanosleep(&fail,NULL);
										pthread_mutex_lock(&mutexGrilleJeu);
										effacerCarres(3,11,3,2);

										pthread_mutex_lock(&mutexDK);
										MAJDK = true;
										pthread_mutex_unlock(&mutexDK);
										pthread_cond_signal(&condDK);

										afficherDKJr(11, 9, 1);
										positionDKJr = 1;
										setGrilleJeu(3,positionDKJr,DKJR);
										etatDKJr = LIBRE_BAS;
									}	
								}

							} 
							break;

						case SDLK_RIGHT:
							if(positionDKJr <= 7)
							{
								setGrilleJeu(1,positionDKJr);
								effacerCarres(7,(positionDKJr * 2) + 7 ,2,2);
								positionDKJr++;
								setGrilleJeu(1,positionDKJr,DKJR);
								afficherGrilleJeu();
								if(positionDKJr < 7)
									afficherDKJr(7, (positionDKJr * 2) + 7,((positionDKJr - 1) % 4) + 1);
								else
									afficherDKJr(7, (positionDKJr * 2) + 7,7);
							}

							break;

						case SDLK_UP:

							if(positionDKJr == 6)
							{
								setGrilleJeu(1,positionDKJr);
								effacerCarres(7,(positionDKJr * 2) + 7 ,2,2);
								setGrilleJeu(0,positionDKJr,DKJR);
								afficherGrilleJeu();
								afficherDKJr(6, (positionDKJr * 2) + 7,7);
								etatDKJr = LIANE_HAUT;		
							}
							else
							{
								if (positionDKJr != 7)
								{
									setGrilleJeu(1,positionDKJr);
									effacerCarres(7,(positionDKJr * 2) + 7 ,2,2);
									setGrilleJeu(0,positionDKJr,DKJR);
									afficherGrilleJeu();
									afficherDKJr(6, (positionDKJr * 2) + 7,8);

									pthread_mutex_unlock(&mutexGrilleJeu);
									nanosleep(&temps,NULL);
									pthread_mutex_lock(&mutexGrilleJeu);

									setGrilleJeu(0,positionDKJr);
									effacerCarres(6,(positionDKJr * 2) + 7 ,2,2);
									setGrilleJeu(1,positionDKJr,DKJR);
									afficherGrilleJeu();
									afficherDKJr(7, (positionDKJr * 2) + 7,((positionDKJr - 1) % 4) + 1);
								}
							}

							break;
					}
					break;	

				case LIANE_HAUT:
					switch(evenement)
					{
						case SDLK_DOWN:
							setGrilleJeu(0,positionDKJr);
							effacerCarres(6,(positionDKJr * 2) + 7 ,2,2);
							setGrilleJeu(1,positionDKJr,DKJR);
							afficherGrilleJeu();
							afficherDKJr(7, (positionDKJr * 2) + 7,((positionDKJr - 1) % 4) + 1);
							etatDKJr = LIBRE_HAUT;
							break;
					}
					break;

				case LIANE_BAS:
					switch(evenement)
					{
						case SDLK_DOWN:
						
							setGrilleJeu(2,positionDKJr);
							effacerCarres(10,(positionDKJr * 2) + 7 ,2,2);
							setGrilleJeu(3,positionDKJr,DKJR);
							afficherGrilleJeu();
							afficherDKJr(11, (positionDKJr * 2) + 7,((positionDKJr - 1) % 4) + 1);
							etatDKJr = LIBRE_BAS;
							break;
						
					}
					break;
		}

	

		pthread_mutex_unlock(&mutexEvenement);
		pthread_mutex_unlock(&mutexGrilleJeu);
		
	}

}



void * FctThreadDK(void * param)
{	
	int cageDev = 1;
	// Affiche la cage DK en entier
	for(int i = 1; i<=4; i++)
		afficherCage(i);

	while(1)
	{	
		pthread_mutex_lock(&mutexDK);
		while(MAJDK == false)
			pthread_cond_wait(&condDK,&mutexDK);
		MAJDK = false;
		pthread_mutex_unlock(&mutexDK);	

		switch(cageDev)
		{
			case 1:
				effacerCarres(2,7,2,2);
				cageDev++;
				break;

			case 2:
				effacerCarres(2,9,2,2);
				cageDev++;
				break;

			case 3:
				effacerCarres(4,7,2,2);
				cageDev++;
				break;

			case 4: 
				effacerCarres(4,9,2,2);
				cageDev++;
				break;
		}

		if(cageDev == 5)
		{
			cageDev = 1;
			for(int i = 1; i<=4; i++)
				afficherCage(i);
		}
		
	}
}




//HANDLER 

// -- SIGQUIT

void HandlerSIGQUIT(int sig)
{
	printf("Réveil du dkjr\n");
}