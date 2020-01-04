#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct
{
    unsigned char r,g,b;
}Pixel;

typedef struct
{    //structura retine chenarele care au corelatia mai mare ca 0.5 si in final le elimina pe cele ce nu respecta conditia de suprapunere
    int lin_st_sus , col_st_sus , lin_dr_jos , col_dr_jos , cifra;
    float corelatie;
}Identificare;

/// *********** CRIPTARE ************

void XORSHIFT32(unsigned int **R,unsigned int n,unsigned int R0)
{
    //functie care genereaza numere pseudo-aleatoare XORSHIFT32
    (*R)=(unsigned int *)malloc((n+1)*sizeof(unsigned int));    //vectorul R va retine toate cele n numere generate pornind de la R0
    unsigned int i;
    (*R)[0]=R0;
    for(i=1;i<=n;i++)
    {
        R0=R0^R0<<13;
        R0=R0^R0>>17;
        R0=R0^R0<<5;
        (*R)[i]=R0;
    }
}

void dimensiune_imagine(char *caleImagine,unsigned int *latime,unsigned int *lungime)
{
    //functie care determina lungimea si latimea unei imagini
    FILE *f;
    f=fopen(caleImagine,"rb");

    if(f==NULL)
    {
        printf("Eroare deschidere fisier pentru aflarea dimensiunii imaginii");
        return ;
    }

    fseek(f,18,0);
    //latimea se afla incepand cu octetul 18 in headerul imaginii si este un numar natural de tip unsigned int
    fread(&(*latime),sizeof(unsigned int),1,f);
    //lungimea se afla incepand cu octetul 22 in headerul imaginii si este un numar natural de tip unsigned int
    fread(&(*lungime),sizeof(unsigned int),1,f);

    fclose(f);
}

void header(char * caleImagine,unsigned char **h)
{
    //functie care copiaza headerul unei imagini intr-un vector
    FILE *f;
    f = fopen ( caleImagine, "rb");
    if(f==NULL)
    {
        printf("Eroare deschidere fisier");
        return ;
    }

    (*h)=(unsigned char *)malloc(55*sizeof(unsigned char));

    if((*h)==NULL)
    {
        printf("Eroare alocare spatiu pentru header");
        return ;
    }
    unsigned char x;
    int i;
    for(i=0;i<54;i++)
    {
        fread(&x,sizeof(unsigned char),1,f);
        (*h)[i]=x;
    }

    fclose(f);
}

void liniarizare(char *caleImagine,Pixel **v)
{
    //functie care copiaza pixelii unei imagini intr-un vector
    FILE *f;
    f=fopen(caleImagine,"rb");

    if(f==NULL)
    {
        printf("\nEroare deschidere fisier la liniarizare\n");
        return ;
    }

    unsigned int lungime_img,latime_img,padding;
    dimensiune_imagine(caleImagine,&latime_img,&lungime_img);

    //printf("Dimensiunea imaginii in pixeli inainte de liniarizare este (latime x inaltime): %u x %u\n",latime_img, lungime_img);

    if(latime_img %4 == 0)                      //calculam paddingul
         padding = 0;                           //daca latimea e multiplu de 4, atunci si 3*latimea e multiplu de 4
    else padding = 4-(3*latime_img)%4;

    (*v)=(Pixel *)malloc(lungime_img*latime_img*sizeof(Pixel));   //alocam spatiu pentru vectorul corespunzator imaginii liniarizate

    if((*v)==NULL)
    {
        printf("\nEroare alocare spatiu pentru vectorul liniarizat\n");
        return ;
    }

    fseek(f,54,0);                              //depasim headerul
    int i,j;
    Pixel p;
    for(i=0 ; i<lungime_img ; i++)
    {
        for(j=0 ; j<latime_img ; j++)           //parcurgem imaginea tip matrice
        {
            fread(&p.b,sizeof(unsigned char),1,f);
            fread(&p.g,sizeof(unsigned char),1,f);
            fread(&p.r,sizeof(unsigned char),1,f);

            (*v)[(lungime_img-1-i)*latime_img+j]=p;
        }

        fseek(f,padding,1);                      //sarim peste padding

    }

    fclose(f);

}


