// Grupo L5: Francisco Iván San Segundo Álvarez 71969196B y Rodrigo Martín Díaz 71178304C
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <ctype.h>


/* MENSAJES DE ERROR */

// Definimos un mensaje de error para cuando el numero de argumentos no es correcto
#define ERROR_NARGUMENTOS "ERROR: El numero de argumentos no es el correcto, deben de ser 6 arguemntos \n"
// Definimos un mensaje de error para cuando el path introducido es nulo
#define ERROR_PATHNULO "ERROR: El path es nulo \n"
// Definimos un mensaje de error para cuando el fichero de apertura no se puede abrir
#define ERROR_FICHEROAPERTURA "ERROR: El fichero no se puede abrir correctamente o no existe \n"
// Definimos un mensaje de error para cuando el tamaño del buffer se excede del tamaño permitido
#define ERROR_TAMAÑOBUFFER "ERROR: El tamaño del buffer no es valido, tiene que estar entre 1 y 5000 \n"
// Definimos un mensjae de error para cuando la cantidad de proveedores no se encuentra dentro del rango permitido
#define ERROR_CANTIDADPROVEEDORES "ERROR: La cantidad de proveedores no es valida, tiene que estar entre 1 y 7 \n"
// Definimos un mensaje de error para cuando la cantidad de consumidores no se encuentra dentro del rango permitido
#define ERROR_CANTIDADCONSUMIDORES "ERROR: La cantidad de consumidores no es valida, tiene que estar entre 1 y 1000 \n"
// Definimos un mensaje de error para cuando la creacion del hilo no se ejecute correctamente
#define ERROR_CREACIONHILO "ERROR: La creacion del hilo no se ha ejecutado correctamente \n"
// Definimos un mensaje de error para cuando algun argumento no es correcto
#define ERROR_PARAMETROSINVALIDOS "ERROR: Alguno de los argumentos con respecto al tamaño del buffer, proveedores o consumidores no es correcto \n"
// Definimos un mensaje de error para cuando la reserva dinamica de memoria no se realiza correctamente
#define ERROR_FALLOMEMORIADINAMICA "ERROR: No se ha reservado correctamente la memoria de dinamica \n"

// Estructura que va a tener el buffer circular
typedef struct bufferst{
    // Campo que hace referencia al producto
    char producto;
    // Campo que hace referencia al identificador del productor
    int idproductor;
} BUFFER;

// Estrcutura que me va a servir para pasar los parametros a los hilos proveedores
typedef struct proveedorst{
    // Campo para el vector de los fichero de lectura de cada proveedor 
    char *f;
    // Campo del identificador del consumidor
    int identificador;
    // Campo del fichero de salida
    FILE *fp;
    // Campo para el buffer donde van a dejar los productos
    BUFFER * buffer;

}proveedor;

// Estructura que me va a servir para pasar los parametros a los hilos consumidores
typedef struct consumidoresst{
    // Campo del identificador del consumidor
    int identificador;
    // Campo para el buffer de donde van a consumir los productos
    BUFFER * buffer ;
    // Campo para guardar el numero de productos de cada proveedor que se consumen
    int  *productosProveedores;
    // Campo para guardar el numero de productos de cada tipo que se consumen
    char *productoTipo;

}consumidor;

// Estructura que me va a servir para pasar los parametros al hilo facturador
typedef struct facturadorst{
    // Campo para el fichero de salida
    FILE *fp;
}facturador;

// Estructura de cada nodo que va a componer la lista enlazada
typedef struct nodost{
    struct nodost *siguiente;
    int indentificador;
    int produstostipo[10];
    int proveedores[7];
    int cantidadproductos;
} Nodo ;
// Declaracion de la funcion del facturador
void *ffacturador(void *arg);
// Declaracion de la funcion de los proveedores
void *fproveedor(void *arg);
// Declaracion de la funcion de los consumidores
void *fconsumidor(void *arg);

