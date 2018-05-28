/*
    Compilation : 
        gcc -o sac knapsack.c -pedantic -ansi -Wall -Werror -Ofast
        
    Compilation (debug) :
        gcc -o sac knapsack.c -pedantic -ansi -Wall -Werror -Ofast -DDBGPRINT

    Lancement :
        syntaxe :
            ./sacvx fichier poids_max nb_groupes item_par_groupe
        
        exemples :
        ./sac 100_85_12015.txt 12015 100 85
        ./sac sukp\ 400_400_49822.txt 49822 400 400
        ./sac sukp\ 400_400_49822.txt 49822 400 400

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

typedef struct s_status {

    int poids_max ;
    int poids_courrant;
    int profit_courrant;
    int meilleur_profit;
    int poids_libre ;
    int nb_permut;

} Status ;

void  permutation(int **items, int *poids, int *grp_pris, int *grp_profit, float *profit, Status *s, int *poids_ro);
int   solve(int **items, int *poids, int *grp_pris, int *grp_profit, float *profit, Status *s, int *poids_ro);
void  supp_grp(int index, int **items, int *grp_profit, int *poids, int *poids_ro, Status *s,int *grp_pris);
int   ajouter_grp(int index,int **items, int *grp_profit, int *poids, Status *s,int *grp_pris);
int   max_possible_id(float *profit, int *grp_pris,int *poids, int poidslibre, int **items);
void  update_ratio(float *profit, int *grp_profit, int *poids, int **items );
int   est_present(int indice_objet, int except, int **items,int *grp_pris);
void  load_data(int *grp_profit, int *poids, int **items, char *filename);
void  detruire_valeur(int indice, int *poids, int **items);
int   somme_relative(int *item, int *tab_valeur);
void  print_status(Status s);

int item_par_groupe, nb_groupes;

int main(int argc, char* argv[]){
    
    int
        *grp_pris,
        *poids_ro,
        *grp_profit,
        *poids,
        **items,
        i;

    float
        *profit;

    Status etat;

    struct timeval tv1, tv2;

    etat.poids_courrant = etat.profit_courrant = etat.nb_permut = 0;

    if(argc < 5){
        puts("./sac fichier poids_max nb_groupes item_par_groupe");
        exit(EXIT_FAILURE);
    }

    etat.poids_max  = atoi(argv[2]);
    nb_groupes      = atoi(argv[3]);
    item_par_groupe = atoi(argv[4]);


    printf("Fichier : %s\nPoids max : %d\nNb Groupes : %d\nItem par groupe : %d",
        argv[1],etat.poids_max,nb_groupes,item_par_groupe);

    poids       =    malloc( item_par_groupe * sizeof(int));
    poids_ro    =    malloc( item_par_groupe * sizeof(int));
    grp_pris    =    malloc( nb_groupes * sizeof(int));
    grp_profit  =    malloc( nb_groupes * sizeof(int));
    profit      =    malloc( nb_groupes * sizeof(float));
    items       =    malloc( nb_groupes * sizeof(int*));
    for(i=0;i<nb_groupes;i++)
        items[i]     =  malloc(item_par_groupe * sizeof(int));


    if( !(grp_profit && poids && items && grp_pris && profit)){
        fprintf(stderr, "Erreur malloc");
        exit(EXIT_FAILURE);
    }

    load_data(grp_profit,poids,items, argv[1]);

    memset(grp_pris,0, nb_groupes * sizeof(int));
    memcpy(poids_ro, poids, item_par_groupe * sizeof(int));

    gettimeofday(&tv1, NULL);
    solve(items, poids,grp_pris,grp_profit,profit,&etat, poids_ro);
    
    puts("\nEtat initial du sac :");
    print_status(etat);
    
    puts("\n\nLancement des permutations ...");
    permutation(items, poids,grp_pris,grp_profit,profit,&etat, poids_ro);

    gettimeofday(&tv2, NULL);

    printf("\nEtat final :\nProfit : %d  poids : %d  temps : %f\nNombre de permutations : %d\n",
        etat.profit_courrant, etat.poids_courrant,
        (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 +
        (double)(tv2.tv_sec - tv1.tv_sec), etat.nb_permut);

    return 0;
}

void permutation(int **items, int *poids, int *grp_pris, int *grp_profit, float *profit, Status *s, int *poids_ro){

    int  i,grp_test, meilleur_indice_remplacement ;

    for(grp_test=0;grp_test<nb_groupes;grp_test++){
        s->meilleur_profit = s->profit_courrant ;
        meilleur_indice_remplacement = -1 ;

        /* Si le groupe n'a pas été selectionné, on ne peut pas l'échanger */
        if(grp_pris[grp_test] == 0) continue;

        for(i=0;i<nb_groupes;i++){

            if(grp_pris[i] != 0 || i == grp_test) continue;

            #ifdef DBGPRINT
            printf("\n\n ====  Tentative - remplacement du groupe %d par le %d  ====", grp_test, i);
            printf("\n initial : profit : %d  poids : %d",s->profit_courrant, s->poids_courrant );
            #endif

            supp_grp(grp_test,items,grp_profit,poids,poids_ro,s,grp_pris);
            ajouter_grp(i,items,grp_profit,poids,s,grp_pris);
            if( (s->profit_courrant > s->meilleur_profit) && (s->poids_courrant <= s->poids_max)){
                s->meilleur_profit = s->profit_courrant;
                #ifdef DBGPRINT
                printf("\nAmélioration trouvée\nnouveau poids : %d\nNouveau profit: %d", s->poids_courrant, s->meilleur_profit);
                exit(EXIT_FAILURE);
                #endif
                meilleur_indice_remplacement = i ;
            }
            supp_grp(i,items,grp_profit,poids,poids_ro,s,grp_pris);
            ajouter_grp(grp_test,items,grp_profit,poids,s,grp_pris);

            #ifdef DBGPRINT
            print_status(*s);
            #endif
        }
        
        if(meilleur_indice_remplacement != -1){
            #ifdef DBGPRINT
            puts("\nOptimisation");
            printf(" Remplacement du groupe %d ar le groupe %d effectif", grp_test,meilleur_indice_remplacement);
            #endif
            s->nb_permut++;
            supp_grp(grp_test,items,grp_profit,poids,poids_ro,s,grp_pris);
            ajouter_grp(meilleur_indice_remplacement,items,grp_profit,poids,s,grp_pris);
        }
        #ifdef DBGPRINT
        else printf("Pas de remplacement pertinent trouvé");
        #endif
    } /* Fin boucle j */
}