void deliniarizare(unsigned char *header_imagine,char *caleImagine, Pixel *v)
{
    FILE *f;
    f=fopen(caleImagine,"wb+");
    if(f==NULL)
    {
        printf("\nEroare deschidere fisier la deliniarizare\n");
        return ;
    }

    int i,j;
    //scriem in fisierul tip bmp headerul imaginii transmis ca parametru
    for(i=0;i<54;i++)
    {
        fwrite(&header_imagine[i],sizeof(unsigned char),1,f);
    }
    fflush(f);                      //golim bufferul inaintea unei citiri intrucat s-a efectuat o scriere

    unsigned int lungime_img,latime_img,padding;
    dimensiune_imagine(caleImagine,&latime_img,&lungime_img);

    //printf("Dimensiunea imaginii in pixeli inainte de deliniarizare este (latime x inaltime): %u x %u\n",latime_img, lungime_img);

    if(latime_img%4==0)padding=0;
    else padding=4-(3*latime_img)%4;


    Pixel p;
    for(i=(lungime_img-1)*latime_img ; i>=0 ; i=i-latime_img)
    {
        for(j=0;j<latime_img;j++)
        {
            p=v[i+j];                                  //scriem pixelii in mod corespunzator in imagine
            fwrite(&p.b,sizeof(unsigned char),1,f);
            fwrite(&p.g,sizeof(unsigned char),1,f);
            fwrite(&p.r,sizeof(unsigned char),1,f);

        }
      unsigned char x=0;
      for(j=0;j<padding;j++)
            fwrite(&x,sizeof(unsigned char),1,f);      //scriem paddingul imaginii
    }

    fclose(f);
}




void algoritmul_Durstenfeld(unsigned int **perm,int n,char *cale_cheie_secreta)
{

    (*perm)=(unsigned int *)malloc(sizeof(unsigned int)*n);
    if((*perm)==NULL)
    {
        printf("Eroare alocare spatiu pentru permutare");
        return ;
    }



    unsigned int i;
    for(i=0;i<n;i++)
        (*perm)[i]=i;   //intai facem permutarea identica


    FILE *f;
    f=fopen(cale_cheie_secreta,"rb");

    if(f==NULL)
    {
        printf("Eroare deschidere fisier ce contine cheia secreta");
        return ;
    }

    unsigned int R0,*R;
    fscanf(f,"%d",&R0);
    XORSHIFT32(&R,n,R0);     //facem vectorul cu numere generate de XORSHIFT32

    for(i=n-1;i>=1;i--)
    {
        unsigned int poz=R[n-i]%(i+1);       //poz=un numar cuprins intre 0 si i datorita %(i+1)
        unsigned int aux=(*perm)[i];
        (*perm)[i]=(*perm)[poz];
        (*perm)[poz]=aux;
    }

    fclose(f);
}


Pixel xor_pixel_pixel(Pixel p,Pixel q)
{
    //functie ce xoreaza 2 pixeli
    Pixel a;
    a.r=p.r^q.r;
    a.g=p.g^q.g;
    a.b=p.b^q.b;
    return a;   //returneaza pixelul xorat
}

Pixel xor_pixel_numar(Pixel p,unsigned int nr)
{
    //functie ce xoreaza un pixel cu un numar
    Pixel a;
    unsigned char *octet;
    octet=(unsigned char *)&nr;
    a.b=(p.b)^(*octet); //(*octet)contine valoarea aflata la adresa octet
    octet++;            //trecem la adresa urmatorului octet
    a.g=p.g^(*octet);
    octet++;
    a.r=p.r^(*octet);
    return a;
}


