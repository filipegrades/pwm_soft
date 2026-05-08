//#############################################################################
//
// ARQUIVO:    pwm_soft.c
//
// TÍTULO:    Geração de PWM por Software e Configuração Simulada
//
//! Este exemplo gera um PWM por software e simula sua configuração via registrador.
//! O registrador contém apenas o valor de comparação (duty‑cycle) e um bit de enable.
//! Observar o brilho do LED e variáveis no depurador do CCS.
//
//#############################################################################
//
// $Data de Lançamento: $
// $Copyright:
// Copyright (C) 2013-2024 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribuição e uso em formatos de código-fonte e binários, com ou sem
// modificação, são permitidos desde que as seguintes condições sejam
// atendidas:
//
//   As redistribuições do código-fonte devem reter o aviso de direitos autorais
//   acima, esta lista de condições e a seguinte isenção de responsabilidade.
//
//   As redistribuições em formato binário devem reproduzir o aviso de direitos autorais
//   acima, esta lista de condições e a seguinte isenção de responsabilidade na
//   documentação e/ou outros materiais fornecidos com a distribuição.
//
//   Nem o nome da Texas Instruments Incorporated nem os nomes de
//   seus colaboradores podem ser usados para endossar ou promover produtos derivados
//   deste software sem permissão prévia por escrito.
//
// ESTE SOFTWARE É FORNECIDO PELOS DETENTORES DOS DIREITOS AUTORAIS E COLABORADORES
// "AS IS" E QUAISQUER GARANTIAS EXPRESSAS OU IMPLÍCITAS, INCLUINDO, MAS NÃO
// SE LIMITANDO A, AS GARANTIAS IMPLÍCITAS DE COMERCIALIZAÇÃO E ADEQUAÇÃO PARA
// UM PROPÓSITO ESPECÍFICO SÃO REJEITADAS. EM NENHUM CASO O DETENTOR DOS DIREITOS AUTORAIS
// OU COLABORADORES SERÃO RESPONSÁVEIS POR QUAISQUER DANOS DIRETOS, INDIRETOS, INCIDENTAIS,
// ESPECIAIS, EXEMPLARES OU CONSEQUENCIAIS (INCLUINDO, MAS NÃO SE LIMITANDO A,
// AQUISIÇÃO DE BENS OU SERVIÇOS SUBSTITUTOS; PERDA DE USO, DADOS OU LUCROS;
// OU INTERRUPÇÃO DE NEGÓCIOS) SEJA QUAL FOR A CAUSA E SOB QUALQUER TEORIA DE
// RESPONSABILIDADE, SEJA EM CONTRATO, RESPONSABILIDADE ESTRITA OU ATO ILÍCITO
// (INCLUINDO NEGLIGÊNCIA OU OUTRO) DECORRENTE DE QUALQUER FORMA DO USO DESTE
// SOFTWARE, MESMO SE AVISADO DA POSSIBILIDADE DE TAL DANO.
// $
//#############################################################################

// Arquivos Incluídos
#include "driverlib.h"
#include "device.h"

// --- Definições ---
#define LED_GPIO_PIN        31U     // GPIO do LED2 (Azul) na LaunchPadXL

#define PWM_COMPARE_MASK    0x03FFU // Máscara para bits 0-9 (valor de comparação)
#define PWM_ENABLE_BIT      (1U << 10) // Bit 10: habilita PWM

#define PWM_PERIOD_US       1000U   // Período total do PWM em microssegundos

// Variáveis Globais (Observar no depurador)
unsigned int g_pwmControlReg = 0x0000U; // Registrador de controle PWM simulado
float g_dutyCyclePercent = 50.0F;       // Ciclo de trabalho desejado (0.0 a 100.0)
unsigned int g_timeOn_us;               // Tempo LIGADO (LED ON)
unsigned int g_timeOff_us;              // Tempo DESLIGADO (LED OFF)
bool g_enablePWM = false;

