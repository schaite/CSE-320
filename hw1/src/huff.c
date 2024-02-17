#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "huff.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif
#define END_OF_BLOCK (256)

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the nodes of the Huffman tree and other data have
 * been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/**
 * @brief Emits a description of the Huffman tree used to compress the current block.
 * @details This function emits, to the standard output, a description of the
 * Huffman tree used to compress the current block.  Refer to the assignment handout
 * for a detailed specification of the format of this description.
 */
//function for printing the post order traversal
void postorder_traversal(NODE *node, int *index, unsigned char *current_byte){
    if(node == NULL){
        return;
    }
    postorder_traversal(node->left,index, current_byte);
    postorder_traversal(node->right,index,current_byte);
    if(node->left==NULL && node->right==NULL){
        //o is already added, so no need to changen anything
    }
    else{
       *current_byte |= (1<<*index);
    }

    //printing each byte
    if(*index==0){
        fputc(*current_byte,stdout);
        *current_byte = 0;
        *index = 7;
    }
    else{
        (*index)--;
    }
}
//function to print the leaf nodes
void print_leaves(NODE *node){
    if(node==NULL){
        return;
    }
    print_leaves(node->left);
    print_leaves(node->right);
    if(node->left==NULL && node->right==NULL){
        if(node->symbol==END_OF_BLOCK){
            fputc(255,stdout);
            fputc(0,stdout);
        }
        else{
            fputc((char)node->symbol,stdout);
        }
    }
}

void emit_huffman_tree() {
    unsigned char current_byte = 0;
    int index = 7;

    //Output the number of nodes in big endian order

    //first byte containing 'n'
    unsigned char high_byte = (unsigned char)((num_nodes>>8)&0xff);
    //second byte containing 'n'
    unsigned char low_byte = (unsigned char)(num_nodes&0xff);
    fputc(high_byte,stdout);
    fputc(low_byte,stdout);

    //perform a postorder traversal of the tree
    current_byte = 0;
    index = 7;
    postorder_traversal(nodes,&index,&current_byte);
    //if any remaining bits are left
    if(index<7){
        fputc(current_byte, stdout);
    }
    //Output symbol values at the leaves of the tree
    print_leaves(nodes);
}

/**
 * @brief Reads a description of a Huffman tree and reconstructs the tree from
 * the description.
 * @details  This function reads, from the standard input, the description of a
 * Huffman tree in the format produced by emit_huffman_tree(), and it reconstructs
 * the tree from the description.  Refer to the assignment handout for a specification
 * of the format for this description, and a discussion of how the tree can be
 * reconstructed from it.
 *
 * @return 0 if the tree is read and reconstructed without error, 1 if EOF is
 * encountered at the start of a block, otherwise -1 if an error occurs.
 */
int assign_symbols(NODE *node){
    if(node==NULL) return 0;
    if(assign_symbols(node->left)!=0) return -1;
    if(assign_symbols(node->right)!=0) return -1;
    if(node->left == NULL&&node->right==NULL){
        int symbol_read = fgetc(stdin);
        if(symbol_read==EOF){
            return -1;
        }
        node->symbol = (short)symbol_read;
    }
    return 0;
}
int read_huffman_tree() {
    int stack_ptr = 0;
    //reading the first two bytes which holds the number of nodes
    num_nodes = fgetc(stdin);
    if(num_nodes == EOF){
        return -1;
    }
    num_nodes = num_nodes << 8;
    int temp = fgetc(stdin);
    if(num_nodes == EOF){
        return -1;
    }
    num_nodes |= temp;
    //initializing the nodes array declarded in huff.h
    for(int i = 0; i<num_nodes; i++){
        (nodes+i)->left = NULL;
        (nodes+i)->right = NULL;
        (nodes+i)->parent = NULL;
        (nodes+i)->symbol = -1;
    }
    //read and process bits until all nodes are accounted for
    int bits_read = 0;
    int bit_value;
    int current_byte = 0;
    int index = 0;

    while(bits_read < num_nodes-1){
        if(index==0){
            current_byte = fgetc(stdin);
            if(current_byte==EOF){
                return -1;
            }
        }
        //get the next bit
        bit_value = (current_byte>>(7-index))&0x1;
        //increment the index
        index = (index+1)%8;

        if(bit_value==1){

            if(stack_ptr<2) return -1;
            //pop two nodes from the stack
            NODE *rightChild = *(node_for_symbol+(--stack_ptr));
            NODE *leftChild = *(node_for_symbol+(--stack_ptr));

            //create a parent for the children
            NODE *parent = nodes+stack_ptr;
            parent->left = leftChild;
            parent->right = rightChild;

            //add parent to the top of the stack
            *(node_for_symbol+stack_ptr)=parent;
            stack_ptr++;
        }
        else{
            *(node_for_symbol+stack_ptr) = nodes + stack_ptr;
            stack_ptr++;
        }
        //increment the number of bits read
        bits_read++;
    }
    //assign symbols to the leaves!!!!
    if(assign_symbols(nodes)!=0){
        return -1;
    }
    return 0;
}

