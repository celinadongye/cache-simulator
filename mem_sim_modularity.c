/***************************************************************************
 * *    Inf2C-CS Coursework 2: Cache Simulation
 * *
 * *    Instructor: Boris Grot
 * *
 * *    TA: Siavash Katebzadeh
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
/* Do not add any more header files */

/*
 * Various structures
 */
typedef enum {FIFO, LRU, Random} replacement_p;

const char* get_replacement_policy(uint32_t p) {
    switch(p) {
    case FIFO: return "FIFO";
    case LRU: return "LRU";
    case Random: return "Random";
    default: assert(0); return "";
    };
    return "";
}

typedef struct {
    uint32_t address;
} mem_access_t;


// UPDATE
// These are statistics for the cache and should be maintained by you.
typedef struct {
    uint32_t cache_hits;
    uint32_t cache_misses;
} result_t;


// FIFO USE
/*
 * Parameters for the cache that will be populated by the provided code skeleton.
 */

replacement_p replacement_policy = FIFO;
uint32_t associativity = 0;
uint32_t number_of_cache_blocks = 0;
uint32_t cache_block_size = 0;

// UPDATE
/*
 * Each of the variables below must be populated by you.
 */
uint32_t g_num_cache_tag_bits = 0; // tag storage bits
uint32_t g_cache_offset_bits= 0; // offset storage bits
result_t g_result;


/* Reads a memory access from the trace file and returns
 * 32-bit physical memory address
 */
mem_access_t read_transaction(FILE *ptr_file) {
    char buf[1002];
    char* token = NULL;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf, 1000, ptr_file)!= NULL) {
        /* Get the address */
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtoul(token, NULL, 16);
        return access;
    }

    /* If there are no more entries in the file return an address 0 */
    access.address = 0;
    return access;
}

void print_statistics(uint32_t num_cache_tag_bits, uint32_t cache_offset_bits, result_t* r) {
    /* Do Not Modify This Function */

    uint32_t cache_total_hits = r->cache_hits;
    uint32_t cache_total_misses = r->cache_misses;
    printf("CacheTagBits:%u\n", num_cache_tag_bits);
    printf("CacheOffsetBits:%u\n", cache_offset_bits);
    printf("Cache:hits:%u\n", r->cache_hits);
    printf("Cache:misses:%u\n", r->cache_misses);
    printf("Cache:hit-rate:%2.1f%%\n", cache_total_hits / (float)(cache_total_hits + cache_total_misses) * 100.0);
}

/*
 *
 * Add any global variables and/or functions here as needed.
 *
 */

// create block struct
typedef struct {
    uint32_t tag;        // tag of the address in the block
    int blockOccupied;   // if an address is stored there then it is 1, if block is empty then it is 0
    int fifo_index;      // keeps track of the first tag that was stored in the array      
    int access_times;    // keeps a record of the number of times the block was accessed, to conclude which one was the least recently used
} block;


uint32_t g_cache_index_bits = 0; // index bits

uint32_t cache_sets = 0;         // number of sets needed for the cache size depending on associativity


