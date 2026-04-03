
#include <math.h>
#include <stdio.h>

#define TABLE_LEN 64
static float s_table[TABLE_LEN + 1];

static float sMinF(float a, float b)
{
	return (a < b) ? a : b;
}

static float sMaxF(float a, float b)
{
	return (a > b) ? a : b;
}

static float sExponentialVolumeEasing(float x)
{
	x = sMaxF(0.0f, sMinF(1.0f, x));

	const int i = (int)(x * (float)(TABLE_LEN));
	const float f = (x * (float)(TABLE_LEN)) - (float)(i);

	return s_table[i] + (s_table[i + 1] - s_table[i]) * f;
}

int main(void)
{
	// Alexander Thomas (2025)
	// Programming Volume Controls: Notice to Programmers of Audio Software and Hardware
	// https://www.dr-lex.be/info-stuff/volumecontrols.html

	printf("static const float s_table[TABLE_LEN + 1] = {\n");

	float x;
	for (int i = 0; i < TABLE_LEN; i += 1)
	{
		// const float b = 6.908754779f; // Proper exponential, Thomas's one, but it feels bad

		const float b = 6.908754779f / 2.0f; // Half way between proper exponential and cheap
		                                     // 'y = xx' easing function, which isn't that bad

		x = (float)(i) / (float)(TABLE_LEN - 1);
		x = (expf(b * fabsf(x)) - 1.0f) / (expf(b) - 1.0f);

		s_table[i] = x;
		printf("\t%ff,\n", x);
	}

	s_table[TABLE_LEN] = x;
	printf("\t%ff};\n", x);

	//

	FILE* fp = fopen("paranoia.bin", "wb");

	for (int i = -50; i < 300; i += 1)
	{
		const float x = (float)(i) / 199.0f;
		const float y = sExponentialVolumeEasing(x);
		fwrite(&y, sizeof(float), 1, fp);
	}

	fclose(fp);

	return 0;
}
