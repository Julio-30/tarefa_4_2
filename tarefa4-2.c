// Bibliotecas utilizadas
#include "hardware/pio.h"  
#include "hardware/i2c.h" 
#include "pico/stdlib.h"
#include "ws2818b.pio.h"   
#include "ssd1306.h"       
#include <stdlib.h>   
#include <stdio.h> 
#include <math.h> 
#include "font.h"     

// Definindo pinos para comunicação I2C
#define I2C_PORT i2c1   
#define I2C_SDA_PIN 14     
#define I2C_SCL_PIN 15   
#define DISPLAY_ADDRESS 0x3C // Endereço I2C do display SSD1306
// Definiindo pinos do LED RGB
#define LED_RED_PIN 13   
#define LED_GREEN_PIN 11 
#define LED_BLUE_PIN 12 

void init_rgb() {
  gpio_init(LED_RED_PIN);   
  gpio_init(LED_GREEN_PIN);
  gpio_init(LED_BLUE_PIN); 
  gpio_set_dir(LED_RED_PIN, GPIO_OUT);  
  gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
  gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);  
}

// Função para controlar o estado dos LEDs RGB
void set_rgb_state(bool red_state, bool green_state, bool blue_state) {
  init_rgb();          // Pinos configurados como saída
  gpio_put(LED_RED_PIN, red_state);     // Define o estado do LED vermelho
  gpio_put(LED_GREEN_PIN, green_state);  // Define o estado do LED verde
  gpio_put(LED_BLUE_PIN, blue_state);  // Define o estado do LED azul
}

ssd1306_t display;
// Inicialização e configurar do I2C e do display OLED SSD1306 
void init_display() {
    i2c_init(I2C_PORT, 400 * 1000); // Comunicação I2C com velocidade de 400 kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);  // Configura o pino SDA para função I2C
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);  // Configura o pino SCL para função I2C
    gpio_pull_up(I2C_SDA_PIN); 
    gpio_pull_up(I2C_SCL_PIN); 

    ssd1306_init(&display, WIDTH, HEIGHT, false, DISPLAY_ADDRESS, I2C_PORT); // Inicializa o display SSD1306 com dimensões e endereço I2C
    ssd1306_config(&display);     // Configura o display com parâmetros adicionais
    ssd1306_send_data(&display);  // Envia dados iniciais ao display

    ssd1306_fill(&display, false); // Limpa o display com pixels apagados
    ssd1306_send_data(&display);   // Atualiza o display para refletir a limpeza
}

#define DEBOUNCE_DELAY_MS 300  // Tempo de debounce
#define BUTTON_A_PIN 5         // Pino do Botão A
#define BUTTON_B_PIN 6         // Pino do Botão B

volatile uint32_t last_irq_time_button_a = 0; // Armazena o tempo da última interrupção do Botão A
volatile uint32_t last_irq_time_button_b = 0; // Armazena o tempo da última interrupção do Botão B

bool led_blue_state = false;  // Estado atual do LED azul
bool led_green_state = false; // Estado atual do LED verde

void button_irq_handler(uint gpio, uint32_t events); // Declaração do callback para interrupção dos botões

void init_buttons(void) {
    gpio_init(BUTTON_A_PIN); // Inicializa o pino do Botão A
    gpio_init(BUTTON_B_PIN); // Inicializa o pino do Botão B

    gpio_set_dir(BUTTON_A_PIN, GPIO_IN); // Configura o Botão A como entrada
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN); // Configura o Botão B como entrada

    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);

    // Configura interrupções com callback para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, button_irq_handler); // Interrupção para Botão A
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, button_irq_handler); // Interrupção para Botão B
}

