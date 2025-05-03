#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"

/* Macros generales para el juego */
#define N 12
#define M 15
#define MAXEDIF 8  // Cantidad máxima de edificios que puede haber en la ciudad (datos en matriz sin errores)
#define MAXCELDAS 10  // Cantidad máxima de celdas que puede tener un edificio (es solo una cantidad tentativa)
#define MAXBOMBARDEOS 5
#define MAXRADIO 3

/* Macros para los valores de puntuación de destrucción (algo adicional que le agregué) */
#define PJE_DESTRUCCION_CELDA 5
#define PJE_DESTRUCCION_CELDA_EDIF 15
#define PJE_DESTRUCCION_PARCIAL_EDIF 10
#define PJE_DESTRUCCION_TOTAL_EDIF 50


typedef struct data Data;
typedef struct coordenada Coordenada;
typedef struct edificio Edificio;


struct data
{
	unsigned short nro_total_edif;
	unsigned short nro_edif_destruccion_parcial;
	unsigned short nro_edif_destruccion_total;
};


struct coordenada
{
	int fila;
	int columna;
};


struct edificio
{
	char caracter;  // Caracter que representa al edificio
	unsigned short nro_celdas;  // Nro de celdas del edificio
	unsigned short nro_celdas_destruidas;  // Nro de celdas destruidas
	unsigned short max_coordenadas;  // Nro de coordenadas que irá almacenando el puntero
	Coordenada* coordenadas; // Puntero a las coordenadas del edificio
};


void mostrar_ciudad(char [N][M]);
void inicializar_edificios(Edificio [MAXEDIF]);
void actualizar_datos(Data*, Edificio[MAXEDIF], char [N][M], unsigned short*, unsigned short);
bool obtener_info_edificios(char [N][M], Edificio [MAXEDIF], unsigned short*);
bool agregar_coordenada(Coordenada, Edificio*, unsigned short);
bool ajustar_bloque_coordenadas_edificios(Edificio [MAXEDIF]);
void destruir(char [N][M], Edificio [MAXEDIF], Coordenada, unsigned short, unsigned short*);
bool pertenece_al_edificio(Coordenada, Edificio, char [N][M]);
bool existe_coordenada(Coordenada, char [N][M]);
void liberar_memoria(Edificio [MAXEDIF]);