void criptare(char *caleImagineInitiala,char *caleImagineCriptata,char *cheieSecreta)
{
    FILE *fkey;

    fkey=fopen(cheieSecreta,"rb");

    if(fkey==NULL)
    {
        printf("Eroare deschidere fisier in functia criptare pentru cheia secreta");
        return ;
    }

    unsigned int R0,seed;
    fscanf(fkey,"%u %u",&R0,&seed);
    fclose(fkey);

    unsigned int lungime_img,latime_img;
    dimensiune_imagine(caleImagineInitiala,&latime_img,&lungime_img);

    //printf("Dimensiunea imaginii in pixeli inainte de criptare este (latime x inaltime): %u x %u\n",latime_img, lungime_img);

    unsigned int *R,*P,i;
    Pixel *L,*C,*P1;
    //generarea numerelor pseudo-aleatoare
    XORSHIFT32(&R,2*lungime_img*latime_img,R0);
    if(R==NULL)
    {
        printf("Eroare alocare spatiu la criptare pentru R");
        return ;
    }

    P1=(Pixel *)malloc((lungime_img*latime_img+1)*sizeof(Pixel));
    if(P1==NULL)
    {
        printf("Eroare alocare spatiu la criptare pentru P1");
        return ;
    }

    C=(Pixel *)malloc((lungime_img*latime_img+1)*sizeof(Pixel));
    if(C==NULL)
    {
        printf("Eroare alocare spatiu la criptare pentru C");
        return ;
    }
    //liniarizarea imaginii de criptat
    liniarizare(caleImagineInitiala,&L);
    if(L==NULL)
    {
        printf("Eroare alocare spatiu la criptare pentru vectorul liniarizat");
        return ;
    }
    //generarea permutarii aleatoare
    algoritmul_Durstenfeld(&P,lungime_img*latime_img,cheieSecreta);
    if(P==NULL)
    {
        printf("Eroare alocare spatiu la criptare pentru vectorul generat de algoritmul Durstenfeld");
        return ;
    }
    //permutarea pixelilor imaginii liniarizate conform permutarii aleatoare
    for(i=0;i<lungime_img*latime_img;i++)
    {
        P1[P[i]]=L[i];

    }
    //relatia de substitutie prin xorare
    C[0]=xor_pixel_numar(P1[0],seed);
    C[0]=xor_pixel_numar(C[0],R[lungime_img*latime_img]);

    for(i=1;i<lungime_img*latime_img;i++)
        {
            C[i]=xor_pixel_pixel(C[i-1],P1[i]);                        //xoram pixelii
            C[i]=xor_pixel_numar(C[i],R[lungime_img*latime_img+i]);
        }


    unsigned char *head;
    header(caleImagineInitiala , &head);
    if(head==NULL)
    {
        printf("Eroare alocare spatiu la criptare pentru header");
        return ;
    }
    deliniarizare( head , caleImagineCriptata , C);

    free(R);
    free(P);
    free(P1);
    free(L);
    free(C);

}


void permutare_inversa(unsigned int *p,unsigned int **q,int n)
{
    //functie care calculeaza inversa unei permutari p date ca parametru cu n elemente
    (*q)=(unsigned int *)malloc(n*sizeof(unsigned int));
    if((*q)==NULL)
    {
        printf("Eroare alocare spatiu la permutarea inversa");
        return ;
    }

    int i;
    for(i=0;i<n;i++)
        (*q)[p[i]]=i;
}


void decriptare(char *caleImagineCriptata,char *caleImagineInitiala,char *cheieSecreta)
{
    //functie care realizeaza decriptarea imaginii data prin calea caleImagineCriptata
    FILE *fkey;

    fkey=fopen(cheieSecreta,"rb");
    if(fkey==NULL)
    {
        printf("Eroare deschidere fisier ce contine cheia secreta");
        return ;
    }
    unsigned int R0,seed;
    fscanf(fkey,"%u %u",&R0,&seed);
    fclose(fkey);



    unsigned int lungime_img,latime_img;
    dimensiune_imagine(caleImagineCriptata,&latime_img,&lungime_img);

    //printf("Dimensiunea imaginii in pixeli inainte de decriptare este (latime x inaltime): %u x %u\n",latime_img, lungime_img);

    unsigned int *R,*P,i,*Inversa;
    Pixel *Init,*C,*C1;
    //generarea numerelor pseudo-aleatoare
    XORSHIFT32(&R,2*lungime_img*latime_img,R0);

    algoritmul_Durstenfeld(&P,lungime_img*latime_img,cheieSecreta);
    permutare_inversa(P,&Inversa,lungime_img*latime_img);


    C1=(Pixel *)malloc((lungime_img*latime_img+1)*sizeof(Pixel));
    C=(Pixel *)malloc((lungime_img*latime_img+1)*sizeof(Pixel));
    //liniarizarea imaginii criptate
    liniarizare(caleImagineCriptata,&C);
     //relatia de substitutie aplicata pe imaginea criptata
    C1[0]=xor_pixel_numar(C[0],seed);
    C1[0]=xor_pixel_numar(C1[0],R[lungime_img*latime_img]);
    for(i=1;i<lungime_img*latime_img;i++)
        {
            C1[i]=xor_pixel_pixel(C[i-1],C[i]);
            C1[i]=xor_pixel_numar(C1[i],R[lungime_img*latime_img+i]);
        }

    Init=(Pixel *)malloc(lungime_img*latime_img*sizeof(Pixel));

    for(i=0;i<lungime_img*latime_img;i++)
    {
        Init[Inversa[i]]=C1[i];

    }


    unsigned char *head;
    header(caleImagineCriptata , &head);                 //aflam headerul

    deliniarizare( head , caleImagineInitiala , Init);   //transformam vectorul in imagine

    free(R);
    free(P);
    free(Inversa);
    free(Init);
    free(C);
    free(C1);
}

