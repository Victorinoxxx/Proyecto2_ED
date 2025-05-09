/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Cuerpo del programa principal
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * Este software está licenciado bajo términos que se encuentran en el archivo LICENSE
  * en el directorio raíz de este componente de software.
  * Si no se incluye un archivo LICENSE, se proporciona TAL CUAL.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "fatfs.h"
#include "ili9341.h"
#include "utils.h"

#include "stdio.h"
#include "string.h"

#include <stdlib.h> // Añadido para rand() y srand()
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_WIDTH  239
#define LCD_HEIGHT 319

#define SPRITE_WIDTH 16
#define SPRITE_HEIGHT 17
#define RING_DERECHA 263
#define RING_IZQUIERDA 56
#define RING_ARRIBA 41
#define RING_ABAJO 169

#define ANCHO_LASER_VER 18
#define LARGO_LASER_VER 1
#define ANCHO_LASER_HOR 1
#define LARGO_LASER_HOR 18

#define ANCHO_FLECHA 25
#define LARGO_FLECHA 17

SPI_HandleTypeDef hspi1;
FATFS fs;           // Sistema de archivos
FIL fil;           // Objeto de archivo
FRESULT fres;      // Resultado de operaciones FATFS
DWORD fre_clust;
uint32_t totalSpace, freeSpace;
DIR dir;           // Objeto de directorio
FILINFO fno;       // Información de archivo
UINT bytesLeidos;       // Declaración para los bytes leídos
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
uint8_t rxData1; // Dato recibido por USART3
uint8_t rxData2; // Dato recibido por UART5

// Externas para los sprites
extern uint8_t mono1_abajo_derecha[], mono1_abajo_izquierda[], mono1_abajo[];
extern uint8_t mono1_arriba_derecha[], mono1_arriba_izquierda[], mono1_arriba[];
extern uint8_t mono1_derecha[], mono1_izquierda[];

extern uint8_t mono2_abajo_derecha[], mono2_abajo_izquierda[], mono2_abajo[];
extern uint8_t mono2_arriba_derecha[], mono2_arriba_izquierda[], mono2_arriba[];
extern uint8_t mono2_derecha[], mono2_izquierda[];

// Variables para poder cambiar el color de personaje que escoja cada jugador
uint8_t *abajo1, *abajo2;
uint8_t *arriba1, *arriba2;
uint8_t *izquierda1, *izquierda2;
uint8_t *derecha1, *derecha2;
uint8_t *arriba_izquierda1, *arriba_izquierda2;
uint8_t *arriba_derecha1, *arriba_derecha2;
uint8_t *abajo_izquierda1, *abajo_izquierda2;
uint8_t *abajo_derecha1, *abajo_derecha2;

//Sprites de los corazones
extern uint8_t corazon1_vida[];
extern uint8_t corazon2_vida[];

// Vidas que entran cada jugador
int vidas_j1 = 3, vidas_j2 = 3;

// Laser que atraviesan el mapa
extern uint8_t laser_vertical[];
extern uint8_t laser_horizontal[];

// Fondo de pantalla de inicio y el fondo de la arena
extern uint8_t fondo[], pantalla_de_inicio[];

// Coordenadas de los laseres
int coor_laser1;
int coor_laser2;

int laser_y ;
int laser_x;

// Coordenadas de los corazones
int corx1 = 0, cory1 = 0; // Posiciones corazon verde
int corx2 = 319-15, cory2 = 0; // Posiciones corazon rojo

// Coordenadas de los jugadores inicio
int x = RING_IZQUIERDA + 1, y = RING_ARRIBA + 1; // Posiciones
int x2 = RING_DERECHA - SPRITE_HEIGHT - 1, y2 = RING_ABAJO - SPRITE_WIDTH - 1;
int anim = 0;     // Frame de animación del personaje principal
int anim2 = 0;    // Frame de animación del segundo personaje (si aplica)

// Opciones para interactuar con el sistema del juago
int estado_juego = 0;
int opcion = 0;
int repe = 1;
int sonido = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */

void transmit_uart(char* string);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Poder enviar frases a la terminal
void transmit_uart(char* string) {
    uint32_t len = strlen(string);
    HAL_UART_Transmit(&huart2, (uint8_t*) string, len, HAL_MAX_DELAY);
    HAL_Delay(10); // Pequeño retraso para que la terminal procese la línea
}

