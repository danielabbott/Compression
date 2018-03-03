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

	// read input data

	FILE * file = fopen(argv[1], "r");
	assert(file != NULL);

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	uint8_t * inputData = malloc(size);
	assert(inputData);
	assert(fread(inputData, size, 1, file) == 1);

	// open output file

	FILE * outputFile = fopen(argv[2], "w");
	assert(outputFile != NULL);

	for(unsigned int i = 0; i < size;) {
		unsigned int controlByte = inputData[i++];
		if(i >= size) break;

		// number of times the byte is repeated
		// or length of uncompressed data
		unsigned int count = controlByte & 0x3f;

		if(controlByte & 0x80) {
			// 14-bit count, add extra 8 bits
			count |= inputData[i++] << 6;
			if(i >= size) break;
		}

		// undo the offset applied to count
		count++;


		int isPlainData = (controlByte & 0x7f) >> 6;

		if(isPlainData) {
			if(i+count > size) break;

			// write data straight to output file
			assert(fwrite(&inputData[i], count, 1, outputFile) == 1);
			i += count;
		}
		else {
			uint8_t data = inputData[i++];

			// write the byte 'count' times
			for(unsigned int j = 0; j < count; j++) {
				assert(fwrite(&data, 1, 1, outputFile) == 1);
			}
		}
	}

	fclose(file);
	fclose(outputFile);

	free(inputData);

	return 0;
}

