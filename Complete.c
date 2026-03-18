
#include <asf.h>

void configure_GCLK(void); // GCLK configuration procedure
void configure_PM(void); // PM configuration procedure
void configure_PORT(void); // PORT configuration procedure
void configure_TC4(void); // TC4 configuration procedure
void configure_TC4_callback(void); // TC4 interrupt configuration procedure
void procedure_traitement_it(void); // procedure sous interruption
void Calcul_Gabarit (int Etat, int VPalier);
void configure_DAC(void);
void configure_TC5_PMW(void);
void configure_ADC(void);
void Asservissement(void);
// Le prototype de TC4_Handler est deja delare dans startup_samd21.h

// int count = 0; // Compteur pour le clignotement de la LED a 1Hz


// Definition for PORT
#define PORT_A 0x41004400
#define PORT_B 0x41004480
#define PORT_PINCFG_DRVNOR (0x0ul << PORT_PINCFG_DRVSTR_Pos)
#define PWM_MAX 800      // Valeur max du PWM (correspond a CC[0] du TC5)
#define KP 2            // Gain proportionnel (a ajuster selon le systeme)


// Definition for GCLK
#define GCLK_CLKCTRL_NOWRTLOCK (0x0ul << GCLK_CLKCTRL_WRTLOCK_Pos)
#define GCLK_GENCTRL_NORUNSTDBY (0x0ul << GCLK_GENCTRL_RUNSTDBY_Pos)
#define GCLK_GENCTRL_NODIVSEL (0x0ul << GCLK_GENCTRL_DIVSEL_Pos)
#define GCLK_GENCTRL_NOOE (0x0ul << GCLK_GENCTRL_OOV_Pos)
#define GCLK_GENCTRL_OOV_0 (0x0ul << GCLK_GENCTRL_OOV_Pos)
#define GCLK_GENCTRL_NOIDC (0x0ul << GCLK_GENCTRL_IDC_Pos)



// Definition for TC4
#define COUNT16_TC_CTRLA = DISABLE;
// VARIABLES GLOBALES

Tc *ptr_TC;              

// Variables pour le gabarit de vitesse
int TMontee = 128;       // Duree acceleration en ms
int TPalier = 512;       // Duree palier en ms
int TDescente = 128;     // Duree decelration en ms
int TRepos = 64;         // Duree repos en ms
int Etat = 1;            // 1: Marche, 0: Arret
int VPalier = 5000;      // Vitesse palier en tr/min (0-5000)
int state = 0;           // Etat machine: 0=Repos, 1=Accel, 2=Vconst, 3=Decel
int VConsigne = 0;       // Vitesse de consigne calculee (0-5000 tr/min)


// CONFIGURATION DU GESTIONNAIRE D'HORLOGE (GCLK)

void configure_GCLK(void)
{
Gclk *ptr_GCLK;
ptr_GCLK = GCLK;

ptr_GCLK->CLKCTRL.reg =  GCLK_CLKCTRL_CLKEN |GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TC4_TC5;
ptr_GCLK->CLKCTRL.reg =  GCLK_CLKCTRL_CLKEN |GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_DAC;
ptr_GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0) |GCLK_GENCTRL_SRC_OSC8M | GCLK_GENCTRL_GENEN;  
ptr_GCLK->CLKCTRL.reg =  GCLK_CLKCTRL_CLKEN |GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_ADC;

}

void configure_DAC(void)
{
Dac *ptr_DAC;
ptr_DAC = DAC;

ptr_DAC->CTRLA.reg = DAC_CTRLA_ENABLE;
ptr_DAC->CTRLB.reg = DAC_CTRLB_EOEN | DAC_CTRLB_REFSEL_AVCC | DAC_CTRLB_VPD;
}

void configure_PM(void)
{
Pm *ptr_PM;
ptr_PM = PM;

ptr_PM->CPUSEL.reg = PM_CPUSEL_CPUDIV_DIV1;
ptr_PM->APBCSEL.reg = PM_APBCSEL_APBCDIV_DIV1;
ptr_PM->APBCMASK.reg = PM_APBCMASK_TC4 | PM_APBCMASK_DAC | PM_APBCMASK_TC5|PM_APBCMASK_ADC;
}


