#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <papi.h>
#include <sys/time.h>

// Variáveis e defines relacionados com a PAPI
#define             NUM_EVENTS          2
int	Events[NUM_EVENTS] = {PAPI_L3_TCM, PAPI_TOT_INS};
int EventSet = PAPI_NULL, retval;
long long int values[NUM_EVENTS];


#define 			RANDOM_GEN 			0
#define 			ALL_1				1
#define 			ONLY_ALLOC			2
#define				BLOCK_SIZE			64

int SIZE;


double clearcache [30000000];

void clearCache (void) {
	for (unsigned i = 0; i < 30000000; ++i)
		clearcache[i] = i;
}

// Medição do tempo
long long unsigned initial_time;
struct timeval begin;
struct timeval end;

void start (void) {
	gettimeofday(&begin, NULL);
}


void stop () {
	gettimeofday(&end, NULL);
	long long duration = (end.tv_sec-begin.tv_sec)*1000000LL + end.tv_usec-begin.tv_usec;
	printf(";%.3f", ((float) duration) / 1000);
}




void blockingTranspose(float * src) {
    int temp;
    for (int ii = 0; ii < SIZE; ii += BLOCK_SIZE)
        for (int jj = 0; jj < SIZE; jj += BLOCK_SIZE)
            for (int i = ii; i < ii + BLOCK_SIZE; i++)
                for (int j = jj + i + 1; j < jj + BLOCK_SIZE; j++)
                {
                    temp = src[i*SIZE + j] ;
                    src[i*SIZE + j]  = src[j*SIZE + i] ;
                    src[j*SIZE + i]  = temp;
                }
}

void transpose (float * src){
    float tmp;
    for(int i = 0; i < SIZE; i ++)
        for(int j = i + 1; j < SIZE; j++)
        {
            tmp = src[i*SIZE + j] ;
            src[i*SIZE + j]  = src[j*SIZE + i] ;
            src[j*SIZE + i]  = tmp;
        }
}



float * createMatrix(int opt){
    float * matrix = (float *) malloc(sizeof(float) * SIZE * SIZE);

    float a = 5.0;
    if(opt != 2) {
        for(int i = 0; i < SIZE; i++)
            for(int j = 0; j < SIZE; j++)
                if(opt == 1)
                    matrix[i*SIZE + j]  = 1.0;
                else
                    //matrix[i*SIZE + j]  = (float) sin(i+j);
                    matrix[i*SIZE + j]  = (float(rand())/float((RAND_MAX)) * a);
	}
	return matrix;
}

// Versões originais

void matrixMultIJK(float * matrix_a, float * matrix_b, float * matrix_c){
    start();
    retval = PAPI_start(EventSet);
    float sum;
    int i, j, k;

    for( i = 0; i < SIZE; i ++)
        for( j = 0; j < SIZE; j++){
            sum = 0;
            for ( k = 0; k < SIZE; k++ )
                sum += matrix_a[i*SIZE + k] * matrix_b[k*SIZE + j] ;
            matrix_c[i*SIZE + j] = sum;
        }

    retval = PAPI_stop(EventSet, values);
    stop();
}

void matrixMultIKJ(float * matrix_a, float * matrix_b, float * matrix_c){
    start();
    retval = PAPI_start(EventSet);
    int i, j, k;

    for( i = 0; i < SIZE; i ++) {
        for( k = 0; k < SIZE; k++){
            for ( j = 0; j < SIZE; j++ )
                matrix_c[i*SIZE + j] += matrix_a[i*SIZE + k] * matrix_b[k*SIZE + j] ;
        }
    }

    retval = PAPI_stop(EventSet, values);
    stop();
}


void matrixMultJKI(float * matrix_a, float * matrix_b, float * matrix_c){
    start();
    retval = PAPI_start(EventSet);
    int i, j, k;

    for( j = 0; j < SIZE; j++){
        for( k = 0; k < SIZE; k++){
            for ( i = 0; i < SIZE; i++ )
                matrix_c[i*SIZE + j] += matrix_a[i*SIZE + k] * matrix_b[k*SIZE + j] ;
        }
    }

    retval = PAPI_stop(EventSet, values);
    stop();
}