// Protótipos de Funções
void initSystemPeripherals(void);
void initLEDGPIO(void);
void enablePWM(void);
void disablePWM(void);
void calculatePWMOnOffTimes(unsigned int compareValue);
unsigned int calculateCompareValueFromDutyCycle(float dutyCycle);
void setPWMDutyCycleAndRegister(float dutyCycle);
void generateSoftwarePWM(void);

// Função Principal
void main(void)
{
    initSystemPeripherals();
    initLEDGPIO();

    // Configura o ciclo de trabalho inicial e o registrador simulado
    setPWMDutyCycleAndRegister(g_dutyCyclePercent);

    // Loop infinito para gerar o PWM
    for(;;)
    {
        if(g_enablePWM)
            {
                setPWMDutyCycleAndRegister(g_dutyCyclePercent);
                generateSoftwarePWM();
            }
        
    }
    
    
}

// Implementações de Funções

void initSystemPeripherals(void)
{
    Device_init();
    Device_initGPIO();
    enablePWM();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    EINT; // Habilita Interrupções Globais
    ERTM; // Habilita Depuração em Tempo Real
}

void initLEDGPIO(void)
{
    GPIO_setPadConfig(LED_GPIO_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(LED_GPIO_PIN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(LED_GPIO_PIN, 1); // LED inicia desligado (ativo baixo)
}

void enablePWM(void)
{
    g_pwmControlReg = g_pwmControlReg | PWM_ENABLE_BIT;
}

void disablePWM(void)
{
    g_pwmControlReg = g_pwmControlReg & (~PWM_ENABLE_BIT);
}

// Calcula tempos ON/OFF a partir do valor de comparação do registrador.
void calculatePWMOnOffTimes(unsigned int compareValue)
{
    g_timeOn_us = compareValue;
    g_timeOff_us = PWM_PERIOD_US - g_timeOn_us;
}

// Converte ciclo de trabalho (%) para valor de comparação (0 a PWM_PERIOD_US).
unsigned int calculateCompareValueFromDutyCycle(float dutyCycle)
{
    if (dutyCycle < 0.0F) dutyCycle = 0.0F;
    else if (dutyCycle > 100.0F) dutyCycle = 100.0F;
    return (unsigned int)((dutyCycle / 100.0F) * PWM_PERIOD_US);
}

// Configura ciclo de trabalho e atualiza registrador simulado e tempos ON/OFF.
void setPWMDutyCycleAndRegister(float dutyCycle)
{
    g_dutyCyclePercent = dutyCycle;

    unsigned int compareVal = calculateCompareValueFromDutyCycle(dutyCycle);

    // Preserva o bit de enable, limpa os bits de comparação e escreve o novo valor
    unsigned int currentConfigBits = g_pwmControlReg & ~PWM_COMPARE_MASK;
    g_pwmControlReg = currentConfigBits | (compareVal & PWM_COMPARE_MASK);

    calculatePWMOnOffTimes(compareVal);
}

// Gera um ciclo da onda PWM por software no pino do LED.
// Apenas lógica normal (ativo baixo: 0 = LED ON, 1 = LED OFF).
void generateSoftwarePWM(void)
{
    if ((g_pwmControlReg & PWM_ENABLE_BIT) != 0U) // Se PWM habilitado
    {
        // Período ON: pino LOW -> LED aceso
        GPIO_writePin(LED_GPIO_PIN, 0);
        DEVICE_DELAY_US(g_timeOn_us);

        // Período OFF: pino HIGH -> LED apagado
        GPIO_writePin(LED_GPIO_PIN, 1);
        DEVICE_DELAY_US(g_timeOff_us);
    }
    else // PWM desabilitado
    {
        GPIO_writePin(LED_GPIO_PIN, 1); // LED OFF
        DEVICE_DELAY_US(PWM_PERIOD_US); // Aguarda período completo
    }
}
