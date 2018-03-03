#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char ** argv)
{
	if(argc != 3) {
		puts("Expected two parameters (input file name and output file name)\n");
		return 1;
	}

	// read input file

	FILE * file = fopen(argv[1], "r");
	assert(file != NULL);

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	assert(size);
	rewind(file);

	uint8_t * inputData = malloc(size);
	assert(inputData);
	assert(fread(inputData, size, 1, file) == 1);

	// open output file

	FILE * outputFile = fopen(argv[2], "w");
	assert(outputFile != NULL);

	// compress

	unsigned int i;
	for(i = 0; i < size-1;) {
		uint8_t x = inputData[i++];
		
		if(inputData[i] == x) {
			// Repeating bytes

			// that last two bytes are known to be equal
			unsigned int count = 2;

			// iterate until end of file or until the limit of the 14-bit integer
			i++;
			for(; i < size && count < 16384;) {
				if(inputData[i] == x) {
					count++;
					i++;
				}
				else {
					// end of repeating bytes
					break;
				}
			}

			// count is offset by one because a value of 0 would otherwise be useless
			count--;

			if(count > 63) {
				uint8_t data[3];
				// 14-bit number for count, repeating byte
				data[0] = 0x80 | (count & 0x3f);
				data[1] = count >> 6;
				data[2] = x;
				assert(fwrite(data, 3, 1, outputFile) == 1);
			}
			else {
				// 6-bit number format, repeating byte
				uint16_t data = (x << 8) | count;
				assert(fwrite(&data, 2, 1, outputFile) == 1);
			}
		}
		else {
			unsigned int startIndex = i-1;
			unsigned int count = 2;

			uint8_t last = inputData[i];
			i++;

			for(; i < size && count < 16384;) {
				uint8_t y = inputData[i];

				if(y == last) {
					// repeating data, backtrack to previous byte
					count--;
					i--;
					break;
				} else {
					count++;
					i++;
				}

				last = y;
			}

			// write metadata byte(s)

			if(count > 64) {
				uint8_t data[2];
				// 14-bit number format, uncompressed data
				data[0] = 0xc0 | ((count-1) & 0x3f);
				data[1] = (count-1) >> 6;
				assert(fwrite(data, 2, 1, outputFile) == 1);
			}
			else {
				// 6-bit number format, uncompressed data
				uint8_t data = 0x40 | (count-1);
				assert(fwrite(&data, 1, 1, outputFile) == 1);
			}

			// write the data
			assert(fwrite(&inputData[startIndex], count, 1, outputFile) == 1);
		}


	}

	// final byte

	if(size-i) {
		// 6-bit number format, 1 byte, uncompressed data
		uint16_t d = ((1 << 6) | 0x40) | (inputData[i] << 8);
		assert(fwrite(&d, 2, 1, outputFile) == 1);
	}


	fclose(file);
	fclose(outputFile);

	free(inputData);

	return 0;
}