void chi_patrat(char *caleImagine)
{
    //testul chi patrat
    Pixel *v;
    liniarizare(caleImagine,&v);
    if(v==NULL)
    {
        printf("Nu s-a reusit alocarea spatiului pentru liniarizare in testul chi patrat");
        return ;
    }

    unsigned int *frecv;
    frecv=(unsigned int *)calloc(256,sizeof(unsigned int));

    unsigned int lungime_img,latime_img;
    dimensiune_imagine(caleImagine,&latime_img,&lungime_img);

    double f_bar,s_r=0,s_b=0,s_g=0;
    f_bar=(lungime_img*latime_img)/256.0;


    unsigned int i;
    //chi patrat pentru canalul r
    for(i=0;i<lungime_img*latime_img;i++)
        frecv[v[i].r]++;   //calculam frecventa fiecarei culori

    for(i=0;i<256;i++)
    {
        s_r+=(float)((frecv[i]-f_bar)*(frecv[i]-f_bar))/f_bar;
        frecv[i]=0;
    }

    for(i=0;i<lungime_img*latime_img;i++)
        frecv[v[i].g]++;

    for(i=0;i<256;i++)
    {
        s_g+=(float)((frecv[i]-f_bar)*(frecv[i]-f_bar))/f_bar;
        frecv[i]=0;   //initializam si cu 0 ca sa nu facem un nou for
    }

    for(i=0;i<lungime_img*latime_img;i++)
        frecv[v[i].b]++;

    for(i=0;i<256;i++)
    {
        s_b+=(float)((frecv[i]-f_bar)*(frecv[i]-f_bar))/f_bar;
        frecv[i]=0;
    }

    printf("Valorile testului chi-patrat pentru %s sunt: %.3f  %.3f  %.3f\n",caleImagine,s_r,s_g,s_b);

    free(v);
    free(frecv);
}



/// ************ TEMPLATE MATCHING ***********

void imagine2matrice(char *caleImagine,Pixel ***a,unsigned int *nr_linii,unsigned int *nr_coloane)
{
    //transforma o imagine intr-o matrice, sarind peste header
    FILE *f;
    f=fopen(caleImagine,"rb");

    if(f==NULL)
    {
        printf("Eroare deschidere fisier");
        return ;
    }

    unsigned int padding;
    int i,j;
    Pixel x;
    dimensiune_imagine(caleImagine,nr_coloane,nr_linii);

    if((*nr_coloane) % 4 != 0)
        padding = 4 - (3 * (*nr_coloane)) % 4;
    else
        padding = 0;

    (*a)=(Pixel **)malloc((*nr_linii)*sizeof(Pixel *));

    if((*a)==NULL)
    {
        printf("Nu s-a reusit alocarea pentru matrice");
        return ;
    }

    fseek(f,54,0);

    for(i=(*nr_linii)-1; i>=0;i--)
    {
        //cum citirea imaginii incepe cu coltul din stanga jos, asa ca parcurgem matricea invers
        (*a)[i]=(Pixel *)malloc((*nr_coloane)*sizeof(Pixel));
        if((*a)[i]==NULL)
        {
              printf("Nu s-a reusit alocarea pentru liniile matricei");
              return ;
        }

        for(j=0;j<(*nr_coloane);j++)
        {
            fread(&x,sizeof(Pixel),1,f);
            (*a)[i][j].b=x.r;
            (*a)[i][j].g=x.g;
            (*a)[i][j].r=x.b;
        }
        fseek(f,padding,1);     //sarim peste padding
    }

    fclose(f);
}

void matrice2imagine(char *caleImagineInitiala,char *caleImagineFinala,Pixel **a,unsigned int nr_linii,unsigned int nr_coloane)
{
    //transforma o matrice intr-o imagine
    FILE *fin,*fout;
    fin=fopen(caleImagineInitiala,"rb");

    if(fin==NULL)
    {
        printf("Eroare deschidere fisier la transformarea unei matrice intr-o imagine");
        return ;
    }

    fout=fopen(caleImagineFinala,"wb");

    if(fout==NULL)
    {
        printf("Eroare deschidere fisier la transformarea unei matrice intr-o imagine");
        return ;
    }

    unsigned int padding;
    int i,j;
    unsigned char elem,pad=0;

    if(nr_coloane % 4 != 0)
        padding = 4 - (3 * nr_coloane) % 4;
    else
        padding = 0;


    for(i=0;i<54;i++)
    {
        //copiem headerul
        fread(&elem,1,1,fin);
        fwrite(&elem,1,1,fout);
    }
     for(i=nr_linii-1; i>=0;i--)
    {
        for(j=0;j<nr_coloane;j++)
        {
            fwrite(&a[i][j].b,1,1,fout);
            fwrite(&a[i][j].g,1,1,fout);
            fwrite(&a[i][j].r,1,1,fout);

        }
        for(j=0;j<padding;j++)
           fwrite(&pad,1,1,fout);   //scriem si paddingul
    }

    fclose(fin);
    fclose(fout);
}