void configure_ADC(void)
{
    Adc *ptr_ADC = ADC;

    // 1. On de
    ptr_ADC->CTRLA.reg = 0;
    while(ptr_ADC->STATUS.bit.SYNCBUSY);

    // 2. Calibration 
    uint32_t bias = (*((uint32_t *)ADC_FUSES_BIASCAL_ADDR) & 
                     ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;
    uint32_t linearity = (*((uint32_t *)ADC_FUSES_LINEARITY_0_ADDR) & 
                          ADC_FUSES_LINEARITY_0_Msk) >> ADC_FUSES_LINEARITY_0_Pos;
    linearity |= ((*((uint32_t *)ADC_FUSES_LINEARITY_1_ADDR) & 
                   ADC_FUSES_LINEARITY_1_Msk) >> ADC_FUSES_LINEARITY_1_Pos) << 5;
    ptr_ADC->CALIB.reg = ADC_CALIB_BIAS_CAL(bias) | ADC_CALIB_LINEARITY_CAL(linearity);

    // 3. Configuration complète
    ptr_ADC->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC1;
    ptr_ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1;
    ptr_ADC->SAMPCTRL.reg = ADC_SAMPCTRL_SAMPLEN(5);
    ptr_ADC->CTRLB.reg = ADC_CTRLB_RESSEL_10BIT | ADC_CTRLB_PRESCALER_DIV16;
    ptr_ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXPOS_PIN1 | 
                             ADC_INPUTCTRL_MUXNEG_GND | 
                             ADC_INPUTCTRL_GAIN_DIV2;

    // 4. Activation
    ptr_ADC->CTRLA.reg = ADC_CTRLA_ENABLE;
    while(ptr_ADC->STATUS.bit.SYNCBUSY);
}


void configure_PORT(void)
{
PortGroup *ptr_Port;
ptr_Port = PORT_B;

/* Configuration pour PB12 */
ptr_Port->DIRSET.reg = PORT_PB12;
ptr_Port->PMUX[6].reg = PORT_PMUX_PMUXE_E;

ptr_Port->PINCFG[12].reg = PORT_PINCFG_PMUXEN;

/* Configuration pour PB06 */
ptr_Port->DIRSET.reg =PORT_PB06;
ptr_Port->PMUX[3].reg = PORT_PMUX_PMUXE_E;
ptr_Port->PINCFG[6].reg = PORT_PINCFG_DRVNOR;

/*Configuration du PB10 pour la PMW */
ptr_Port->DIRSET.reg =PORT_PB11;
ptr_Port->PMUX[5].reg = PORT_PMUX_PMUXO_E;
ptr_Port->PINCFG[10].reg = PORT_PINCFG_PMUXEN;

/*Configuration du PB11 pour la PMW */
ptr_Port->DIRSET.reg =PORT_PB11;
ptr_Port->PMUX[5].reg = PORT_PMUX_PMUXO_E;
ptr_Port->PINCFG[11].reg = PORT_PINCFG_PMUXEN;

ptr_Port = PORT_A;
/* Configuration pour PA02 (DAC) */
ptr_Port->DIRSET.reg = PORT_PA02;
ptr_Port->PMUX[1].reg = PORT_PMUX_PMUXE_B;
ptr_Port->PINCFG[2].reg = PORT_PINCFG_PMUXEN;

/*Configuration du PA03 pour l'ADC */
ptr_Port->DIRSET.reg =PORT_PA03;
ptr_Port->DIRCLR.reg = PORT_PA03;  // ENTRÉE
ptr_Port->PMUX[1].reg |= PORT_PMUX_PMUXO_B;
ptr_Port->PINCFG[3].reg = PORT_PINCFG_PMUXEN;
}

void configure_TC4(void)
{
ptr_TC->COUNT16.CTRLA.reg =TC_CTRLA_MODE_COUNT16 |TC_CTRLA_WAVEGEN_MFRQ|TC_CTRLA_PRESCALER_DIV1|TC_CTRLA_PRESCSYNC_GCLK;
ptr_TC->COUNT16.CTRLBCLR.reg = TC_CTRLBCLR_DIR |TC_CTRLBCLR_ONESHOT_Pos|TC_CTRLBCLR_CMD_NONE ;
ptr_TC->COUNT16.CTRLC.reg =TC_CTRLC_INVEN1_Pos|  TC_CTRLC_CPTEN1_Pos;
ptr_TC->COUNT16.CC[0].reg =  8000; //500 Hz (4000 => 1kHz)

ptr_TC->COUNT16.CTRLA.reg |=TC_CTRLA_ENABLE;
}

//Configuration du TC5 pour la PMW
void configure_TC5_PMW(void){
Tc *ptr_TC= TC5;
ptr_TC->COUNT16.CTRLA.reg =TC_CTRLA_MODE_COUNT16 |TC_CTRLA_PRESCALER_DIV1|TC_CTRLA_WAVEGEN_MPWM|TC_CTRLA_ENABLE;
ptr_TC->COUNT16.CTRLBCLR.reg = TC_CTRLBCLR_DIR |TC_CTRLBCLR_ONESHOT_Pos|TC_CTRLBCLR_CMD_NONE ;
ptr_TC->COUNT16.CTRLC.reg =TC_CTRLC_INVEN1_Pos|  TC_CTRLC_CPTEN1_Pos;
ptr_TC ->COUNT16.CC[0].reg =800;
ptr_TC ->COUNT16.CC[1].reg =400;
ptr_TC->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
}

void calcul_PWM(void)
{
//ptr_TC ->COUNT16.CC[1].reg =ptr_TC ->COUNT16.CC[0].reg*0.1;
}
void configure_TC4_callback(void)
{
/* Autorisation de l'IT TC4 dans le NVIC (registre ISER[0]).
   Voir la definition de la structure de NVIC_Type dans core_cm0plus.h.
   SYSTEM_INTERRUPT_MODULE_TC4 = TC4_IRQn = 19 */
system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_TC4);

/* Validation de l'IT sur Overflow du compteur dans le TC4. */
ptr_TC->COUNT16.INTENSET.reg =TC_INTENSET_OVF ;
}



