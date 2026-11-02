#include "image.h"
#include "alpha.h"
#include "fft.h"
#include "window.h"
#include "pcm.h"
#include "stats.h"
#include <math.h>
#include <stdlib.h>

/* naming convension stuff:

nsutil_<area>_FunctionName_variant()

U8 - uint8_t data

destination pointer comes first
*/

static inline uint8_t mul_div_255(unsigned int a, unsigned int b)
{
	return (uint8_t)(a * b / 255);
}

int nsutil_alpha_Premultiply_RGB32(void *image, size_t image_stride, int width, int height)
{
	uint8_t *row = (uint8_t *)image;
	for (int y = 0; y < height; y++, row += image_stride)
	{
		uint8_t *pixel = row;
		for (int x = 0; x < width; x++, pixel += 4)
		{
			unsigned int alpha = pixel[3];
			pixel[0] = mul_div_255(pixel[0], alpha);
			pixel[1] = mul_div_255(pixel[1], alpha);
			pixel[2] = mul_div_255(pixel[2], alpha);
		}
	}
	return 0;
}

int nsutil_alpha_PremultiplyValue_RGB8(void *image, size_t image_stride, int width, int height, uint8_t alpha)
{
	uint8_t *row = (uint8_t *)image;
	for (int y = 0; y < height; y++, row += image_stride)
	{
		for (int x = 0; x < width; x++)
			row[x] = mul_div_255(row[x], alpha);
	}
	return 0;
}

/* Real FFT of 2^order samples, computed as a complex FFT of half that size.
   The result is left in place, packed as R(0), R(N/2), R(1), I(1), R(2), I(2), ... and is not scaled. */
struct nsutil_fft_struct_F32R
{
	int size;
	float *twiddle; // re,im pairs of e^(-2*pi*i*k/size), for k < size/2
	int *bit_reverse;
};

int nsutil_fft_Create_F32R(nsutil_fft_t *out_fft, int order, int /*accuracy*/)
{
	if (order < 1 || order > 24)
		return 1;

	nsutil_fft_struct_F32R *fft = (nsutil_fft_struct_F32R *)calloc(1, sizeof(nsutil_fft_struct_F32R));
	if (!fft)
		return 1;

	const int size = 1 << order, half = size / 2;
	fft->size = size;
	fft->twiddle = (float *)malloc(sizeof(float) * size);
	fft->bit_reverse = (int *)malloc(sizeof(int) * half);
	if (!fft->twiddle || !fft->bit_reverse)
	{
		nsutil_fft_Destroy_F32R(fft);
		return 1;
	}

	for (int k = 0; k < half; k++)
	{
		const double angle = -2.0 * 3.14159265358979323846 * k / size;
		fft->twiddle[k * 2] = (float)cos(angle);
		fft->twiddle[k * 2 + 1] = (float)sin(angle);
	}

	for (int i = 0, reversed = 0; i < half; i++)
	{
		fft->bit_reverse[i] = reversed;
		int bit = half >> 1;
		while (bit && (reversed & bit))
		{
			reversed ^= bit;
			bit >>= 1;
		}
		reversed |= bit;
	}

	*out_fft = fft;
	return 0;
}

