#include <stdio.h>
#include "pico/stdlib.h"

//vetor para criar imagem na matriz de led - 0
double num0[25] =   {0, 0, 0, 0, 0,
                     0, 0, 1, 0, 0, 
                     0, 1, 1, 1, 0,
                     0, 0, 1, 0, 0,
                     0, 0, 0, 0, 0};

//vetor para criar imagem na matriz de led - 1
double num1[25] =   {0, 1, 0, 1, 0,
                        1, 0, 1, 0, 1, 
                        0, 1, 1, 1, 0,
                        1, 0, 1, 0, 1,
                        0, 1, 0, 1, 0};

//vetor para criar imagem na matriz de led - 2
double num2[25] =   {1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 
                        1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1};

//vetor para criar imagem na matriz de led - 3
double num3[25] =   {0, 1, 0, 1, 0,
                    1, 0, 1, 0, 1, 
                    0, 1, 1, 1, 0,
                    1, 0, 1, 0, 1,
                    0, 1, 0, 1, 0};

//vetor para criar imagem na matriz de led - 4
double num4[25] =   {0, 0, 0, 0, 0,
                    0, 0, 1, 0, 0, 
                    0, 1, 1, 1, 0,
                    0, 0, 1, 0, 0,
                    0, 0, 0, 0, 0};


double num5[25] =   {0, 0, 0, 0, 0,
                    0.0, 0, 0.0, 0, 0.0, 
                    0.0, 0, 0, 0, 0.0,
                    0.0, 0, 0.0, 0, 0.0,
                    0, 0, 0, 0, 0};


//vetor de imagens da matriz
double *nums[6] = {num0, num1, num2, num3, num4, num5};