// Definimos una funcion para comprobar si la cadena que pasamos esta compuesta unicamente por numeros
int esNum(char * cad){
    // Recorro la longitud de la cadena comprobando que cada caracter de ella sea distinto de \n y que sea digito
    for(int i=0;i<strlen(cad);i++){
        if(!isdigit(cad[i]) && cad[i]!='\n'){
            return 0;
        }
        else{
            return 1;
        }
    }
}

/*  DEFINICIONES DE SEMAFOROS Y DE VARIABLES GLOBALES   */

// Declaramos el semaforo que me va a servir para la exclusion mutua de los proveedores
sem_t mutex_sigllenar;
// Declaramos el semaforo que me va a servir para la exclusion mutua de los consumidores
sem_t mutex_sigvaciar;
// Declaramos el semaforo que me va a servir para los huecos que haya en el buffer
sem_t hayhueco;
// Declaramos el semaforo que me va a sevrir para el numero de productos que haya en el buffer
sem_t haydato;
// Declaramos el semaforo para empezar a meter los productos facturados
sem_t hayConsumicion;
// Declaro el semaforo que me va a servir para la exclusion mutua del fichero de salida
sem_t mutex_ficherosalida;
// Declaramos el semaforo que me va a servir para la exclusion mutua de la variable proveedoresrestantes
sem_t mutex_proveedoresrestantes;
// Declaramos el semaforo que me va a servir para la exclusion mutua de la lista enlazada de consumidores
sem_t mutex_listaenlazada;
// Declaramos una variable que va a ser comun entre todos los productores y que me sirve para que sepan cual es la siguiente posicion del buffer a llenar
int sigllenar=0;
// Declaramos una variable que va a ser comun entre todos los consumidores y que me sirve para que sepan cual es la siguiente posicion del buffer a extraer el producto
int sigextraer=0;
// Declaramos una variable global para saber el numero de proveedores que han terminado
int proveedoresrestantes;
// Declaramos una variabl global para guardar el tamaño del buffer
int TAMBUFFER;
// Declaramos la variable que me va a guardar el numero de consumidores
int numeroconsumidores;
// Declaramos la variable que me va a guardar el numero de proveedores
int numeroproveedores;
// Declaramos un puntero que me va a servir para saber donde se encuentra la lista enlazada
Nodo *listaEnlazada=NULL;
Nodo *fin=NULL;