int main()
{
	FILE* archivo_ciudad = NULL;
	char ciudad[N][M] = {'\0'};
	Edificio edificios[MAXEDIF];
	Data data = {0};
	Coordenada centro_destruccion;
	unsigned short radio_destruccion;
	unsigned short nro_bombardeo;
	unsigned short puntaje, puntaje_acum;
	unsigned short puntaje_por_ganar;
	unsigned short i = 0;
	unsigned short j = 0;
		
	archivo_ciudad = fopen("ciudad.txt", "r");

	if (archivo_ciudad == NULL)
	{
		printf("No se pudo abrir el archivo con la matriz de edificios\n");
		return 1;
	}

	/* Aqui se van leyendo los datos de la ciudad y se van guardando en la matriz */
	while (i < N && fscanf(archivo_ciudad, "%c ", &ciudad[i][j]) != EOF)  // Primero se verifica que i < N para evitar violaciones de segmento (si hay más datos, se ignoran)
	{
		if (j == M-1)  // Cuando se acabe de leer el último dato de la columna, se incrementa la fila
		{
			i++;
		}

		j = (j+1) % M;  // Para que se modifique el índice de la columna, pero en forma cíclica dentro de los límites del array
	}

	fclose(archivo_ciudad);

	/* Se muestra info sobre el juego al usuario */
	printf("\nCIUDAD BAJO FUEGO\n\n");
	printf("- Intente destruir la maxima cantidad de edificios dentro de la ciudad con la minima cantidad de intentos y el minimo radio de destruccion posible\n");
	printf("- Antes de cada proceso de bombardeo debera usted especificar la coordenada central del bombardeo y el radio de destruccion\n");
	printf("- Luego de cada bombardeo, usted vera como quedo la ciudad ademas de un pequeno catalogo con el dano que usted hizo a la ciudad\n");	
	printf("- Presione la tecla enter para continuar, o Ctrl+C si no desea jugar\n");

	while (getchar() != '\n');

	mostrar_ciudad(ciudad);
	inicializar_edificios(edificios);
	obtener_info_edificios(ciudad, edificios, &data.nro_total_edif);
	ajustar_bloque_coordenadas_edificios(edificios);
	
	/* Se inicializan las variables puntaje y nro_bombardeo */
	puntaje = 0;
	nro_bombardeo = 1;

	/* Proceso de juego (bombardeo de la ciudad) */
	/* El ciclo se detendrá ya sea cuando se acaben los intentos de bombardeo permitidos, o bien cuando todos los edificios se hayan destruido */
	while (data.nro_edif_destruccion_total < data.nro_total_edif && nro_bombardeo <= MAXBOMBARDEOS)  
	{
		puntaje_acum = puntaje;

		printf("\n# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #\n");

		printf("BOMBARDEO N°%hu\n\n", nro_bombardeo);

		do
		{
			printf("- Ingrese la fila del centro de destruccion: ");
			scanf("%d", &centro_destruccion.fila);

			printf("- Ingrese la columna del centro de destruccion: ");
			scanf("%d", &centro_destruccion.columna);
			
			printf("\n");
		}
		while (existe_coordenada(centro_destruccion, ciudad) == false);  /* El usuario debe ingresar una coordenada valida */

		do
		{
			printf("- Ingrese el radio de destrucción (debe ser menor a %hu): ", MAXRADIO+1);
			scanf("%hu", &radio_destruccion);
		}
		while (radio_destruccion > MAXRADIO);
		
		printf("# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #\n");

		destruir(ciudad, edificios, centro_destruccion, radio_destruccion, &puntaje);
		actualizar_datos(&data, edificios, ciudad, &nro_bombardeo, puntaje-puntaje_acum);
	}

	liberar_memoria(edificios);

	/* Bonus por los edificios destruidos parcial y completamente al final del juego */
	puntaje = puntaje + data.nro_edif_destruccion_total * PJE_DESTRUCCION_TOTAL_EDIF;
	puntaje = puntaje + data.nro_edif_destruccion_parcial * PJE_DESTRUCCION_PARCIAL_EDIF;

	/* Caso en que se haya ganado el juego */
	if (data.nro_edif_destruccion_total == data.nro_total_edif)
	{
		/* El puntaje por ganar lo pensé como el número restantes de bombardeos + 1, y eso por 100 */
		puntaje_por_ganar = 100 * (MAXBOMBARDEOS - (nro_bombardeo - 1) + 1);
		puntaje += puntaje_por_ganar;  // Ese puntaje se adiciona como un bonus adicional por ganar el juego (destruir todos los edificios)
		printf("\nFELICIDADES...  HA GANADO\n\n");
	}

	printf("SU PUNTAJE FINAL: %hu\n\n", puntaje);
	
	return 0;
}


/**
 * Esta función, como su nombre lo dice, muestra la matriz con los datos de la ciudad.
 * @param ciudad Es la matriz que contiene todos los datos de la ciudad.
 */
void mostrar_ciudad(char ciudad[N][M])
{
	unsigned short i, j;

	printf("\n- La ciudad luce asi:\n\n");

	for (i=0; i<N; i++)
	{
		printf("  ");

		for (j=0; j<M; j++)
		{
			printf("%c ", ciudad[i][j]);
		}

		printf("\n");
	}

	printf("\n");

	return;
}


/**
 * Esta función inicializa los edificios para evitar que contengan datos "basura".
 * @param edificios Es el arreglo que contiene la información de todos los edificios de la ciudad.
 */
void inicializar_edificios(Edificio edificios[MAXEDIF])
{
	unsigned short i;

	for (i=0; i<MAXEDIF; i++)
	{
		edificios[i].caracter = '\0';
		edificios[i].nro_celdas = 0;
		edificios[i].nro_celdas_destruidas = 0;
		edificios[i].max_coordenadas = 0;
		edificios[i].coordenadas = NULL;
	}

	return;
}


/**
 * Esta función actualiza los datos sobre la magnitud de destrucción de cada edificio y también muestra la actualización de cómo quedó la ciudad.
 * @param data Es la estructura que contiene los datos con esa información, además del nro total de edificios que había al inicio.
 * @param edificios Es el arreglo con todos los edificios.
 * @param ciudad La matriz con los datos de la ciudad.
 * @param nro_bombardeo Es un puntero que sirve para modificar la impresión dependiendo qué proceso de bombardeo fue el más reciente.
 * @pje_por_bombardeo Es el puntaje obtenido por cada bombardeo (Se pasa como parámetro para que la función lo muestre por pantalla).
 */