int ajouter_grp(int index,int **items, int *grp_profit, int *poids, Status *s,int *grp_pris){

    int add_poids, i; 

    grp_pris[index] = 1 ;

    /* Ajout des données de la ligne au total */
    add_poids = somme_relative(items[index], poids);

    s->poids_courrant += add_poids ;
    s->profit_courrant += grp_profit[index];

    /* Mise à zéro des poids utilisés */
    for(i = 0; i < item_par_groupe; i++){
        if(items[index][i] == 1){
            poids[i] = 0;
        }
    }

    #ifdef DBGPRINT
    printf("\nAjout groupe :%d  Ajout du poids : %d  ajout du profit : %d",index, add_poids,grp_profit[index] );
    #endif

    return 1;
}

void supp_grp(int index, int **items, int *grp_profit, int *poids, int *poids_ro, Status *s,int *grp_pris){

    int i ;
    /* supprimer le profit */
    s->profit_courrant -= grp_profit[index] ;

    #ifdef DBGPRINT
        printf("\nGroupe retiré : %d profit retiré %d poids retiré: ",index,grp_profit[index]);
    #endif
    /* supprimer le poids des objets ne se trouvant pas dans d'autres groupes */
    for(i=0;i<item_par_groupe;i++){
        /* Un objet objet présent dans le groupe */
        if(items[index][i] == 1 ){
            /* Si non présent dans d'autres groupes */
            if(!est_present(i, index, items, grp_pris)){
                /* On réinitialise son poids (passé à 0 lors de la selection de l'objet */
                poids[i] = poids_ro[i];
                /* On supprime son poids du poids total */
                s->poids_courrant -= poids[i];
                #ifdef DBGPRINT
                    printf("%d ", poids[i]);
                #endif
            }
        }
    }
    /* Libérer le groupe */
    grp_pris[index] = 0 ;
}