// Chequeo de colisiones
uint8_t CheckCollision(int x1, int y1, int x2, int y2) {
    // Verifica si los rectángulos de los sprites se superponen
    if (x1 < x2 + SPRITE_WIDTH &&
        x1 + SPRITE_WIDTH > x2 &&
        y1 < y2 + SPRITE_HEIGHT &&
        y1 + SPRITE_HEIGHT > y2) {
        return 1; // Colisión detectada
    }
    return 0; // No hay colisión
}

// Poder habilitar y deshabilitar la interrupcion del USART3 y UART 5
void DisableUARTInterrupts(void) {
    // Deshabilitar interrupciones de recepción en UART3
    USART3->CR1 &= ~(USART_CR1_RXNEIE);

    // Deshabilitar interrupciones de recepción en UART5
    UART5->CR1 &= ~(USART_CR1_RXNEIE);

    // Deshabilitar interrupciones en el NVIC
    NVIC_DisableIRQ(USART3_IRQn);
    NVIC_DisableIRQ(UART5_IRQn);
}

void EnableUARTInterrupts(void) {
    // Habilitar interrupciones de recepción en UART3
    USART3->CR1 |= USART_CR1_RXNEIE;

    // Habilitar interrupciones de recepción en UART5
    UART5->CR1 |= USART_CR1_RXNEIE;

    // Habilitar interrupciones en el NVIC
    NVIC_EnableIRQ(USART3_IRQn);
    NVIC_EnableIRQ(UART5_IRQn);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  MX_FATFS_Init();
  MX_USART2_UART_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */

  // Habilitar interrupciones UART para recepción
  HAL_UART_Receive_IT(&huart3, &rxData1, 1);
  HAL_UART_Receive_IT(&huart5, &rxData2, 1);

  // 2. Monta el sistema de archivos
     fres = f_mount(&fs, "", 1);
     if (fres != FR_OK) {
         char msg[64];
         sprintf(msg, "Mount failed: %d\n", fres);
         //transmit_uart(msg);
         while (1);  // Detén si no se montó bien
     } else {
         transmit_uart("Filesystem mounted.\n");
     }

  // Abrimos el archivo en modo lectura, en este caso flecha
  fres = f_open(&fil, "flecha.txt", FA_READ);
  if (fres == FR_OK) {
      //transmit_uart("File opened for reading.\n");
  } else if (fres != FR_OK){
      //transmit_uart("File was not opened for reading!\n");
  }

  char line[39000];
  uint32_t index = 0;
  unsigned char buffer[30000];

  // Aqui pasamos el archivo flecha a la variable buffer
  // Aqui usamos la SD, guardamos la variable en un char y despues se la pasamos a buffer
   while (f_gets(line, sizeof(line), &fil) && index < 1800 * 4) {
       char *token = strtok(line, ",\n\r");
       while (token != NULL && index < 1800 * 4) {
           while (*token == ' ') token++;
           if (strncmp(token, "0x", 2) == 0) token += 2;
           uint8_t value = (uint8_t) strtol(token, NULL, 16);
           buffer[index++] = value;
           token = strtok(NULL, ",\n\r");
       }
   }

  // Enciede el pin para poder escuchar la musica
  HAL_GPIO_WritePin(Sonido1_GPIO_Port, Sonido1_Pin, 1);
  LCD_Init();
  LCD_Clear(0x0000); // Limpiar pantalla (negro)

  //Personajes  por Default
  abajo1 = mono1_abajo;
  arriba1 = mono1_arriba;
  izquierda1 = mono1_izquierda;
  derecha1 = mono1_derecha;
  arriba_izquierda1 = mono1_arriba_izquierda;
  arriba_derecha1 = mono1_arriba_derecha;
  abajo_izquierda1 = mono1_abajo_izquierda;
  abajo_derecha1 = mono1_abajo_derecha;

  abajo2 = mono2_abajo;
  arriba2 = mono2_arriba;
  izquierda2 = mono2_izquierda;
  derecha2 = mono2_derecha;
  arriba_izquierda2 = mono2_arriba_izquierda;
  arriba_derecha2 = mono2_arriba_derecha;
  abajo_izquierda2 = mono2_abajo_izquierda;
  abajo_derecha2 = mono2_abajo_derecha;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // Aqui el estado del juego esta en menu
	  if (estado_juego == 0){
		  if(opcion == 0){
			  LCD_Bitmap(0, 0, LCD_HEIGHT, LCD_WIDTH, pantalla_de_inicio);
			  LCD_Bitmap(65, 92, ANCHO_FLECHA, LARGO_FLECHA, buffer); // Aqui imprimimos la variable cargada anteriormente de la SD
		  }else if(opcion == 1){
			  LCD_Bitmap(0, 0, LCD_HEIGHT, LCD_WIDTH, pantalla_de_inicio);
			  LCD_Bitmap(65, 118, ANCHO_FLECHA, LARGO_FLECHA, buffer);
		  }else if(opcion == 2){
			  LCD_Bitmap(0, 0, LCD_HEIGHT, LCD_WIDTH, pantalla_de_inicio);
			  LCD_Bitmap(65, 144, ANCHO_FLECHA, LARGO_FLECHA, buffer);
		  }
		  HAL_GPIO_WritePin(Sonido3_GPIO_Port, Sonido3_Pin, 1);
		  HAL_Delay(100);
		  HAL_GPIO_WritePin(Sonido3_GPIO_Port, Sonido3_Pin, 0);
	  }else if(estado_juego == 1){				// Aqui el juego se esta ejecutando
			  // Redibujar fondo y personajes
					  DisableUARTInterrupts(); // Deshabilitar interrupciones UART

					  LCD_Bitmap(0, 0, LCD_HEIGHT, LCD_WIDTH, fondo);
					  HAL_Delay(100);
					  //Redibujar corazones
					  int temp_corx1 = 0;
					  for (int i = 0; i < vidas_j1; i++) {
						  LCD_Bitmap(temp_corx1, cory1, 15, 15, corazon1_vida);
						  temp_corx1 += 15;
					  }
					  int temp_corx2 = 319 - 15;
					  for (int i = 0; i < vidas_j2; i++) {
						  LCD_Bitmap(temp_corx2, cory2, 15, 15, corazon2_vida);
						  temp_corx2 -= 15;
					  }
					  int laser_stop = 1;
					  int direc_laser;
					  EnableUARTInterrupts(); // Reactivar interrupciones UART

					if(vidas_j1 > 0 && vidas_j2 > 0){ 	// Una vez de que uno de los jugadores perdio ya no vuelva a tirar laseres
						HAL_Delay(3000);
						// Nos da un numero ramdom para ver si el laser saldra de forma vertical u horizontal
						direc_laser = get_ramdom(1, 2);
						HAL_GPIO_WritePin(Sonido2_GPIO_Port, Sonido2_Pin, 1);
						HAL_Delay(100);
						HAL_GPIO_WritePin(Sonido2_GPIO_Port, Sonido2_Pin, 0);
					}
					if(vidas_j1 <= 0){			// Judador 1 ha perdido ya
						DisableUARTInterrupts(); // Deshabilitar interrupciones UART

						LCD_Print("GANADOR!", 100, 90, 2, 0xffa0, 0xed04);
						LCD_Print("JUGADOR 2!", 85, 110, 2, 0xffa0, 0xed04);

						estado_juego = 0;
						HAL_Delay(5000);
						EnableUARTInterrupts(); // Reactivar interrupciones UART
						vidas_j1 = 3;
						vidas_j2 = 3;
					}else if(vidas_j2 <= 0){		// Jugador 2 ha perdido
						DisableUARTInterrupts(); // Deshabilitar interrupciones UART

						LCD_Print("GANADOR!", 100, 90, 2, 0xffa0, 0xed04);
						LCD_Print("JUGADOR 1!", 85, 110, 2, 0xffa0, 0xed04);

						estado_juego = 0;
						HAL_Delay(5000);
						EnableUARTInterrupts(); // Reactivar interrupciones UART
						vidas_j1 = 3;
						vidas_j2 = 3;
					}

						if (direc_laser == 1) { // VERTICAL
							direc_laser = get_ramdom(3, 4);
							if (direc_laser == 3) { // ARRIBA
								coor_laser1 = get_ramdom(RING_IZQUIERDA, ((RING_DERECHA - RING_IZQUIERDA)/2) + RING_IZQUIERDA - ANCHO_LASER_VER);
								HAL_Delay(10);
								coor_laser2 = get_ramdom(((RING_DERECHA - RING_IZQUIERDA)/2) + RING_IZQUIERDA - ANCHO_LASER_VER, RING_DERECHA - ANCHO_LASER_VER);
								HAL_Delay(10);
								laser_y = 0;
								for (int i = 0; i < LCD_WIDTH; i++) {
									if(laser_stop == 1){
										LCD_Bitmap(coor_laser1, laser_y, ANCHO_LASER_VER, LARGO_LASER_VER, laser_vertical);
										LCD_Bitmap(coor_laser2, laser_y, ANCHO_LASER_VER, LARGO_LASER_VER, laser_vertical);
										// Verificar colisión con mono1
										if (CheckCollision(coor_laser1, laser_y, x, y) || CheckCollision(coor_laser2, laser_y, x, y)) {
											vidas_j1--;
											x = RING_IZQUIERDA + 1, y = RING_ARRIBA + 1; // Posiciones
											x2 = RING_DERECHA - SPRITE_HEIGHT - 1, y2 = RING_ABAJO - SPRITE_WIDTH - 1;
											laser_stop = 0;
											//transmit_uart("choque personaje 1\n");
										}
										// Verificar colisión con mono2
										if (CheckCollision(coor_laser1, laser_y, x2, y2) || CheckCollision(coor_laser2, laser_y, x2, y2)) {
											vidas_j2--;
											x = RING_IZQUIERDA + 1, y = RING_ARRIBA + 1; // Posiciones
											x2 = RING_DERECHA - SPRITE_HEIGHT - 1, y2 = RING_ABAJO - SPRITE_WIDTH - 1;
											laser_stop = 0;
											//transmit_uart("choque personaje 2\n");
										}
										laser_y++;
										HAL_Delay(1);
									}
								}
							} else if (direc_laser == 4) { // ABAJO
								coor_laser1 = get_ramdom(RING_IZQUIERDA, ((RING_DERECHA - RING_IZQUIERDA)/2) + RING_IZQUIERDA - ANCHO_LASER_VER);
								HAL_Delay(10);
								coor_laser2 = get_ramdom(((RING_DERECHA - RING_IZQUIERDA)/2) + RING_IZQUIERDA - ANCHO_LASER_VER, RING_DERECHA - ANCHO_LASER_VER);
								HAL_Delay(10);
								laser_y = 238;
								for (int i = 0; i < LCD_WIDTH; i++) {
									if(laser_stop == 1){
										LCD_Bitmap(coor_laser1, laser_y, ANCHO_LASER_VER, LARGO_LASER_VER, laser_vertical);
										LCD_Bitmap(coor_laser2, laser_y, ANCHO_LASER_VER, LARGO_LASER_VER, laser_vertical);
										// Verificar colisión con mono1
										if (CheckCollision(coor_laser1, laser_y, x, y) || CheckCollision(coor_laser2, laser_y, x, y)) {
											vidas_j1--;
											x = RING_IZQUIERDA + 1, y = RING_ARRIBA + 1; // Posiciones
											x2 = RING_DERECHA - SPRITE_HEIGHT - 1, y2 = RING_ABAJO - SPRITE_WIDTH - 1;
											laser_stop = 0;
											//transmit_uart("choque personaje 1\n");
										}
										// Verificar colisión con mono2
										if (CheckCollision(coor_laser1, laser_y, x2, y2) || CheckCollision(coor_laser2, laser_y, x2, y2)) {
											vidas_j2--;
											x = RING_IZQUIERDA + 1, y = RING_ARRIBA + 1; // Posiciones
											x2 = RING_DERECHA - SPRITE_HEIGHT - 1, y2 = RING_ABAJO - SPRITE_WIDTH - 1;
											laser_stop = 0;
											//transmit_uart("choque personaje 2\n");
										}
										laser_y--;
										HAL_Delay(1);
									}
								}
							}
						} else if (direc_laser == 2) { // HORIZONTAL
							direc_laser = get_ramdom(3, 4);
							if (direc_laser == 3) { // IZQUIERDA
								coor_laser1 = get_ramdom(RING_ARRIBA, ((RING_ABAJO - RING_ARRIBA)/2) + RING_ARRIBA - LARGO_LASER_HOR);
								coor_laser2 = get_ramdom(((RING_ABAJO - RING_ARRIBA)/2) + RING_ARRIBA - LARGO_LASER_HOR, RING_ABAJO - LARGO_LASER_HOR);
								laser_x = 0;
								for (int i = 0; i < LCD_HEIGHT; i++) {
									if(laser_stop == 1){
										LCD_Bitmap(laser_x, coor_laser1, ANCHO_LASER_HOR, LARGO_LASER_HOR, laser_horizontal);
										LCD_Bitmap(laser_x, coor_laser2, ANCHO_LASER_HOR, LARGO_LASER_HOR, laser_horizontal);
										// Verificar colisión con mono1
										if (CheckCollision(laser_x, coor_laser1, x, y) || CheckCollision(laser_x, coor_laser2, x, y)) {
											vidas_j1--;
											x = RING_IZQUIERDA + 1, y = RING_ARRIBA + 1; // Posiciones
											x2 = RING_DERECHA - SPRITE_HEIGHT - 1, y2 = RING_ABAJO - SPRITE_WIDTH - 1;
											laser_stop = 0;
											//transmit_uart("choque personaje 1\n");
										}
										// Verificar colisión con mono2
										if (CheckCollision(laser_x, coor_laser1, x2, y2) || CheckCollision(laser_x, coor_laser2, x2, y2)) {
											vidas_j2--;
											x = RING_IZQUIERDA + 1, y = RING_ARRIBA + 1; // Posiciones
											x2 = RING_DERECHA - SPRITE_HEIGHT - 1, y2 = RING_ABAJO - SPRITE_WIDTH - 1;
											laser_stop = 0;
											//transmit_uart("choque personaje 2\n");
										}
										laser_x++;
										HAL_Delay(1);
									}
								}
							} else if (direc_laser == 4) { // DERECHA
								coor_laser1 = get_ramdom(RING_ARRIBA, ((RING_ABAJO - RING_ARRIBA)/2) + RING_ARRIBA - LARGO_LASER_HOR);
								coor_laser2 = get_ramdom(((RING_ABAJO - RING_ARRIBA)/2) + RING_ARRIBA - LARGO_LASER_HOR, RING_ABAJO - LARGO_LASER_HOR);
								laser_x = 318;
								for (int i = 0; i < LCD_HEIGHT; i++) {
									if(laser_stop == 1){
										LCD_Bitmap(laser_x, coor_laser1, ANCHO_LASER_HOR, LARGO_LASER_HOR, laser_horizontal);
										LCD_Bitmap(laser_x, coor_laser2, ANCHO_LASER_HOR, LARGO_LASER_HOR, laser_horizontal);
										// Verificar colisión con mono1
										if (CheckCollision(laser_x, coor_laser1, x, y) || CheckCollision(laser_x, coor_laser2, x, y)) {
											vidas_j1--;
											x = RING_IZQUIERDA + 1, y = RING_ARRIBA + 1; // Posiciones
											x2 = RING_DERECHA - SPRITE_HEIGHT - 1, y2 = RING_ABAJO - SPRITE_WIDTH - 1;
											laser_stop = 0;
											//transmit_uart("choque personaje 1\n");
										}
										// Verificar colisión con mono2
										if (CheckCollision(laser_x, coor_laser1, x2, y2) || CheckCollision(laser_x, coor_laser2, x2, y2)) {
											vidas_j2--;
											x = RING_IZQUIERDA + 1, y = RING_ARRIBA + 1; // Posiciones
											x2 = RING_DERECHA - SPRITE_HEIGHT - 1, y2 = RING_ABAJO - SPRITE_WIDTH - 1;
											laser_stop = 0;
											//transmit_uart("choque personaje 2\n");
										}
										laser_x--;
										HAL_Delay(1);
										}
									}
								}
							}
	  	 }else if(estado_juego == 2){ 		// Esto es el menu para escoger que personaje quiere cada jugador
	  		 if(repe == 1){ // Esto es para que no se este imprimiendo y borrando a cada rato el fondo que solo se repita una vez
	  		  FillRect(0, 0, LCD_HEIGHT, LCD_WIDTH, 0x6ec8);
	  		  LCD_Print("SELECCION DE", 50, 15, 2, 0x0, 0x6ec8);
	  		  LCD_Print("PERSONAJES", 70, 35, 2, 0x0, 0x6ec8);
	  		  LCD_Print("J1", 60, 120, 2, 0xffff, 0x6ec8);
	  		  LCD_Print("J2", 200, 120, 2, 0xffff, 0x6ec8);

	  		  LCD_Sprite(67, 140, 16, 17, mono1_abajo, 4, 0, 0, 0);
	  		  LCD_Sprite(209, 140, 16, 17, mono2_abajo, 4, 0, 0, 0);
	  		  repe = 0;
	  		 }
	  	 }else if(estado_juego == 3){
	  		 HAL_GPIO_TogglePin(Sonido1_GPIO_Port, Sonido1_Pin);
	  		 estado_juego = 0;
	  	 }
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */
  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */
  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */
  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */
  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */
  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */
  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */
  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, Sonido1_Pin|LCD_RST_Pin|Sonido2_Pin|Sonido3_Pin
                          |LCD_D1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : Sonido1_Pin Sonido2_Pin Sonido3_Pin */
  GPIO_InitStruct.Pin = Sonido1_Pin|Sonido2_Pin|Sonido3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RST_Pin LCD_D1_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin LCD_D7_Pin
                           LCD_D0_Pin LCD_D2_Pin */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_D6_Pin LCD_D3_Pin LCD_D5_Pin
                           LCD_D4_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_SS_Pin */
  GPIO_InitStruct.Pin = SD_SS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(SD_SS_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
	//Retroseder en menu
	if(rxData1 == 'o'){
		estado_juego = 0;
	}
    	if(estado_juego == 0){
    		//Cambiar opcion
    		if(rxData1 == '1'){ 		// Joystick hacia arriba
    		    opcion = 0;
    		}else if(rxData1 == '3'){	// Joystick hacia la derecha
    			opcion = 1;
    		}else if(rxData1 == '5'){	// Joystick hacia abajo
    			opcion = 2;
    		}
    		//Interactuar con el menu

    		if(opcion == 0 && rxData1 == 'x'){			//Inicia el juego
    			estado_juego = 1;
    		}else if(opcion == 1 && rxData1 == 'x'){	//Seleccion de epersonajes
    			estado_juego = 2;
    		}else if(opcion == 2 && rxData1 == 'x'){	//Sonido
    			estado_juego = 3;
    		}

    		repe = 1;			// para el estado de personajes, para que imprima el fondo una vez por seleccion

    	}else if(estado_juego == 1){
				// Guardar la posición anterior para restaurarla si hay colisión
				int prev_x = x;
				int prev_y = y;

				// Esto es para definir los parametros que va a tener el personaje y no se salga del ring
				switch (rxData1) {
					case '0':
						anim = 0; // Sin animación para estado estático
						break;
					case '1': // Arriba
						y--;
						if (y <= RING_ARRIBA) y = RING_ARRIBA;
						anim = (y/10)%4;
						break;
					case '2': // Arriba-derecha
						y--;
						x++;
						if (y <= RING_ARRIBA) y = RING_ARRIBA;
						if (x >= RING_DERECHA - SPRITE_WIDTH) x = RING_DERECHA - SPRITE_WIDTH;
						anim = ((x+y)/10)%4;
						break;
					case '3': // Derecha
						x++;
						if (x >= RING_DERECHA - SPRITE_WIDTH) x = RING_DERECHA - SPRITE_WIDTH;
						anim = (x/10)%4;
						break;
					case '4': // Abajo-derecha
						y++;
						x++;
						if (y >= RING_ABAJO - SPRITE_HEIGHT) y = RING_ABAJO - SPRITE_HEIGHT;
						if (x >= RING_DERECHA - SPRITE_WIDTH) x = RING_DERECHA - SPRITE_WIDTH;
						anim = ((x+y)/10)%4;
						break;
					case '5': // Abajo
						y++;
						if (y >= RING_ABAJO - SPRITE_HEIGHT) y = RING_ABAJO - SPRITE_HEIGHT;
						anim = (y/10)%4;
						break;
					case '6': // Abajo-izquierda
						y++;
						x--;
						if (y >= RING_ABAJO - SPRITE_HEIGHT) y = RING_ABAJO - SPRITE_HEIGHT;
						if (x <= RING_IZQUIERDA) x = RING_IZQUIERDA;
						anim = (x+y/10)%4;
						break;
					case '7': // Izquierda
						x--;
						if (x <= RING_IZQUIERDA) x = RING_IZQUIERDA;
						anim = ((x)/10)%4;
						break;
					case '8': // Arriba-izquierda
						x--;
						y--;
						if (x <= RING_IZQUIERDA) x = RING_IZQUIERDA;
						if (y <= RING_ARRIBA) y = RING_ARRIBA;
						anim = (x+y/10)%4;
						break;
				}

				// Verificar colisión con el segundo personaje
				if (CheckCollision(x, y, x2, y2) && rxData1 != '0') {
					// Si hay colisión, restaurar la posición anterior
					x = prev_x;
					y = prev_y;
					anim = 0; // Mostrar sprite estático
				}

				// Dibujar sprite según la dirección y animacion
				switch (rxData1) {
					case '0':
						LCD_Sprite(x, y, 16, 17, abajo1, 4, 0, 0, 0);
						break;
					case '1':
						LCD_Sprite(x, y, 16, 17, arriba1, 4, anim, 0, 0);
						break;
					case '2':
						LCD_Sprite(x, y, 16, 17, arriba_derecha1, 4, anim, 0, 0);
						break;
					case '3':
						LCD_Sprite(x, y, 16, 17, derecha1, 4, anim, 0, 0);
						break;
					case '4':
						LCD_Sprite(x, y, 16, 17, abajo_derecha1, 4, anim, 0, 0);
						break;
					case '5':
						LCD_Sprite(x, y, 16, 17, abajo1, 4, anim, 0, 0);
						break;
					case '6':
						LCD_Sprite(x, y, 16, 17, abajo_izquierda1, 4, anim, 0, 0);
						break;
					case '7':
						LCD_Sprite(x, y, 16, 17, izquierda1, 4, anim, 0, 0);
						break;
					case '8':
						LCD_Sprite(x, y, 16, 17, arriba_izquierda1, 4, anim, 0, 0);
						break;
				}
    	}else if(estado_juego == 2){		// Que color de personaje escoge el jugador 1
    		if(rxData1 == '7'){ 		// Joystick Izquierdda
    			LCD_Sprite(67, 140, 16, 17, mono1_abajo, 4, 0, 0, 0);
    			abajo1 = mono1_abajo;
				arriba1 = mono1_arriba;
				izquierda1 = mono1_izquierda;
				derecha1 = mono1_derecha;
				arriba_izquierda1 = mono1_arriba_izquierda;
				arriba_derecha1 = mono1_arriba_derecha;
				abajo_izquierda1 = mono1_abajo_izquierda;
				abajo_derecha1 = mono1_abajo_derecha;
			}else if(rxData1 == '3'){	// Joystick Derecha
				LCD_Sprite(67, 140, 16, 17, mono2_abajo, 4, 0, 0, 0);
				abajo1 = mono2_abajo;
				arriba1 = mono2_arriba;
				izquierda1 = mono2_izquierda;
				derecha1 = mono2_derecha;
				arriba_izquierda1 = mono2_arriba_izquierda;
				arriba_derecha1 = mono2_arriba_derecha;
				abajo_izquierda1 = mono2_abajo_izquierda;
				abajo_derecha1 = mono2_abajo_derecha;
			}
    	}
			// Preparar recepción otra vez
			HAL_UART_Receive_IT(&huart3, &rxData1, 1);
		}else if (huart->Instance == UART5) { // Lee el UART5

			if(estado_juego == 1){
					// Guardar la posición anterior para restaurarla si hay colisión
							int prev_x2 = x2;
							int prev_y2 = y2;

							// Esto es para definir los parametros que va a tener el personaje y no se salga del ring
							switch (rxData2) {
								case '0':
									anim2 = 0; // Sin animación para estado estático
									break;
								case '1': // Arriba
									y2--;
									if (y2 <= RING_ARRIBA) y2 = RING_ARRIBA;
									anim2 = (y2/10)%4;
									break;
								case '2': // Arriba-derecha
									y2--;
									x2++;
									if (y2 <= RING_ARRIBA) y2 = RING_ARRIBA;
									if (x2 >= RING_DERECHA - SPRITE_WIDTH) x2 = RING_DERECHA - SPRITE_WIDTH;
									anim2 = ((x2+y2)/10)%4;
									break;
								case '3': // Derecha
									x2++;
									if (x2 >= RING_DERECHA - SPRITE_WIDTH) x2 = RING_DERECHA - SPRITE_WIDTH;
									anim2 = (x2/10)%4;
									break;
								case '4': // Abajo-derecha
									y2++;
									x2++;
									if (y2 >= RING_ABAJO - SPRITE_HEIGHT) y2 = RING_ABAJO - SPRITE_HEIGHT;
									if (x2 >= RING_DERECHA - SPRITE_WIDTH) x2 = RING_DERECHA - SPRITE_WIDTH;
									anim2 = ((x2+y2)/10)%4;
									break;
								case '5': // Abajo
									y2++;
									if (y2 >= RING_ABAJO - SPRITE_HEIGHT) y2 = RING_ABAJO - SPRITE_HEIGHT;
									anim2 = (y2/10)%4;
									break;
								case '6': // Abajo-izquierda
									y2++;
									x2--;
									if (y2 >= RING_ABAJO - SPRITE_HEIGHT) y2 = RING_ABAJO - SPRITE_HEIGHT;
									if (x2 <= RING_IZQUIERDA) x2 = RING_IZQUIERDA;
									anim2 = (x2+y2/10)%4;
									break;
								case '7': // Izquierda
									x2--;
									if (x2 <= RING_IZQUIERDA) x2 = RING_IZQUIERDA;
									anim2 = ((x2)/10)%4;
									break;
								case '8': // Arriba-izquierda
									x2--;
									y2--;
									if (x2 <= RING_IZQUIERDA) x2 = RING_IZQUIERDA;
									if (y2 <= RING_ARRIBA) y2 = RING_ARRIBA;
									anim2 = (x2+y2/10)%4;
									break;
							}

							// Verificar colisión con el segundo personaje
							if (CheckCollision(x2, y2, x, y) && rxData2 != '0') {
								// Si hay colisión, restaurar la posición anterior
								x2 = prev_x2;
								y2 = prev_y2;
								anim2 = 0; // Mostrar sprite estático
							}

							// Dibujar sprite según la dirección
							switch (rxData2) {
								case '0':
									LCD_Sprite(x2, y2, 16, 17, abajo2, 4, 0, 0, 0);
									break;
								case '1':
									LCD_Sprite(x2, y2, 16, 17, arriba2, 4, anim2, 0, 0);
									break;
								case '2':
									LCD_Sprite(x2, y2, 16, 17, arriba_derecha2, 4, anim2, 0, 0);
									break;
								case '3':
									LCD_Sprite(x2, y2, 16, 17, derecha2, 4, anim2, 0, 0);
									break;
								case '4':
									LCD_Sprite(x2, y2, 16, 17, abajo_derecha2, 4, anim2, 0, 0);
									break;
								case '5':
									LCD_Sprite(x2, y2, 16, 17, abajo2, 4, anim2, 0, 0);
									break;
								case '6':
									LCD_Sprite(x2, y2, 16, 17, abajo_izquierda2, 4, anim2, 0, 0);
									break;
								case '7':
									LCD_Sprite(x2, y2, 16, 17, izquierda2, 4, anim2, 0, 0);
									break;
								case '8':
									LCD_Sprite(x2, y2, 16, 17, arriba_izquierda2, 4, anim2, 0, 0);
									break;
							}

				}else if(estado_juego == 2){		// Que color de personaje escoge el jugador 2
		    		if(rxData2 == '7'){
		    			LCD_Sprite(209, 140, 16, 17, mono1_abajo, 4, 0, 0, 0);
		    			abajo2 = mono1_abajo;
						arriba2 = mono1_arriba;
						izquierda2 = mono1_izquierda;
						derecha2 = mono1_derecha;
						arriba_izquierda2 = mono1_arriba_izquierda;
						arriba_derecha2 = mono1_arriba_derecha;
						abajo_izquierda2 = mono1_abajo_izquierda;
						abajo_derecha2 = mono1_abajo_derecha;
					}else if(rxData2 == '3'){
						LCD_Sprite(209, 140, 16, 17, mono2_abajo, 4, 0, 0, 0);
						abajo2 = mono2_abajo;
						arriba2 = mono2_arriba;
						izquierda2 = mono2_izquierda;
						derecha2 = mono2_derecha;
						arriba_izquierda2 = mono2_arriba_izquierda;
						arriba_derecha2 = mono2_arriba_derecha;
						abajo_izquierda2 = mono2_abajo_izquierda;
						abajo_derecha2 = mono2_abajo_derecha;
					}
		    	}
			HAL_UART_Receive_IT(&huart5, &rxData2, 1);
		}

}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
