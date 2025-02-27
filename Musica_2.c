#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include <math.h>
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/timer.h"

#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "inc/notas.h"

// Arquivo que contém o programa PIO
#include "pio_matrix.pio.h"

#include "inc/desenho.h"

//Definição das Constantes
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define NUM_PIXELS 25 // Número de LEDs na matriz
#define BUZZER_PIN 21  // Pino do buzzer
#define btA 5        // Pino do botão A - Proxima Musica
#define btB 6        // Pino do botão B - Musica Anterior
#define bt_joy 22
#define move_X 26 // Pino para Eixo X
#define Matrix_LED 7 // Pino de controle da matriz de LEDs


// Variáveis Globais
static volatile uint32_t last_time = 0; //Tempo da ultima interrupção 
int volume = 50;  // Volume inicial (50%)
int qual_musica = 1; // 1 - Marcha Imperial; 2 - Mario; 0 - PAUSE
bool pause_play = true;  // Indica se está pausado
uint *frequencia = NULL; //Ponteiro para as frequencias da musica
uint *duracao = NULL; //Ponteiro para as durações musica
struct repeating_timer timer_musica; // timer de repetição para controlar a  musica
struct repeating_timer timer_volume; // timer de repetição para controlar o volume
struct repeating_timer timer_matrix; // timer de repetição para para controlar a matriz led 5x5
const uint LED_G = 11; //LED verde
const uint LED_B = 12; //LED azul
const uint LED_R = 13; //LED vermelho
int tamanho = 0; //Tamanho do vetor de frequencias 
int i = 0; //Índice da musica atual
const int ADC_C0 = 0; //canal ADC para eixo X do joystick
uint16_t vrx_value; //variavel para captar valor de x e y
uint16_t div_value_x; //variavel onde será salvo o valor x divivido 
char buffer[25];// buffer para exibir texto no OLED
bool cor_borda = true; //borda on / off
ssd1306_t ssd; //Estrutura para controlar o diplay OLED
volatile uint16_t adc_value = 0; //valor lido do ADC
static volatile uint intensidade = 25; // Armazena a intensidade da matriz de leds
static volatile uint intensidade_V = 0;// //Armazena a intensidade da matriz de led verde
uint sm; //maquina de estado do PIO
PIO pio = pio0;  // Seleciona o primeiro bloco de PIO
bool ok; // verificação de clock
double Led_R = 0.0, Led_B = 0.0, Led_G = 0.0;//Cores do RGB
uint32_t valor_led;//valor de cor para a matriz de led
uint notas = 0;// contador para mudar o desenho na matriz de led
uint max_music = 3;// maximo de musicas
const uint16_t WRAP = 2000;//top do pwm da musica(buzzer)
const uint16_t WRAP_L = 100;//top do pwm do led
const float DIVISER = 1;//divisor do pwm do led
bool pwm_enabled = true; //estado do pwd dos led

// Função para definir a intensidade das cores do LED
uint32_t matrix_rgb(double led_b, double led_r, double led_g) {
    unsigned char led_R, led_G, led_B;
    led_R = led_r * intensidade;
    led_G = led_g * intensidade_V;
    led_B = led_b * intensidade;
    return (led_G << 24) | (led_R << 16) | (led_B << 8); // Retorna a cor em formato de 32 bits
  }

// Função que escreve na matriz de LEDs usando PIO
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double led_r, double led_g, double led_b) {
    for (int16_t i = 0; i < NUM_PIXELS; i++) {
        valor_led = matrix_rgb(desenho[24 - i], desenho[24 - i], desenho[24 - i]); // Define a cor do LED
        pio_sm_put_blocking(pio, sm, valor_led); // Envia o valor para a PIO
    }
}

//configuração do pino para modo pwm
void init_pwm(uint led){
    gpio_set_function(led, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(led); //obter o canal PWM da GPIO
    pwm_set_clkdiv(slice, DIVISER); //define o divisor de clock do PWM
    pwm_set_wrap(slice, WRAP_L);
    pwm_set_gpio_level(led, 0); //começa desligado
    pwm_set_enabled(slice,true);
}

//função para ligar ou desligar o pwm, além de mudar o valor do pwm no pino, neste caso do led
void set_pwm(uint led, uint16_t value){ 
    if (pwm_enabled){
        uint slice = pwm_gpio_to_slice_num(led); 
        pwm_set_gpio_level(led, value); // altera o valor do led
    }else{
        pwm_set_gpio_level(led, 0); // desliga led
    }
}

// Configuração da PIO para controle da matriz de LEDs
void init_pio(){
    
  uint offset = pio_add_program(pio, &pio_matrix_program); // Carrega o programa PIO
  sm = pio_claim_unused_sm(pio, true);               // Adquire uma máquina de estado
  pio_matrix_program_init(pio, sm, offset, Matrix_LED);   // Inicializa a PIO

  //desliga a matriz LEDs 5x5
  desenho_pio(nums[5], valor_led, pio, sm, Led_R, Led_G, Led_B);
}

//configuração do ADC do joystick
void setup_joystick()
{
  // Inicializa o ADC e os pinos de entrada analógica
  adc_init();         // Inicializa o módulo ADC
  adc_gpio_init(move_X); // Configura o pino VRX (eixo X) para entrada ADC
}

//configuração do botão
void set_pin_button(uint button){
    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button);
}