/**
 * @brief Reads one block of data from standard input and emits corresponding
 * compressed data to standard output.
 * @details This function reads raw binary data bytes from the standard input
 * until the specified block size has been read or until EOF is reached.
 * It then applies a data compression algorithm to the block and outputs the
 * compressed block to the standard output.  The block size parameter is
 * obtained from the global_options variable.
 *
 * @return 0 if compression completes without error, -1 if an error occurs.
 */
void insert_node(int *queue_size, NODE *new_node){
    int i;
    for(i = *queue_size; i>0; i--){
        if((nodes+i-1)->weight>new_node->weight){
            *(nodes+i) = *(nodes+i-1);
        }
        else{
            break;
        }
    }
    *(nodes+i) = *new_node;
    (*queue_size)++;
}
//initializes the leaves with
void init_leaves(int *queue_size){
    NODE *node_ptr = nodes;
    for(int i = 0; i<MAX_SYMBOLS; i++,node_ptr++){
        if(node_ptr->weight>0){
            insert_node(queue_size,node_ptr);
        }
    }
}
NODE create_parent_node(NODE *left_child, NODE *right_child){
    NODE parent_node;
    parent_node.left = left_child;
    parent_node.right = right_child;
    parent_node.weight = left_child->weight + right_child->weight;
    parent_node.symbol=-1;//indicates internal node
    return parent_node;
}
void build_huffman_tree(int queue_size){
    while(queue_size>1){
        NODE *left_child = nodes;
        NODE *right_child = nodes+1;
        NODE parent_node =  create_parent_node(left_child,right_child);
        //clean the left and rightchild space in the array
        for(int i = 2; i<queue_size; i++){
            *(nodes+i-2) = *(nodes+i);
        }
        queue_size-=2;
        //insert the new parent
        insert_node(&queue_size,&parent_node);
        queue_size++;
    }
}

int compress_block() {
    int queue_size = 0;

    //1. Initialize the histogram with all possible characters
    for(int i = 0; i<MAX_SYMBOLS; i++){
        NODE *current_node = nodes+i;
        current_node->left = NULL;
        current_node->right = NULL;
        current_node->parent = NULL;
        current_node->symbol = i;
        current_node->weight = 0;
    }
    //get the blocksize, add one as it was substracted while adding the blocksize
    int block_size = ((global_options>>16)&0xFFFF)+1;
    //2. Add frequencies for all symbols
    //bit counter for the block
    int count = 0;
    while(count<block_size){
        int ch = fgetc(stdin);
        if(ch == EOF){
            if(ferror(stdin)){
                return -1;
            }
            break;
        }
        //increment weight for the character encountered
        (nodes+ch)->weight++;
        //increment bit counter
        count++;
    }
    //set the END marker to -1
    (nodes+MAX_SYMBOLS-1)->weight = -1;
    //3. Initialize leaf nodes for non-zero frequesncies
    init_leaves(&queue_size);
    //4.Build the huffman tree
    build_huffman_tree(queue_size);
    //5. Emit_huffman_tree
    emit_huffman_tree();
    return 0;
}

/**
 * @brief Reads one block of compressed data from standard input and writes
 * the corresponding uncompressed data to standard output.
 * @details This function reads one block of compressed data from the standard
 * input, it decompresses the block, and it outputs the uncompressed data to
 * the standard output.  The input data blocks are assumed to be in the format
 * produced by compress().  If EOF is encountered before a complete block has
 * been read, it is an error.
 *
 * @return 0 if decompression completes without error, -1 if an error occurs.
 */