// Função para tratar debounce e alternar o estado dos LEDs
void debounce_button(uint pin, volatile uint32_t *last_irq_time, bool *led_state) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual em ms
    if (current_time - *last_irq_time > DEBOUNCE_DELAY_MS) { // Verifica se o tempo desde a última interrupção é suficiente
       
        *last_irq_time = current_time;  // Atualiza o tempo da última interrupção
        *led_state = !(*led_state);     // Alterna o estado do LED

        if (pin == BUTTON_A_PIN) {
            bool color = true;
            ssd1306_fill(&display, !color); // Limpa o display
            ssd1306_rect(&display, 3, 3, 122, 58, color, !color); // Desenha um retângulo
            ssd1306_draw_string(&display, "LED azul", 30, 10);  // Exibe texto no display
            ssd1306_draw_string(&display, "aceso", 30, 30); // Exibe texto indicando LED azul
            ssd1306_draw_string(&display, *led_state ? "Ligado" : "Desligado", 20, 50); // Exibe estado do LED
            ssd1306_send_data(&display); // Atualiza o display
            printf("Led azul %s\n", *led_state ? "ligado" : "desligado"); // Log no console
            set_rgb_state(0, 0, *led_state); // Alterna o estado do LED azul

        } else if (pin == BUTTON_B_PIN) { // Verifica se o pino é o Botão B
            bool color = true;
            ssd1306_fill(&display, !color); // Limpa o display
            ssd1306_rect(&display, 3, 3, 122, 58, color, !color); // Desenha um retângulo
            ssd1306_draw_string(&display, "LED verde", 30, 10); // Exibe texto no display
            ssd1306_draw_string(&display, "aceso", 30, 30);  // Exibe texto indicando LED verde
            ssd1306_draw_string(&display, *led_state ? "Ligado" : "Desligado", 20, 50); // Exibe estado do LED
            ssd1306_send_data(&display); // Atualiza o display
            printf("Led verde %s\n", *led_state ? "ligado" : "desligado"); // Log no console
            set_rgb_state(0, *led_state, 0); // Alterna o estado do LED verde
}}}

// Callback chamado quando ocorre interrupção em algum botão
void button_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A_PIN) { // Verifica se o botão pressionado foi o Botão A
        debounce_button(BUTTON_A_PIN, &last_irq_time_button_a, &led_blue_state);
    } else if (gpio == BUTTON_B_PIN) { // Verifica se o botão pressionado foi o Botão B
        debounce_button(BUTTON_B_PIN, &last_irq_time_button_b, &led_green_state);
}}

//Configuração da matriz ws2812
#define MATRIX_PIN 7 // Pino de controle da matriz de LEDs
#define NUM_LEDS 25  // Número total de LEDs na matriz

// Função para converter as posições (x, y) da matriz para um índice do vetor de LEDs
int get_led_index(int x, int y) {
    if (x % 2 == 0) { // Se a linha for par
        return 24 - (x * 5 + y); // Calcula o índice do LED considerando a ordem da matriz
    } else { // Se a linha for ímpar
        return 24 - (x * 5 + (4 - y)); // Calcula o índice invertendo a posição dos LEDs
}}

// Definição da estrutura de cor para cada LED
struct pixel_t {
    uint8_t red, green, blue; // Componentes de cor: vermelho, verde e azul
};
typedef struct pixel_t led_t; // Cria um tipo led_t baseado em pixel_t
led_t leds[NUM_LEDS]; // Vetor que armazena as cores de todos os LEDs

// PIO e state machine para controle dos LEDs
PIO pio; // PIO utilizado para comunicação com os LEDs
uint state_machine;   // State machine associada ao PIO

// Função para atualizar os LEDs da matriz
void update_leds() {
    for (uint i = 0; i < NUM_LEDS; ++i) { // Percorre todos os LEDs
        pio_sm_put_blocking(pio, state_machine, leds[i].red); // Envia valor do componente vermelho
        pio_sm_put_blocking(pio, state_machine, leds[i].green); // Envia valor do componente verde
        pio_sm_put_blocking(pio, state_machine, leds[i].blue); // Envia valor do componente azul
    }
    sleep_us(100); // Aguarda 100 microsegundos para estabilizar
}
// Função de controle inicial da matriz de LEDs
void init_matrix(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program); // Carrega programa para controlar LEDs no PIO
    pio = pio0; // Seleciona o PIO 0
    state_machine = pio_claim_unused_sm(pio, true); // Reivindica uma state machine livre
    ws2818b_program_init(pio, state_machine, offset, pin, 800000.f); // Inicializa o controle da matriz no pino especificado

    for (uint i = 0; i < NUM_LEDS; ++i) { // Inicializa todos os LEDs com a cor preta (desligados)
        leds[i].red = leds[i].green = leds[i].blue = 0;
    }
    update_leds(); // Atualiza o estado dos LEDs
}
// Função para configurar a cor de um LED específico
void set_led_color(const uint index, const uint8_t red, const uint8_t green, const uint8_t blue) {
    leds[index].red = red; // Define o componente vermelho
    leds[index].green = green; // Define o componente verde
    leds[index].blue = blue; // Define o componente azul
}
// Função para desligar todos os LEDs
void turn_off_leds() {
    for (uint i = 0; i < NUM_LEDS; ++i) { // Percorre todos os LEDs
        set_led_color(i, 0, 0, 0); // Define a cor preta (desligado) para cada LED
    }
    update_leds();
}