void main(int argc,char * argv[]){
    // Declaramos una cadena de caracteres para guardar el path al fichero de cada proveedor
    char f[7][255];
    // Creamos la variable que me va a servir de contador del numero de productos consumidos
    int productosconsumidos=0;
    // Definimos una variable que me va a servir para guardar el producto que hemos consumido del buffer
    char productoconsumido;
    // Definimos una variable en la que vamos a guardar el indentificador del proveedor cuyo producto hemos consumido
    int proveedorproducto;
    // Creamos la variable que me va a guardar el identificador de hilo
    int hilo;
   // Creamos la variable de tipo FILE para el fichero de salida donde vamos a guardar los datos
    FILE *fpdestino;
    // Creamos la variable de tipo FILE para el fichero que va a leer cada proveedor
    FILE *fpproveedor;
    // Creamos el buffer como una variable del tipo BUFFER que hemos creado
    BUFFER *buffercircular=NULL;
    //Declaramos la estructura de proveedores
    proveedor *hiloproveedores=NULL;
    //Declaramos la estructura de consumidores
    consumidor *hiloconsumidores=NULL; 
    //Inicializamos la estructura de facturador
    facturador hilofacturador;
    // Declaramos un vector de variables de tipo pthread_t para guardar el identificador de cada uno de los hilos
    pthread_t *tidproveedores=NULL;
    pthread_t *tidconsumidores=NULL;
    pthread_t *tidfacturador=NULL;

    // Comprobamos que el número de argumentos sea excatamente 6, en caso de que esto no se cumpla lanzamos un mensaje de error
    if(argc!=6){
        // Mostramos el mensaje por pantalla
        printf("%s",ERROR_NARGUMENTOS);
        // Retornamos con un valor -1 para indicar que se ha producido un error
        exit(-1);
    }
    //Comprbamos que el fichero que nos hayan pasado por parametro se pueda abrir correctamente, en caso contrario lanzamos un error
    if ((fpdestino=fopen(argv[2],"w"))==NULL){
        // Mostramos el mensaje por pantalla
        printf("%s",ERROR_FICHEROAPERTURA);
        // Retornamos con un valor -1 para indicar que se ha producido un error
        exit(-1);
    }
    if(esNum(argv[3])==0 ||esNum(argv[4])==0 || esNum(argv[5])==0 ){
        printf("%s",ERROR_PARAMETROSINVALIDOS);
        exit(-1);
    }
    // Comprobamos que el tamaño de buffer que nos hayan pasado como parametro este dentro de los rangos permitidos
    if(atoi(argv[3])<1 || atoi(argv[3])>5000){
        // Mostramos el mensaje por pantalla
        printf("%s",ERROR_TAMAÑOBUFFER);
        // Retornamos con un valor -1 para indicar que se ha producido un error
        exit(-1);
    }
    TAMBUFFER=atoi(argv[3]);
    // Comprobamos que el numero de productores que nos hayan pasado como parametro esten detro del rango permitido
    if(atoi(argv[4])<1 || atoi(argv[4])>7){
        // Mostramos el mensaje por pantalla
        printf("%s",ERROR_CANTIDADPROVEEDORES);
        // Retornamos con un valor -1 para indicar que se ha producido un error
        exit(-1);
    }
    //Guardamos el numero de proveedores
    numeroproveedores=atoi(argv[4]);
    //Guardamos el numero de proveedores restantes
    proveedoresrestantes=numeroproveedores;
    // Comprobamos que el numero de consumidores que nos hayan pasado como parametros este dentro del ranfo permitido
    if(atoi(argv[5])<1 || atoi(argv[5])> 1000){
        // Mostramos el mensaje por pantalla
        printf("%s",ERROR_CANTIDADCONSUMIDORES);
        // Retornamos con un valor -1 para indicar que se ha producido un error
        exit(-1);
    }
    // Guardo el numero de consumidores 
    numeroconsumidores=atoi(argv[5]);
    // Guardo en f la ruta del fichero de cada proveedor
    for(int i=0;i<numeroproveedores;i++){
        sprintf(f[i],"%s/proveedor%d.dat",argv[1],i);
    }
    FILE *prueba;
    // Compruebo que cada fichero del proveedor se puede abrir correctamente
    for(int i=0;i<numeroproveedores;i++){
        if((prueba=fopen(f[i],"r"))==NULL){
            printf("%s",ERROR_FICHEROAPERTURA);
            exit(-1);
        }
        // borrar cuando hagas el programa
        fclose(prueba);
    }
    //Compruebo que la reserva de memoria se ha realizado correctamente y si no lanzamos un error
    if((hiloproveedores=(proveedor *) malloc(numeroproveedores*sizeof(proveedor)))==NULL){
        printf("%s",ERROR_FALLOMEMORIADINAMICA);
        exit(-1);
    }
    //Compruebo que la reserva de memoria se ha realizado correctamente y si no lanzamos un error
    if((hiloconsumidores=(consumidor *) malloc(numeroconsumidores*sizeof(consumidor)))==NULL){
        printf("%s",ERROR_FALLOMEMORIADINAMICA);
        exit(-1);
    }
    //Compruebo que la reserva de memoria se ha realizado correctamente y si no lanzamos un error
    if((buffercircular =(BUFFER *) malloc(sizeof(BUFFER)*TAMBUFFER))==NULL){
        printf("%s", ERROR_FALLOMEMORIADINAMICA);
        exit(-1);
    }
    //Ponemos el campo de producto como null y el de productor como 0 ya que todavia no se ha escrito nada en el
    for(int i=0;i<TAMBUFFER;i++){
        buffercircular[i].producto='\0';
        buffercircular[i].idproductor=0;
    }
    //Inicializamos el valor del semaforo con el valor 1 ya que es de exclusion mutua
    sem_init(&mutex_sigvaciar,0,1);
    //Inicializamos el valor del semaforo con el valor 1 ya que es de exclusion mutua
    sem_init(&mutex_sigllenar,0,1);
    //Inicializamos el valor del semaforo con el valor TAMBUFFER ya que al inicio hay tantos huecos como tamaño del buffer
    sem_init(&hayhueco,0,TAMBUFFER);
    //Inicializamos el valor del semaforo con el valor 0 ya que al inicio no hay productos
    sem_init(&haydato,0,0);
    //Inicializamos el valor del semaforo con el valor 1 ya que es de exclusion mutua
    sem_init(&mutex_ficherosalida,0,1);
    //Inicializamos el valor del semaforo con el valor 1 ya que es de exclusion mutua
    sem_init(&mutex_proveedoresrestantes,0,1);
    //Inicializamos el valor del semaforo con el valor 0 ya que el facturador no puede entrar hasta que un consumidor termine
    sem_init(&hayConsumicion,0,0);
    sem_init(&mutex_listaenlazada,0,1);
    //Compruebo que la reserva de memoria se ha realizado correctamente y si no, lanazamos un error
    if((tidproveedores=(pthread_t *) malloc((numeroproveedores)*sizeof(pthread_t)))==NULL){
        printf("%s", ERROR_FALLOMEMORIADINAMICA);
        exit(-1);
    }
    if((tidconsumidores=(pthread_t *) malloc((numeroconsumidores)*sizeof(pthread_t)))==NULL){
        printf("%s", ERROR_FALLOMEMORIADINAMICA);
        exit(-1);
    }
    if((tidfacturador=(pthread_t *) malloc((1)*sizeof(pthread_t)))==NULL){
        printf("%s", ERROR_FALLOMEMORIADINAMICA);
        exit(-1);
    }
    // Modifico el campo de cada posicion de proveedores para pasarlos los argumentos a los hilos, creando estos y comprobando que se crean correctamente
    for(int i=0;i<numeroproveedores;i++){
        hiloproveedores[i].f=f[i];
        hiloproveedores[i].identificador=i;
        hiloproveedores[i].fp=fpdestino;
        hiloproveedores[i].buffer=buffercircular;
        if(pthread_create(&tidproveedores[i],NULL,fproveedor,(void *) &hiloproveedores[i])!=0){
            printf("%s",ERROR_CREACIONHILO);
            exit(-1);
        }

    }
    // Modifico el campo de cada posicion de consumidores para pasarlos los argumentos a los hilos, creando estos y comprobando que se crean correctamente
    for(int i=0;i<numeroconsumidores;i++){
        hiloconsumidores[i].identificador=i;
        hiloconsumidores[i].buffer=buffercircular;
        if(pthread_create(&tidconsumidores[i],NULL,fconsumidor,(void *) &hiloconsumidores[i])!=0){
            printf("%s",ERROR_CREACIONHILO);
            exit(-1);
        }

    }
    // Modifico el campo de fcaturador para pasarle los argumentos al hilo,creando este y comprobando que se crea correctamente
    hilofacturador.fp=fpdestino;
    if(pthread_create(&tidfacturador[0],NULL,ffacturador,(void *) &hilofacturador)!=0){
        printf("%s",ERROR_CREACIONHILO);
        exit(-1);
    }
    // Espero a la terminacion de cada hilo proveedor
    for(int i=0;i<numeroproveedores;i++){
        pthread_join(tidproveedores[i],NULL);
    }
    // Espero a la terminacion de cada hilo consumidor
    for(int i=0;i<numeroconsumidores;i++){
        pthread_join(tidconsumidores[i],NULL);
    }
    // Espero a la terminacion del hilo facturador
    pthread_join(tidfacturador[0],NULL);
    fclose(fpdestino);
    free(tidproveedores);
    free(tidconsumidores);
    free(tidfacturador);
    // borrar cuando hagas el programa
    free(hiloproveedores);
    // borrar cuando hagas el programa
    free(hiloconsumidores);
    printf("TERMINADO \n");
    sem_destroy(&mutex_sigvaciar);
    sem_destroy(&mutex_sigllenar);
    sem_destroy(&hayhueco);
    sem_destroy(&haydato);
    sem_destroy(&mutex_ficherosalida);
    sem_destroy(&mutex_proveedoresrestantes);
    sem_destroy(&mutex_listaenlazada);
}