// Versões com transposta sem blocking

void matrixMultIJK_transpose(float * matrix_a, float * matrix_b, float * matrix_c){
    start();
    retval = PAPI_start(EventSet);
    float sum;
    int i, j, k;

    transpose(matrix_b);
    for( i = 0; i < SIZE; i ++)
        for( j = 0; j < SIZE; j++){
            sum = 0;
            for ( k = 0; k < SIZE; k++ )
                sum += matrix_a[i*SIZE + k] * matrix_b[j*SIZE + k];
            matrix_c[i*SIZE + j] = sum;
        }

    retval = PAPI_stop(EventSet, values);
    stop();
}

void matrixMultIKJ_transpose(float * matrix_a, float * matrix_b, float * matrix_c){
    start();
    retval = PAPI_start(EventSet);
    int i, j, k;
    for( i = 0; i < SIZE; i ++)
        for( k = 0; k < SIZE; k++){
            for ( j = 0; j < SIZE; j++ )
                matrix_c[i*SIZE + j] += matrix_a[i*SIZE + k] * matrix_b[k*SIZE + j];
        }

    retval = PAPI_stop(EventSet, values);
    stop();
}


void matrixMultJKI_transpose(float * matrix_a, float * matrix_b, float * matrix_c){
    start();
    retval = PAPI_start(EventSet);
    int i, j, k;
    transpose(matrix_a);
	transpose(matrix_b);
    for( j = 0; j < SIZE; j ++)
        for( k = 0; k < SIZE; k++){
            for ( i = 0; i < SIZE; i++ )
                matrix_c[i*SIZE + j] += matrix_a[k*SIZE + i]  * matrix_b[k*SIZE + j];
        }

    retval = PAPI_stop(EventSet, values);
    stop();
}


// Main

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Argumentos incorretos\n");
        return -1;
    }


    //Inicialização da PAPI
	retval = PAPI_library_init(PAPI_VER_CURRENT);
	retval = PAPI_create_eventset(&EventSet);
	retval = PAPI_add_events(EventSet, Events, NUM_EVENTS);


    float *matrix_a, *matrix_b, *matrix_c;
    float *matrix_aa, *matrix_bb, *matrix_cc;
    SIZE = atoi(argv[2]);
    int imp = atoi(argv[1]);

    matrix_a = createMatrix(RANDOM_GEN);
    matrix_b = createMatrix(ALL_1);
    matrix_c = createMatrix(ONLY_ALLOC);
    matrix_aa = createMatrix(RANDOM_GEN);
    matrix_bb = createMatrix(ALL_1);
    matrix_cc = createMatrix(ONLY_ALLOC);


	if(imp == 1) {
        matrixMultIJK(matrix_a, matrix_b, matrix_c);
    }
    else if(imp == 2) {
        matrixMultIKJ(matrix_a, matrix_b, matrix_c);
    }
    else if(imp == 3) {
        matrixMultJKI(matrix_a, matrix_b, matrix_c);
    }
    else if(imp == 4) {
        matrixMultIJK_transpose(matrix_a, matrix_b, matrix_c);
    }
    else if(imp == 5) {
        matrixMultIKJ_transpose(matrix_a, matrix_b, matrix_c);
    }
    else if(imp == 6) {
        matrixMultJKI_transpose(matrix_a, matrix_b, matrix_c);
    }
    else {
        printf("Nenhuma implementação corresponde ao primeiro argumento!!\n");
        return -1;
    }

    printf("/%.9f", values[0]/ (float) values[1]);



    free(matrix_a);
    free(matrix_b);
    free(matrix_c);
    free(matrix_aa);
    free(matrix_bb);
    free(matrix_cc);

    return 0;
}
