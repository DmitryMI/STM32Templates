#include "gpio.h"

const int tab32[32] = {
     0,  9,  1, 10, 13, 21,  2, 29,
    11, 14, 16, 18, 22, 25,  3, 30,
     8, 12, 20, 28, 15, 17, 24,  7,
    19, 27, 23,  6, 26,  5,  4, 31};

int log2_32(uint32_t value)
{
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return tab32[(uint32_t)(value*0x07C4ACDD) >> 27];
}

rf24_gpio_pin_t encode_pin(GPIO_TypeDef* port, uint16_t pin)
{
	uint16_t port_base = 0;
	bool port_valid = false;

#ifdef GPIOA
	if(port == GPIOA)
	{
		port_base = 0 * 16;
		port_valid = true;
	}
#endif

#ifdef GPIOB
	if(port == GPIOB)
	{
		port_base = 1 * 16;
		port_valid = true;
	}
#endif

#ifdef GPIOC
	if(port == GPIOC)
	{
		port_base = 2 * 16;
		port_valid = true;
	}
#endif

#ifdef GPIOD
	if(port == GPIOD)
	{
		port_base = 3 * 16;
		port_valid = true;
	}
#endif

#ifdef GPIOE
	if(port == GPIOE)
	{
		port_base = 4 * 16;
		port_valid = true;
	}
#endif

#ifdef GPIOF
	if(port == GPIOF)
	{
		port_base == 5 * 16;
		port_valid = true;
	}
#endif

#ifdef GPIOG
	if(port == GPIOG)
	{
		port_base == 6 * 16;
		port_valid = true;
	}
#endif

	if(!port_valid)
	{
		return RF24_PIN_INVALID;
	}

	return static_cast<rf24_gpio_pin_t>(port_base + log2_32(pin));
}

GPIO_TypeDef* decode_pin(uint8_t pin, uint16_t* decoded_pin)
{
    GPIO_TypeDef* port;
    *decoded_pin = 1 << (pin % 16);

    switch (pin / 16) {
#if defined(GPIOA)
        case 0:
            port = GPIOA;
            break;
#endif
#if defined(GPIOB)
        case 1:
            port = GPIOB;
            break;
#endif
#if defined(GPIOC)
        case 2:
            port = GPIOC;
            break;
#endif
#if defined(GPIOD)
        case 3:
            port = GPIOD;
            break;
#endif
#if defined(GPIOE)
        case 4:
            port = GPIOE;
            break;
#endif
#if defined(GPIOF)
        case 5:
            port = GPIOF;
            break;
#endif
#if defined(GPIOG)
        case 6:
            port = GPIOG;
            break;
#endif
        default:
            break;
    }

    return port;
}

void pinMode(uint8_t pin, uint8_t direction)
{
    uint16_t decoded_pin;
    GPIO_TypeDef* port = decode_pin(pin, &decoded_pin);

    GPIO_InitTypeDef config;
    config.Pull = GPIO_NOPULL;
    config.Speed = GPIO_SPEED_FREQ_HIGH;
    config.Pin = 1 << (pin % 16);
    config.Mode = direction;
    HAL_GPIO_Init(port, &config);
}

void digitalWrite(uint8_t pin, uint8_t value)
{
    uint16_t decoded_pin;
    GPIO_TypeDef* port = decode_pin(pin, &decoded_pin);
    HAL_GPIO_WritePin(port, decoded_pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