// Definimos una funcion que va a ser la funcion de los proveedores, pasando como parametros el fichero de salida donde los proveedores tienen que escribir sus datos y el buffer donde se van a colocar los datos
void *fproveedor(void *arg){  
    // Creo una estructura de tipo proveedor
    proveedor *prov;
    // Guardo en ella los argumentos que le he pasado al hilo
    prov=(proveedor *) arg;
    // Definimos una variable que me va a servir para guardar cada producto que el proveedor lea de su fichero
    char productofichero;
    // Definimos una variable que me va a servir para guardar el identificador del hilo proveedor
    int nproveedor=prov->identificador;
    // Definimos una variable donde voy a guardar la siguiente posicion a llenar
    int i=0;
    // Definimos un vector donde vamos a guardar cuantas veces se ha leido cada caracter valido
    int nleidocaracter[10]; //0 es a,1 es b ....... 9 es j
    // Definimos una variable donde voy a guardar el caracter leido
    int caracterLeido;
    // Definimos una variable donde vamos a guardar el numero de caracteres invalidos que leemos
    int invalidos=0;
    // Definimos una variable para guardar el fichero de cada proveedor
    FILE *fpproveedor;
    // Definimos una variable de caracteres donde voy a guardar el fichero de cada proveedor
    char *f=prov->f;
    // Definimos una variable para guardar la ruta del fichero de salida donde hay que escribir
    FILE *fpdestino=prov->fp;
    // Definimos una variable para guardar el buffer donde vamos a escribir
    BUFFER *buffercircular =prov->buffer;
    //Creamos la variable de productos leidos , los cuales son validos
    int contadorValidos=0;
    // Creamos el contador de productos leidos
    int contadorLeidos=0;
    // Inicializo cada posicion a 0 ya que todavia no hemos leido ningun caracter
    for(int i=0;i<10;i++){
        nleidocaracter[i]=0;
    }
    // Comprobamos que el fichero de cada proveedor se puede abrir correctamente y en caso de que no, lanzamos un error
    if((fpproveedor=fopen(f,"r"))==NULL){
        printf("%s",ERROR_FICHEROAPERTURA);
        exit(-1);
    }
    // Leemos cada caracter del fichero
    productofichero=fgetc(fpproveedor);
    // Mientras que el caracter leido del fichero sea distinto de EOF seguimos en el bucle
    while(productofichero!=EOF){  
        contadorLeidos++;
        // Si el caracter leido del fichero esta en comprendido entre el caracter a y el caracter j, es que es un producto valido y por lo tanbto, lo guardamos en el buffer
        if(productofichero>='a' && productofichero<='j'){
            // Ejecutamos la funcion sem_wait para que mientras que no haya huecos el proceso pase a la lista de bolqueados y que se bloquee
            sem_wait(&hayhueco);
            // Ejecutamos la funcion sem_wait para verificar que solo un proveedor acceda a la posicion a llenar del buffer asegurandonos exclusion mutua
            sem_wait(&mutex_sigllenar);
            // Actualizamos la variable siguiente llenar con la siguiente posicion
            buffercircular[sigllenar].producto=productofichero;
            buffercircular[sigllenar].idproductor=nproveedor;
            sigllenar=(sigllenar+1)%TAMBUFFER;
            // Una vez que ya hemos conseguido la siguiente posicion a llenar , ejecutamos la funcion sem_post para indicar que ya hemos termiando las acciones sobre la sección critica permitiendo a otros hilos proveedores acceder a ella
            sem_post(&mutex_sigllenar);
            // Guardamos el la siguiente posicion a llenar del buffer el producto que hemos leido del fichero y de que proveedor e
            // Una vez que ya hemos escrito el producto en el buffer, ejecutamos la funcion sem_post para indicar que ya hemos termiando las acciones sobre la sección critica permitiendo a otros hilos proveedores acceder a ella
            sem_post(&haydato);
            // Incrementamos en 1 el valor de caracteres validos
            contadorValidos++;
            // Obtenemos que caracter hemos leido restando al caracter el valor a consiguiendo 0 si el caracter es a, 1 si es b y asi sucesivamente
            caracterLeido=productofichero-'a';
            // Aumentamos en 1 el caracter leido
            nleidocaracter[caracterLeido]++;
        }
        // Guardamos el siguiente caracter a procesar
        productofichero=fgetc(fpproveedor);
    }
    invalidos=contadorLeidos-contadorValidos;
    sem_wait(&mutex_proveedoresrestantes);
    proveedoresrestantes=proveedoresrestantes-1;
    if(proveedoresrestantes==0){
        for(int i=0;i<numeroconsumidores;i++){
            sem_post(&haydato);
        }
    }
    sem_post(&mutex_proveedoresrestantes);
    sem_wait(&mutex_ficherosalida);
    // Escribimos sobre el fichero de salida los datos correspondientes
    fprintf(fpdestino,"Proveedor %d: \n",nproveedor);
    fprintf(fpdestino,"Productos totales: %d. \n",contadorLeidos);
    fprintf(fpdestino,"Productos invalidos: %d. \n",invalidos);
    fprintf(fpdestino,"Productos validos: %d.De los cuales se han insertado:\n",contadorValidos);
    fprintf(fpdestino,"%d de tipo a \n",nleidocaracter[0]);
    fprintf(fpdestino,"%d de tipo b \n",nleidocaracter[1]);
    fprintf(fpdestino,"%d de tipo c \n",nleidocaracter[2]);
    fprintf(fpdestino,"%d de tipo d \n",nleidocaracter[3]);
    fprintf(fpdestino,"%d de tipo e \n",nleidocaracter[4]);
    fprintf(fpdestino,"%d de tipo f \n",nleidocaracter[5]);
    fprintf(fpdestino,"%d de tipo g \n",nleidocaracter[6]);
    fprintf(fpdestino,"%d de tipo h \n",nleidocaracter[7]);
    fprintf(fpdestino,"%d de tipo i \n",nleidocaracter[8]);
    fprintf(fpdestino,"%d de tipo j \n",nleidocaracter[9]);
    sem_post(&mutex_ficherosalida);
    // El hilo sale de su funcion ya que ha realizado todas sus tareas
    fclose(fpproveedor);
    printf("TERMINAN");
    pthread_exit(NULL);
}