void grayscale_image(char* nume_fisier_sursa,char* nume_fisier_destinatie)
{
   FILE *fin, *fout;
   unsigned int latime_img, inaltime_img;
   unsigned char pRGB[3], aux;

   //printf("nume_fisier_sursa = %s \n",nume_fisier_sursa);

   fin = fopen(nume_fisier_sursa, "rb");
   if(fin == NULL)
   	{
   		printf("Nu am gasit imaginea sursa din care citesc");
   		return;
   	}

   fout = fopen(nume_fisier_destinatie, "wb+");

   /*fseek(fin, 2, SEEK_SET);
   fread(&dim_img, sizeof(unsigned int), 1, fin);
   printf("Dimensiunea imaginii in octeti: %u\n", dim_img);*/

   fseek(fin, 18, SEEK_SET);
   fread(&latime_img, sizeof(unsigned int), 1, fin);
   fread(&inaltime_img, sizeof(unsigned int), 1, fin);
   //printf("Dimensiunea imaginii in pixeli a imaginii ce urmeaza sa fie transformata in grayscale e (latime x inaltime): %u x %u\n",latime_img, inaltime_img);

   //copiaza octet cu octet imaginea initiala in cea noua
	fseek(fin,0,SEEK_SET);
	unsigned char c;
	while(fread(&c,1,1,fin)==1)
	{
		fwrite(&c,1,1,fout);
		fflush(fout);
	}
	fclose(fin);

	//calculam padding-ul pentru o linie
	int padding;
    if(latime_img % 4 != 0)
        padding = 4 - (3 * latime_img) % 4;
    else
        padding = 0;

	fseek(fout, 54, SEEK_SET);
	int i,j;
	for(i = 0; i < inaltime_img; i++)
	{
		for(j = 0; j < latime_img; j++)
		{
			//citesc culorile pixelului
			fread(pRGB, 3, 1, fout);
			//fac conversia in pixel gri
			aux = 0.299*pRGB[2] + 0.587*pRGB[1] + 0.114*pRGB[0];
			pRGB[0] = pRGB[1] = pRGB[2] = aux;
        	fseek(fout, -3, SEEK_CUR);
        	fwrite(pRGB, 3, 1, fout);
        	fflush(fout);
		}
		fseek(fout,padding,SEEK_CUR);
	}
	fclose(fout);
}

float medie_intensitati_s(Pixel **s,int nr_linii,int nr_coloane)
{
    //calculeaza media intensitatilor culorilor unui sablon
    float s_bar=0;
    int i,j;
    for(i=0;i<nr_linii;i++)
        for(j=0;j<nr_coloane;j++)
           s_bar+=s[i][j].r;

    s_bar=s_bar/(float)(nr_linii*nr_coloane);
    return s_bar;
}

float deviatia_standard_s(Pixel **s,int nr_linii,int nr_coloane,float s_bar)
{
    //s_bar reprezinta media intensitatilor sablonului s
    float deviatie=0;
    int i,j;
    for(i=0 ; i<nr_linii ; i++)
    {
        for(j=0 ; j<nr_coloane ; j++)
            deviatie+=((s[i][j].r-s_bar)*(s[i][j].r-s_bar));
    }

    deviatie=deviatie/(float)(nr_linii*nr_coloane-1);

    deviatie=sqrt(deviatie);

    return deviatie;
}


float medie_intensitati_i(Pixel **img,unsigned int nr_linii,unsigned int nr_coloane,int x,int y)
{
    //calculam media intensitatii imaginii i pe o portiune de 11*15 pornind de la pozitia x y

    float fi_bar=0;
    int i,j;
    for(i=x;i<x+nr_linii;i++)
    {
        for(j=y;j<y+nr_coloane;j++)
            fi_bar+=img[i][j].r;
    }
    fi_bar=fi_bar/(float)(nr_linii*nr_coloane);

    return fi_bar;
}


float deviatia_standard_fi(Pixel **img,unsigned int nr_linii,unsigned int nr_coloane,int x,int y,float fi_bar)
{
    float deviatie=0;
    int i,j;
    for(i=x;i<x+nr_linii;i++)
    {
        for(j=y;j<y+nr_coloane;j++)
            deviatie+=((img[i][j].r-fi_bar)*(img[i][j].r-fi_bar));

    }
    deviatie=deviatie/(float)(nr_linii*nr_coloane-1);

    deviatie=sqrt(deviatie);

    return deviatie;
}

