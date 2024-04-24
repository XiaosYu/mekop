/*
 * tcs34725.c
 *
 *  Created on: Mar 31, 2024
 *      Author: 纸鸢
 */

#include "tcs34725.h"

COLOR_RGBC rgb;
COLOR_HSL hsl;

I2C_HandleTypeDef *I2cx;

void SwicthI2c(char sw)
{
	I2cx=&hi2c1;
}

char TCS34725_Read(uint8_t subAddr,unsigned char *dataBuffer, uint16_t bytesNumber)
{
	subAddr |= TCS34725_COMMAND_BIT;
	uint8_t sendadd = (TCS34725_ADDRESS << 1) | 0x00;
	if(HAL_I2C_Master_Transmit(I2cx,sendadd,&subAddr,1,1000)==0x00)
	{
		sendadd = (TCS34725_ADDRESS << 1) | 0x01;
		if(HAL_I2C_Master_Receive(I2cx,sendadd,dataBuffer,bytesNumber,1000)==0x00)
		{
			return 1;
		}

	}
	return 0;
}

char TCS34725_Write(uint8_t subAddr,uint8_t *dataBuffer,uint16_t bytesNumber)
{
	uint8_t sendadd = (TCS34725_ADDRESS << 1) | 0x00;
	uint8_t sendBuffer[10]={0,};
	sendBuffer[0]=subAddr|TCS34725_COMMAND_BIT;
	for(uint8_t i=1;i<=bytesNumber;i++)
	{
		sendBuffer[i]=dataBuffer[i-1];
	}
	if(HAL_I2C_Master_Transmit(I2cx,sendadd,sendBuffer,bytesNumber+1,1000)==0)
	{
		return 1;
	}

	return 0;
}


void TCS34725_SetIntegrationTime(uint8_t time)
{
	TCS34725_Write(TCS34725_ATIME,&time,1);
}
void TCS34725_SetGain(uint8_t gain)
{
	TCS34725_Write(TCS34725_CONTROL, &gain, 1);
}
void TCS34725_Enable(void)
{
	uint8_t cmd = TCS34725_ENABLE_PON;

	TCS34725_Write(TCS34725_ENABLE, &cmd, 1);
	cmd = TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN;
	TCS34725_Write(TCS34725_ENABLE, &cmd, 1);
}

unsigned char TCS34725_Init(void)
{
	unsigned char id=0;
	uint16_t number=0;
	uint8_t status = TCS34725_STATUS_AVALID;
	TCS34725_Read(TCS34725_ID,&id,1);
	//debug("Read I2C ID:%02X\r\n",id);
	if(id==0x4D || id ==0x44)
	{
		TCS34725_SetIntegrationTime(TCS34725_INTEGRATIONTIME_154MS);//转换时间
		TCS34725_SetGain(TCS34725_GAIN_16X);
		TCS34725_Enable();
		return 1;
	}
	return 0;
}


