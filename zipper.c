// code written by nsulti
// reinventing the bicycle cuz very epic
// date started: no idea
// date finished: 
// made in: vim
// lang: pure c


// libraries + definitions
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define FLAG_EXTRACT (1 << 0)
#define FLAG_COMPRESS (1 << 1)
#define FLAG_HELP (1 << 2)
#define FLAG_VERBOSE (1 << 3)
#define FLAG_OUTPUT (1 << 4)

#define BLOCK_SIZE 8192
#define MAX_ARGUMENTS 5

#define _FILE_OFFSET_BITS 64

//structs
typedef struct node {
	unsigned char symbol;
	size_t freq;
	size_t id;
	
	struct node * left;
	struct node * right;	
} node;

// variables
const char help[] = "zipper (2026) -- an archive tool made by nsulti in leisure, do not expect extraordinary effects. the file must be in the same folder as program is. working with simple huffman tree using nodes.\n\nallowed arguments:\n\t-h -- help\n\t-x -- extract\n\t-c -- compress (if multiple files given, compresses into one archive)\n\t-v -- verbose output\n\t-o -- where to output\n\nusage: zipper [arguments] file1 file2...\n";



// declarations

// for both
void make_flags(char *arg, short * attrs);
void validate_flags(short * attrs);
void free_tree(node * root);
node * create_node(unsigned char symbol, size_t freq);
node * pop_min(node * node_tree[], int *size);
node * create_parent(node * node1, node * node2); 
node * huffman_tree(node * nodes[], int * size);
void create_codes(node * root, char * code, int step, char* table[]);

// for compression (yeaah...)
void frequency_analysis(size_t * freq_arr, char * filename, short *attrs);
void read_file_compress(char * filename, short *attrs, char * out);
void write_to_file_compress(char * codes[], char * file, char * output, short * attrs, uint64_t frequency_arr[]);
void compress_logic(char * filenames[], short attributes, int file_count, int argc, char * argv[], char * output);
uint64_t get_file_size(FILE * file);

// for extraction
void main_extraction_logic(char * archive, short attributes);
void get_frequency_table(uint64_t * freq_arr, FILE * archive, short attrs);
char * get_file_name_from_archive(FILE * archive); 
void decompress(FILE * archive, node* root, uint64_t size, char* filename, short attrs); 

// main
void main(int argc, char * argv[]) {
	// necessary variables
	unsigned short file_count = 0;
	char * filenames[argc];
	char * output;
	unsigned short attributes = 0;
	// bad case #1
	if (argc < 2) {
		fprintf(stderr, "zipper [ERROR]: missing file and/or attributes\n");
		exit(EXIT_FAILURE);
	}
	// checking flags and filenames
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && strlen(argv[i]) > 1) {
			make_flags(argv[i], &attributes);
			continue;
		}
		if (argv[i][0] == '-' && strlen(argv[i]) <= 1) {
			fprintf(stderr, "zipper [ERROR]: empty attribute\n");
			exit(EXIT_FAILURE);
		}
		filenames[file_count] = argv[i];
		file_count++;
	}
	validate_flags(&attributes);
	// working out the flags
	if (attributes & FLAG_HELP) {
		printf(help);
		exit(EXIT_SUCCESS);
	}
	
	if (attributes & FLAG_EXTRACT) {
		// main_extraction_logic...
		if (file_count > 1 || file_count == 0) { fprintf(stderr, "zipper [ERROR]: unable to extract more than one/no archive\n"); exit(EXIT_FAILURE);  };
		main_extraction_logic(filenames[0], attributes);
		printf("zipper [EXTRACT]: success! all files extracted from an archive.\n");
		exit(EXIT_SUCCESS);
	}
	if (attributes & FLAG_COMPRESS) {
		// compress_logic...
		compress_logic(filenames, attributes, file_count, argc, argv, output);
		printf("zipper [COMPRESS]: success! all files compressed into an archive.\n");
		exit(EXIT_SUCCESS);
	}
}



// function definitions
// file structure: HF(frequency table)(null_term)(size_of_file)(file_name + null_term to finish reading filename)(filebytes/will be read by fread + size_of_file)
// main logic for extraction
uint64_t file_counter_extraction = 1;