float corelatie(Pixel **s,int lungime_s,int latime_s,Pixel **img,float deviatie_fi,float deviatie_s,float fi_bar,float s_bar,int x,int y)
{
    //lungime_s si latime_s sunt dimensiunile sablonului. Pixel **s e matricea ce contine un sablon
    unsigned int i,j;
    float corr=0;

    for(i=x;i<x+lungime_s;i++)
    {
        for(j=y;j<y+latime_s;j++)
        {
            corr+=(img[i][j].r-fi_bar)*(s[i-x][j-y].r-s_bar);
        }
    }

    float dev=deviatie_s*deviatie_fi;
    corr=(float)corr/(float)(lungime_s*latime_s);
    corr=(float)corr/dev;

    return corr;
}



void template_matching_pentru_un_sablon(char *caleImagine,char *caleSablon,float prag,Identificare **d,int *nr)
{


    float deviatie_fi,deviatie_s,fi_bar, s_bar;

    Pixel **img,**s;
    unsigned int lungime_img,latime_img,lungime_s,latime_s;

    imagine2matrice(caleImagine,&img,&lungime_img,&latime_img);
    if(img==NULL)
    {
        printf("Eroare la alocarea spatiului pentru matricea imaginii test.bmp in template_matching_pentru_un_sablon");
        return ;
    }
    imagine2matrice(caleSablon,&s,&lungime_s,&latime_s);
    if(s==NULL)
    {
        printf("Eroare la alocarea spatiului pentru matricea sablonului in template_matching_pentru_un_sablon");
        return ;
    }

    s_bar=medie_intensitati_s(s,lungime_s,latime_s);
    deviatie_s=deviatia_standard_s(s,lungime_s,latime_s,s_bar);//calculam media intensitatilor si deviatia standard a sablonului s transmis ca parametru


    int i,j;
    for(i=0;i<=lungime_img-lungime_s;i++)
    {
        for(j=0;j<=latime_img-latime_s;j++)
            {
               fi_bar=medie_intensitati_i(img,lungime_s,latime_s,i,j);  //calculam media intensitatilor si deviatia standard a ferestrei ce incepe in punctul i j(colt stanga sus)
               deviatie_fi=deviatia_standard_fi(img,lungime_s,latime_s,i,j,fi_bar);
               float corr=corelatie(s,lungime_s,latime_s,img,deviatie_fi,deviatie_s,fi_bar,s_bar,i,j);
               //   printf("lin = %d   col = %d  f_med = %f  s_med = %f  dev_sablon = %f  dev_matrice = %f corr = %f \n",i,j,fi_bar,s_bar,deviatie_s,deviatie_fi,corr);
               if(corr>=prag)
                {
                    //daca corelatia ferestrei depaseste pragul, adaugam datele despre ea in vectorul d
                    Identificare *aux;
                    (*nr)++;
                    aux=(Identificare *)realloc((*d),(*nr)*sizeof(Identificare));

                    if(aux==NULL)
                    {
                        printf("Eroare realocare");
                        return ;
                    }

                    (*d)=aux;
                    (*d)[(*nr)-1].cifra=caleSablon[1]-'0';
                    (*d)[(*nr)-1].corelatie=corr;
                    (*d)[(*nr)-1].lin_st_sus=i;
                    (*d)[(*nr)-1].col_st_sus=j;
                    (*d)[(*nr)-1].lin_dr_jos=i+lungime_s-1;
                    (*d)[(*nr)-1].col_dr_jos=j+latime_s-1;

                }

            }
    }
    //dezalocam memoria
    for(i=0;i<lungime_s;i++)
        free(s[i]);
    free(s);
    for(i=0;i<lungime_img;i++)
        free(img[i]);
    free(img);
}

void contur(Pixel **img,unsigned int lungime_s,unsigned int latime_s,int x,int y,Pixel c)//x si y reprezinta coltul din stanga sus. de asemenea se stie lungimea derestrei 15 si latimea 11
{
  //functie care realizeaza conturul unei feresre
  int i;
  for(i=y;i<y+latime_s;i++)
      img[x][i]=c;   //linia paralela cu axa ox de sus

  for(i=x+1;i<x+lungime_s;i++)
    {
        img[i][y]=c;
        img[i][y+latime_s-1]=c;   //liniile laterale paralele cu o
    }

   for(i=y;i<y+latime_s;i++)
     img[x+lungime_s-1][i]=c;   //linia paralela cu axa ox de jos
}