//configuração do pino do led
void set_pin_led(uint pin){
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, false);
}

//configuração da comunicação I2C
void setup_i2c(){
    i2c_init(I2C_PORT, 400 * 1000);
  
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
}

//configuração da tela OLED
void init_OLED(){
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

     // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

//Função que lê o valor da posição X do joystick
void joystick_read_axis(uint16_t *vx_value){
    // Leitura do valor do eixo X do joystick
    adc_select_input(ADC_C0); // Seleciona o canal ADC para o eixo X
    sleep_us(2);// Pequeno delay para estabilidade
    uint16_t x = 4095 - adc_read(); // Lê o valor do eixo X (0-4095) e inverte o calculo
}

// Configuração do PWM
void inicia_buzzer_pwm(uint16_t freq, int volume) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    
    if (freq == 0) {
        pwm_set_enabled(slice_num, false);  // Silencia o buzzer em pausas
        return;
    }

    float clock_div = 16.0f;  
    pwm_set_clkdiv(slice_num, clock_div);
    
    uint16_t wrap = (125000000 / (clock_div * freq)) - 1;

    // Ajusta o duty cycle com base no volume (0 a 100)
    uint16_t duty_cycle = (wrap * volume) / 2000;

    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), duty_cycle);
    pwm_set_enabled(slice_num, true);
}

//callback para alteração do volume do buzzer
bool volume_callback(struct repeating_timer *t) {
    adc_select_input(ADC_C0);
    uint16_t adc_raw = adc_read(); // Lê valor bruto do ADC (0-4095)

    vrx_value = (adc_raw*100/4095);

    int margem_neutra = 5; // Margem para evitar oscilações involuntárias

    if (vrx_value > 70 + margem_neutra && volume <= 100) { 
        volume += 1;
        if (volume > 100) volume = 100; 
    }

    if (vrx_value < 30 - margem_neutra && volume > 0) {
        volume -= 1;
        if (volume < 0) volume = 0;
    }

   // printf("ADC: %d | Volume: %d%%\n", vrx_value, volume);

    return true; // Mantém o timer rodando
}


void toca_musica();

// Callback para tocar a próxima nota
bool repeating_timer_callback(struct repeating_timer *t) {
    if (!pause_play && qual_musica > 0 && qual_musica <= max_music) return true;//Se pausado, apenas espera
    if(notas == 5) notas = 0;
    notas++;
    i++;  // Avança para a próxima nota

    printf("musica %d tocando\n", qual_musica);
    if (i < tamanho) {
        inicia_buzzer_pwm(frequencia[i], volume);
        pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), true);

        // Reinicia o temporizador com a nova duração
        add_repeating_timer_ms(duracao[i], repeating_timer_callback, NULL, &timer_musica);
        return false; // Para o temporizador atual, pois um novo será iniciado
    } else {
        pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), false);  // Finaliza a música
        notas = 0;
        i = 0;  // Reseta o índice para nova execução
        qual_musica++;

        if(qual_musica > max_music) qual_musica = 1;
        toca_musica();
        return false; // Para o temporizador ao final da música
    }
}

// Callback para matrix de led
bool matrix_LED_callback(struct repeating_timer *t){
    if(notas == 0) desenho_pio(nums[5], valor_led, pio, sm, Led_R, Led_G, Led_B); set_pwm(LED_B, 0);set_pwm(LED_R, 0);// desliga LED

    for(int i=1; i < 5; i++){
        if(notas == i && notas <= 4) desenho_pio(nums[i], valor_led, pio, sm, Led_R, Led_G, Led_B); set_pwm(LED_B, volume);set_pwm(LED_R, volume);
    }

    return true;
}
    
// Estrutura para armazenar músicas
typedef struct {
    uint *frequencia;
    uint *duracao;
    int tamanho;
} Musica;

// Definição das músicas disponíveis
Musica musicas[] = {
    {NULL, NULL, 0}, // Índice 0 - Pausa
    {frequencia_marcha_Imperial, duracao_marcha_Imperial, sizeof(frequencia_marcha_Imperial) / sizeof(frequencia_marcha_Imperial[0])},
    {frequencia_mario, duracao_mario, sizeof(frequencia_mario) / sizeof(frequencia_mario[0])},
    {pacman_frequency, pacman_duracao, sizeof(pacman_frequency) / sizeof(pacman_frequency[0])},
    {NULL, NULL, 0} // Índice 3 - Pausa
};