void main_extraction_logic(char * archive, short attributes) {
	FILE * input_archive = fopen(archive, "rb");
	if (input_archive == NULL) {
		fprintf(stderr, "zipper [ERROR]: failed to open input archive (does it exist?)\n");
		exit(EXIT_FAILURE);
	}

	if (attributes & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: getting file size\n");	
	}
	
	uint64_t file_size = get_file_size(input_archive);
	rewind(input_archive);

	if (file_size < 2) {
		fprintf(stderr, "zipper [ERROR]: file too small to be an archive\n");
		exit(EXIT_FAILURE);
	}
	
	if (attributes & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: checking headers\n");
	}
	
	char buffer_check_archive[2];	
	
	fread(buffer_check_archive, sizeof(char), 2, input_archive);
	
	if (strncmp(buffer_check_archive, "HF", 2) != 0) {
		fprintf(stderr, "zipper [ERROR]: header check failed\n");
		exit(EXIT_FAILURE);
	}

	if (attributes & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: header check success\n");
	}

	int c;
	while ((c = fgetc(input_archive)) != EOF) {
		ungetc(c, input_archive);

		uint64_t frequency_arr[256] = {0};
		node * node_array[256] = {NULL};
		int size = 0;
		node * root_of_one_tree;
			
		char code[256];
		char * table[256] = {NULL};
		if (attributes & FLAG_VERBOSE) { printf("zipper [VEROBSE]: getting to work on file %lld\n", file_counter_extraction); }
		

		// read frequency table -> find root
		get_frequency_table(frequency_arr, input_archive, attributes);
		for (int i = 0; i < 256; i++) {
			if (frequency_arr[i] > 0) { node_array[size] = create_node((char) i, frequency_arr[i]); size++; }
		}

		root_of_one_tree = huffman_tree(node_array, &size);
		
		if (attributes & FLAG_VERBOSE) { printf("zipper [VERBOSE]: root of tree of file %lld found\n", file_counter_extraction); }

		// get file size
		uint64_t file_size;
		fread(&file_size, sizeof(uint64_t), 1, input_archive);
		if (attributes & FLAG_VERBOSE) { printf("zipper [VERBOSE]: found size: %lld\n", file_size); };
		// get filename
		char * filename = get_file_name_from_archive(input_archive);
		if (attributes & FLAG_VERBOSE) { printf("zipper [VERBOSE]: found name of file %lld: %s\n", file_counter_extraction, filename); }
		// read bytes
		decompress(input_archive, root_of_one_tree, file_size, filename, attributes);

		// free allocated memory
		free_tree(root_of_one_tree);
		free(filename);
		file_counter_extraction++;
	}	
	fclose(input_archive);
	
	
	
}

void decompress(FILE * archive, node* root, uint64_t size, char* filename, short attrs) {
	FILE * output_file = fopen(filename, "wb");
	
	node * current = root;
	uint64_t bytes_written = 0;
	int ch;
		

	clearerr(archive);

	if (attrs & FLAG_VERBOSE) { printf("zipper [VERBOSE]: decompressing file: %s\n", filename); }
	while (bytes_written < size && (ch = fgetc(archive)) != EOF) {
		for (int i = 7; i >= 0; i--) {
			int bit = (ch >> i) & 1;
			
			if (bit == 0) {current = current->left;}
			else {current = current->right;}
			
			if (current->left == NULL && current->right == NULL) {
				fputc(current->symbol, output_file); 
				bytes_written++;
				current = root;
				if (bytes_written >= size) {
					break;
				}
			}

		}
	}
	if (attrs & FLAG_VERBOSE) { printf("zipper [VERBOSE]: file %s decompressed\n", filename); }
	fclose(output_file);
}


char * get_file_name_from_archive(FILE * archive) {
	char buffer[600];
	int index = 0;
	int d;

	while (((d = fgetc(archive)) != '\0') && index < 600) {
		buffer[index] = (char) d;
		index++;
	}

	buffer[index] = '\0';
	char * filename = malloc(index + 1);
	if (filename == NULL) {
		fprintf(stderr, "zipper [ERROR]: failed to allocate memory for filename\n");
	 	exit(EXIT_FAILURE);	
	}
	strcpy(filename, buffer);
	return filename;
	
}