int nsutil_fft_Forward_F32R_IP(nsutil_fft_t _fft, float *signal)
{
	const nsutil_fft_struct_F32R *fft = (const nsutil_fft_struct_F32R *)_fft;
	const int size = fft->size, half = size / 2;
	const float *twiddle = fft->twiddle;

	for (int i = 0; i < half; i++)
	{
		const int j = fft->bit_reverse[i];
		if (j > i)
		{
			float t = signal[i * 2]; signal[i * 2] = signal[j * 2]; signal[j * 2] = t;
			t = signal[i * 2 + 1]; signal[i * 2 + 1] = signal[j * 2 + 1]; signal[j * 2 + 1] = t;
		}
	}

	for (int span = 2; span <= half; span *= 2)
	{
		const int step = size / span;
		for (int start = 0; start < half; start += span)
		{
			for (int j = 0; j < span / 2; j++)
			{
				float *a = &signal[(start + j) * 2], *b = &signal[(start + j + span / 2) * 2];
				const float wr = twiddle[j * step * 2], wi = twiddle[j * step * 2 + 1];
				const float tr = wr * b[0] - wi * b[1], ti = wr * b[1] + wi * b[0];
				b[0] = a[0] - tr; b[1] = a[1] - ti;
				a[0] += tr; a[1] += ti;
			}
		}
	}

	/* separate the half size result into the spectrum of the real input */
	const float r0 = signal[0], i0 = signal[1];
	signal[0] = r0 + i0;
	signal[1] = r0 - i0;
	for (int k = 1; k <= half / 2; k++)
	{
		const int m = half - k;
		const float zr = signal[k * 2], zi = signal[k * 2 + 1], yr = signal[m * 2], yi = signal[m * 2 + 1];
		const float er = (zr + yr) * 0.5f, ei = (zi - yi) * 0.5f;
		const float or_ = (zi + yi) * 0.5f, oi = (yr - zr) * 0.5f;
		const float wr = twiddle[k * 2], wi = twiddle[k * 2 + 1];
		const float tr = wr * or_ - wi * oi, ti = wr * oi + wi * or_;
		signal[k * 2] = er + tr;
		signal[k * 2 + 1] = ei + ti;
		signal[m * 2] = er - tr;
		signal[m * 2 + 1] = ti - ei;
	}
	return 0;
}

int nsutil_fft_Destroy_F32R(nsutil_fft_t _fft)
{
	nsutil_fft_struct_F32R *fft = (nsutil_fft_struct_F32R *)_fft;
	if (fft)
	{
		free(fft->twiddle);
		free(fft->bit_reverse);
		free(fft);
	}
	return 0;
}

int nsutil_window_Hann_F32_IP(float *signal, size_t number_of_samples)
{
	if (number_of_samples < 2)
		return 0;

	const double scale = 2.0 * 3.14159265358979323846 / (double)(number_of_samples - 1);
	for (size_t i = 0; i < number_of_samples; i++)
		signal[i] *= (float)(0.5 * (1.0 - cos(scale * (double)i)));
	return 0;
}

int nsutil_window_FillHann_F32_IP(float *window, size_t number_of_samples)
{
	for (size_t i = 0; i < number_of_samples; i++)
		window[i] = 1.0f;
	return nsutil_window_Hann_F32_IP(window, number_of_samples);
}

int nsutil_window_Multiply_F32_IP(float *signal, const float *window, size_t number_of_samples)
{
	for (size_t i = 0; i < number_of_samples; i++)
		signal[i] *= window[i];
	return 0;
}

int nsutil_image_FillRectAlpha_RGB32(RGB32 *destination_image, size_t destination_stride /* bytes! */, uint32_t width, uint32_t height, RGB32 color, int alpha)
{
	uint8_t *row = (uint8_t *)destination_image;

	if (alpha == 255)
	{
		const uint8_t c[3] = {
			(uint8_t)((color & 0xFF0000) >> 16),
			(uint8_t)((color & 0xFF00) >> 8),
			(uint8_t)(color & 0xFF)
		};
		for (uint32_t y = 0; y < height; y++, row += destination_stride)
		{
			uint8_t *pixel = row;
			for (uint32_t x = 0; x < width; x++, pixel += 4)
			{
				pixel[0] = c[0];
				pixel[1] = c[1];
				pixel[2] = c[2];
			}
		}
	}
	else
	{
		const unsigned int c[3] = {
			((color & 0xFF0000) * alpha / 255) >> 16,
			((color & 0xFF00) * alpha / 255) >> 8,
			((color & 0xFF) * alpha / 255)
		};
		const unsigned int inverse = 255 - alpha;
		for (uint32_t y = 0; y < height; y++, row += destination_stride)
		{
			uint8_t *pixel = row;
			for (uint32_t x = 0; x < width; x++, pixel += 4)
			{
				for (int channel = 0; channel < 3; channel++)
				{
					const unsigned int value = mul_div_255(pixel[channel], inverse) + c[channel];
					pixel[channel] = (uint8_t)(value > 255 ? 255 : value);
				}
				pixel[3] = (uint8_t)inverse;
			}
		}
	}
	return 0;
}

int nsutil_stats_RMS_F32(const float *buffer, size_t num_samples, float *rms)
{
	double sum = 0.0;
	for (size_t i = 0; i < num_samples; i++)
		sum += (double)buffer[i] * (double)buffer[i];
	*rms = (float)sqrt(sum);
	return 0;
}