int decompress_block() {
    NODE *current_node =  nodes;
    debug("nooooooooooooooo");
    read_huffman_tree();
    debug("read");
    int byte;
    while((byte=fgetc(stdin))!=EOF){
        for(int i = 7; i>=0;i--){
            int current_bit = (byte>>i)&1;
            if(current_bit==0){
                current_node = current_node->left;
            }
            else{
                current_node=current_node->right;
            }
            if(current_node->left==NULL&&current_node->right==NULL){
                if(current_node->symbol == END_OF_BLOCK){
                    return 0;
                }
              //debug("%c",current_node->symbol);
                fputc(current_node->symbol,stdout);
                current_node = nodes;
            }
        }
    }
    debug("please work");
    return (feof(stdin)&& !ferror(stdin))?0:-1;
}




/**
 * @brief Reads raw data from standard input, writes compressed data to
 * standard output.
 * @details This function reads raw binary data bytes from the standard input in
 * blocks of up to a specified maximum number of bytes or until EOF is reached,
 * it applies a data compression algorithm to each block, and it outputs the
 * compressed blocks to standard output.  The block size parameter is obtained
 * from the global_options variable.
 *
 * @return 0 if compression completes without error, -1 if an error occurs.
 */
int compress() {
    while(!feof(stdin)){
        if(compress_block()==-1){
            return -1;
        }
    }
    fflush(stdout);
    return 0;
    //abort();
}

/**
 * @brief Reads compressed data from standard input, writes uncompressed
 * data to standard output.
 * @details This function reads blocks of compressed data from the standard
 * input until EOF is reached, it decompresses each block, and it outputs
 * the uncompressed data to the standard output.  The input data blocks
 * are assumed to be in the format produced by compress().
 *
 * @return 0 if decompression completes without error, -1 if an error occurs.
 */
int decompress() {
    debug("%s","stressssss");
    while(!feof(stdin)){
        debug("ajdgagdakjgd");
        if(decompress_block()==-1){
            return -1;
        }
    }
    fflush(stdout);
    return 0;
}



/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
//Helper functions
int is_digit(char ch){
    return (ch>='0'&&ch <= '9');
}
int string_to_int(char *str){
    int result = 0;
    while(*str != '\0' && is_digit(*str)){
        result = result*10 + (*str-'0');
        str++;
    }
    return result;
}
int valid_block_size(char *block_size){
    char *ptr = block_size;
    //confirm that block_size only contains digits
    while(*ptr!='\0'){
        if(!is_digit(*ptr)){
            return 0;
        }
        ptr++;
    }
    //confirm blocksize if within range
    int size = string_to_int(block_size);
    return (size>=1024 && size <=65536);

}
int validargs(int argc, char **argv)
{
    //initialize global_options to default value
    global_options = 0x0;
    //No flags are provided
    if(argc==1){
        return -1;
    }
    //iteragte over the args
    for(int i = 1; i<argc; i++){
        //arg contains current argument
        char *arg = *(argv+i);
        //if the arg is -, check what's the next arg and set the global option.
        if(*arg=='-'){
            switch(*(arg+1)){
            case 'h':
                if(i==1){
                    global_options |= 0x1; //-h bit set
                    return 0; //no arg before -h, success
                }
                else{
                    return -1; //-h is not the first flag
                }
                break;
            case 'c':
                if(global_options){//checks if there were some flag before
                    return -1;
                }
                else{
                    if(argc==i+1){
                        global_options |= 0xffff0002;
                        return 0;
                    }
                    else{
                        global_options |= 0x2;
                    }
                }
                break;
            case 'd':
                if(global_options){//checks if some flag was already set
                    return -1;
                }
                else{
                    if(i==argc-1){
                        global_options |= 0xffff0004;
                        return 0;
                    }
                    else{
                        return -1;
                    }
                }
                break;
            case 'b':
                i++;
                //check blocksize
                if((i>=argc) || (!valid_block_size(*(argv+i)))){
                    return -1;
                }
                //check if -c has been set
                else if(global_options==0x2){
                    /*
                    * Check blocksize is given
                    * Check that the given blocksize is a number in range [1024,65536]
                    * Set the blocksize in global_options
                    */
                    int block_size =string_to_int(*(argv+i))-1;
                    global_options &= 0x0000ffff; //clear the previous blocksize
                    global_options |= ((block_size<<16)&0xffff0000);
                    return 0;
                }
                else{
                    return -1; //-b option came although -c wasn't set
                }
                break;
            default: return -1;
                break;
            }
        }

    }
    return -1;
}