void get_frequency_table(uint64_t * freq_arr, FILE * archive, short attrs) {
	int ch;	

	if (attrs & FLAG_VERBOSE) { printf("zipper [VERBOSE]: reading frequency table of file %i\n", file_counter_extraction); }
	
	// checking if we have first ascii table symbol (null term) as first with frequency, a crutch of sorts
	ch = fgetc(archive);
	
	if (ch == '\0') {
		fread(&freq_arr[ch], sizeof(uint64_t), 1, archive);
	} else {
		ungetc(ch, archive);
	}
	
	
	
	while ((ch = fgetc(archive)) != '\0' && ch != EOF) {
		fread(&freq_arr[ch], sizeof(uint64_t), 1, archive);
	}

	if (attrs & FLAG_VERBOSE) { printf("zipper [VERBOSE]: finished reading frequencies\n"); }	
}


// main logic for huffman tree and all the works (the compression stuff)
void compress_logic(char * filenames[], short attributes, int file_count, int argc, char * argv[], char * output) { 
	// yaaay
	printf("zipper [COMPRESS]: working...\n");
	// bad case #2 (i.e. someone puts flags and no filename, argc in bad case 1 wont detect it)
	if (file_count == 0) {
		fprintf(stderr, "zipper [ERROR]: no files to work with\n");
		exit(EXIT_FAILURE);
	}
	if ((attributes & FLAG_OUTPUT) && (strncmp(argv[argc - 1], "-o", 2) == 0)) {
		fprintf(stderr, "zipper [ERROR]: attribute -o provided, but no filename given\n");
		exit(EXIT_FAILURE);
	}


	output = filenames[file_count - 1];
	if (attributes & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: received archive filename: %s\n", output);
	}


	// main logic in different files
	for (int i = 0; i < file_count - 1; i++) {
		printf("zipper [WORKING]: received file \'%s\'\n", filenames[i]);
		read_file_compress(filenames[i], &attributes, output);
		printf("zipper [WORKING]: file \'%s\' done\n", filenames[i]);
	}
	
}

// no overflow!
void free_tree(node * root) {
	if (root == NULL) { return; }

	free_tree(root->left);
	free_tree(root->right);
	
	free(root);
}

// this was in main before i realised that i also had to EXTRACT files. lol.
void read_file_compress(char * filename, short *attrs, char * out) { 
	uint64_t frequency_arr[256] = {0}; // <--- used to count frequencies, very cool
	node * node_array[256]; // <--- no overflowing because yeah
	int size = 0; // <---- size of huffman tree (very necessary)

	char code[256]; // to create a code for a symbol from ascii table
	char * table[256] = {NULL};


	frequency_analysis(frequency_arr, filename, attrs);
		
	for (int i = 0; i < 256; i++) {
		if (frequency_arr[i] > 0) {
			node_array[size] = create_node((char) i, frequency_arr[i]);
			size++;
		}
	}
	
	node * root_of_tree = huffman_tree(node_array, &size);

	if (*attrs & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: root of tree made, creating codes...\n");
	}
	create_codes(root_of_tree, code, 0, table);

	if (*attrs & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: finished creating codes.\n");
	}
	
	// pls don't segfault pls dont segfault
	
	if (*attrs & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: encoding to output...\n");
	}

	write_to_file_compress(table, filename, out, attrs, frequency_arr);
	
	if (*attrs & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: finished writing to output\n"); 
	}
	
	if (*attrs & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: cleaning up the tree...\n");
	}

	free_tree(root_of_tree);
	
	if (*attrs & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: finished cleaning up, moving on.\n");
	}

}



// get file size
uint64_t get_file_size(FILE * file) {
	if (!file) {
		fprintf(stderr, "zipper [ERROR]: failed to open file to get file size\n");
		exit(EXIT_FAILURE);
	}

	fseeko(file, 0, SEEK_END);

	off_t signed_size = ftello(file);
	
	if (signed_size == -1) {
		fprintf(stderr, "zipper [ERROR]: failed getting file size\n");
		fclose(file);
		exit(EXIT_FAILURE);
	}
	
	uint64_t file_size = (uint64_t) signed_size;
	rewind(file);
	
	return file_size;
}


// writing table(s) to output file

bool fileExists = false;

