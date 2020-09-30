#line 1 "src/factors/continuous/opencl/CKDE.cl.src"

/*
 *****************************************************************************
 **       This file was autogenerated from a template  DO NOT EDIT!!!!      **
 **       Changes should be made to the original source (.src) file         **
 *****************************************************************************
 */

#line 1
/* This code assumes column major data for matrices. */

#define IDX(i, j, rows) (i) + ((j)*(rows))
#define ROW(idx, rows) (idx) % (rows)
#define COL(idx, rows) (idx) / (rows)

#define MAX_ASSIGN(n1, n2) n1 = max((n1), (n2))
#define SUM_ASSIGN(n1, n2) n1 += (n2)

#line 13


#line 19

__kernel void max1d_double(__constant double *input,
                                      __private uint input_length,
                                      __local double *localMaxs,
                                      __global double *output,
                                      __private uint output_offset)
{
    uint global_id = get_global_id(0);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);

    if (group_id == num_groups-1) {
        group_size = input_length - group_id*group_size;

        if (global_id < input_length) {
            localMaxs[local_id] = input[global_id];
        }
    }
    else {
        localMaxs[local_id] = input[global_id];
    }

    while (group_size > 1) {
        int stride = group_size / 2;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (group_size % 2 == 0) {
            if (local_id < stride) {
                MAX_ASSIGN(localMaxs[local_id], localMaxs[local_id + stride]);
            }

            group_size = group_size / 2;
        }
        else {
            if (local_id < stride) {
                MAX_ASSIGN(localMaxs[local_id + 1], localMaxs[local_id + 1 + stride]);
            }
            group_size = (group_size / 2) + 1;
        }
    }

    if (local_id == 0) {
        output[output_offset + group_id] = localMaxs[0];
    }
}

__kernel void max_mat_cols_double(__constant double *mat,
                                             __private uint mat_rows,
                                             __local double *localMaxs,
                                             __global double *output,
                                             __private uint output_offset)
{
    uint global_id_row = get_global_id(0);
    uint global_id_col = get_global_id(1);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);


    if (group_id == num_groups-1) {
        group_size = mat_rows - group_id*group_size;

        if (global_id_row < mat_rows) {
            localMaxs[local_id] = mat[IDX(global_id_row, global_id_col, mat_rows)];
        }
    }
    else {
        localMaxs[local_id] = mat[IDX(global_id_row, global_id_col, mat_rows)];
    }

    while (group_size > 1) {
        int stride = group_size / 2;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (group_size % 2 == 0) {
            if (local_id < stride) {
                MAX_ASSIGN(localMaxs[local_id], localMaxs[local_id + stride]);
            }
            group_size = group_size / 2;
        }
        else {
            if (local_id < stride) {
                MAX_ASSIGN(localMaxs[local_id+1], localMaxs[local_id+1+stride]);
            }
            group_size = (group_size / 2) + 1;
        }
    }

    if (local_id == 0) {
        output[IDX(group_id, output_offset + global_id_col, num_groups)] = localMaxs[0];
    }
}


#line 19

__kernel void sum1d_double(__constant double *input,
                                      __private uint input_length,
                                      __local double *localMaxs,
                                      __global double *output,
                                      __private uint output_offset)
{
    uint global_id = get_global_id(0);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);

    if (group_id == num_groups-1) {
        group_size = input_length - group_id*group_size;

        if (global_id < input_length) {
            localMaxs[local_id] = input[global_id];
        }
    }
    else {
        localMaxs[local_id] = input[global_id];
    }

    while (group_size > 1) {
        int stride = group_size / 2;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (group_size % 2 == 0) {
            if (local_id < stride) {
                SUM_ASSIGN(localMaxs[local_id], localMaxs[local_id + stride]);
            }

            group_size = group_size / 2;
        }
        else {
            if (local_id < stride) {
                SUM_ASSIGN(localMaxs[local_id + 1], localMaxs[local_id + 1 + stride]);
            }
            group_size = (group_size / 2) + 1;
        }
    }

    if (local_id == 0) {
        output[output_offset + group_id] = localMaxs[0];
    }
}