// Definimos una funcion que va a realizar la funcion de los consumidores, pasando como parametros el buffer de donde se van a extraer los productos
void *fconsumidor(void *arg){
    // Creo una estructura de tipo consumidor
    consumidor *consu;
    // Guardo en ella los argumentos que le he pasado al hilo
    consu=(consumidor *) arg;
    char producto='0';
    int idenprovedor;
    int proveedoresrestantesconsumidor;
    // Definimos una variable en la que vamos a guardar el proveedor de cada producto que consumimos
    int *proveedorproducto;
    if((proveedorproducto=malloc(numeroproveedores*sizeof(int)))==NULL){
        printf("%s",ERROR_FALLOMEMORIADINAMICA);
        exit(-1);
    }
    // Definimos una variable en la que vamos a guardar el producto que consumimos
    int productoconsumido;
    // Definimos una variable para guardar las consumiciones de producto que hace cada consumidor
    int consumiciones=0;
    // Creamos una lista para guardar datos de consumiciones de tamaño el tamaño del buffer
    Nodo *consumidori=NULL;
    if((consumidori=malloc(1*sizeof(Nodo)))==NULL){
        printf("%s",ERROR_FALLOMEMORIADINAMICA);
        exit(-1);
    }
    consumidori->indentificador=consu->identificador;
    consumidori->siguiente=NULL;
    // Declaramos una variable para guardar el buffer de donde vamos a consumir
    BUFFER *buffercircular=consu->buffer;
    // Inicializamos cada posicion de la lista a 0
    for(int i=0;i<7;i++){
        consumidori->proveedores[i]=0;
    }
    for(int i=0;i<10;i++){
        consumidori->produstostipo[i]=0;
    }
    consumidori->cantidadproductos=0;
    // Creamos una variable para guardar la siguiente posicion a consumir
    int j=0;
    // Mientras que el numero de proveedores sea distinto de 0 continuamos en el bucle
    while(producto!='\0'){
        // En caso de que el numero de proveedores sea 0 salimos del bucle
        sem_wait(&mutex_proveedoresrestantes);
        proveedoresrestantesconsumidor=proveedoresrestantes;
        sem_post(&mutex_proveedoresrestantes);
        // Ejecutamos la funcion sem_wait sobre el semaforo nprodutos para que mientras que valga 0 lo cual significa que no hay ningun producto en el buffer el proceso consumidor pase a la lista de procesos bloqueados y se bloquee
        if(proveedoresrestantesconsumidor!=0){
            sem_wait(&haydato);
        }
        // Ejecutamos la funcion sem_wait sobre el semaforo mutexconsumidores para asegurarnos de que solo un consumidor accede sobre la posicion a extraer
        sem_wait(&mutex_sigvaciar);
        // Guardamos en j la siguiente posicion a extraer
        producto=buffercircular[sigextraer].producto;
        // Ejecutamos la funcion post para indicar que otro hilo puede acceder a la siguiente posicion a extraer
        if(producto!='\0'){
            buffercircular[sigextraer].producto='\0';
            idenprovedor=buffercircular[sigextraer].idproductor;
            sem_post(&hayhueco);
        }
        sigextraer=(sigextraer+1)%TAMBUFFER;
        sem_post(&mutex_sigvaciar);
        productoconsumido=(producto-'a');
        // Guardamos en la variable productoconsumido el caracter del producto que hemos consumido del buffer
        // Si el producto consumido es distinto de el \0
        if(producto!='\0'){
            // Ponemos esa posicion a null
            // Guardamos en esa posicion el producto consumido y el proveedor del que hemos consumido
            consumidori->proveedores[idenprovedor]++;
            consumidori->produstostipo[productoconsumido]++;
            consumidori->cantidadproductos++;
            // Hacemos post indicando que hay un hueco disponible
        }
    }
    sem_wait(&mutex_listaenlazada);
    if(listaEnlazada==NULL){
        listaEnlazada=consumidori;
        fin=consumidori;
    }
    else{
        fin->siguiente=consumidori;
        fin=consumidori;
    }
    sem_post(&mutex_listaenlazada);
    //Mandamos la señal de que ya hay algo que se ha consumido
    sem_post(&hayConsumicion);
    // El hilo termina ya que ha realiazdo todas sus tareas
    pthread_exit (NULL); 
}