// Atribuição otimizada
void seleciona_musica() {
    frequencia = musicas[qual_musica].frequencia;
    duracao = musicas[qual_musica].duracao;
    tamanho = musicas[qual_musica].tamanho;
}
// Função principal para tocar a música
void toca_musica() {
    seleciona_musica();

    if (tamanho > 0) {
        i = 0;
        inicia_buzzer_pwm(frequencia[i], volume);
        pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), true);

        // Inicia o temporizador para tocar a próxima nota após a duração da primeira
        add_repeating_timer_ms(duracao[i], repeating_timer_callback, NULL, &timer_musica);
    }
}

//interrupção dos botões A, B e joystick
void button_press(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (current_time - last_time <= 200000) return; // Debounce do botão
    last_time = current_time;
        
        if (gpio == btA && qual_musica <= max_music) qual_musica++; 
        if (gpio == btB && qual_musica > 0) qual_musica--;

        if (gpio == btA || gpio == btB) {
            pause_play = true;
            frequencia = duracao = NULL;
            cancel_repeating_timer(&timer_musica);
            pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), false);
            toca_musica();
        }
    if (gpio == bt_joy) {
        pause_play = !pause_play;
        pause_play && (qual_musica > 0 && qual_musica <= max_music) ? inicia_buzzer_pwm(frequencia[i], volume)
                   : pwm_set_gpio_level(BUZZER_PIN, 0); 
    }
}

// função que altera a posição do quadrado na tela OLED
void music_oled(){

    snprintf(buffer, sizeof(buffer), "Volume: %d", volume);

    ssd1306_fill(&ssd, !cor_borda);
    ssd1306_rect(&ssd, 1, 1, 122, 58, cor_borda, !cor_borda);
    
    if(pause_play){
        if(qual_musica == 1){
            ssd1306_draw_string(&ssd, "Musica Tocando", 7, 5); // Desenha uma string
            ssd1306_draw_string(&ssd, "Marcha", 40, 20); // Desenha uma string 
            ssd1306_draw_string(&ssd, "Imperial", 32, 35); // Desenha uma string  
            ssd1306_draw_string(&ssd, buffer, 25, 50); // Desenha uma string
        }
        else if(qual_musica == 2){
            ssd1306_draw_string(&ssd, "Musica Tocando", 7, 10); // Desenha uma string
            ssd1306_draw_string(&ssd, "Mario", 44, 25); // Desenha uma string  
            ssd1306_draw_string(&ssd, buffer, 25, 40); // Desenha uma string
        }else if(qual_musica == 3){
            ssd1306_draw_string(&ssd, "Musica Tocando", 7, 10); // Desenha uma string
            ssd1306_draw_string(&ssd, "PACMAN", 40, 25); // Desenha uma string  
            ssd1306_draw_string(&ssd, buffer, 25, 40); // Desenha uma string
        } else{
            ssd1306_draw_string(&ssd, "Nao tem mais", 15, 10); // Desenha uma string
            ssd1306_draw_string(&ssd, "Musicas", 38, 25); // Desenha uma string
            ssd1306_draw_string(&ssd, buffer, 25, 40); // Desenha uma string
        }
    }
    else{
        ssd1306_draw_string(&ssd, "Musica Pausada", 7, 20); // Desenha uma string
        ssd1306_draw_string(&ssd, buffer, 25, 35); // Desenha uma string
    }

    
    
    ssd1306_send_data(&ssd);
}

//função que inicializa e configura tudo;
void setup() {
    stdio_init_all();
    setup_joystick();
    set_pin_button(btA);
    set_pin_button(btB);
    set_pin_button(bt_joy);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    setup_i2c();
    init_OLED();
    init_pio();
    init_pwm(LED_B);
    init_pwm(LED_G);
    init_pwm(LED_R);
}

int main() {
    setup();//tudo inicializado e configurado

    gpio_set_irq_enabled_with_callback(btA, GPIO_IRQ_EDGE_FALL, true, &button_press); //configura e habilita a interrupção
    gpio_set_irq_enabled(btB, GPIO_IRQ_EDGE_FALL, true);  // Apenas habilita interrupção 
    gpio_set_irq_enabled(bt_joy, GPIO_IRQ_EDGE_FALL, true);  // Apenas habilita interrupção 

    // Configura um timer para chamar volume_callback e matrix_LED_callback a cada 10ms
    add_repeating_timer_ms(10, volume_callback, NULL, &timer_volume);
    add_repeating_timer_ms(10, matrix_LED_callback, NULL, &timer_matrix);

   //chama a função para iniciar a música
    toca_musica();

    //loop principal
    while (1) {
        music_oled();//atualiza o display OLED a cada 10ms
        sleep_ms(10);
    }

    return 0;
}