int est_present(int indice_objet, int except, int **items,int *grp_pris){
    
    int i;
    
    for(i = 0 ; i < nb_groupes ; i++){
        if( grp_pris[i] == 1 && i != except && items[i][indice_objet] == 1){
            return 1 ;
        }
    }
    return 0;
}

int solve(int **items, int *poids, int *grp_pris, int *grp_profit, float *profit, Status *s, int *poids_ro){

    int
        meilleur_benefice_indice,i;

    i = 0;
    while(s->poids_courrant <= s->poids_max){

        #ifdef DBGPRINT
            printf("\nGroupe utilisés :  %d/%d",i,nb_groupes);
        #endif

        /* Plus de groupe disponible */
        if(i++ == nb_groupes){
            #ifdef DBGPRINT
                printf("\nPlus de groupe disponible, arrêt.");
            #endif
            break;
        }

        /* Chargement des données & calcul des ratios */
        update_ratio(profit, grp_profit, poids, items);

        /* Selection de la ligne avec le meilleur bénéfice */
        meilleur_benefice_indice = max_possible_id(profit, grp_pris, poids, (s->poids_max - s->poids_courrant), items);

        if(!meilleur_benefice_indice){
                #ifdef DBGPRINT
                    puts("\nLe poids max est dépassé.");
                    printf("\nDernier profit valide : %d", s->profit_courrant);
                    printf("\nDernier poids valide  : %d", s->poids_courrant);
                #endif
                break;
        }

        #ifdef DBGPRINT
            printf("\nLigne la plus bénéfique : %d", meilleur_benefice_indice);
        #endif
        ajouter_grp(meilleur_benefice_indice,items,grp_profit,poids,s,grp_pris);

        #ifdef DBGPRINT
            puts("\n   ================   ");
        #endif
    }
    return s->profit_courrant ;
}

int somme_relative(int *item, int *tab_valeur){
    
    int total,i;

    for(total = 0, i = 0; i < item_par_groupe; i++){
        total += item[i] * tab_valeur[i];
    }
    return total ;
}

int max_possible_id(float *profit, int *grp_pris, int *poids, int poidslibre, int **items){
    
    int i, max_i ;
    float max;

    max = i = max_i = 0 ;

    for(i=0;i<nb_groupes;i++){
        if(grp_pris[i] == 1)
            continue;
        else if( profit[i] > max && somme_relative(items[i], poids) <=  poidslibre){
                max_i = i;
                max = profit[i];    
        }
    }

    return max_i ;
}

void print_status(Status s){
    printf("\nNouveau poids total : %d\nNouveau profit total : %d\nEspace disponible : %d",
            s.poids_courrant,s.profit_courrant,s.poids_max - s.poids_courrant);
}


void update_ratio(float *profit, int *grp_profit, int *poids, int **items ){

    int i, j, *current, poids_ligne, item_present ;

    for(i=0;i<nb_groupes;i++){
        current = items[i];
        for(j = poids_ligne = 0; j < item_par_groupe; j++){
            item_present = current[j];
            if(item_present){
                poids_ligne += poids[j];
            }
        }
        profit[i] = (float)grp_profit[i] / (float)poids_ligne ;
    }
}

void load_data(int *grp_profit, int *poids, int **items, char *filename){
    
    int *current, i, j;

    FILE *in;
    in = fopen(filename,"r");
    
    if(!in){
        printf("Erreur lors de l'ouverture de '%s'", filename);
        exit(1);
    }

    current = grp_profit ;

    for(i=0;i<nb_groupes;i++){
        fscanf(in, "%d", current++);
    }
    
    current = poids ;

    for(i=0;i<item_par_groupe;i++){
        fscanf(in, "%d", current++);
    }

    for(i=0;i<nb_groupes;i++){
        current = items[i];
        for(j = 0; j < item_par_groupe; j++){
            fscanf(in, "%d", current++);
        }
    }
    fclose(in);
}