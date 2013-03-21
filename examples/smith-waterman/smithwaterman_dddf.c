#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "ocr-dddf.h"

#define DDDF_HOME(id) column_distribution(id, num_ranks, n_tiles_width, num_tiles_in_outer_width, tile_id_offset)

#define TILE_ID_START(i,j) tile_id_start(i, j, n_tiles_width, tile_id_offset)

int tile_id_start(int i, int j, int n_tiles_width, int tile_id_offset) {
	int id = (((i * n_tiles_width) + j) * 3) + tile_id_offset;
	//printf("TILE(%d, %d) ID %d\n", i, j, id);
	return id;
}

#define DIAG_DEP(i,j) diag_dep_fn(i, j, n_tiles_width, tile_id_offset)
#define ROW_DEP(i,j) row_dep_fn(i, j, n_tiles_width, tile_id_offset)
#define COL_DEP(i,j) col_dep_fn(i, j, n_tiles_width, tile_id_offset)
int diag_dep_fn(int i, int j , int n_tiles_width, int tile_id_offset) { return (TILE_ID_START(i-1,j-1)); }
int row_dep_fn(int i, int j, int n_tiles_width, int tile_id_offset)  { return (TILE_ID_START(i-1,j)+1); }
int col_dep_fn(int i, int j, int n_tiles_width, int tile_id_offset)  { return (TILE_ID_START(i,j-1)+2); }


#define GAP_PENALTY -1
#define TRANSITION_PENALTY -2
#define TRANSVERSION_PENALTY -4
#define MATCH 2

#define FLAGS 0xdead
#define PROPERTIES 0xdead

enum Nucleotide {GAP=0, ADENINE, CYTOSINE, GUANINE, THYMINE};

signed char char_mapping ( char c ) {
    signed char to_be_returned = -1;
    switch(c) {
        case '_': to_be_returned = GAP; break;
        case 'A': to_be_returned = ADENINE; break;
        case 'C': to_be_returned = CYTOSINE; break;
        case 'G': to_be_returned = GUANINE; break;
        case 'T': to_be_returned = THYMINE; break;
    }
    return to_be_returned;
}

static char alignment_score_matrix[5][5] =
{
    {GAP_PENALTY,GAP_PENALTY,GAP_PENALTY,GAP_PENALTY,GAP_PENALTY},
    {GAP_PENALTY,MATCH,TRANSVERSION_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY},
    {GAP_PENALTY,TRANSVERSION_PENALTY, MATCH,TRANSVERSION_PENALTY,TRANSITION_PENALTY},
    {GAP_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY, MATCH,TRANSVERSION_PENALTY},
    {GAP_PENALTY,TRANSVERSION_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY, MATCH}
};

size_t clear_whitespaces_do_mapping ( signed char* buffer, long size ) {
    size_t non_ws_index = 0, traverse_index = 0;

    while ( traverse_index < (size_t)size ) {
        char curr_char = buffer[traverse_index];
        switch ( curr_char ) {
            case 'A': case 'C': case 'G': case 'T':
                /*this used to be a copy not also does mapping*/
                buffer[non_ws_index++] = char_mapping(curr_char);
                break;
        }
        ++traverse_index;
    }
    return non_ws_index;
}

signed char* read_file( FILE* file, size_t* n_chars ) {
    fseek (file, 0L, SEEK_END);
    long file_size = ftell (file);
    fseek (file, 0L, SEEK_SET);

    signed char *file_buffer = (signed char *)malloc((1+file_size)*sizeof(signed char));

    fread(file_buffer, sizeof(signed char), file_size, file);
    file_buffer[file_size] = '\n';

    /* shams' sample inputs have newlines in them */
    *n_chars = clear_whitespaces_do_mapping(file_buffer, file_size);
    return file_buffer;
}

int column_distribution(int id, int num_ranks, int n_tiles_width, int num_tiles_in_outer_width, int tile_id_offset) {
    int rank = ((((id - tile_id_offset) / 3) % n_tiles_width) % num_tiles_in_outer_width) % num_ranks;
	//printf("id: %d rank %d\n", id, rank);
	return rank;
}