void procedure_traitement_it(void)
{
PortGroup *ptr_Port;
ptr_Port = PORT_B;

// count ++;
Calcul_Gabarit(Etat, VPalier);
Asservissement();
/* Verification du bon fonctionnement de l'IT par commutation broche PB06 */
ptr_Port->OUTTGL.reg = PORT_PB06;
}

/* Procedure declaree dans la table d'exception allouee
   dans startup_samd21.c, correspondant a la routine
   d'interruption appelee si TC4 a genere une IT. */

void TC4_Handler(void)
{
/* Appel de la procedure de traitement de l'IT du niveau application */
procedure_traitement_it();

/* Remise a zero de l'indicateur d'interruption dans le TC4 */

ptr_TC->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF;

}


void Asservissement(void)
{
Adc *ptr_ADC = ADC;
Dac *ptr_DAC = DAC;
Tc  *ptr_TC  = TC5;

//int Vitesse_Consigne = 0;


ptr_ADC->SWTRIG.bit.START = 1; // Demarrer une conversion ADC
while (ptr_ADC->INTFLAG.bit.RESRDY == 0);// Attendre la fin de la conversion

uint16_t adc_value = ptr_ADC->RESULT.reg;// Lire le resultat de la conversion

ptr_ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY; // Effacer le flag de resultat pret

int Vitesse_Mesuree = (adc_value * 5000) / 1023; // Conversion de la valeur ADC en vitesse (0-1023 => 0-5000 tr/min)
int Erreur = VConsigne - Vitesse_Mesuree;
int Commande_PWM    = KP * Erreur; // Correcteur proportionnel simple

if (Commande_PWM > PWM_MAX) Commande_PWM = PWM_MAX;
if (Commande_PWM < 0)       Commande_PWM = 0;

// Attention : tu configures TC5 en COUNT16, pas COUNT8
ptr_TC->COUNT16.CC[1].reg = (uint16_t)Commande_PWM; // Application de la commande au PWM (CC[1] = duty cycle)
}


void Calcul_Gabarit (int Etat, int VPalier)
{

Dac *ptr_DAC;
ptr_DAC = DAC;
Tc  *ptr_TC5 = TC5;
//static int VConsigne = 0;
static int Temps = 0;

switch(state) {
case 0 :
VConsigne = 0;
if (Temps >= TRepos) {
if (Etat == 1) {
state = 1;
Temps = 0;
}
} else {
Temps = Temps + 1;
}
break;

case 1 :
if (Temps >= TMontee) {
state = 2;
Temps = 0;
VConsigne = VPalier;
} else {
Temps = Temps + 1;
VConsigne = (VPalier * Temps) / TMontee;
}
break;

case 2 :
if (Temps >= TPalier) {
state = 3;
Temps = 0;
} else {
Temps = Temps + 1;
}
break;

case 3 :
if (Temps >= TDescente) {
state = 0;
Temps = 0;
VConsigne = 0;
} else {
Temps = Temps + 1;
VConsigne = (VPalier * (TDescente - Temps)) / TDescente;
}
break;
}
uint16_t dac_value = (VConsigne * 1023) / 5000;
ptr_DAC->DATA.reg = dac_value;
//ptr_DAC->DATA.reg = VConsigne*1023/5000; // sortie vers le DAC
//uint16_t cc = (uint16_t)((VConsigne * 800) / 5000); // PER=199
//ptr_TC5->COUNT16.CC[1].reg = cc;
}

int main (void)
{
system_init();

/* Insert application code here, after the board has been initialized. */
ptr_TC = TC4;

configure_GCLK();
configure_PM();
configure_PORT();
configure_TC4();
configure_TC4_callback();
configure_DAC();
configure_ADC();  
configure_TC5_PMW();

while (1) {
}
}