void actualizar_datos(Data* data, Edificio edificios[MAXEDIF], char ciudad[N][M], unsigned short* nro_bombardeo, unsigned short pje_por_bombardeo)
{
	unsigned short i;

	/* Limpieza de estos atributos */
	data->nro_edif_destruccion_total = 0;
	data->nro_edif_destruccion_parcial = 0;

	/* Actualización de los datos */
	for (i=0; i<data->nro_total_edif; i++)
	{
		if (edificios[i].nro_celdas_destruidas != 0)
		{
			if (edificios[i].nro_celdas_destruidas == edificios[i].nro_celdas)
			{
				data->nro_edif_destruccion_total++;
			}

			else
			{
				data->nro_edif_destruccion_parcial++;
			}
		}
	}

	/* Muestreo por pantalla */
	printf("\n\n* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
	printf("CATALOGO DESPUES DEL BOMBARDEO N° %hu:\n", *nro_bombardeo);
	mostrar_ciudad(ciudad);
	printf("- Nro total edificios: %hu\n", data->nro_total_edif);
	printf("- Nro edificios completamente destruidos: %hu\n", data->nro_edif_destruccion_total);
	printf("- Nro edificios parcialmente destruidos: %hu\n", data->nro_edif_destruccion_parcial);
	printf("- Puntaje por este bombardeo: %hu", pje_por_bombardeo);
	printf("\n* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");

	(*nro_bombardeo)++;

	return;
}


/**
 * Esta función recorre la matriz de la ciudad y recolecta información de los edificios que va encontrando, además de devolver el nro de edificios detectados en su último parámetro.
 * @param ciudad Es la matriz con los datos de la ciudad.
 * @param edificios Es el arreglo con los edificios.
 * @nro_edificios_detectados Es un puntero que va contando los edificios que hay (se pasa por referencia desde el main() para que se modifique).
 * @return true Si la función se ejecuta con éxito.
 * @return false Si ocurrió algún error de asignación o reasignación de memoria dinámica dentro de la función.
 */
bool obtener_info_edificios(char ciudad[N][M], Edificio edificios[MAXEDIF], unsigned short* nro_edificios_detectados)
{
	unsigned short i, j, k;  // Contadores
	bool nuevo_edificio = false;  // Valor booleano que identifica si realmente el caracter detectado es un identificador para un nuevo edificio
	Coordenada coordenada;  // Variable auxiliar para guardar la coordenada actual de la ciudad que se recorre

	/* Aqui se recorre la matriz con los datos de la ciudad */
	for (i=0; i<N; i++)
	{
		coordenada.fila = i;

		for (j=0; j<M; j++)
		{
			coordenada.columna = j;

			if (ciudad[i][j] != '-')  // Las celdas que no forman parte de ningun edificio en la ciudad se toman como '-'
			{
				k = 0;
				nuevo_edificio = true;

				while (k < MAXEDIF && edificios[k].caracter != '\0' && nuevo_edificio == true)  
				{
					if (ciudad[i][j] == edificios[k].caracter)
					{
						edificios[k].nro_celdas++;
						nuevo_edificio = false;  // Para poder contabilizar un nuevo edificio, el caracter detectado no debe ser antiguo
						
						if (agregar_coordenada(coordenada, &edificios[k], edificios[k].nro_celdas) == false)
						{
							liberar_memoria(edificios);
							return false;
						}
					}

					k++;
				}

				if (nuevo_edificio == true)
				{
					edificios[*nro_edificios_detectados].caracter = ciudad[i][j];
					edificios[*nro_edificios_detectados].nro_celdas = 1;

					/* Se solicita memoria dinamica para ir almacenando las celdas del nuevo edificio detectado */
					edificios[*nro_edificios_detectados].coordenadas = (Coordenada *) malloc(MAXCELDAS * sizeof(Coordenada));
					
					if (edificios[*nro_edificios_detectados].coordenadas == NULL)
					{
						for (k=0; k<*nro_edificios_detectados; k++)
						{
							free(edificios[k].coordenadas);
							edificios[k].coordenadas = NULL;
						}

						return false;
					}

					edificios[*nro_edificios_detectados].max_coordenadas = MAXCELDAS;
					agregar_coordenada(coordenada, &edificios[*nro_edificios_detectados], 1);
					(*nro_edificios_detectados)++;
				}
			}
		}
	}

	return true;
}


/**
 * Esta función agrega una coordenada a un edificio dentro de la ciudad.
 * @param coordenada Es la coordenada que se desea agregar a la ciudad.
 * @param edificio Es un puntero al edificio al cual se le desea agregar dicha coordenada (se pasó por referencia para poder modificar uno de sus atributos).
 * @param nro_coordenada_a_agregar: Es el número de coordenada que se agregará al edificio.
 * @return true Si se logra añadir la coordenada con éxito.
 * @return false Si no se logra añadir con éxito debido a algún fallo en la asignación de memoria dinámica.
 */
bool agregar_coordenada(Coordenada coordenada, Edificio* edificio, unsigned short nro_coordenada_a_agregar)
{
	unsigned short index = nro_coordenada_a_agregar - 1;
	Coordenada* temp = NULL;

	/* Se verifica el caso en que el edificio tiene más coordenadas de las que inicialmente se preveían */
	if (nro_coordenada_a_agregar > edificio->max_coordenadas)
	{
		edificio->max_coordenadas *= 2;  // Se duplica el máximo de coordenadas que soportará el edificio

		temp = (Coordenada *) realloc(edificio->coordenadas, edificio->max_coordenadas * sizeof(Coordenada));  // Y realocamos agrandando el bloque

		if (temp == NULL)
		{
			free(edificio->coordenadas);
			edificio->coordenadas = NULL;
			return false;
		}

		else
		{
			edificio->coordenadas = temp;
		}
	}

	edificio->coordenadas[index] = coordenada;   // Se agrega la coordenada si todo sale bien

	return true;
}


/**
 * Esta función ajusta el bloque de coordenadas de los edificios para evitar el desperdicio de memoria RAM
 * @param edificios Es el arreglo con todos los edificios de la ciudad.
 * @return true Si se logra ajustar con éxito el bloque de memoria de los edificios que usaban más memoria de la necesaria.
 * @return false Si ocurre algún error al achicar el tamaño del bloque dentro de la función.
 */
bool ajustar_bloque_coordenadas_edificios(Edificio edificios[MAXEDIF])
{
	Coordenada* temp = NULL;
	unsigned short i = 0;

	while (edificios[i].coordenadas != NULL)
	{
		if (edificios[i].max_coordenadas > edificios[i].nro_celdas)
		{
			temp = (Coordenada *) realloc(edificios[i].coordenadas, edificios[i].nro_celdas * sizeof(Coordenada));

			if (temp == NULL)
			{
				liberar_memoria(edificios);
				return false;
			}
		}

		i++;
	}

	return true;
}


/**
 * Esta función hace la destrucción (bombardeo) de la ciudad de acuerdo a cómo el usuario jugador desee.
 * @param ciudad Es la matriz con los datos de la ciudad que se desea destruir.
 * @param edificios Es el arreglo que contiene a todos los edificios de la ciudad (los que podrían ser afectados con la destrucción).
 * @param centro_destruccion Es la celda que será el centro del ataque.
 * @param radio_danho Es cuánto se dañará de la ciudad partiendo desde el centro de ataque.
 * @param puntaje Es un puntero al puntaje actual.  Dentro de esta función, dependendiendo de la magnitud de la destrucción y del radio de daño, se actualiza.
 */
void destruir(char ciudad[N][M], Edificio edificios[MAXEDIF], Coordenada centro_destruccion, unsigned short radio_danho, unsigned short* puntaje)
{
	int i, j;
	unsigned short k, m;  // Contador que se ocupará para recorrer los edificios al momento de la destrucción
	Coordenada coordenada;
	bool pertenencia = false;  // Variable booleana que servirá como bandera para identificar si una celda determinada forma parte de un edificio
	unsigned short puntaje_jugada = 0;  // Variable de tipo natural que servirá para calcular el puntaje de la jugada

	/* Aquí se recorre el rango de destrucción posible*/
	for (i = centro_destruccion.fila - radio_danho; i <= centro_destruccion.fila + radio_danho; i++)
	{
		coordenada.fila = i;

		for (j = centro_destruccion.columna - radio_danho; j <= centro_destruccion.columna + radio_danho; j++)
		{
			coordenada.columna = j;

			/* Primero se comprueba si existe la coordenada dentro de la ciudad para evitar errores de violación de segmento */
			if (existe_coordenada(coordenada, ciudad))
			{
				if (ciudad[i][j] != 'X')  // Ahora se comprueba que la celda ya no haya sido destruida
				{
					/* Limpieza de variables */
					k = 0;
					pertenencia = false;

					/* Aquí se ve si es que existe algún edificio al que le corresponda la coordenada actual */
					while (k < MAXEDIF && edificios[k].caracter != '\0' && pertenencia == false)  
					{
						if (pertenece_al_edificio(coordenada, edificios[k], ciudad))
						{
							pertenencia = true;
							
							ciudad[i][j] = 'X';  // Se destruye la celda
							edificios[k].nro_celdas_destruidas++; 
							puntaje_jugada += PJE_DESTRUCCION_CELDA_EDIF;  // Se actualiza el puntaje de la jugada

							if ((float) edificios[k].nro_celdas_destruidas / edificios[k].nro_celdas >= 0.5)
							{
								/* Aqui se destruyen todas las celdas del edificio */
								for (m=0; m<edificios[k].nro_celdas; m++)
								{
									if (ciudad[edificios[k].coordenadas[m].fila][edificios[k].coordenadas[m].columna] != 'X')
									{
										ciudad[edificios[k].coordenadas[m].fila][edificios[k].coordenadas[m].columna] = 'X';
										puntaje_jugada += PJE_DESTRUCCION_CELDA_EDIF;  // Se actualiza el puntaje de la jugada
									}
								} 
							
								edificios[k].nro_celdas_destruidas = edificios[k].nro_celdas;

							}
						}

						k++;
					}

					if (pertenencia == false)
					{
						ciudad[i][j] = 'X';  // Se destruye la celda
						puntaje_jugada += PJE_DESTRUCCION_CELDA;  // Se actualiza el puntaje de la jugada
					}
				}
			}
		}
	}

	/* Para que el puntaje obtenido en cada jugada sea inversamente proporcional al cuadrado del radio del daño */
	puntaje_jugada = puntaje_jugada / (radio_danho * radio_danho);

	/* Se actualiza el puntaje acumulado en el juego */
	(*puntaje) += puntaje_jugada;

	return;
}


/**
 * Esta función verifica si una celda determinada por sus coordenadas x e y, pertenece a un cierto edificio.
 * @param coordenada Es la coordenada que se desea saber si pertenece a un determinado edificio.
 * @param edificio Es el edificio del cual se desea saber si dicha coordenada le pertenece o no.
 * @param ciudad Es la matriz con los datos de la ciudad a la cual pertenece el edificio.
 * @return true Si la coordenada ingresada en el primer parámetro de esta función pertenece al edificio.
 * @return false Si la coordenada ingresada en el primer parámetro de esta función no pertenece al edificio.
 */
bool pertenece_al_edificio(Coordenada coordenada, Edificio edificio, char ciudad[N][M])
{
	/* Se verifica si la ciudad en esa posición (coordenada) contiene el caracter que identifica al edificio */
	if (existe_coordenada(coordenada, ciudad) && ciudad[coordenada.fila][coordenada.columna] == edificio.caracter)
	{
		return true;
	}

	return false;
}


/**
 * Esta función verifica si existe una determinada coordenada dentro de la ciudad.
 * @param coordenada La coordenada que se desea verificar si es válida.
 * @param ciudad La matriz con los datos de la ciudad en la cual se desea saber si existe la coordenada ingresada en el primer parámetro.
 * @return true Si la coordenada no excede ningún límite dentro de la matriz ciudad.
 * @return false Si la coordenada no es posible de indexar dentro del código sin que dé errores.
 */
bool existe_coordenada(Coordenada coordenada, char ciudad[N][M])
{
	if (coordenada.fila >= 0 && coordenada.columna >= 0 && coordenada.fila < N && coordenada.columna < M)
	{
		return true;
	}

	return false;
}


/**
 * Esta función libera toda la memoria dinámica utilizada en el programa.
 * @param edificios Es el arreglo con todos los edificios de la ciudad.
 */
void liberar_memoria(Edificio edificios[MAXEDIF])
{
	unsigned short i = 0;

	for (i=0; i<MAXEDIF; i++)
	{
		free(edificios[i].coordenadas);
		edificios[i].coordenadas = NULL;
	}
	
	return;
}