int main(int argc, char** argv) {
    time_t t;
    /* Intializes random number generator */
    /* Important: *DO NOT* call this function anywhere else. */
    srand((unsigned) time(&t));
    /* ----------------------------------------------------- */
    /* ----------------------------------------------------- */

    /*
     *
     * Read command-line parameters and initialize configuration variables.
     *
     */
    int improper_args = 0;
    char file[10000];
    if (argc < 6) {
        improper_args = 1;
        printf("Usage: ./mem_sim [replacement_policy: FIFO LRU Random] [associativity: 1 2 4 8 ...] [number_of_cache_blocks: 16 64 256 1024] [cache_block_size: 32 64] mem_trace.txt\n");
    } else  {
        /* argv[0] is program name, parameters start with argv[1] */
        if (strcmp(argv[1], "FIFO") == 0) {
            replacement_policy = FIFO;
        } else if (strcmp(argv[1], "LRU") == 0) {
            replacement_policy = LRU;
        } else if (strcmp(argv[1], "Random") == 0) {
            replacement_policy = Random;
        } else {
            improper_args = 1;
            printf("Usage: ./mem_sim [replacement_policy: FIFO LRU Random] [associativity: 1 2 4 8 ...] [number_of_cache_blocks: 16 64 256 1024] [cache_block_size: 32 64] mem_trace.txt\n");
        }
        associativity = atoi(argv[2]);
        number_of_cache_blocks = atoi(argv[3]);
        cache_block_size = atoi(argv[4]);
        strcpy(file, argv[5]);
    }
    if (improper_args) {
        exit(-1);
    }
    
    assert(number_of_cache_blocks == 16 || number_of_cache_blocks == 64 || number_of_cache_blocks == 256 || number_of_cache_blocks == 1024);
    assert(cache_block_size == 32 || cache_block_size == 64);
    assert(number_of_cache_blocks >= associativity);
    assert(associativity >= 1);

    printf("input:trace_file: %s\n", file);
    printf("input:replacement_policy: %s\n", get_replacement_policy(replacement_policy));
    printf("input:associativity: %u\n", associativity);
    printf("input:number_of_cache_blocks: %u\n", number_of_cache_blocks);
    printf("input:cache_block_size: %u\n", cache_block_size);
    printf("\n");
    //printf("aaaa");
    /* Open the file mem_trace.txt to read memory accesses */
    
    FILE *ptr_file;
    ptr_file = fopen(file,"r");
    if (!ptr_file) {
        printf("Unable to open the trace file: %s\n", file);
        exit(-1);
    }

    /* result structure is initialized for you. */
    memset(&g_result, 0, sizeof(result_t));
     
    /* Do not delete any of the lines below.
     * Use the following snippet and add your code to finish the task. */

    /* You may want to setup your Cache structure here. */

    //////////////////////////////////// CACHE STRUCTURE //////////////////////////////////////////

    // Calculate the bits needed for the offset, index, tag
    
    g_cache_offset_bits = log2(cache_block_size);
    g_cache_index_bits = log2(number_of_cache_blocks / associativity);
    g_num_cache_tag_bits = 32 - g_cache_offset_bits - g_cache_index_bits;

    // Calculate number of sets in cache depending on associativity to create the cache
    // The index is used to determine which cache set the address should go in
    cache_sets = number_of_cache_blocks / associativity;


    // Create 2D array for cache structure using pointers
    // First create a one dimensional array of type blocks for the sets
    
    block **myBlocks;
    
    myBlocks = (block**)malloc(cache_sets*sizeof(block*));
    // Then create the actual two dimensional array (sets x associativity) using the number of cache associativity
    for (int i = 0; i < cache_sets; i++) {
        myBlocks[i] = (block*)malloc(associativity*sizeof(block));
        for (int j = 0; j < associativity; j++) {
            // Initialise each block in the 2D array
            myBlocks[i][j].tag = 0;
            myBlocks[i][j].blockOccupied = 0;
            myBlocks[i][j].fifo_index = 0;
            myBlocks[i][j].access_times = 0;
        }
    }

    // Read address
    mem_access_t access;

    /* Loop until the whole trace file has been read. */
    while(1) {
        access = read_transaction(ptr_file);
        // If no transactions left, break out of loop.
        if (access.address == 0)
            break;

        /* Add your code here */
        
        // Get the bits for the offset, index and tag from the address 
        uint32_t address_tag = access.address >> (g_cache_index_bits + g_cache_offset_bits);
        uint32_t address_index;
        if (g_cache_index_bits == 0) {
            address_index = 0;
        } else {
            address_index = access.address << g_num_cache_tag_bits >> (g_num_cache_tag_bits + g_cache_offset_bits);
        }
 
        //printf("Tag: %u, Set index: %u, Offset: %u\n", address_tag, address_index, address_offset);
        // Store the address bits in the cache
        // 1. Set index determines the cache set
        // 2. For each block in a cache set we compare the block with the tag of the address (hit or miss)
        // 2.1. If block and tag match, look at valid bit, if it is 1 then it is a hit
        // 2.2. Else it is a miss
        

        // Fill the cache until full and then apply replacement policy (malloc expensive)
        int address_stored = 0;
        int evict_index = 0;
       
        for (int j = 0; j < associativity; j++) {
    
            if (myBlocks[address_index][j].blockOccupied == 0) {
                
                myBlocks[address_index][j].tag = address_tag;
                myBlocks[address_index][j].blockOccupied = 1;
                address_stored = 1;
                g_result.cache_misses++;
                //break;
            }
        
        }

        


/*
            else if (myBlocks[address_index][j].blockOccupied == 1) {

                if (myBlocks[address_index][j].tag = address_tag) {
                    g_result.cache_hits++;
                    address_stored = 1;
                    break;
                } 
            }
*/

            
            // When cache is full evict according to replacement policy
            if (address_stored == 0) {
                if (replacement_policy == FIFO) {

                }

                else if (replacement_policy == LRU) {

                }

                else if (replacement_policy = Random) {
                    if (address_stored == 0) {
                    // Store the address in a random block
                    // The only number that changes every time we execute the program is the time of our computer
                    // The code at the start of the maing takes the seed for random index as the time, so the seed changes and thus the random number value changes
                    // srand((unsigned) time(&t));             
                    random_index = rand() % (associativity);  // Random block number is between 0 and (associativity - 1)
                    myBlocks[address_index][random_index].tag = address_tag;
                    g_result.cache_misses++;
            }
                }
            }
        




        // If set is full store depending on replacement policy: FIFO, LRU, Random
        //printf("%u\n", address_index);
        //if (strcmp(get_replacement_policy(replacement_policy), "FIFO") == 0) {
        if (replacement_policy == FIFO) {                                         
            int address_stored = 0;                                                  // indicates whether the address has been stored in the 2D array or not
            int evict_index = 0;                                                     // index of the block to evict and replace, minimum index
        
                for (int j = 0; j < associativity; j++) {
                    if (myBlocks[address_index][j].blockOccupied == 0) { 
                                 
                            myBlocks[address_index][j].tag = address_tag;            // if the block is empty, save the address tag in the block tag
                            myBlocks[address_index][j].blockOccupied = 1;            // set the block to occupied
                            address_stored = 1;                                      // means that the address has been stored
                            g_result.cache_misses++;
                            break;
                    }
                    
                    else if (myBlocks[address_index][j].blockOccupied == 1) {        // check if the block is occupied
                        
                        if (myBlocks[address_index][j].tag == address_tag) {
                            g_result.cache_hits++;                                   // if address tag matches the block tag then it is a 'hit'
                            address_stored = 1;
                            break;
                        }
                    }
                }
               

                if (address_stored == 0) {
                    // Means that the blocks in the set are full and the address tag did not match any of block tags
                    // By the FIFO replacement policy, we evict the first block that was access and store our address tag there
                    int current_fifo_index = myBlocks[address_index][0].fifo_index;
                    for (int j = 0; j < associativity; j++) {
                        if (current_fifo_index > myBlocks[address_index][j].fifo_index) {
                            current_fifo_index = myBlocks[address_index][j].fifo_index;
                            evict_index = j;
                        }
                    }
                    myBlocks[address_index][evict_index].tag = address_tag;
                    g_result.cache_misses++;
                    myBlocks[address_index][evict_index].fifo_index += associativity;
                }

        }
         

        if (replacement_policy == LRU) {
            int address_stored = 0;                                                  // indicates whether the address has been stored in the 2D array or not
            int evict_index = 0;                                                     // index of the block to evict and replace, least recently used block
            
            

            for (int j = 0; j < associativity; j++) {
                    if (myBlocks[address_index][j].blockOccupied == 0) {          
                            myBlocks[address_index][j].tag = address_tag;            // if the block is empty, save the address tag in the block tag
                            myBlocks[address_index][j].blockOccupied = 1;            // set the block to occupied
                            myBlocks[address_index][j].access_times++;               
                            address_stored = 1;                                      // means that the address has been stored
                            g_result.cache_misses++;
                            break;
                    }
                    else if (myBlocks[address_index][j].blockOccupied == 1) {        // check if the block is occupied
                        if (myBlocks[address_index][j].tag == address_tag) {
                            myBlocks[address_index][j].access_times++;
                            g_result.cache_hits++;                                   // if address tag matches the block tag then it is a 'hit'
                            address_stored = 1;
                            break;
                        }
                    }
            }

            if (address_stored == 0) {
                // By LRU replacement policy, we evict the least recently used block and store our address in said block
                int least_used_block = myBlocks[address_index][0].access_times;
                for (int j = 0; j < associativity; j++) {
                    if (least_used_block > myBlocks[address_index][j].access_times) {
                        least_used_block = myBlocks[address_index][j].access_times;
                        evict_index = j;
                    }
                }
                myBlocks[address_index][evict_index].tag = address_tag;
                g_result.cache_misses++;
            }

        }
        

        if (replacement_policy == Random) {
            int address_stored = 0;                                                  // indicates whether the address has been stored in the 2D array or not
            int random_index = 0;                                                    // random index of block to replace when the blocks in the set are full

            for (int j = 0; j < associativity; j++) {
                    if (myBlocks[address_index][j].blockOccupied == 0) {          
                            myBlocks[address_index][j].tag = address_tag;            // if the block is empty, save the address tag in the block tag
                            myBlocks[address_index][j].blockOccupied = 1;            // set the block to occupied
                            address_stored = 1;                                      // means that the address has been stored
                            g_result.cache_misses++;
                            break;
                    }
                    else if (myBlocks[address_index][j].blockOccupied == 1) {        // check if the block is occupied
                        if (myBlocks[address_index][j].tag == address_tag) {
                            g_result.cache_hits++;                                   // if address tag matches the block tag then it is a 'hit'
                            address_stored = 1;
                            break;
                        }
                    }
            }

            if (address_stored == 0) {
                // Store the address in a random block
                // The only number that changes every time we execute the program is the time of our computer
                // The code at the start of the maing takes the seed for random index as the time, so the seed changes and thus the random number value changes
                // srand((unsigned) time(&t));             
                random_index = rand() % (associativity);  // Random block number is between 0 and (associativity - 1)
                myBlocks[address_index][random_index].tag = address_tag;
                g_result.cache_misses++;
            }

        }
 
       
    }
    // Free the 2D array because the allocated memory is not needed anymore
    free(myBlocks);



    /* Do not modify code below. */
    /* Make sure that all the parameters are appropriately populated. */
    print_statistics(g_num_cache_tag_bits, g_cache_offset_bits, &g_result);

    /* Close the trace file. */
    fclose(ptr_file);
    return 0;
}