__kernel void sum_mat_cols_double(__constant double *mat,
                                             __private uint mat_rows,
                                             __local double *localMaxs,
                                             __global double *output,
                                             __private uint output_offset)
{
    uint global_id_row = get_global_id(0);
    uint global_id_col = get_global_id(1);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);


    if (group_id == num_groups-1) {
        group_size = mat_rows - group_id*group_size;

        if (global_id_row < mat_rows) {
            localMaxs[local_id] = mat[IDX(global_id_row, global_id_col, mat_rows)];
        }
    }
    else {
        localMaxs[local_id] = mat[IDX(global_id_row, global_id_col, mat_rows)];
    }

    while (group_size > 1) {
        int stride = group_size / 2;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (group_size % 2 == 0) {
            if (local_id < stride) {
                SUM_ASSIGN(localMaxs[local_id], localMaxs[local_id + stride]);
            }
            group_size = group_size / 2;
        }
        else {
            if (local_id < stride) {
                SUM_ASSIGN(localMaxs[local_id+1], localMaxs[local_id+1+stride]);
            }
            group_size = (group_size / 2) + 1;
        }
    }

    if (local_id == 0) {
        output[IDX(group_id, output_offset + global_id_col, num_groups)] = localMaxs[0];
    }
}





__kernel void logsumexp_coeffs_double(__global double *input,
                                        __private uint input_rows,
                                        __constant double *max) {
    uint idx = get_global_id(0);
    uint col = COL(idx, input_rows);
    input[idx] = exp(input[idx] - max[col]);
}


__kernel void solve_double(__global double *diff_matrix, 
                        __private uint diff_matrix_rows, 
                        __private uint matrices_cols,
                        __constant double *cholesky_matrix) {
    uint r = get_global_id(0);
    
    for (uint c = 0; c < matrices_cols; c++) {
        for (uint i = 0; i < c; i++) {
            diff_matrix[IDX(r, c, diff_matrix_rows)] -= cholesky_matrix[IDX(c, i, matrices_cols)] * diff_matrix[IDX(r, i, diff_matrix_rows)];
        }
        diff_matrix[IDX(r, c, diff_matrix_rows)] /= cholesky_matrix[IDX(c, c, matrices_cols)];
    }
}

__kernel void square_double(__global double *m) {
    uint idx = get_global_id(0);
    double d = m[idx];
    m[idx] = d * d;
}


__kernel void logl_values_1d_mat_double(__constant double *train_vector,
                                           __private uint train_rows,
                                           __constant double *test_vector,
                                           __private uint test_offset,
                                           __constant double *standard_deviation,
                                           __private double lognorm_factor,
                                           __global double *result) 
{
    int i = get_global_id(0);
    int train_idx = ROW(i, train_rows);
    int test_idx = COL(i, train_rows);
    double d = (train_vector[train_idx] - test_vector[test_offset + test_idx]) / standard_deviation[0];

    result[i] = (-0.5*d*d) + lognorm_factor;
}

__kernel void substract_double(__constant double *training_matrix,
                                     __private uint training_physical_rows,
                                     __private uint training_offset,
                                     __private uint training_rows,
                                     __constant double *test_matrix,
                                     __private uint test_physical_rows,
                                     __private uint test_offset,
                                     __private uint test_row_idx,
                                     __global double *res
                                )
{
    uint i = get_global_id(0);
    uint r = ROW(i, training_rows) + training_offset;
    uint c = COL(i, training_rows);

    res[i] = test_matrix[IDX(test_offset + test_row_idx, c, test_physical_rows)] - training_matrix[IDX(r, c, training_physical_rows)];
}