int cmpDescrescator_dupa_corelatie(const void *a,const void *b)
{
    //functie de comparare
    Identificare va=*(Identificare *)a;
    Identificare vb=*(Identificare *)b;

    if(va.corelatie>vb.corelatie)return -1;
    else if(va.corelatie<vb.corelatie)return 1;
    return 0;
}

void sortare_detectii(Identificare *d,int nr_detectii)
{
    //functie ce sorteaza descrescator dupa corelatie vectorul d cu nr_detectii elemente
    qsort(d,nr_detectii,sizeof(Identificare),cmpDescrescator_dupa_corelatie);
}


int minim(int a,int b)
{
    if(a<b)return a;
    return b;
}


int maxim(int a,int b)
{
    if(a>b)return a;
    return b;
}


float suprapunere(Identificare a,Identificare b)
{
    int x1=maxim(a.lin_st_sus,b.lin_st_sus);//coltul stanga sus al intersectiei
    int y1=maxim(a.col_st_sus,b.col_st_sus);

    int x2=minim(a.lin_dr_jos,b.lin_dr_jos); //coltul din dreapta jos al intersectiei
    int y2=minim(a.col_dr_jos,b.col_dr_jos);

    float arie_intersectie,suprapus;
    if(x1>x2 || y1>y2)arie_intersectie=0;     //nu se intersecteaza
    else arie_intersectie=(x2-x1+1)*(y2-y1+1);

    if(arie_intersectie!=0)
    {
        // aria_sablon=165;
        suprapus=(float)arie_intersectie/(165+165-arie_intersectie);
        return suprapus;
    }
    return -1;  //nu se suprapun

}

void eliminare(Identificare *v,int n,int poz)
{
    //functie care elimina un element
    int i;
    for(i=poz+1;i<n;i++)
        v[i-1]=v[i];
}

void suprimarea_non_maximelor(Identificare *d,int *nr_detectii)
{
    //elimina ferestrele a caror corelatie se suprapun
    int i,j;
    for(i=0;i<(*nr_detectii)-1;i++)
    {
        for(j=i+1;j<(*nr_detectii);j++)
              {
                  float suprapus=suprapunere(d[i],d[j]);
                  if(suprapus>0.2)
                  {
                      eliminare(d,(*nr_detectii),j);
                      j--;
                      (*nr_detectii)--;
                  }
              }
    }
}

void colorare_imagine(Pixel **initial,int lungime_s,int latime_s,Identificare *detectie,int nr_detectii)
{
    int i;

    sortare_detectii(detectie,nr_detectii);
    suprimarea_non_maximelor(detectie,&nr_detectii);


   for(i=0;i<nr_detectii;i++)
    {
        Pixel c;
        int cif=detectie[i].cifra;   //alegem culoarea in functie de cifra
        switch(cif)
        {
        case 0:c.r=255;   c.g=0;  c.b=0;
            break;
        case 1:c.r=255;   c.g=255;  c.b=0;
            break;
        case 2:c.r=0;   c.g=255;  c.b=0;
            break;
        case 3:c.r=0;   c.g=255;  c.b=255;
            break;
        case 4:c.r=255;   c.g=0;  c.b=255;
            break;
        case 5:c.r=0;   c.g=0;  c.b=255;
            break;
        case 6:c.r=192;   c.g=192;  c.b=192;
            break;
        case 7:c.r=255;   c.g=140;  c.b=0;
            break;
        case 8:c.r=128;   c.g=0;  c.b=128;
            break;
        case 9:c.r=128;   c.g=0;  c.b=0;
            break;
        }

        contur(initial,lungime_s,latime_s,detectie[i].lin_st_sus,detectie[i].col_st_sus,c);
    }
}

void construire_denumire(int cif,char **denumireGrayscale)
{
    //contruim denumirea sabloanelor in grayscale in functie de cifra pe care o reprezinta.
    //are formatul "gcifra.bmp"
    (*denumireGrayscale)=NULL;
    (*denumireGrayscale)=(char *)malloc(8*sizeof(char));
    if((*denumireGrayscale)==NULL)
    {
        printf("Eroare alocare spatiu");
        return ;
    }
    char cifra[2];
    cifra[0]=cif+'0';
    cifra[1]='\0';
    strcpy((*denumireGrayscale),"g");
    strcat((*denumireGrayscale),cifra);
    strcat((*denumireGrayscale),".bmp");
    (*denumireGrayscale)[6]='\0';


}