/*******************************************************************************
 * @brief TCS34725获取单个通道数据
 *
 * @return data - 该通道的转换值
*******************************************************************************/
uint16_t TCS34725_GetChannelData(uint8_t reg)
{
	uint8_t tmp[2] = {0,0};
	uint16_t data;

	TCS34725_Read(reg, tmp, 2);
	data = (tmp[1] << 8) | tmp[0];
	return data;
}
/*******************************************************************************
 * @brief TCS34725获取各个通道数据
 *
 * @return 1 - 转换完成，数据可用
 *   	   0 - 转换未完成，数据不可用
*******************************************************************************/
char TCS34725_GetRawData(COLOR_RGBC *rgbc)
{
	uint8_t status = TCS34725_STATUS_AVALID;

	TCS34725_Read(TCS34725_STATUS, &status, 1);

	if(status & TCS34725_STATUS_AVALID)
	{
		rgbc->c = TCS34725_GetChannelData(TCS34725_CDATAL);
		rgbc->r = TCS34725_GetChannelData(TCS34725_RDATAL);
		rgbc->g = TCS34725_GetChannelData(TCS34725_GDATAL);
		rgbc->b = TCS34725_GetChannelData(TCS34725_BDATAL);
		return 1;
	}
	return 0;
}
/******************************************************************************/
//RGB转HSL
void RGBtoHSL(COLOR_RGBC *Rgb, COLOR_HSL *Hsl)
{
	uint8_t maxVal,minVal,difVal;
	uint8_t r = Rgb->r*100/Rgb->c;   //[0-100]
	uint8_t g = Rgb->g*100/Rgb->c;
	uint8_t b = Rgb->b*100/Rgb->c;

	maxVal = max3v(r,g,b);
	minVal = min3v(r,g,b);
	difVal = maxVal-minVal;

	//计算亮度
	Hsl->l = (maxVal+minVal)/2;   //[0-100]

	if(maxVal == minVal)//若r=g=b,灰度
	{
		Hsl->h = 0;
		Hsl->s = 0;
	}
	else
	{
		//计算色调
		if(maxVal==r)
		{
			if(g>=b)
				Hsl->h = 60*(g-b)/difVal;
			else
				Hsl->h = 60*(g-b)/difVal+360;
		}
		else
			{
				if(maxVal==g)Hsl->h = 60*(b-r)/difVal+120;
				else
					if(maxVal==b)Hsl->h = 60*(r-g)/difVal+240;
			}

		//计算饱和度
		if(Hsl->l<=50)Hsl->s=difVal*100/(maxVal+minVal);  //[0-100]
		else
			Hsl->s=difVal*100/(200-(maxVal+minVal));
	}
}
/******************************************************************************/
//计算RGB的比例和Lux
void RGBto255RGB(COLOR_RGBC *Rgb, Sepan_RGBC *RGB255)
{
	double maxVal,minVal,difVal;
	double r_255=0.0,g_255=0.0,b_255=0.0;

	r_255 = (double)Rgb->r/Rgb->c*255;
	g_255 = (double)Rgb->g/Rgb->c*255;
	b_255 = (double)Rgb->b/Rgb->c*255;

	maxVal = max3v(r_255,g_255,b_255);
	r_255 = r_255/maxVal*255;
	g_255 = g_255/maxVal*255;
	b_255 = b_255/maxVal*255;

	RGB255->r = (unsigned char)r_255;
	RGB255->g = (unsigned char)g_255;
	RGB255->b = (unsigned char)b_255;
	double lux=(0.299*Rgb->r)+(0.587*Rgb->g)+(0.114*Rgb->b);
	RGB255->Lux = (unsigned short)lux;

	if(RGB255->r==255&&RGB255->g==255&&RGB255->b==255)
	{
		if(Rgb->c<255)
		{
			RGB255->r = 0;
			RGB255->g = 0;
			RGB255->b = 0;
		}
	}
}
//
//计算CCT色温
double calculateColorTemperature(COLOR_RGBC *Rgb)
{
	double trimX = 0;
  double trimY = 0;
  double trimZ = 0;
  double coorX = 0, coorY = 0;
  double CCT = 0;
  double n = 0;
  int R = Rgb->r;//255;
  int G = Rgb->g;//231;
  int B = Rgb->b;//131;

	//以下公式实现RGB转三刺激值
	trimX = 2.789 * R + 1.7517 * G + 1.1302 * B;
	trimY = 1 * R + 4.5907 * G + 0.0601 * B;
	trimZ = 0 * R + 0.0565 * G + 5.5943 * B;
	//以下公式实现三刺激值转色坐标
	coorX = trimX / (trimX + trimY + trimZ);
	coorY = trimY / (trimX + trimY + trimZ);
	n = (coorX - 0.3320) / (0.1858 - coorY);
	//以下公式实现色坐标转色温
	CCT = 437 * n * n * n + 3601 * n * n + 6831 * n + 5517;
	return CCT;
}