// Definimos una funcion que va a realizar la funcion del facturador, pasando como parametros el fichero de salida donde va a tener que escribir los datos correspondientes
void *ffacturador(void *arg){
    // Creo una estructura de tipo facturador
    facturador *fact;
    // Guardo en ella los argumentos que le he pasado al hilo
    fact=(facturador *) arg;
    int *productoscadaproveedor;
    if((productoscadaproveedor=malloc(numeroproveedores*sizeof(int)))==NULL){
        printf("%s",ERROR_FALLOMEMORIADINAMICA);
        exit(-1);
    }
    for(int i=0;i<numeroproveedores;i++){
        productoscadaproveedor[i]=0;
    }
    // Definimos una variable para guardar el consumidor que mas productos ha consumido
    int consumidormax=0;
    // Definimos una variable para guardar el maximo de consumiciones
    int consumicionesmax=0;
    // Definimos una variable para guardar la suma de consumiciones
    int sumaconsumiciones=0;
    // Definimos una variable de tipo file para guardar el fichero de salida
    FILE *fpdestino = fact->fp;
    // Guardamos el numero de proveedores que hay 
    int numeroproveedores=numeroproveedores;
    // Definimos una variable para cada proveedor
    int i=0;
    // Mientras que el campo actual es distinto de null recorremos la lista enlazada
    Nodo *aux=NULL;
    for(int i=0;i<numeroconsumidores;i++){
        sem_wait(&hayConsumicion);
        sem_wait(&mutex_listaenlazada);
        aux=listaEnlazada;
        // Ponemos como nodo actual el siguiente
        listaEnlazada=listaEnlazada->siguiente;
        sem_post(&mutex_listaenlazada);
        // Escribimos en el fichero de salida los datos que necesitamos
        fprintf(fpdestino,"Cliente consumidor %d: \n",aux->indentificador);
        fprintf(fpdestino,"Productos consumidos: %d. De los cuales: \n",aux->cantidadproductos);
        fprintf(fpdestino,"Productos de tipo a: %d \n",aux->produstostipo[0]);
        fprintf(fpdestino,"Productos de tipo b: %d \n",aux->produstostipo[1]);
        fprintf(fpdestino,"Productos de tipo c: %d \n",aux->produstostipo[2]);
        fprintf(fpdestino,"Productos de tipo d: %d \n",aux->produstostipo[3]);
        fprintf(fpdestino,"Productos de tipo e: %d \n",aux->produstostipo[4]);
        fprintf(fpdestino,"Productos de tipo f: %d \n",aux->produstostipo[5]);
        fprintf(fpdestino,"Productos de tipo g: %d \n",aux->produstostipo[6]);
        fprintf(fpdestino,"Productos de tipo h: %d \n",aux->produstostipo[7]);
        fprintf(fpdestino,"Productos de tipo i: %d \n",aux->produstostipo[8]);
        fprintf(fpdestino,"Productos de tipo j: %d \n",aux->produstostipo[9]);
        // Si el numero de consumiciones de este consumidores es menor que las consumiciones maximas guardamos estas como maximas y el consumidor que tiene estas consumiciones y actualizamos las consumiciones totales
        if(consumicionesmax<aux->cantidadproductos){
            consumicionesmax=aux->cantidadproductos;
            consumidormax=aux->indentificador;
        }
        sumaconsumiciones=sumaconsumiciones+aux->cantidadproductos;
        for(int i=0;i<numeroproveedores;i++){
            productoscadaproveedor[i]=productoscadaproveedor[i]+(aux->proveedores[i]);
        }
        free(aux);
        aux=NULL;
    }
    // Escribimos en el fichero de salida el numero total de productos consumidos
    fprintf(fpdestino,"Total de productos consumidos: %d \n",sumaconsumiciones);
    // Recorremos el numero de proveedores escribiendo en el fichero de salida la cantidad de productos de cada proveedor
    for(int i=0;i<numeroproveedores;i++){
        printf("BUCLE");
        fflush(0);
        fprintf(fpdestino,"%d del proveedor %d. \n",productoscadaproveedor[i],i);
    }
    // Escribimos en el fichero de salida el cliente aue mas ha consumido
    fprintf(fpdestino,"Cliente que mas ha consumido: %d \n",consumidormax);
    pthread_exit ( NULL );
}