void template_matching(char *caleImagine,float prag)
{
    Identificare *detectie;
    detectie=NULL;
    int nr_detectii=0;


    grayscale_image(caleImagine,"test_grayscale.bmp");

    FILE *fin;
    fin=fopen("caleSabloane.fin","r");

    if(fin==NULL)
    {
        printf("Eroare deschidere imagine la template matching");
        return ;
    }
    int i;

    char *denum_grayscale,*nume_sablon;
    nume_sablon=(char *)malloc(12*sizeof(char));
    if(nume_sablon==NULL)
    {
        printf("Eroare alocare spatiu pentru numele sabloanelor");
        return ;
    }
    for(i=0;i<=9;i++)
    {

        fscanf(fin,"%s",nume_sablon);
        construire_denumire(i,&denum_grayscale);
        grayscale_image(nume_sablon,denum_grayscale);
        template_matching_pentru_un_sablon("test_grayscale.bmp",denum_grayscale,prag,&detectie,&nr_detectii);
    }

    fclose(fin);

    Pixel **matrice_test;
    unsigned int nr_linii_test,nr_coloane_test;
    imagine2matrice(caleImagine,&matrice_test,&nr_linii_test,&nr_coloane_test);
    //coloram chenarele in culorile corespunzatoare
    colorare_imagine(matrice_test,15,11,detectie,nr_detectii); //15 si 11 sunt dimensiunile unui sablon
    matrice2imagine(caleImagine,"rezultat_template_matching.bmp",matrice_test,nr_linii_test,nr_coloane_test); //transformam iar in imagine
    free(matrice_test);
    free(detectie);
}


int main()
{
    char *nume_imagine,*nume_imagine_criptata1,*nume_imagine_criptata2,*nume_imagine_decriptata,*nume_fisier_cheie_secreta1,*nume_fisier_cheie_secreta2,*nume_template_matching;
    nume_imagine=(char *)malloc(40*sizeof(char));
    if(nume_imagine==NULL)
    {
        printf("Eroare alocare");
        return 0;
    }
    nume_imagine_criptata1=(char *)malloc(40*sizeof(char));
    if(nume_imagine_criptata1==NULL)
    {
        printf("Eroare alocare");
        return 0;
    }
    nume_imagine_criptata2=(char *)malloc(40*sizeof(char));
    if(nume_imagine_criptata2==NULL)
    {
        printf("Eroare alocare");
        return 0;
    }
    nume_imagine_decriptata=(char *)malloc(40*sizeof(char));
    if(nume_imagine_decriptata==NULL)
    {
        printf("Eroare alocare");
        return 0;
    }
    nume_fisier_cheie_secreta1=(char *)malloc(40*sizeof(char));
    if(nume_fisier_cheie_secreta1==NULL)
    {
        printf("Eroare alocare");
        return 0;
    }
    nume_fisier_cheie_secreta2=(char *)malloc(40*sizeof(char));
    if(nume_fisier_cheie_secreta2==NULL)
    {
        printf("Eroare alocare");
        return 0;
    }
    nume_template_matching=(char *)malloc(40*sizeof(char));
    if(nume_template_matching==NULL)
    {
        printf("Eroare alocare");
        return 0;
    }
    FILE *f1,*f2,*f3;
    f1=fopen("denumire_imagini_criptare.fin","r");
    if(f1==NULL)
    {
        printf("Eroare deschidere fisier");
        return 0;
    }
    f2=fopen("denumire_imagini_decriptare.fin","r");
    if(f2==NULL)
    {
        printf("Eroare deschidere fisier");
        return 0;
    }
    f3=fopen("denumire_template_matching.fin","r");
    if(f3==NULL)
    {
        printf("Eroare deschidere fisier");
        return 0;
    }
    fscanf(f1,"%s%s%s",nume_imagine,nume_imagine_criptata1,nume_fisier_cheie_secreta1);
    fscanf(f2,"%s%s%s",nume_imagine_criptata2,nume_imagine_decriptata,nume_fisier_cheie_secreta2);
    fscanf(f3,"%s",nume_template_matching);
    criptare(nume_imagine,nume_imagine_criptata1,nume_fisier_cheie_secreta1);
    decriptare(nume_imagine_criptata2,nume_imagine_decriptata,nume_fisier_cheie_secreta2);
    chi_patrat(nume_imagine);
    chi_patrat(nume_imagine_criptata1);
    template_matching(nume_template_matching,0.5);

    fclose(f1);
    fclose(f2);
    fclose(f3);

    free(nume_imagine);
    free(nume_imagine_criptata1);
    free(nume_imagine_criptata2);
    free(nume_imagine_decriptata);
    free(nume_fisier_cheie_secreta1);
    free(nume_fisier_cheie_secreta2);
    return 0;
}