u8 smith_waterman_kernel ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int index, ii, jj;

    intptr_t* typed_paramv = *paramv;
    int rank = (int) typed_paramv[0];
    int num_ranks = (int) typed_paramv[1];
    signed char* string1 = (signed char* ) typed_paramv[2];
    signed char* string2 = (signed char* ) typed_paramv[3];
    int tile_width = (int) typed_paramv[4];
    int tile_height = (int) typed_paramv[5];
    int outer_tile_width = (int) typed_paramv[6];
    int outer_tile_height = (int) typed_paramv[7];
    int* diags_initial_row = (int*) typed_paramv[8];
    int* diags_initial_col = (int*) typed_paramv[9];
    int** data_initial_row = (int**) typed_paramv[10];
    int** data_initial_col = (int**) typed_paramv[11];
    int n_tiles_width = (int)typed_paramv[12];
    int n_tiles_height = (int) typed_paramv[13];
    int i = (int) typed_paramv[14];
    int j = (int) typed_paramv[15];

	int num_tiles_in_outer_width = outer_tile_width / tile_width;
	int tile_id_offset = num_ranks + 1;
	printf("Running tile (%d, %d) on rank %d\n", i, j, rank);
/*
	int * left_tile_right_column;
	int * above_tile_bottom_row;
	int * diagonal_tile_bottom_right;

	if (i==0) {
		if (j==0) {
			left_tile_right_column = data_initial_col[i];
			above_tile_bottom_row = data_initial_row[j];
			diagonal_tile_bottom_right = &(diags_initial_row[j]);
		} else {
			left_tile_right_column = (int *) depv[0].ptr;
			above_tile_bottom_row = data_initial_row[j];
			diagonal_tile_bottom_right = &(diags_initial_row[j]);
		}
	} else {
		if (j==0) {
			left_tile_right_column = data_initial_col[i];
			above_tile_bottom_row = (int *) depv[0].ptr;
			diagonal_tile_bottom_right = &(diags_initial_col[i]);
		} else {
		}
	}
*/
	int * diagonal_tile_bottom_right = (int *) depv[0].ptr;
	int * above_tile_bottom_row = (int *) depv[1].ptr;
	int * left_tile_right_column = (int *) depv[2].ptr;

    int  * curr_tile_tmp = (int*)malloc(sizeof(int)*(1+tile_width)*(1+tile_height));
    int ** curr_tile = (int**)malloc(sizeof(int*)*(1+tile_height));
    for (index = 0; index < tile_height+1; ++index) {
        curr_tile[index] = &curr_tile_tmp[index*(1+tile_width)];
    }

    curr_tile[0][0] = diagonal_tile_bottom_right[0];
    for ( index = 1; index < tile_height+1; ++index ) {
        curr_tile[index][0] = left_tile_right_column[index-1];
    }

    for ( index = 1; index < tile_width+1; ++index ) {
        curr_tile[0][index] = above_tile_bottom_row[index-1];
    }

    for ( ii = 1; ii < tile_height+1; ++ii ) {
        for ( jj = 1; jj < tile_width+1; ++jj ) {
            signed char char_from_1 = string1[(j-1)*tile_width+(jj-1)];
            signed char char_from_2 = string2[(i-1)*tile_height+(ii-1)];

            int diag_score = curr_tile[ii-1][jj-1] + alignment_score_matrix[char_from_2][char_from_1];
            int left_score = curr_tile[ii  ][jj-1] + alignment_score_matrix[char_from_1][GAP];
            int  top_score = curr_tile[ii-1][jj  ] + alignment_score_matrix[GAP][char_from_2];

            int bigger_of_left_top = (left_score > top_score) ? left_score : top_score;
            curr_tile[ii][jj] = (bigger_of_left_top > diag_score) ? bigger_of_left_top : diag_score;
        }
    }

    ocrGuid_t db_guid_i_j_br;
    void* db_guid_i_j_br_data;
    ocrDbCreate( &db_guid_i_j_br, &db_guid_i_j_br_data, sizeof(int), FLAGS, NULL, NO_ALLOC );

    int* curr_bottom_right = (int*)db_guid_i_j_br_data;
    curr_bottom_right[0] = curr_tile[tile_height][tile_width];
    OCR_DDDF_SATISFY(TILE_ID_START(i,j), db_guid_i_j_br);

    ocrGuid_t db_guid_i_j_rc;
    void* db_guid_i_j_rc_data;
    ocrDbCreate( &db_guid_i_j_rc, &db_guid_i_j_rc_data, sizeof(int)*tile_height, FLAGS, NULL, NO_ALLOC );

    int* curr_right_column = (int*)db_guid_i_j_rc_data;
    for ( index = 0; index < tile_height; ++index ) {
        curr_right_column[index] = curr_tile[index+1][tile_width];
    }
    OCR_DDDF_SATISFY(TILE_ID_START(i,j)+2, db_guid_i_j_rc);

    ocrGuid_t db_guid_i_j_brow;
    void* db_guid_i_j_brow_data;
    ocrDbCreate( &db_guid_i_j_brow, &db_guid_i_j_brow_data, sizeof(int)*tile_width, FLAGS, NULL, NO_ALLOC );

    int* curr_bottom_row = (int*)db_guid_i_j_brow_data;
    for ( index = 0; index < tile_width; ++index ) {
        curr_bottom_row[index] = curr_tile[tile_height][index+1];
    }

    OCR_DDDF_SATISFY(TILE_ID_START(i,j)+1, db_guid_i_j_brow);

    free(curr_tile);
    free(curr_tile_tmp);
    if ( (i == (n_tiles_height-1)) && (j == (n_tiles_width-1))) {
//        fprintf(stdout, "score: %d\n", curr_bottom_row[tile_width-1]);
//        ocrFinish();
    }
}