// Desenhar os números na matriz de LEDs
void draw_number0() {  // Função para desenhar o número 0 na matriz de LEDs
    int matrix[5][5][3] = {  // Matriz tridimensional para representar a cor de cada LED (5x5)
        // Cada elemento [linha][coluna][RGB] define a cor de um LED
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},   
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {255, 0, 0},  {0, 0, 0}},   
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {255, 0, 0},  {0, 0, 0}},    
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {255, 0, 0},  {0, 0, 0}},    
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}}    
    };
    // Loop para percorrer cada linha da matriz
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {  // Loop para percorrer cada coluna
            int position = get_led_index(row, col);  // Converte a posição da matriz para índice do vetor de LEDs
            // Define a cor do LED na posição correspondente
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        }}  update_leds();
}
void draw_number1() {
    int matrix[5][5][3] = {
        {{0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}}
    };
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int position = get_led_index(row, col);
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        }}  update_leds();
}
void draw_number2() {
    int matrix[5][5][3] = {
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}}
    };
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int position = get_led_index(row, col);
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        }}  update_leds();
}
void draw_number3() {
    int matrix[5][5][3] = {
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0  }},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0,   0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0,   0}},
        {{0, 0, 0}, {0, 0, 0},     {0, 0, 0}, {255, 0, 0}, {0, 0,   0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0,   0}}
    };
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int position = get_led_index(row, col);
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        }}  update_leds();
}
void draw_number4() {
    int matrix[5][5][3] = {
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}}
    };
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int position = get_led_index(row, col);
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        }}  update_leds();
}
void draw_number5() {
    int matrix[5][5][3] = {
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}}
    };
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int position = get_led_index(row, col);
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        }}  update_leds();
}
void draw_number6() {
    int matrix[5][5][3] = {
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}}
    };
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int position = get_led_index(row, col);
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        }} update_leds();
}
void draw_number7() {
    int matrix[5][5][3] = {
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}}
    };
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int position = get_led_index(row, col);
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        }}  update_leds();
}
void draw_number8() {
    int matrix[5][5][3] = {
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}}
    };
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int position = get_led_index(row, col);
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        }}  update_leds();
}
void draw_number9() {
    int matrix[5][5][3] = {
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}}
    };
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int position = get_led_index(row, col);
            set_led_color(position, matrix[row][col][0], matrix[row][col][1], matrix[row][col][2]);
        } } update_leds();
}

char current_input = 0; // Estado atual do sistema
void handle_input(char current_input) {
    switch (current_input) {
        case '0': 
            draw_number0(); 
            break;
        case '1': 
            draw_number1(); 
            break;
        case '2': 
            draw_number2(); 
            break;
        case '3': 
            draw_number3(); 
            break;
        case '4': 
            draw_number4(); 
            break;
        case '5': 
            draw_number5(); 
            break;
        case '6': 
            draw_number6(); 
            break;
        case '7': 
            draw_number7(); 
            break;
        case '8': 
            draw_number8(); 
            break;
        case '9': 
            draw_number9(); 
            break;
    }
}

int main() {
    init_matrix(MATRIX_PIN);// Configura controle na matriz
    stdio_init_all();    // Inicializa entrada/saída padrão
    init_buttons();   // Inicialização dos botões
    init_rgb();     // LEDs RGB 
    init_display();        // Inicializa o display
    bool color = true; // Declarado corretamente antes de seu uso

      // Exibição inicial no display OLED
      ssd1306_fill(&display, !color);                             // Limpa o display preenchendo com a cor oposta ao valor atual de "color"
      ssd1306_rect(&display, 3, 3, 122, 58, color, !color);        // Desenha um retângulo com bordas dentro das coordenadas especificadas
      ssd1306_draw_string(&display, "Utilize", 35, 10);       // Escreve "Utilize" na posição (35, 10) do display
      ssd1306_draw_string(&display, "Seu", 50, 30);
      ssd1306_draw_string(&display, "Teclado", 35, 50);
      ssd1306_send_data(&display);                         // Envia os dados para atualizar o display

while (true) {
    color = !color;  // Alterna a cor
    ssd1306_send_data(&display);  // Atualiza o display
    sleep_ms(500);  // Aguarda 500 ms

    printf("Digite um caractere: ");                        
    fflush(stdout);                                          // Certifica-se de que o buffer de saída seja limpo antes de aguardar a entrada
    if (scanf(" %c", &current_input) == 1) {                  
        handle_input(current_input);                               
        printf("%c\n", current_input);                        // Exibe o caractere digitado
        ssd1306_fill(&display, !color);                       // Limpa o display com a cor oposta
        ssd1306_rect(&display, 3, 3, 122, 58, color, !color);
        ssd1306_draw_string(&display, &current_input, 60, 30);
        ssd1306_send_data(&display);                     // Atualiza o display com as novas informações
    }}
    return 0;
}