__kernel void logl_values_mat_column_double(__constant double *square_data,
                                                    __private uint square_cols,
                                                    __global double *sol_mat,
                                                    __private uint sol_rows,
                                                    __private uint sol_col_idx,
                                                    __private double lognorm_factor) {
    uint test_idx = get_global_id(0);
    uint square_rows = get_global_size(0);
    
    uint sol_idx = IDX(test_idx, sol_col_idx, sol_rows);

    double summation = square_data[IDX(test_idx, 0, square_rows)];
    for (uint i = 1; i < square_cols; i++) {
        summation += square_data[IDX(test_idx, i, square_rows)];
    }

    sol_mat[sol_idx] = (-0.5 * summation) + lognorm_factor;
}

__kernel void logl_values_mat_row_double(__constant double *square_data,
                                                    __private uint square_cols,
                                                    __global double *sol_mat,
                                                    __private uint sol_rows,
                                                    __private uint sol_row_idx,
                                                    __private double lognorm_factor) {
    uint test_idx = get_global_id(0);
    uint square_rows = get_global_size(0);
    
    uint sol_idx = IDX(sol_row_idx, test_idx, sol_rows);

    double summation = square_data[IDX(test_idx, 0, square_rows)];
    for (uint i = 1; i < square_cols; i++) {
        summation += square_data[IDX(test_idx, i, square_rows)];
    }

    sol_mat[sol_idx] = (-0.5 * summation) + lognorm_factor;
}

__kernel void finish_lse_offset_double(__global double *res, __private uint res_offset, __constant double *max_vec) {
    uint idx = get_global_id(0);
    res[idx + res_offset] = log(res[idx + res_offset]) + max_vec[idx];
}


__kernel void substract_vectors_double(__global double *v1, __constant double *v2) {
    uint idx = get_global_id(0);
    v1[idx] -= v2[idx];
}

__kernel void exp_elementwise_double(__global double *mat) {
    uint idx = get_global_id(0);

    mat[idx] = exp(mat[idx]);
}


// A variation of the code in https://community.khronos.org/t/is-there-any-opencl-library-having-prefix-sum/3945/4.
// This version works in the columns of a matrix and exp() the input values before performing the prefix sum.
// It is best explained in Blelloch, 1990 "Prefix Sums and Their Applications":
//  https://www.cs.cmu.edu/~guyb/papers/Ble93.pdf
__kernel void accum_sum_mat_cols_double(__global double *mat, 
                                         __private uint mat_rows,
                                         __local double *local_block,
                                         __global double *sums)
{
    uint row_id = get_global_id(0);
    uint col_id = get_global_id(1);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);

    if (2*row_id+1 < mat_rows) {
        local_block[2*local_id] = mat[IDX(2*row_id, col_id, mat_rows)];
        local_block[2*local_id+1] = mat[IDX(2*row_id+1, col_id, mat_rows)];

    } else {
        local_block[2*local_id] = 0;
        local_block[2*local_id+1] = 0;
    }

    if (group_id == num_groups-1) {
        local_block[mat_rows - group_id*group_size - 1] = mat[IDX(mat_rows-1, col_id, mat_rows)];
    }


	uint offset = 1;
	/* build the sum in place up the tree */
	// for (int d = block_size >> 1; d > 0; d >>= 1)
	for (uint d = group_size; d > 0; d /= 2)
	{
		barrier (CLK_LOCAL_MEM_FENCE);


		if (local_id < d)
		{
			uint ai = offset * (2 * local_id + 1) - 1;
			uint bi = offset * (2 * local_id + 2) - 1;

			local_block[bi] += local_block[ai];
		}
		offset *= 2;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	/* store the value in sum buffer before making it to 0 */
	sums[IDX(group_id, col_id, get_num_groups(0))] = local_block[2*group_size - 1];

	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

	// /* scan back down the tree */

	// /* clear the last element */
	local_block[2*group_size - 1] = 0;

	/* traverse down the tree building the scan in the place */
	for (uint d = 1; d <= group_size; d *= 2)
	{
		// offset >>= 1;
		offset /= 2;
		barrier(CLK_LOCAL_MEM_FENCE);

		if (local_id < d)
		{
			uint ai = offset * (2 * local_id + 1) - 1;
			uint bi = offset * (2 * local_id + 2) - 1;

			double t = local_block[ai];
			local_block[ai] = local_block[bi];
			local_block[bi] += t;
		}
	}

	barrier(CLK_LOCAL_MEM_FENCE);

    // /*write the results back to global memory */

    if ((2*row_id+1) < mat_rows) {
        mat[IDX(2*row_id, col_id, mat_rows)] = local_block[2*local_id];
        mat[IDX(2*row_id+1, col_id, mat_rows)] = local_block[2*local_id+1];
    } else if (2*row_id < mat_rows) {
        mat[IDX(2*row_id, col_id, mat_rows)] = local_block[2*local_id];
    }
}