void write_to_file_compress(char * codes[], char * file, char * output, short * attrs, uint64_t frequency_arr[]) { 
	if (*attrs & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: preparing input file \'%s\' for output to \'%s\'\n", file, output);
	}
	FILE * output_file = NULL;	
	if (!fileExists) {
		output_file = fopen(output, "wb");
		fileExists = true;
		fwrite("HF", 1, 2, output_file);
	} else {
		output_file = fopen(output, "ab");
	}
	if (output_file == NULL) {
		fprintf(stderr, "zipper [ERROR]: failed opening destination for writing\n");
		exit(EXIT_FAILURE);
	}
	
	FILE * input_file = fopen(file, "rb");
	if (input_file == NULL) {
		fprintf(stderr, "zipper [ERROR]: failed opening input file for reading (how does this happen if it already was read once?)\n");
		exit(EXIT_FAILURE);
	}
	rewind(input_file);

	unsigned char bit_buffer = 0;
	int bit_count = 0;
	int c;
	
	uint64_t file_size = get_file_size(input_file);
	

	for (int i = 0; i < 256; i++) {
		if (frequency_arr[i] > 0) {
			fputc((char) i, output_file);
			fwrite(&frequency_arr[i], sizeof(uint64_t), 1, output_file);
		}
	}
	
	// file structure: HF(frequency table)(null_term)(size_of_file)(file_name + null_term to finish reading filename)(filebytes/will be read by fread + size_of_file)
	char null_term = '\0';
	
	fwrite(&null_term, sizeof(char), 1, output_file); 
	fwrite(&file_size, sizeof(uint64_t), 1, output_file);
	fwrite(file, sizeof(char), strlen(file) + 1, output_file);
		

	if (*attrs & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: outputting bits into the target file...\n");
	}
	while ((c = fgetc(input_file)) != EOF) {
		char * code = codes[c];
		for (int i = 0; code[i] != '\0'; i++) {
			int bit = code[i] - '0';
			bit_buffer = (bit_buffer << 1) | bit;
			bit_count++;
			
			if (bit_count == 8) { 
				fputc(bit_buffer, output_file);
				bit_count = 0;
				bit_buffer = 0;
			}
		}
	}
	if (*attrs & FLAG_VERBOSE) {
		printf("zipper [VERBOSE]: finished archiving file \'%s\'\n", file);
	}

	if (bit_count > 0) {
		bit_buffer = bit_buffer << (8 - bit_count);
		fputc(bit_buffer, output_file);
	}
	fclose(output_file);
	fclose(input_file);

}

// edits attribute variable
void make_flags(char *arg, short * attrs) { // +
	switch (arg[1]) {
		case 'x': *attrs |= FLAG_EXTRACT; break;
		case 'c': *attrs |= FLAG_COMPRESS; break;
		case 'h': *attrs |= FLAG_HELP; break;
		case 'v': *attrs |= FLAG_VERBOSE; break;
		case 'o': *attrs |= FLAG_OUTPUT; break;
		default: fprintf(stderr, "zipper [ERROR]: invalid argument %s\nallowed arguments:\n\t-h -- help\n\t-x -- extract\n\t-c -- compress\n\t-v -- verbose output\n\t-o -- where to output\n\nusage: zipper [arguments] file1 file2... -o filename\n", arg); exit(EXIT_FAILURE);	
	}
}




// validates the attribute variable
void validate_flags(short * attrs) {
	if ((*attrs & FLAG_COMPRESS) && (*attrs & FLAG_EXTRACT)) {
		fprintf(stderr, "zipper [ERROR]: conflicting arguments (-c and -x)\n");
		exit(EXIT_FAILURE);
	} else if ((*attrs & FLAG_HELP) && ((*attrs & FLAG_EXTRACT) || (*attrs & FLAG_COMPRESS) || (*attrs & FLAG_VERBOSE) || (*attrs & FLAG_OUTPUT))) {
		fprintf(stderr, "zipper [ERROR]: conflicting arguments (-h)\n");
		exit(EXIT_FAILURE);
	} else if (!(*attrs & FLAG_OUTPUT) && !(*attrs & FLAG_HELP) && !(*attrs & FLAG_EXTRACT)) {
		fprintf(stderr, "zipper [ERROR]: no output specified (-o filename)\n");
		exit(EXIT_FAILURE);
	} else if ((*attrs & FLAG_OUTPUT) && (*attrs & FLAG_EXTRACT)) {
		fprintf(stderr, "zipper [ERROR]: conflicting arguments (-o and -x)\n");
	}
}