u8 smith_waterman_init (signed char * string1, signed char * string2, 
						int tile_width, int tile_height, 
						int outer_tile_width, int outer_tile_height, 
						int n_char_in_file_1, int n_char_in_file_2) 
{
	int i, j;
	int rank = OCR_DDDF_RANK(); 
	int num_ranks = OCR_DDDF_NUM_RANKS();
	int tile_id_offset = num_ranks + 1;
	int n_tiles_width = n_char_in_file_1/tile_width;
	int n_tiles_height = n_char_in_file_2/tile_height;
	int num_tiles_in_outer_width = outer_tile_width / tile_width;

/*	int ** data_initial_row = (int**) malloc (sizeof(int*) * n_tiles_width);
	int ** data_initial_col = (int**) malloc (sizeof(int*) * n_tiles_height);
	int * diags_initial_row = (int*) malloc (sizeof(int) * (n_tiles_width+1));
	int * diags_initial_col = (int*) malloc (sizeof(int) * (n_tiles_height+1));
*/
	ocrGuid_t * data_initial_row = (ocrGuid_t*) malloc (sizeof(ocrGuid_t) * n_tiles_width+1);
	ocrGuid_t * data_initial_col = (ocrGuid_t*) malloc (sizeof(ocrGuid_t) * n_tiles_height+1);
	ocrGuid_t * diags_initial_row = (ocrGuid_t*) malloc (sizeof(ocrGuid_t) * (n_tiles_width+1));
	ocrGuid_t * diags_initial_col = (ocrGuid_t*) malloc (sizeof(ocrGuid_t) * (n_tiles_height+1));
	for (i = 0; i < n_tiles_width+1; i++) {
        ocrEventCreate(&(data_initial_row[i]), OCR_EVENT_STICKY_T, true);
        ocrEventCreate(&(diags_initial_row[i]), OCR_EVENT_STICKY_T, true);
	}

	for (i = 0; i < n_tiles_height+1; i++) {
        ocrEventCreate(&(data_initial_col[i]), OCR_EVENT_STICKY_T, true);
        ocrEventCreate(&(diags_initial_col[i]), OCR_EVENT_STICKY_T, true);
	}

    ocrGuid_t db_guid_0_0_br;
    void* db_guid_0_0_br_data;
    ocrDbCreate( &db_guid_0_0_br, &db_guid_0_0_br_data, sizeof(int), FLAGS, NULL, NO_ALLOC );
    int* allocated = (int*)db_guid_0_0_br_data;
    allocated[0] = 0;
    ocrEventSatisfy(diags_initial_row[0], db_guid_0_0_br);
    ocrEventSatisfy(diags_initial_col[0], db_guid_0_0_br);

    for ( j = 1; j < n_tiles_width + 1; ++j ) {
        ocrGuid_t db_guid_0_j_brow;
        void* db_guid_0_j_brow_data;
        ocrDbCreate( &db_guid_0_j_brow, &db_guid_0_j_brow_data, sizeof(int)*tile_width, FLAGS, NULL, NO_ALLOC );

        allocated = (int*)db_guid_0_j_brow_data;
        for( i = 0; i < tile_width ; ++i ) {
            allocated[i] = GAP_PENALTY*((j-1)*tile_width+i+1);
        }

        ocrEventSatisfy(data_initial_row[j-1], db_guid_0_j_brow);

        ocrGuid_t db_guid_0_j_br;
        void* db_guid_0_j_br_data;
        ocrDbCreate( &db_guid_0_j_br, &db_guid_0_j_br_data, sizeof(int), FLAGS, NULL, NO_ALLOC );
        allocated = (int*)db_guid_0_j_br_data;
        allocated[0] = GAP_PENALTY*(j*tile_width); //sagnak: needed to handle tilesize 2

        ocrEventSatisfy(diags_initial_row[j], db_guid_0_j_br);
    }

    for ( i = 1; i < n_tiles_height + 1; ++i ) {
        ocrGuid_t db_guid_i_0_rc;
        void* db_guid_i_0_rc_data;
        ocrDbCreate( &db_guid_i_0_rc, &db_guid_i_0_rc_data, sizeof(int)*tile_height, FLAGS, NULL, NO_ALLOC );
        allocated = (int*)db_guid_i_0_rc_data;
        for ( j = 0; j < tile_height ; ++j ) {
            allocated[j] = GAP_PENALTY*((i-1)*tile_height+j+1);
        }
        ocrEventSatisfy(data_initial_col[i-1], db_guid_i_0_rc);

        ocrGuid_t db_guid_i_0_br;
        void* db_guid_i_0_br_data;
        ocrDbCreate( &db_guid_i_0_br, &db_guid_i_0_br_data, sizeof(int), FLAGS, NULL, NO_ALLOC );

        allocated = (int*)db_guid_i_0_br_data;
        allocated[0] = GAP_PENALTY*(i*tile_height); //sagnak: needed to handle tilesize 2

        ocrEventSatisfy(diags_initial_col[i], db_guid_i_0_br);
    }
/*
	diags_initial_row[0] = 0;
	diags_initial_col[0] = 0;

    for ( j = 1; j < n_tiles_width + 1; ++j ) {
		data_initial_row[j-1] = (int*) malloc (sizeof(int) * tile_width);
        for( i = 0; i < tile_width ; ++i ) {
            data_initial_row[j-1][i] = GAP_PENALTY*((j-1)*tile_width+i+1);
        }
        diags_initial_row[j] = GAP_PENALTY*(j*tile_width); 
    }

    for ( i = 1; i < n_tiles_height + 1; ++i ) {
		data_initial_col[i-1] = (int*) malloc (sizeof(int) * tile_height);
        for ( j = 0; j < tile_height ; ++j ) {
            data_initial_col[i-1][j] = GAP_PENALTY*((i-1)*tile_height+j+1);
        }
        diags_initial_col[i] = GAP_PENALTY*(i*tile_height); //sagnak: needed to handle tilesize 2
    }
*/
    for ( i = 0; i < n_tiles_height; ++i ) {
        for ( j = 0; j < n_tiles_width; ++j ) {
		int home = DDDF_HOME( TILE_ID_START(i,j) );
		if (home == rank) {

    		intptr_t **p_paramv = (intptr_t **)malloc(sizeof(intptr_t*));
    		intptr_t *pars = (intptr_t *)malloc(16*sizeof(intptr_t));
    		pars[0]=(intptr_t)rank;
    		pars[1]=(intptr_t)num_ranks;
    		pars[2]=(intptr_t) string1;
    		pars[3]=(intptr_t) string2;
    		pars[4]=(intptr_t)tile_width;
    		pars[5]=(intptr_t)tile_height;
    		pars[6]=(intptr_t)outer_tile_width;
    		pars[7]=(intptr_t)outer_tile_height;
    		pars[8]=(intptr_t)diags_initial_row;
    		pars[9]=(intptr_t)diags_initial_col;
    		pars[10]=(intptr_t)data_initial_row;
    		pars[11]=(intptr_t)data_initial_col;
            pars[12]=(intptr_t)n_tiles_width;
            pars[13]=(intptr_t)n_tiles_height;
            pars[14]=(intptr_t)i;
            pars[15]=(intptr_t)j;
    		*p_paramv = pars;

            ocrGuid_t task_guid;
            ocrEdtCreate(&task_guid, smith_waterman_kernel, 16, NULL, (void **) p_paramv, PROPERTIES, 3, NULL);

			if (i == 0) {
				if (j == 0) {
					ocrAddDependence(diags_initial_row[j], task_guid, 0);
					ocrAddDependence(data_initial_row[j], task_guid, 1);
					ocrAddDependence(data_initial_col[j], task_guid, 2);
				} else {
					ocrAddDependence(diags_initial_row[j], task_guid, 0);
					ocrAddDependence(data_initial_row[j], task_guid, 1);
					int col_id = COL_DEP(i,j);
					OCR_DDDF_ADD_DEPENDENCE(col_id, task_guid, 2);
				}
			} else {
				if (j == 0) {
					ocrAddDependence(diags_initial_row[i], task_guid, 0);
					int row_id = ROW_DEP(i,j);
					OCR_DDDF_ADD_DEPENDENCE(row_id, task_guid, 1);
					ocrAddDependence(data_initial_col[i], task_guid, 2);
				} else {
					int diag_id = DIAG_DEP(i,j);
					OCR_DDDF_ADD_DEPENDENCE(diag_id, task_guid, 0);
					int row_id = ROW_DEP(i,j);
					OCR_DDDF_ADD_DEPENDENCE(row_id, task_guid, 1);
				    int col_id = COL_DEP(i,j);
					OCR_DDDF_ADD_DEPENDENCE(col_id, task_guid, 2);
				}
			}

            ocrEdtSchedule(task_guid);
		}
        }
    }

	/* Get everybody to synchronize before we start */
/* TODO: Deferring this for now till we get timing fixed.
	int slot = 0;
   	ocrGuid_t edt_sync_guid;
	ocrEdtCreate(&edt_sync_guid, smith_waterman_task, 12, NULL, (void**) p_paramv, PROPERTIES, num_ranks - 1, NULL);
	for (i = 1; i < num_ranks; i++) {
		if (i != rank)
			ocrD3FAddDependence(1+i, i, edt_sync_guid, slot++);
	}
	ocrEdtSchedule(edt_sync_guid);

	if (rank != 0) {
		ocrDbCreate( &db_guid, &db_data, sizeof(int), FLAGS, NULL, NO_ALLOC );
		ocrD3FSatisfy(rank+1, rank, db_guid);
	}
*/
    return 0;
}