__kernel void add_accum_sum_mat_cols_double(__global double *mat, 
                                             __private uint mat_rows,
                                             __private uint mat_offset,
                                             __private uint size_per_group,
                                             __private uint num_groups,
                                             __global double *sums) {
    

    uint row_id = get_global_id(0);
    uint col_id = get_global_id(1);

    mat[IDX(row_id + mat_offset, col_id, mat_rows)] += sums[IDX((row_id / size_per_group)+1, col_id, num_groups)];
}

__kernel void normalize_accum_sum_mat_cols_double(__global double *mat, 
                                                   __private uint mat_rows,
                                                   __global double *sums) {
    

    uint row_id = get_global_id(0);
    uint col_id = get_global_id(1);


    mat[IDX(row_id + 1, col_id, mat_rows)] /= sums[col_id];
}


__kernel void find_random_indices_double(__global double *mat,
                                       __private uint mat_rows,
                                       __private uint mat_offset,
                                       __global double *random_numbers,
                                       __global int *indices) {
    
    int row_id = get_global_id(0);
    int col_id = get_global_id(1);
    
    double rn = random_numbers[mat_offset + col_id];
    if (mat[IDX(row_id, col_id, mat_rows)] <= rn && rn < mat[IDX(row_id+1, col_id, mat_rows)])
        indices[mat_offset + col_id] = row_id;
}



#line 13


#line 19

__kernel void max1d_float(__constant float *input,
                                      __private uint input_length,
                                      __local float *localMaxs,
                                      __global float *output,
                                      __private uint output_offset)
{
    uint global_id = get_global_id(0);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);

    if (group_id == num_groups-1) {
        group_size = input_length - group_id*group_size;

        if (global_id < input_length) {
            localMaxs[local_id] = input[global_id];
        }
    }
    else {
        localMaxs[local_id] = input[global_id];
    }

    while (group_size > 1) {
        int stride = group_size / 2;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (group_size % 2 == 0) {
            if (local_id < stride) {
                MAX_ASSIGN(localMaxs[local_id], localMaxs[local_id + stride]);
            }

            group_size = group_size / 2;
        }
        else {
            if (local_id < stride) {
                MAX_ASSIGN(localMaxs[local_id + 1], localMaxs[local_id + 1 + stride]);
            }
            group_size = (group_size / 2) + 1;
        }
    }

    if (local_id == 0) {
        output[output_offset + group_id] = localMaxs[0];
    }
}