// reads files and increments stuff
void frequency_analysis(size_t * freq_arr, char * filename, short *attrs) {
	FILE * file = fopen(filename, "rb");
	size_t bytesRead;
	char buffer[BLOCK_SIZE];

	int block_counter = 0;
	
	if (get_file_size(file) == 0) {
		fprintf(stderr, "zipper [ERROR]: file empty! cant create frequency table\n");
		exit(EXIT_FAILURE);
	}

	if (file == NULL) {
		fprintf(stderr, "zipper [ERROR]: opened file returned NULL ptr (does it exist?)\n");
		exit(EXIT_FAILURE);
	}

	while ((bytesRead = fread(buffer, sizeof(unsigned char), BLOCK_SIZE, file)) > 0) {
		if (*attrs & FLAG_VERBOSE) {
			printf("zipper [VERBOSE]: reading block %i of file \'%s\' (block size: %i)\n", block_counter, filename, BLOCK_SIZE);
			block_counter++;
		}
		for (size_t i = 0; i < bytesRead; i++) {
			unsigned char currentByte = buffer[i];
			freq_arr[currentByte]++;
		}
	}
	if (ferror(file)) {
		fprintf(stderr, "zipper [ERROR]: error reading file.\n");
		exit(EXIT_FAILURE);
	} else if (!ferror(file) && (*attrs & FLAG_VERBOSE)) {
		printf("zipper [VERBOSE]: finishing reading file\n");
	}
	fclose(file);
} 




// creates nodes for the huffman tree out of symbols frequencies
node * create_node(unsigned char symbol, size_t freq) {
	node * newNode = (node *) malloc(sizeof(node)); // allocating some of that sweet memory heh
	if (newNode == NULL) {
		fprintf(stderr, "zipper [ERROR]: failed to create node (out of memory?)");
		exit(EXIT_FAILURE);
	}

	// initializing known symbols
	newNode->freq = freq;
	newNode->symbol = symbol;
	newNode->id = (int) symbol;

	// initializing stuff for trees
	newNode->left = NULL;
	newNode->right = NULL;
	return newNode;
}




// pops the minimum from huffman array of pointers FUCK THEM POINTERS
node * pop_min(node * node_tree[], int *size) {
	int min_node_index = 0;

	// self explanotary (if ur stoopid: find min frequency)
	for (int i = 1; i < *size; i++) {
		if (node_tree[i]->freq < node_tree[min_node_index]->freq) {
			min_node_index = i;
		} else if (node_tree[i]->freq == node_tree[min_node_index]->freq) {
			min_node_index = (node_tree[i]->id < node_tree[min_node_index]->id) ? i : min_node_index;
		}
	}

	node * min_node = node_tree[min_node_index];
	
	// this is hard to understand: we overwrite the minimum index with neighbours from right, since size is decreasing, we can use it to access that same last index
	for (int i = min_node_index; i < *size - 1; i++) {
		node_tree[i] = node_tree[i + 1];
	}

	(*size)--;

	return min_node;
}


int counter = 255;

// asdjklasdsalkjdfjfs literally identical  (is it efficient with create_node already there?)
node * create_parent(node * node1, node * node2) {
	node * newParentNode = (node *) malloc(sizeof(node));
	if (newParentNode == NULL) {
		fprintf(stderr, "zipper [ERROR]: failed to create parent (out of memory?)");
		exit(EXIT_FAILURE);
	}

	newParentNode->freq = node1->freq + node2->freq;
	newParentNode->symbol = '\0'; // fym null terminator
	newParentNode->id = counter++;

	newParentNode->left = node1;
	newParentNode->right = node2;

	return newParentNode;
	
}


// we create a huffman tree here hmm hmm
node * huffman_tree(node * nodes[], int * size) {
	while (*size > 1) {
		node * left = pop_min(nodes, size);
		node * right = pop_min(nodes, size);

		node * parent = create_parent(left, right);

		nodes[*size] = parent;
		(*size)++;
	}
	
	return nodes[0];
}


// we read the fucking huffman tree here to extract the damn codes (AHHHHHHHHHHHHHHH)
void create_codes(node * root, char * code, int step, char* table[]) {
	if (root == NULL) { return; } // incase shit goes very wrong

	if (root->left == NULL && root->right == NULL) {
		code[step] = '\0';
		table[(unsigned char) root->symbol] = (char *) malloc((step + 1) * sizeof(char)); // god is not here

		if (table[(unsigned char) root->symbol] == NULL) {
			fprintf(stderr, "zipper [ERROR]: failed creating code for a symbol with malloc (out of memory?)");
			exit(EXIT_FAILURE);
		}

		strcpy(table[(unsigned char) root->symbol], code);
		return;
	}
	
	code[step] = '0';
	create_codes(root->left, code, step + 1, table);

	code[step] = '1';
	create_codes(root->right, code, step + 1, table);
}