u8 smith_waterman_remote_start ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) 
{
	int i, j;
    int* inputs = (int*)depv[0].ptr;
	
	int tile_width = inputs[0];
	int tile_height = inputs[1];
	int outer_tile_width = inputs[2];
	int outer_tile_height = inputs[3];
	int n_char_in_file_1 = inputs[4];
	int n_char_in_file_2 = inputs[5];
	signed char * str = (signed char *)(&inputs[6]);

	signed char * string1 = (signed char *)malloc(sizeof(signed char) * n_char_in_file_1);
	signed char * string2 = (signed char *)malloc(sizeof(signed char) * n_char_in_file_2);

	for ( i = 0; i < n_char_in_file_1; i++)
		string1[i] = str[i];

	for ( j = 0; j < n_char_in_file_2; j++, i++)
		string1[j] = str[i];

	smith_waterman_init(string1, string2, tile_width, tile_height, outer_tile_width, outer_tile_height, n_char_in_file_1, n_char_in_file_2);

	return 0;
}

u8 smith_waterman_termination ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) 
{
    ocrGuid_t db_guid;
    void* db_data;
    ocrDbCreate( &db_guid, &db_data, sizeof(int), FLAGS, NULL, NO_ALLOC );
	ocrD3FSatisfy(0, 0, db_guid);
}