__kernel void max_mat_cols_float(__constant float *mat,
                                             __private uint mat_rows,
                                             __local float *localMaxs,
                                             __global float *output,
                                             __private uint output_offset)
{
    uint global_id_row = get_global_id(0);
    uint global_id_col = get_global_id(1);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);


    if (group_id == num_groups-1) {
        group_size = mat_rows - group_id*group_size;

        if (global_id_row < mat_rows) {
            localMaxs[local_id] = mat[IDX(global_id_row, global_id_col, mat_rows)];
        }
    }
    else {
        localMaxs[local_id] = mat[IDX(global_id_row, global_id_col, mat_rows)];
    }

    while (group_size > 1) {
        int stride = group_size / 2;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (group_size % 2 == 0) {
            if (local_id < stride) {
                MAX_ASSIGN(localMaxs[local_id], localMaxs[local_id + stride]);
            }
            group_size = group_size / 2;
        }
        else {
            if (local_id < stride) {
                MAX_ASSIGN(localMaxs[local_id+1], localMaxs[local_id+1+stride]);
            }
            group_size = (group_size / 2) + 1;
        }
    }

    if (local_id == 0) {
        output[IDX(group_id, output_offset + global_id_col, num_groups)] = localMaxs[0];
    }
}


#line 19

__kernel void sum1d_float(__constant float *input,
                                      __private uint input_length,
                                      __local float *localMaxs,
                                      __global float *output,
                                      __private uint output_offset)
{
    uint global_id = get_global_id(0);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);

    if (group_id == num_groups-1) {
        group_size = input_length - group_id*group_size;

        if (global_id < input_length) {
            localMaxs[local_id] = input[global_id];
        }
    }
    else {
        localMaxs[local_id] = input[global_id];
    }

    while (group_size > 1) {
        int stride = group_size / 2;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (group_size % 2 == 0) {
            if (local_id < stride) {
                SUM_ASSIGN(localMaxs[local_id], localMaxs[local_id + stride]);
            }

            group_size = group_size / 2;
        }
        else {
            if (local_id < stride) {
                SUM_ASSIGN(localMaxs[local_id + 1], localMaxs[local_id + 1 + stride]);
            }
            group_size = (group_size / 2) + 1;
        }
    }

    if (local_id == 0) {
        output[output_offset + group_id] = localMaxs[0];
    }
}

__kernel void sum_mat_cols_float(__constant float *mat,
                                             __private uint mat_rows,
                                             __local float *localMaxs,
                                             __global float *output,
                                             __private uint output_offset)
{
    uint global_id_row = get_global_id(0);
    uint global_id_col = get_global_id(1);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);


    if (group_id == num_groups-1) {
        group_size = mat_rows - group_id*group_size;

        if (global_id_row < mat_rows) {
            localMaxs[local_id] = mat[IDX(global_id_row, global_id_col, mat_rows)];
        }
    }
    else {
        localMaxs[local_id] = mat[IDX(global_id_row, global_id_col, mat_rows)];
    }

    while (group_size > 1) {
        int stride = group_size / 2;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (group_size % 2 == 0) {
            if (local_id < stride) {
                SUM_ASSIGN(localMaxs[local_id], localMaxs[local_id + stride]);
            }
            group_size = group_size / 2;
        }
        else {
            if (local_id < stride) {
                SUM_ASSIGN(localMaxs[local_id+1], localMaxs[local_id+1+stride]);
            }
            group_size = (group_size / 2) + 1;
        }
    }

    if (local_id == 0) {
        output[IDX(group_id, output_offset + global_id_col, num_groups)] = localMaxs[0];
    }
}





__kernel void logsumexp_coeffs_float(__global float *input,
                                        __private uint input_rows,
                                        __constant float *max) {
    uint idx = get_global_id(0);
    uint col = COL(idx, input_rows);
    input[idx] = exp(input[idx] - max[col]);
}


__kernel void solve_float(__global float *diff_matrix, 
                        __private uint diff_matrix_rows, 
                        __private uint matrices_cols,
                        __constant float *cholesky_matrix) {
    uint r = get_global_id(0);
    
    for (uint c = 0; c < matrices_cols; c++) {
        for (uint i = 0; i < c; i++) {
            diff_matrix[IDX(r, c, diff_matrix_rows)] -= cholesky_matrix[IDX(c, i, matrices_cols)] * diff_matrix[IDX(r, i, diff_matrix_rows)];
        }
        diff_matrix[IDX(r, c, diff_matrix_rows)] /= cholesky_matrix[IDX(c, c, matrices_cols)];
    }
}

