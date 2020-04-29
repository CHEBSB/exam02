#include "mbed.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include <math.h>
#define UINT14_MAX        16383
// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1
// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7

I2C i2c(PTD9, PTD8);
Serial pc(USBTX, USBRX);
int m_addr = FXOS8700CQ_SLAVE_ADDR1;

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);

Timeout tout;	// counting 10 sec
bool Tout = false;
void changeMode() { Tout = true; }

float tx[100], ty[100], tz[100];
float hopl[100];

bool Fcm[100];  // > 5cm
int i = 0;

InterruptIn sw(SW2);
Thread thre;	// to release queue
EventQueue tiltQ;	// to call interrupt of tilt
DigitalOut led(LED2);	// the LED to blink
/* tilt is that |X| > 0.5 or |Y| > 0.5 */
void TenSRec();	// 10 sec accelero recording


uint8_t who_am_i, data[2], res[6];
int16_t acc16;
float t[3];
int main() {
	pc.baud(115200);

	// Enable the FXOS8700Q
	FXOS8700CQ_readRegs(FXOS8700Q_CTRL_REG1, &data[1], 1);
	data[1] |= 0x01;
	data[0] = FXOS8700Q_CTRL_REG1;
	FXOS8700CQ_writeRegs(data, 2);
	// Get the slave address
	FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);
	thre.start(callback(&tiltQ, &EventQueue::dispatch_forever));
	sw.rise(tiltQ.event(TenSRec));

}
void TenSRec() {
	tout.attach(&changeMode, 10.0);	// start 10 sec countdown
	float hpx = 0;   // initial x-placement = 0
	float hpy = 0;   // initial y-placement = 0
	while (!Tout) {
        led = !led; // blink
		FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

		acc16 = (res[0] << 6) | (res[1] >> 2);
		if (acc16 > UINT14_MAX / 2)
			acc16 -= UINT14_MAX;
		t[0] = ((float)acc16) / 4096.0f;

		acc16 = (res[2] << 6) | (res[3] >> 2);
		if (acc16 > UINT14_MAX / 2)
			acc16 -= UINT14_MAX;
		t[1] = ((float)acc16) / 4096.0f;

		acc16 = (res[4] << 6) | (res[5] >> 2);
		if (acc16 > UINT14_MAX / 2)
			acc16 -= UINT14_MAX;
		t[2] = ((float)acc16) / 4096.0f;

		/*	pc.printf("FXOS8700Q ACC: X=%1.4f(%x%x) Y=%1.4f(%x%x) Z=%1.4f(%x%x)\r\n", \
				t[0], res[0], res[1], \
				t[1], res[2], res[3], \
				t[2], res[4], res[5]\
			);		*/

		if (i < 100) {
			tx[i] = t[0];
			ty[i] = t[1];
			tz[i] = t[2];
			i++;
            hpx += 0.5*9.8*t[0];
            hpy += 0.5*9.8*t[1];
            hopl[i] = sqrt(hpx*hpx + hpy*hpy);
            if (hopl[i] > 5) Fcm[i] = 1;
            else             Fcm[i] = 0;
		}
		wait(0.1f);
	}
	/* Then, send data to pc*/
	for (int i = 0; i < 100; i++) {
		pc.printf("%1.3f %1.3f %1.3f %d %1.3f\r\n", tx[i], ty[i], tz[i], Fcm[i], hopl[i]);
		wait_us(0.05f);
	}
	Tout = false;	// reset Tout so it can run again
}
void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
	char t = addr;
	i2c.write(m_addr, &t, 1, true);
	i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
	i2c.write(m_addr, (char *)data, len);
}