int main ( int argc, char* argv[] ) 
{
    OCR_DDDF_INIT(&argc, argv, 0, NULL);

	int rank = OCR_DDDF_RANK();

	if (rank == 0) {

	    if ( argc < 7 ) {
	        fprintf(stderr, "Usage: %s fileName1 fileName2 tile_Width tile_Height outer_tile_Width outer_tile_Height\n", argv[0]);
	        exit(1);
	    }
	
		int i, j;
	    char* file_name_1 = argv[1];
	    char* file_name_2 = argv[2];
	    int tile_width = (int) atoi (argv[3]);
	    int tile_height = (int) atoi (argv[4]);
	    int outer_tile_width = (int) atoi (argv[5]);
	    int outer_tile_height = (int) atoi (argv[6]);
	
        if (( outer_tile_width % tile_width != 0 ) || (outer_tile_height % tile_height != 0)) {
            fprintf(stderr, "Outer tile dimensions should be a multiple of tile dimensions\n", outer_tile_width, tile_width);
	        exit(1);
        }
	
	    FILE* file_1 = fopen(file_name_1, "r");
	    if (!file_1) { fprintf(stderr, "could not open file %s\n",file_name_1); exit(1); }
	    size_t n_char_in_file_1 = 0;
	    signed char* string1 = read_file(file_1, &n_char_in_file_1);
	    fprintf(stdout, "Size of input string 1 is %zu\n", n_char_in_file_1 );
	
	    FILE* file_2 = fopen(file_name_2, "r");
	    if (!file_2) { fprintf(stderr, "could not open file %s\n",file_name_2); exit(1); }
	    size_t n_char_in_file_2 = 0;
	    signed char* string2 = read_file(file_2, &n_char_in_file_2);
	    fprintf(stdout, "Size of input string 2 is %zu\n", n_char_in_file_2 );
	
	    fprintf(stdout, "Tile width is %d\n", tile_width);
	    fprintf(stdout, "Tile height is %d\n", tile_height);

	    int n_tiles_width = n_char_in_file_1/tile_width;
	    int n_tiles_height = n_char_in_file_2/tile_height;
	
	    fprintf(stdout, "Imported %d x %d tiles.\n", n_tiles_width, n_tiles_height);
	
		int num_tiles_in_outer_width = outer_tile_width / tile_width;
		int num_tiles_in_outer_height = outer_tile_height / tile_height;

	    fprintf(stdout, "Number of intra-node tiles on x is %d\n", num_tiles_in_outer_width);
	    fprintf(stdout, "Number of intra-node tiles on y is %d\n", num_tiles_in_outer_height);
	
        /* Distribute the input strings and params to all other node by packing everything into one put */
        size_t input_size = sizeof(signed char) * (n_char_in_file_1 + n_char_in_file_2) + sizeof(int) * 4;
		ocrGuid_t db_input_guid;
		void * db_input_data;
		ocrDbCreate( &db_input_guid, &db_input_data, (sizeof(signed char) * (n_char_in_file_1 + n_char_in_file_2) + sizeof(int) * 6), FLAGS, NULL, NO_ALLOC );
		int * ptr = (int*) db_input_data;
		ptr[0] = tile_width;
		ptr[1] = tile_height;
		ptr[2] = outer_tile_width;
		ptr[3] = outer_tile_height;
		ptr[4] = n_char_in_file_1;
		ptr[5] = n_char_in_file_2;
		signed char * string_start = (signed char *)(&ptr[6]);
		for (i = 0; i < n_char_in_file_1; i++)
			string_start[i] = string1[i];
		for (j = 0; j < n_char_in_file_2; j++, i++)
			string_start[i] = string2[j];

		ocrD3FSatisfy(1, 0, db_input_guid);

		smith_waterman_init(string1, string2, tile_width, tile_height, outer_tile_width, outer_tile_height, n_char_in_file_1, n_char_in_file_2);

    	ocrGuid_t task_guid;
		ocrEdtCreate(&task_guid, smith_waterman_termination, 0, NULL, NULL, PROPERTIES, 1, NULL);
		int num_ranks = OCR_DDDF_NUM_RANKS();
		int tile_id_offset = num_ranks + 1;
		int id = TILE_ID_START(n_tiles_width-1, n_tiles_height-1);
		OCR_DDDF_ADD_DEPENDENCE(id, task_guid, 0);
		ocrEdtSchedule(task_guid);
		
    } else {
		// Wait for inputs from rank 0
    	ocrGuid_t task_guid;
		ocrEdtCreate(&task_guid, smith_waterman_remote_start, 0, NULL, NULL, PROPERTIES, 1, NULL);
		ocrD3FAddDependence(1, 0, task_guid, 0);
		ocrEdtSchedule(task_guid);
	}

    ocrD3FModelFinalize(0, 0); // Termination DDDF has id 0
    return 0;
}