__kernel void square_float(__global float *m) {
    uint idx = get_global_id(0);
    double d = m[idx];
    m[idx] = d * d;
}


__kernel void logl_values_1d_mat_float(__constant float *train_vector,
                                           __private uint train_rows,
                                           __constant float *test_vector,
                                           __private uint test_offset,
                                           __constant float *standard_deviation,
                                           __private float lognorm_factor,
                                           __global float *result) 
{
    int i = get_global_id(0);
    int train_idx = ROW(i, train_rows);
    int test_idx = COL(i, train_rows);
    float d = (train_vector[train_idx] - test_vector[test_offset + test_idx]) / standard_deviation[0];

    result[i] = (-0.5*d*d) + lognorm_factor;
}

__kernel void substract_float(__constant float *training_matrix,
                                     __private uint training_physical_rows,
                                     __private uint training_offset,
                                     __private uint training_rows,
                                     __constant float *test_matrix,
                                     __private uint test_physical_rows,
                                     __private uint test_offset,
                                     __private uint test_row_idx,
                                     __global float *res
                                )
{
    uint i = get_global_id(0);
    uint r = ROW(i, training_rows) + training_offset;
    uint c = COL(i, training_rows);

    res[i] = test_matrix[IDX(test_offset + test_row_idx, c, test_physical_rows)] - training_matrix[IDX(r, c, training_physical_rows)];
}


__kernel void logl_values_mat_column_float(__constant float *square_data,
                                                    __private uint square_cols,
                                                    __global float *sol_mat,
                                                    __private uint sol_rows,
                                                    __private uint sol_col_idx,
                                                    __private float lognorm_factor) {
    uint test_idx = get_global_id(0);
    uint square_rows = get_global_size(0);
    
    uint sol_idx = IDX(test_idx, sol_col_idx, sol_rows);

    float summation = square_data[IDX(test_idx, 0, square_rows)];
    for (uint i = 1; i < square_cols; i++) {
        summation += square_data[IDX(test_idx, i, square_rows)];
    }

    sol_mat[sol_idx] = (-0.5 * summation) + lognorm_factor;
}

__kernel void logl_values_mat_row_float(__constant float *square_data,
                                                    __private uint square_cols,
                                                    __global float *sol_mat,
                                                    __private uint sol_rows,
                                                    __private uint sol_row_idx,
                                                    __private float lognorm_factor) {
    uint test_idx = get_global_id(0);
    uint square_rows = get_global_size(0);
    
    uint sol_idx = IDX(sol_row_idx, test_idx, sol_rows);

    float summation = square_data[IDX(test_idx, 0, square_rows)];
    for (uint i = 1; i < square_cols; i++) {
        summation += square_data[IDX(test_idx, i, square_rows)];
    }

    sol_mat[sol_idx] = (-0.5 * summation) + lognorm_factor;
}

__kernel void finish_lse_offset_float(__global float *res, __private uint res_offset, __constant float *max_vec) {
    uint idx = get_global_id(0);
    res[idx + res_offset] = log(res[idx + res_offset]) + max_vec[idx];
}


__kernel void substract_vectors_float(__global float *v1, __constant float *v2) {
    uint idx = get_global_id(0);
    v1[idx] -= v2[idx];
}

__kernel void exp_elementwise_float(__global float *mat) {
    uint idx = get_global_id(0);

    mat[idx] = exp(mat[idx]);
}


// A variation of the code in https://community.khronos.org/t/is-there-any-opencl-library-having-prefix-sum/3945/4.
// This version works in the columns of a matrix and exp() the input values before performing the prefix sum.
// It is best explained in Blelloch, 1990 "Prefix Sums and Their Applications":
//  https://www.cs.cmu.edu/~guyb/papers/Ble93.pdf
__kernel void accum_sum_mat_cols_float(__global float *mat, 
                                         __private uint mat_rows,
                                         __local float *local_block,
                                         __global float *sums)
{
    uint row_id = get_global_id(0);
    uint col_id = get_global_id(1);
    uint local_id = get_local_id(0);
    uint group_size = get_local_size(0);
    uint group_id = get_group_id(0);
    uint num_groups = get_num_groups(0);

    if (2*row_id+1 < mat_rows) {
        local_block[2*local_id] = mat[IDX(2*row_id, col_id, mat_rows)];
        local_block[2*local_id+1] = mat[IDX(2*row_id+1, col_id, mat_rows)];

    } else {
        local_block[2*local_id] = 0;
        local_block[2*local_id+1] = 0;
    }

    if (group_id == num_groups-1) {
        local_block[mat_rows - group_id*group_size - 1] = mat[IDX(mat_rows-1, col_id, mat_rows)];
    }


	uint offset = 1;
	/* build the sum in place up the tree */
	// for (int d = block_size >> 1; d > 0; d >>= 1)
	for (uint d = group_size; d > 0; d /= 2)
	{
		barrier (CLK_LOCAL_MEM_FENCE);


		if (local_id < d)
		{
			uint ai = offset * (2 * local_id + 1) - 1;
			uint bi = offset * (2 * local_id + 2) - 1;

			local_block[bi] += local_block[ai];
		}
		offset *= 2;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	/* store the value in sum buffer before making it to 0 */
	sums[IDX(group_id, col_id, get_num_groups(0))] = local_block[2*group_size - 1];

	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

	// /* scan back down the tree */

	// /* clear the last element */
	local_block[2*group_size - 1] = 0;

	/* traverse down the tree building the scan in the place */
	for (uint d = 1; d <= group_size; d *= 2)
	{
		// offset >>= 1;
		offset /= 2;
		barrier(CLK_LOCAL_MEM_FENCE);

		if (local_id < d)
		{
			uint ai = offset * (2 * local_id + 1) - 1;
			uint bi = offset * (2 * local_id + 2) - 1;

			float t = local_block[ai];
			local_block[ai] = local_block[bi];
			local_block[bi] += t;
		}
	}

	barrier(CLK_LOCAL_MEM_FENCE);

    // /*write the results back to global memory */

    if ((2*row_id+1) < mat_rows) {
        mat[IDX(2*row_id, col_id, mat_rows)] = local_block[2*local_id];
        mat[IDX(2*row_id+1, col_id, mat_rows)] = local_block[2*local_id+1];
    } else if (2*row_id < mat_rows) {
        mat[IDX(2*row_id, col_id, mat_rows)] = local_block[2*local_id];
    }
}

__kernel void add_accum_sum_mat_cols_float(__global float *mat, 
                                             __private uint mat_rows,
                                             __private uint mat_offset,
                                             __private uint size_per_group,
                                             __private uint num_groups,
                                             __global float *sums) {
    

    uint row_id = get_global_id(0);
    uint col_id = get_global_id(1);

    mat[IDX(row_id + mat_offset, col_id, mat_rows)] += sums[IDX((row_id / size_per_group)+1, col_id, num_groups)];
}

__kernel void normalize_accum_sum_mat_cols_float(__global float *mat, 
                                                   __private uint mat_rows,
                                                   __global float *sums) {
    

    uint row_id = get_global_id(0);
    uint col_id = get_global_id(1);


    mat[IDX(row_id + 1, col_id, mat_rows)] /= sums[col_id];
}


__kernel void find_random_indices_float(__global float *mat,
                                       __private uint mat_rows,
                                       __private uint mat_offset,
                                       __global float *random_numbers,
                                       __global int *indices) {
    
    int row_id = get_global_id(0);
    int col_id = get_global_id(1);
    
    float rn = random_numbers[mat_offset + col_id];
    if (mat[IDX(row_id, col_id, mat_rows)] <= rn && rn < mat[IDX(row_id+1, col_id, mat_rows)])
        indices[mat_offset + col_id] = row_id;
}




