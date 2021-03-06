//
// Created by yanz on 2017/9/12.
//
#include "encodeText.h"

#define BYTE_LENGTH 8


void initialTextEncoderContext(const int max_block_num, struct TextEncoderContext **text_encoder_context) {
    int num_pixel = max_block_num * MB_WIDTH * MB_HEIGHT;
    struct TextEncoderContext *context = (struct TextEncoderContext *) calloc(1, sizeof(struct TextEncoderContext));
    if (NULL==context){
        releaseTextEncoderContext(context);
        printf("context---->Allocated memory fail!!");
        return;
    }
    *text_encoder_context = context;
    struct TextQuantizePara *text_quan = (struct TextQuantizePara *) calloc(1, sizeof(struct TextQuantizePara));
    if (NULL==text_quan){
        releaseMemoryPreEncode(text_quan);
        printf("text_quan---->Allocated memory fail!!");
        return;
    }
    context->text_quan_para = text_quan;
    text_quan->color_space_formate = GLOBAL_COLOR_FORMAT;
    //初始化量化、预测的中间变量
    allocMemoryPreEncode(max_block_num, context->text_quan_para);
    //给哈夫曼编码分配内存
    HuffmanEncodeContext *huffman_context = NULL;
    initializeHuffmanEncodeContext(HUFFMAN_CODE_SYMBOL_NUM, &huffman_context);
    context->huffman_encode_context = huffman_context;

    context->estimate_symbols = (enum EstimateSymbol *) calloc(num_pixel, sizeof(enum EstimateSymbol));
    if (NULL==context->estimate_symbols){
        releaseTextEncoderContext(context);
        printf("context->estimate_symbols---->Allocated memory fail!!");
        return;
    }
    context->p_group_symbols = (short *) calloc(num_pixel / 4, sizeof(short));
    if (NULL==context->p_group_symbols){
        releaseTextEncoderContext(context);
        printf("context->p_group_symbols---->Allocated memory fail!!");
        return;
    }
    context->p_o_symbol_group = (short *) calloc(num_pixel, sizeof(short));
    if (NULL==context->p_o_symbol_group){
        releaseTextEncoderContext(context);
        printf("context->p_o_symbol_group---->Allocated memory fail!!");
        return;
    }
    context->p_escape_color = (short *) calloc(3 * num_pixel, sizeof(short));
    if (NULL==context->p_escape_color){
        releaseTextEncoderContext(context);
        printf("context->p_escape_color---->Allocated memory fail!!");
        return;
    }
    context->p_ktext_object=(struct TextEncodeObject *)calloc(sizeof(struct TextEncodeObject), 1);
    if (NULL==context->p_ktext_object){
        releaseTextEncoderContext(context);
        printf("context->p_ktext_object---->Allocated memory fail!!");
        return;
    }

    context->p_ktext_object->ktext_mb_count = 0;
	context->p_ktext_object->t_cnt_rdo = 0;
    context->p_ktext_object->p_ktext_mb = (pixel *)calloc(max_block_num*3*16*16, sizeof(pixel));
    context->p_ktext_object->p_ktext_mb_histogram = (int *)calloc(3*MAX_COLOR_COUNT_OF_ONE_COMPONENT*max_block_num, sizeof(int));

    context->group_symbols_num = 0;
    context->escape_color_num = 0;
    //初始化量化因子
    context->quantize_factor.delta1 = PRESET_DELTA1;
    context->quantize_factor.delta2 = PRESET_DELTA2;

    return;
}

void releaseTextEncoderContext(struct TextEncoderContext *text_encoder_context) {

    if (text_encoder_context != NULL) {
        //释放初始化量化、预测的中间变量
        releaseMemoryPreEncode(text_encoder_context->text_quan_para);

        //释放哈夫曼编码分配内存
        releaseHuffmanEncodeContext(text_encoder_context->huffman_encode_context);
        safe_free(text_encoder_context->estimate_symbols);
        safe_free(text_encoder_context->p_group_symbols);
        safe_free(text_encoder_context->p_o_symbol_group);
        safe_free(text_encoder_context->p_escape_color);
    }

    safe_free(text_encoder_context->p_ktext_object->p_ktext_mb);
    safe_free(text_encoder_context->p_ktext_object->p_ktext_mb_histogram);
    safe_free(text_encoder_context->p_ktext_object);

    safe_free(text_encoder_context);

    return;
}

int histogramGenerate_char(pixel *p_data, int mb_height, int mb_width, int *histogram) {

    memset(histogram, 0, MAX_COLOR_COUNT_OF_ONE_COMPONENT * sizeof(int));
	int i = 0;
	for (i = 0; i < mb_height*mb_width; i++) {
		histogram[p_data[i]]++;
	}    
    return 0;
}

int collectKTextMBData(pixel *p_text_mb_y, pixel *p_text_mb_u, pixel *p_text_mb_v, struct TextEncodeObject *p_object) {

	int ktext_index = p_object->ktext_mb_count;
    pixel *p_ktext_mb_y = p_object->p_ktext_mb + ktext_index * 3*MB_PIXEL_COUNT;
    pixel *p_ktext_mb_u = p_object->p_ktext_mb + (ktext_index * 3 + 1)*MB_PIXEL_COUNT;
    pixel *p_ktext_mb_v = p_object->p_ktext_mb + (ktext_index * 3 + 2)*MB_PIXEL_COUNT;
	memcpy(p_ktext_mb_y, p_text_mb_y, MB_PIXEL_COUNT * sizeof(pixel));
	memcpy(p_ktext_mb_u, p_text_mb_u, MB_PIXEL_COUNT * sizeof(pixel));
	memcpy(p_ktext_mb_v, p_text_mb_v, MB_PIXEL_COUNT * sizeof(pixel));
    
    int *histogram_y = p_object->p_ktext_mb_histogram + ktext_index*3*MAX_COLOR_COUNT_OF_ONE_COMPONENT;
    int *histogram_u = p_object->p_ktext_mb_histogram + (ktext_index*3+1)*MAX_COLOR_COUNT_OF_ONE_COMPONENT;
    int *histogram_v = p_object->p_ktext_mb_histogram + (ktext_index*3+2)*MAX_COLOR_COUNT_OF_ONE_COMPONENT;
    histogramGenerate_char(p_ktext_mb_y, MB_HEIGHT, MB_WIDTH, histogram_y);
    histogramGenerate_char(p_ktext_mb_u, MB_HEIGHT, MB_WIDTH, histogram_u);
    histogramGenerate_char(p_ktext_mb_v, MB_HEIGHT, MB_WIDTH, histogram_v);

    p_object->ktext_mb_count ++;
    return 1;
}


void releaseMemoryPreEncode(struct TextQuantizePara *text_quantize) {
    if (text_quantize != NULL) {
        if(text_quantize->bc_type.base_color420 != NULL){
            safe_free(text_quantize->bc_type.base_color420);
        }
        if(text_quantize->bc_type.base_color444 != NULL){
            safe_free(text_quantize->bc_type.base_color444);
        }
        safe_free(text_quantize->quantized_image);
        safe_free(text_quantize->o_quantized_index_map);
        safe_free(text_quantize->quantized_index_map);
        safe_free(text_quantize->Tree_base_color);
    }
    safe_free(text_quantize);

    return;
}

void allocMemoryPreEncode(int block_num, struct TextQuantizePara *text_quantize_para) {
    int num_pixel = block_num * (MB_WIDTH * MB_HEIGHT * 3);

    if (text_quantize_para != NULL) {
        text_quantize_para->Tree_base_color = (short *) calloc(block_num*9, sizeof(short));
        if (NULL==text_quantize_para->Tree_base_color){
            releaseMemoryPreEncode(text_quantize_para);
            printf("text_quantize_para->Tree_base_color---->Allocated memory fail!!");
            return;
        }
        if(kYUV444 == text_quantize_para->color_space_formate){
            // num_pixel = block_num * (MB_WIDTH * MB_HEIGHT * 3);
            text_quantize_para->bc_type.base_color444 = (struct BaseColor444 *) calloc(block_num, sizeof(struct BaseColor444));
            if (NULL==text_quantize_para->bc_type.base_color444){
                releaseMemoryPreEncode(text_quantize_para);
                printf("text_quantize_para->bc_type.base_color444---->Allocated memory fail!!");
                return;
            }
            text_quantize_para->quantized_image = (pixel *) calloc(num_pixel, sizeof(pixel));
            if (NULL==text_quantize_para->quantized_image){
                releaseMemoryPreEncode(text_quantize_para);
                printf("text_quantize_para->quantize_para---->Allocated memory fail!!");
                return;
            }
            text_quantize_para->quantized_index_map = (struct QuantizeIndex *) calloc(num_pixel, sizeof(struct QuantizeIndex));
            if (NULL==text_quantize_para->quantized_index_map){
                releaseMemoryPreEncode(text_quantize_para);
                printf("text_quantize_para->quantized_index_map---->Allocated memory fail!!");
                return;
            }
        }
        else{
            printf("Illegal color space formate!!");
            return;
        }
        //NULL_POINT_CHECK(text_quantize_para->base_color);
        //text_quantize_para->quantized_index_map = (struct QuantizeIndex *) calloc(num_pixel, sizeof(struct QuantizeIndex));
        //NULL_POINT_CHECK(text_quantize_para->quantized_index_map);
        text_quantize_para->o_quantized_index_map = (struct QuantizeIndex *) calloc(num_pixel, sizeof(struct QuantizeIndex));
        if (NULL==text_quantize_para->o_quantized_index_map){
            releaseMemoryPreEncode(text_quantize_para);
            printf("text_quantize_para->o_quantized_index_map---->Allocated memory fail!!");
            return;
        }
        //NULL_POINT_CHECK(text_quantize_para->o_quantized_index_map);
    }
    return;
}

void codeTextBitStream(const unsigned int num_blocks,
                       struct TextQuantizePara *const text_quantize,
                       struct HuffmanEncodeContext *const huff_context,
                       const struct QuantizeFactor *quantize_factor,
                       short *const escape_color,
                       int escape_color_num,
                       short *const group_symbols,
                       int group_symbols_num,
                       short *const o_symbol_group,
                       int o_symbols_num,
                       bs_t *bitstream) {

    struct TextQuantizePara *quantize_context = text_quantize;

	bs_write(bitstream, BYTE_LENGTH * 2, (uint32_t)num_blocks);    

    //encode base color
    if(kYUV444 == quantize_context->color_space_formate) {
        int base_color_length = encodeWithHuffman((short *) quantize_context->bc_type.base_color444, num_blocks * 3 * BASE_COLOR_NUMBER,
                                                  TEXT_NORMAL_BASE_COLOR_RANGE, huff_context, bitstream);
    }
    else  if(kYUV420p == quantize_context->color_space_formate){
        int base_color_length = encodeWithHuffman((short *) quantize_context->bc_type.base_color420,  num_blocks * ( BASE_COLOR_NUMBER + 2 * BASE_COLOR_NUMBER2),
                                                  TEXT_NORMAL_BASE_COLOR_RANGE, huff_context, bitstream);
    }    
    //encode group symbols
    int group_lenght = encodeWithHuffman(group_symbols, group_symbols_num,
                                         GROUP_SYMBOL_HUFF_SYMBOL_NUMBER,
                                         huff_context, bitstream);

    //encode O symbol
    int o_symbol_length = encodeWithHuffman(o_symbol_group, o_symbols_num, O_SYMBOL_HUFF_SYMBOL_NUMBER,
                                            huff_context, bitstream);


    //encode escape color
    int escape_color_huff_symbol_num = (COLOR_RANGE - 1)/quantize_factor->delta2 + 1;
    int escape_color_length = encodeWithHuffman(escape_color, escape_color_num,
                                                escape_color_huff_symbol_num,
                                                huff_context, bitstream);
	bs_align_1(bitstream);

	return;
}

static short fitLinePattern(const enum EstimateSymbol *const line_start) {
    enum EstimateSymbol reference = line_start[0];
    for (int i = 1; i < MB_WIDTH; i++) {
        if (reference != line_start[i]) {
            //Line pattern not fit
            return -1;
        }
    }
    return reference;
}

void hierarchyPatternCode(const enum EstimateSymbol *const estimate_symbols,
                          const int num_blocks, short *const group_symbols,
                          int *group_symbols_size) {
    int group_size = 0;
    int group_symbol_value = 0;
    short *group_line_symbols = group_symbols;
    const enum EstimateSymbol const *symbols_block = estimate_symbols;
    int squre_mult_factor = square_value(ESTIMATE_SYMBOL_NUMBER);
    int cube_mult_factor = cube_value(ESTIMATE_SYMBOL_NUMBER);

    for (int i = 0; i < num_blocks; i++) {
        enum EstimateSymbol const *symbols_line = symbols_block;
        for (int j = 0; j < MB_HEIGHT; j++) {
            short result = fitLinePattern(symbols_line);
            if (result >= 0) {
                group_symbol_value = (short) (result + ESTIMATE_SYMBOL_LINE_PATTERN_OFFSET);
                *group_line_symbols++ = group_symbol_value;
                group_size++;
            } else {
                group_symbol_value = symbols_line[0]
                                     + symbols_line[1] * ESTIMATE_SYMBOL_NUMBER
                                     + symbols_line[2] * squre_mult_factor
                                     + symbols_line[3] * cube_mult_factor;
                *group_line_symbols++ = group_symbol_value;

                group_symbol_value = symbols_line[4]
                                     + symbols_line[5] * ESTIMATE_SYMBOL_NUMBER
                                     + symbols_line[6] * squre_mult_factor
                                     + symbols_line[7] * cube_mult_factor;
                *group_line_symbols++ = group_symbol_value;

                group_symbol_value = symbols_line[8]
                                     + symbols_line[9] * ESTIMATE_SYMBOL_NUMBER
                                     + symbols_line[10] * squre_mult_factor
                                     + symbols_line[11] * cube_mult_factor;
                *group_line_symbols++ = group_symbol_value;

                group_symbol_value = symbols_line[12]
                                     + symbols_line[13] * ESTIMATE_SYMBOL_NUMBER
                                     + symbols_line[14] * squre_mult_factor
                                     + symbols_line[15] * cube_mult_factor;
                *group_line_symbols++ = group_symbol_value;

                group_size += 4;
            }

            symbols_line += MB_WIDTH;
        }

        symbols_block += MB_PIXEL_COUNT;
    }

    *group_symbols_size = group_size;
    return;
}

void updateNeighborBlockPixel(int block_index, struct QuantizeIndex *quan_index_block, struct QuantizeIndex *up_neighbor_index, struct QuantizeIndex *left_neighbor_index)
{

    if (0 != block_index){
        struct QuantizeIndex *left_block_index =
                quan_index_block + (block_index - 1) * MB_PIXEL_COUNT;
        for (int i = 0; i < MB_HEIGHT; i++) {
            left_neighbor_index[i] = *(left_block_index + MB_WIDTH - 1);
            left_block_index += MB_WIDTH;
        }
    }else{
        memset(left_neighbor_index, 0xff, sizeof(struct QuantizeIndex) * MB_HEIGHT);
    }
    memset(up_neighbor_index, 0xff, sizeof(struct QuantizeIndex) * MB_WIDTH);
    return;
}

static int isQuantizeIndexEqual(const struct QuantizeIndex *index1, const struct QuantizeIndex *index2) {
    int result = (index1->component1 == index2->component1) && (index1->component2 == index2->component2) &&
                  (index1->component3 == index2->component3);
    return result;
}

void estimateSymbolFromIndex(struct QuantizeIndex *quan_index_block, struct QuantizeIndex *up_neighbor_index,
                             struct QuantizeIndex *left_neighbor_index, enum EstimateSymbol *estimated_symbols,
                             struct QuantizeIndex *o_quantize_index, int *o_quantize_index_num, short *o_symbol_group) {

    int num_o_index = 0;
    short o_symbol_value = 0;

    short *o_symbol = o_symbol_group;
    struct QuantizeIndex *o_index = o_quantize_index;

    int mult_factor1 = BASE_COLOR_NUMBER + 1;
    int mult_factor2 = square_value(mult_factor1);
    struct QuantizeIndex *quantize_index = quan_index_block;
    enum EstimateSymbol *est_symbols = estimated_symbols;
    struct QuantizeIndex left_index = left_neighbor_index[0];

    struct QuantizeIndex up_index[MB_WIDTH];
    memcpy(up_index, up_neighbor_index, MB_WIDTH*sizeof(struct QuantizeIndex));

    for (int j = 0; j < MB_HEIGHT; j++) {

        left_index = left_neighbor_index[j];
        for (int i = 0; i < MB_WIDTH; i++) {
            //uodate up index
            if (j != 0) {
                up_index[i] = *(quantize_index - MB_WIDTH + i);
            }

            if (isQuantizeIndexEqual(quantize_index + i, &(left_index))) {
                est_symbols[i] = Left;
                //left_index = *(quantize_index + i);
            }
            else if (isQuantizeIndexEqual(quantize_index + i, &(up_index[i]))) {
                est_symbols[i] = Up;

                //update up neighbor index
            }
            else {
                est_symbols[i] = Other;
                memcpy(o_index, quantize_index + i, sizeof(struct QuantizeIndex));

                o_symbol_value = o_index->component1 * mult_factor2
                                 + o_index->component2 * mult_factor1
                                 + o_index->component3;

                *o_symbol = o_symbol_value;
                o_index++;
                num_o_index++;
                o_symbol++;
            }

            left_index = *(quantize_index + i);
        }

        //next row offset
        quantize_index += MB_WIDTH;
        est_symbols += MB_WIDTH;
    }

    *o_quantize_index_num = num_o_index;
    return;
}

void QuantizedIndexPredict(const unsigned int num_blocks,
                           enum EstimateSymbol *const estimated_symbols,
                           struct TextQuantizePara *const text_quantize,
                           int *num_o_quantized_index,
                           short *const o_symbol_group)
{
    int o_index_total = 0;
    short *o_symbol_group_block = o_symbol_group;
    struct QuantizeIndex up_neighbor_quantize_index[MB_WIDTH] = {0xff};
    struct QuantizeIndex left_neighbor_quantize_index[MB_HEIGHT] = {0xff};

    enum EstimateSymbol *est_symbols = estimated_symbols;
    struct QuantizeIndex *quantize_index = text_quantize->quantized_index_map;
    struct QuantizeIndex *quantize_index_update = text_quantize->quantized_index_map;
    struct QuantizeIndex *o_quantize_index = text_quantize->o_quantized_index_map;

    for (int block_index = 0; block_index < num_blocks; block_index++) {
        updateNeighborBlockPixel(block_index, quantize_index, up_neighbor_quantize_index, left_neighbor_quantize_index);

        int o_index_num = 0;
        //estimate symbol(the quantize_index_update used to update the next block quantize index)
        estimateSymbolFromIndex(quantize_index_update, up_neighbor_quantize_index, left_neighbor_quantize_index,
                                est_symbols, o_quantize_index, &o_index_num, o_symbol_group_block);

        //next macro block
        quantize_index_update += MB_PIXEL_COUNT;
        est_symbols += MB_PIXEL_COUNT;
        o_quantize_index += o_index_num;
        o_symbol_group_block += o_index_num;
        o_index_total += o_index_num;
    }

    *num_o_quantized_index = o_index_total;
    return;
}

/*
static void calcTextBaseColorComponent(const int delta1, const int *image_histogram,
	short *base_color_component, int base_color_num,
	int *base_color_map, char *quantize_index_map) {
	int range = delta1 * 2;
	int num = ((COLOR_RANGE - 1) / range) + 1;
	//int range_frequency[num][range + 1];  //vs似乎不支持c99
	int col_size = range + 1;
	int * range_frequency = (int *)malloc(num * col_size * sizeof(int));
	if (!range_frequency)
		return;
	memset(range_frequency, 0, num*(range + 1) * sizeof(int));
	int range_hist[COLOR_RANGE];
	for (int j = 0; j < COLOR_RANGE; ++j) {
		range_hist[j] = 0;
	}

	for (int r = 0; r < TEXT_MB_HISTOGRAM_COMP_STRIDE; r++) {
		for (int i = 0; i <= delta1; ++i) {
			if (r - i >= 0 && image_histogram[r - i] > 0) {
				range_hist[r] += image_histogram[r - i];
			}
			if (r + i < COLOR_RANGE && image_histogram[r + i] > 0) {
				range_hist[r] += image_histogram[r + i];
			}
		}
	}

	//寻找区间内频率最大的灰度值
	for (int i = 0; i < base_color_num; i++) {
		int max_position = 0;
		for (int l = 0; l < COLOR_RANGE; ++l) {
			if (range_hist[l] > range_hist[max_position]) {
				max_position = l;
			}
		}
		base_color_component[i] = max_position;

		//映射范围以base color为中心的四个像素点
		for (int kk = 0; kk <= 1.5*delta1; kk++) {
			if (((base_color_component[i] + kk) < COLOR_RANGE) && (base_color_map[base_color_component[i] + kk] == -1)) {
				base_color_map[base_color_component[i] + kk] = base_color_component[i];
				quantize_index_map[base_color_component[i] + kk] = i;
				range_hist[base_color_component[i] + kk] = 0;
			}

			if (((base_color_component[i] - kk) > 0) && (base_color_map[base_color_component[i] - kk] == -1)) {
				base_color_map[base_color_component[i] - kk] = base_color_component[i];
				quantize_index_map[base_color_component[i] - kk] = i;
				range_hist[base_color_component[i] - kk] = 0;
			}
		}
		range_hist[max_position] = 0;
	}

	//将众数变为加权平均
	for (int l = 0; l < base_color_num; ++l) {
		int count = 0;
		int sum = 0;
		float temp = 0;
		for (int i = 0; i < MB_PIXEL_COUNT; ++i) {
			if (quantize_index_map[i] == l) {
				sum += image_histogram[i] * i;
				count += image_histogram[i];
			}
		}
		temp = (float)sum / count;
		if (temp < base_color_component[l]) {
			base_color_component[l] = (short)ceil(temp);
		}
		else {
			base_color_component[l] = (short)floor(temp);
		}
	}

	free(range_frequency);
}
*/

static void calcTextBaseColorComponent(const int delta1, const int *image_histogram,
	short *base_color_component, int base_color_num,
	int *base_color_map, char *quantize_index_map) {
	int range_hist[COLOR_RANGE];
	for (int j = 0; j < COLOR_RANGE; ++j) {
		range_hist[j] = 0;
	}

	for (int r = 0; r < TEXT_MB_HISTOGRAM_COMP_STRIDE; r++) {
		for (int i = 0; i <= delta1; ++i) {
			if (r - i >= 0 && image_histogram[r - i] > 0) {
				range_hist[r] += image_histogram[r - i];
			}
			if (r + i < COLOR_RANGE && image_histogram[r + i] > 0) {
				range_hist[r] += image_histogram[r + i];
			}
		}
	}

	//寻找区间内频率最大的灰度值
	for (int i = 0; i < base_color_num; i++) {
		int max_position = 0;
		for (int l = 0; l < COLOR_RANGE; ++l) {
			if (range_hist[l] > range_hist[max_position]) {
				max_position = l;
			}
		}
		base_color_component[i] = max_position;

		//映射范围以base color为中心的四个像素点
		for (int kk = 0; kk <= delta1; kk++) {
			if (((base_color_component[i] + kk) < COLOR_RANGE) && (base_color_map[base_color_component[i] + kk] == -1)) {
				if ((i <= 1) && image_histogram[base_color_component[i] + kk] != 0
					&& image_histogram[base_color_component[i] + kk] >= 2 * image_histogram[base_color_component[i]]) {
					continue;
				}
				base_color_map[base_color_component[i] + kk] = base_color_component[i];
				quantize_index_map[base_color_component[i] + kk] = i;
				range_hist[base_color_component[i] + kk] = 0;
			}
		}
		for (int kk = 0; kk <= delta1; kk++) {
			if (((base_color_component[i] - kk) > 0) && (base_color_map[base_color_component[i] - kk] == -1)) {
				if ((i <= 1) && image_histogram[base_color_component[i] - kk] && image_histogram[base_color_component[i] + kk] != 0
					&& image_histogram[base_color_component[i] + kk] >= 2 * image_histogram[base_color_component[i]]) {
					continue;
				}
				base_color_map[base_color_component[i] - kk] = base_color_component[i];
				quantize_index_map[base_color_component[i] - kk] = i;
				range_hist[base_color_component[i] - kk] = 0;
			}
		}
		range_hist[max_position] = 0;
	}

	for (int l = 0; l < base_color_num; ++l) {
		int count = 0;
		int sum = 0;
		float temp = 0;
		for (int i = 0; i < MB_PIXEL_COUNT; ++i) {
			if (quantize_index_map[i] == l) {
				sum += image_histogram[i] * i;
				count += image_histogram[i];
			}
		}
		temp = (float)sum / count;
		if (temp < base_color_component[l]) {
			base_color_component[l] = (short)ceil(temp);
		}
		else {
			base_color_component[l] = (short)floor(temp);
		}
	}
}


void quantizePixelPatternNoResidual(const struct QuantizeFactor *quantize_factor, pixel image_pixel,
                                    pixel *quantized_image, int *quantize_index, short *escape_color,  int *num_escape_color,
                                    int base_color_num, int *base_color_map, char *quantize_index_map) {

    const int delta2 = quantize_factor->delta2;

    if (quantize_index_map[image_pixel] != BASE_COLOR_NUMBER) {
        *quantized_image =(pixel) base_color_map[image_pixel];
        *quantize_index = quantize_index_map[image_pixel];
        return;
    }

    short quantize_value = 0;
    int quantize_pixel = 0;
    quantize_value = image_pixel/delta2;
    quantize_pixel = (quantize_value * delta2 + (delta2 >> 1)) < COLOR_RANGE ? ( quantize_value * delta2 +
                                                                                 (delta2 >> 1)) : COLOR_RANGE;
    *quantized_image =(pixel)quantize_pixel;
    *quantize_index = base_color_num;
    *escape_color = quantize_value;
    (*num_escape_color)++;

    return;
}

void textQuantizeNoResidual444Formate(const unsigned int num_blocks, const pixel *image_data,  const int *image_histogram,
                                      const struct QuantizeFactor *quantize_factor, struct TextQuantizePara *const text_quantize,
                                      short *const escape_color, int *escape_color_num)
{
    short *escape_color_block = escape_color;
    int quantized_index_value = 0;
    int base_color_map[TEXT_MB_HISTOGRAM_BLOCK_STRIDE];
    char quantize_index_map[TEXT_MB_HISTOGRAM_BLOCK_STRIDE];
    for (int block_index = 0; block_index < num_blocks; block_index++) {
        int block_y_pos = block_index * XOR_STRIDE;
        int block_u_pos = block_y_pos + MB_PIXEL_COUNT;
        int block_v_pos = block_u_pos + MB_PIXEL_COUNT;
        int block_pos = block_index * MB_PIXEL_COUNT;
        int block_histogram_pos = block_index * TEXT_MB_HISTOGRAM_BLOCK_STRIDE;

        memset(quantize_index_map, BASE_COLOR_NUMBER, TEXT_MB_HISTOGRAM_BLOCK_STRIDE*sizeof(char)); //将索引赋初值4
        memset(base_color_map, -1, TEXT_MB_HISTOGRAM_BLOCK_STRIDE*sizeof(int));

        calcTextBaseColorComponent(quantize_factor->delta1, &(image_histogram[block_histogram_pos]),
                                   text_quantize->bc_type.base_color444[block_index].component1,
                                   BASE_COLOR_NUMBER, &(base_color_map[0]), &(quantize_index_map[0]));
        calcTextBaseColorComponent(quantize_factor->delta1, &(image_histogram[block_histogram_pos + TEXT_MB_HISTOGRAM_COMP_STRIDE]),
                                   text_quantize->bc_type.base_color444[block_index].component2,
                                   BASE_COLOR_NUMBER, &(base_color_map[TEXT_MB_HISTOGRAM_COMP_STRIDE]),
                                   &(quantize_index_map[TEXT_MB_HISTOGRAM_COMP_STRIDE]));
        calcTextBaseColorComponent(quantize_factor->delta1, &(image_histogram[block_histogram_pos + TEXT_MB_HISTOGRAM_COMP_STRIDE*2]),
                                   text_quantize->bc_type.base_color444[block_index].component3,
                                   BASE_COLOR_NUMBER,  &(base_color_map[TEXT_MB_HISTOGRAM_COMP_STRIDE * 2]),
                                   &(quantize_index_map[TEXT_MB_HISTOGRAM_COMP_STRIDE * 2]));

        for (int i = 0; i < MB_HEIGHT; i++) {
            for (int j = 0; j < MB_WIDTH; j++) {
                int size_escape_color1 = 0;
                quantizePixelPatternNoResidual(quantize_factor, image_data[block_y_pos + i * MB_WIDTH + j],
                                               &(text_quantize->quantized_image[block_y_pos + i * MB_WIDTH + j]),
                                               &quantized_index_value, escape_color_block, &size_escape_color1,
                                               BASE_COLOR_NUMBER, &(base_color_map[0]), &(quantize_index_map[0]));
                text_quantize->quantized_index_map[block_pos + i * MB_WIDTH + j].component1 = quantized_index_value;
                escape_color_block += size_escape_color1;
                *escape_color_num += size_escape_color1;
            }
        }
        for (int i = 0; i < MB_HEIGHT; i++) {
            for (int j = 0; j < MB_WIDTH; j++) {
                int size_escape_color2 = 0;
                quantizePixelPatternNoResidual(quantize_factor, image_data[block_u_pos + i * MB_WIDTH + j],
                                               &(text_quantize->quantized_image[block_u_pos + i * MB_WIDTH + j]),
                                               &quantized_index_value, escape_color_block, &size_escape_color2,
                                               BASE_COLOR_NUMBER, &(base_color_map[TEXT_MB_HISTOGRAM_COMP_STRIDE]),
                                               &(quantize_index_map[TEXT_MB_HISTOGRAM_COMP_STRIDE]));
                text_quantize->quantized_index_map[block_pos + i * MB_WIDTH + j].component2 = quantized_index_value;
                escape_color_block += size_escape_color2;
                *escape_color_num += size_escape_color2;
            }
        }
        for (int i = 0; i < MB_HEIGHT; i++) {
            for (int j = 0; j < MB_WIDTH; j++) {
                int size_escape_color3 = 0;
                quantizePixelPatternNoResidual(quantize_factor, image_data[block_v_pos + i * MB_WIDTH + j],
                                               &(text_quantize->quantized_image[block_v_pos + i * MB_WIDTH + j]),
                                               &quantized_index_value, escape_color_block, &size_escape_color3,
                                               BASE_COLOR_NUMBER,  &(base_color_map[TEXT_MB_HISTOGRAM_COMP_STRIDE * 2]),
                                               &(quantize_index_map[TEXT_MB_HISTOGRAM_COMP_STRIDE * 2]));
                text_quantize->quantized_index_map[block_pos + i * MB_WIDTH + j].component3 = quantized_index_value;
                escape_color_block += size_escape_color3;

                *escape_color_num += size_escape_color3;
            }
        }
    }
    return;
}

void  textQuantize444(const unsigned int num_blocks, const pixel *image_data, const int *image_histogram,
                      const struct QuantizeFactor *quantize_factor, struct TextQuantizePara *const text_quantize,
                      short *const escape_color, int *escape_color_num, ResidualBlock *residual_block, int residual_flag){
	textQuantizeNoResidual444Formate(num_blocks, image_data, image_histogram, quantize_factor, text_quantize, escape_color, escape_color_num);    
}

int encodeText(bs_t *bitstream,struct TextEncoderContext *const text_encoder_context) {

    int residual_flag=0;
    ResidualBlock *residual_block=NULL;
    struct TextEncodeObject *text_mb_recognize_result=text_encoder_context->p_ktext_object;

    //计算宏块数量
    int escape_color_size = 0;
    int num_symbol_group = 0;
    const unsigned int num_blocks = text_mb_recognize_result->ktext_mb_count;
    struct TextQuantizePara *text_quantize = text_encoder_context->text_quan_para;
    struct HuffmanEncodeContext *huffman_context = text_encoder_context->huffman_encode_context;
    short *group_symbols = text_encoder_context->p_group_symbols;
    short *o_symbol_group = text_encoder_context->p_o_symbol_group;
    enum EstimateSymbol *estimate_symbol = text_encoder_context->estimate_symbols;
    short *escape_color = text_encoder_context->p_escape_color;
    ResidualBlock *residual = residual_block;

	textQuantize444(num_blocks, text_mb_recognize_result->p_ktext_mb, text_mb_recognize_result->p_ktext_mb_histogram,
		&(text_encoder_context->quantize_factor), text_quantize, escape_color, &escape_color_size, residual, residual_flag);

    text_encoder_context->escape_color_num = escape_color_size;

	//predict quantize index，estimate symbol, o_quantize_index
    int num_o_quantized_index = 0;
    QuantizedIndexPredict(num_blocks, estimate_symbol,
                          text_quantize, &num_o_quantized_index, o_symbol_group);

    //predict group symbols
    hierarchyPatternCode(estimate_symbol, num_blocks, group_symbols, &num_symbol_group);
    text_encoder_context->group_symbols_num = num_symbol_group;

    //Generate bit stream
    codeTextBitStream(num_blocks, text_quantize, huffman_context, &(text_encoder_context->quantize_factor),
                      escape_color, escape_color_size, group_symbols,
                      num_symbol_group, o_symbol_group, num_o_quantized_index, bitstream);

    return 1 ;
}

void quantizeTextMB(int block_index, const pixel *image_data,  const int *image_histogram, const struct QuantizeFactor *quantize_factor,
                    struct TextQuantizePara *text_quantize, short *const escape_color, int *escape_color_num) {
    short *escape_color_block = escape_color;
    int quantized_index_value = 0;
    int base_color_map[TEXT_MB_HISTOGRAM_BLOCK_STRIDE];
    char quantize_index_map[TEXT_MB_HISTOGRAM_BLOCK_STRIDE];
    int block_y_pos = block_index * XOR_STRIDE;
    int block_u_pos = block_y_pos + MB_PIXEL_COUNT;
    int block_v_pos = block_u_pos + MB_PIXEL_COUNT;
    int block_pos = block_index * MB_PIXEL_COUNT;
    int block_histogram_pos = block_index * TEXT_MB_HISTOGRAM_BLOCK_STRIDE;

    memset(quantize_index_map, BASE_COLOR_NUMBER, TEXT_MB_HISTOGRAM_BLOCK_STRIDE*sizeof(char)); //将索引赋初值4
    memset(base_color_map, -1, TEXT_MB_HISTOGRAM_BLOCK_STRIDE*sizeof(int));

    calcTextBaseColorComponent(quantize_factor->delta1, &(image_histogram[block_histogram_pos]),
                               text_quantize->bc_type.base_color444[block_index].component1,
                               BASE_COLOR_NUMBER, &(base_color_map[0]), &(quantize_index_map[0]));
    calcTextBaseColorComponent(quantize_factor->delta1, &(image_histogram[block_histogram_pos + TEXT_MB_HISTOGRAM_COMP_STRIDE]),
                               text_quantize->bc_type.base_color444[block_index].component2,
                               BASE_COLOR_NUMBER, &(base_color_map[TEXT_MB_HISTOGRAM_COMP_STRIDE]),
                               &(quantize_index_map[TEXT_MB_HISTOGRAM_COMP_STRIDE]));
    calcTextBaseColorComponent(quantize_factor->delta1, &(image_histogram[block_histogram_pos + TEXT_MB_HISTOGRAM_COMP_STRIDE*2]),
                               text_quantize->bc_type.base_color444[block_index].component3,
                               BASE_COLOR_NUMBER,  &(base_color_map[TEXT_MB_HISTOGRAM_COMP_STRIDE * 2]),
                               &(quantize_index_map[TEXT_MB_HISTOGRAM_COMP_STRIDE * 2]));

    for (int i = 0; i < MB_HEIGHT; i++) {
        for (int j = 0; j < MB_WIDTH; j++) {
            int size_escape_color1 = 0;
            quantizePixelPatternNoResidual(quantize_factor, image_data[block_y_pos + i * MB_WIDTH + j],
                                           &(text_quantize->quantized_image[block_y_pos + i * MB_WIDTH + j]),
                                           &quantized_index_value, escape_color_block, &size_escape_color1,
                                           BASE_COLOR_NUMBER, &(base_color_map[0]), &(quantize_index_map[0]));
            text_quantize->quantized_index_map[block_pos + i * MB_WIDTH + j].component1 = quantized_index_value;
            escape_color_block += size_escape_color1;
            *escape_color_num += size_escape_color1;
        }
    }
    for (int i = 0; i < MB_HEIGHT; i++) {
        for (int j = 0; j < MB_WIDTH; j++) {
            int size_escape_color2 = 0;
            quantizePixelPatternNoResidual(quantize_factor, image_data[block_u_pos + i * MB_WIDTH + j],
                                           &(text_quantize->quantized_image[block_u_pos + i * MB_WIDTH + j]),
                                           &quantized_index_value, escape_color_block, &size_escape_color2,
                                           BASE_COLOR_NUMBER, &(base_color_map[TEXT_MB_HISTOGRAM_COMP_STRIDE]),
                                           &(quantize_index_map[TEXT_MB_HISTOGRAM_COMP_STRIDE]));
            text_quantize->quantized_index_map[block_pos + i * MB_WIDTH + j].component2 = quantized_index_value;
            escape_color_block += size_escape_color2;
            *escape_color_num += size_escape_color2;
        }
    }
    for (int i = 0; i < MB_HEIGHT; i++) {
        for (int j = 0; j < MB_WIDTH; j++) {
            int size_escape_color3 = 0;
            quantizePixelPatternNoResidual(quantize_factor, image_data[block_v_pos + i * MB_WIDTH + j],
                                           &(text_quantize->quantized_image[block_v_pos + i * MB_WIDTH + j]),
                                           &quantized_index_value, escape_color_block, &size_escape_color3,
                                           BASE_COLOR_NUMBER,  &(base_color_map[TEXT_MB_HISTOGRAM_COMP_STRIDE * 2]),
                                           &(quantize_index_map[TEXT_MB_HISTOGRAM_COMP_STRIDE * 2]));
            text_quantize->quantized_index_map[block_pos + i * MB_WIDTH + j].component3 = quantized_index_value;
            escape_color_block += size_escape_color3;

            *escape_color_num += size_escape_color3;
        }
    }	
    return;
}


int calcEscapeColor(struct TextEncoderContext *const text_encoder_context, unsigned char *p_text_mb_y, unsigned char *p_text_mb_u, unsigned char *p_text_mb_v, unsigned int stride) {
	int escape_num = 0;
	struct TextQuantizePara *text_quantize = text_encoder_context->text_quan_para;
	short *escape_color = text_encoder_context->p_escape_color;
	unsigned char *text_mb = (unsigned char *)calloc(768, sizeof(unsigned char));
	int *text_his = (int *)calloc(768, sizeof(int));
	unsigned char *p_ktext_mb_y = text_mb;
	unsigned char *p_ktext_mb_u = text_mb + 256;
	unsigned char *p_ktext_mb_v = text_mb + 512;
	for (int j = 0; j < MB_HEIGHT; j++) {
		memcpy(p_ktext_mb_y, p_text_mb_y, MB_WIDTH);
		memcpy(p_ktext_mb_u, p_text_mb_u, MB_WIDTH);
		memcpy(p_ktext_mb_v, p_text_mb_v, MB_WIDTH);

		p_ktext_mb_y += MB_WIDTH;
		p_ktext_mb_u += MB_WIDTH;
		p_ktext_mb_v += MB_WIDTH;

		//注意，这里由于输入的是整帧图像, 因此stride为输入参数
		p_text_mb_y += stride;
		p_text_mb_u += stride;
		p_text_mb_v += stride;
	}
	p_ktext_mb_y = text_mb;
	p_ktext_mb_u = text_mb + 256;
	p_ktext_mb_v = text_mb + 512;
	int *histogram_y = text_his;
	int *histogram_u = text_his + MAX_COLOR_COUNT_OF_ONE_COMPONENT;
	int *histogram_v = text_his + 2 * MAX_COLOR_COUNT_OF_ONE_COMPONENT;
	histogramGenerate_char(p_ktext_mb_y, MB_HEIGHT, MB_WIDTH, histogram_y);
	histogramGenerate_char(p_ktext_mb_u, MB_HEIGHT, MB_WIDTH, histogram_u);
	histogramGenerate_char(p_ktext_mb_v, MB_HEIGHT, MB_WIDTH, histogram_v);
	quantizeTextMB(0, text_mb, text_his, &(text_encoder_context->quantize_factor), text_quantize, escape_color, &escape_num);
	safe_free(text_mb);
	safe_free(text_his);
	return escape_num;
}

int preProcessTextMB(struct TextEncoderContext *text_encoder_context, pixel **p_text_mb_y, pixel **p_text_mb_u,pixel **p_text_mb_v)
{
    struct TextEncodeObject *p_text_object=text_encoder_context->p_ktext_object;
    collectKTextMBData(*p_text_mb_y, *p_text_mb_u, *p_text_mb_v, p_text_object);
    int escape_color_size = 0;
    struct TextQuantizePara *text_quantize = text_encoder_context->text_quan_para;
    short *escape_color = text_encoder_context->p_escape_color;
    int block_index = p_text_object->ktext_mb_count - 1;

    quantizeTextMB(block_index, p_text_object->p_ktext_mb, p_text_object->p_ktext_mb_histogram,
                   &(text_encoder_context->quantize_factor), text_quantize,escape_color,&escape_color_size);   

	int block_y_pos = block_index * XOR_STRIDE;
	int block_u_pos = block_y_pos + MB_PIXEL_COUNT;
	int block_v_pos = block_u_pos + MB_PIXEL_COUNT;
	*p_text_mb_y = text_quantize->quantized_image + block_y_pos;
	*p_text_mb_u = text_quantize->quantized_image + block_u_pos;
	*p_text_mb_v = text_quantize->quantized_image + block_v_pos;
	return 1;
}

int calculateMbSSE(unsigned char *original_mb, unsigned char *quantized_mb, double *sse) {
	double t_sse = 0;
	if (original_mb == NULL || quantized_mb == NULL)
		return -1;

	for (int i = 0; i < 3 * MB_PIXEL_COUNT; ++i) {
		t_sse += (original_mb[i] - quantized_mb[i])*(original_mb[i] - quantized_mb[i]);
	}
	*sse = t_sse;
	return 0;
}

int QuantizedIndexPredictMB(int num_blocks, enum EstimateSymbol *const estimated_symbols, struct TextQuantizePara *const text_quantize,
	int *num_o_quantized_index, short *const o_symbol_group) {
	short *o_symbol_group_block = o_symbol_group;
	struct QuantizeIndex up_neighbor_quantize_index[MB_WIDTH] = { 0xff };
	struct QuantizeIndex left_neighbor_quantize_index[MB_HEIGHT] = { 0xff };
	int o_index_num = 0;

	enum EstimateSymbol *est_symbols = estimated_symbols;
	struct QuantizeIndex *quantize_index_map = text_quantize->quantized_index_map;
	struct QuantizeIndex *quantize_index_update = &text_quantize->quantized_index_map[num_blocks*MB_PIXEL_COUNT];
	struct QuantizeIndex *o_quantize_index = &text_quantize->o_quantized_index_map[num_blocks*MB_PIXEL_COUNT];

	if (0 != num_blocks) {
		struct QuantizeIndex *left_block_index =
			quantize_index_map + (num_blocks - 1) * MB_PIXEL_COUNT;
		for (int i = 0; i < MB_HEIGHT; i++) {
			left_neighbor_quantize_index[i] = *(left_block_index + MB_WIDTH - 1);
			left_block_index += MB_WIDTH;
		}
	}
	else {
		memset(left_neighbor_quantize_index, 0xff, sizeof(struct QuantizeIndex) * MB_HEIGHT);
	}
	memset(up_neighbor_quantize_index, 0xff, sizeof(struct QuantizeIndex) * MB_WIDTH);

	estimateSymbolFromIndex(quantize_index_update, up_neighbor_quantize_index, left_neighbor_quantize_index,
		est_symbols, o_quantize_index, &o_index_num, o_symbol_group_block);
	*num_o_quantized_index = o_index_num;
	return 0;
}

int hierarchyPatternCodeMB(const enum EstimateSymbol *const estimate_symbols, const int num_blocks, short *const group_symbols,
	int *group_symbols_size) {
	int group_symbol_size = 0;
	int group_symbol_value = 0;
	short *group_line_symbols = group_symbols;
	const enum EstimateSymbol const *symbols_block = estimate_symbols;
	int squre_mult_factor = square_value(ESTIMATE_SYMBOL_NUMBER);
	int cube_mult_factor = cube_value(ESTIMATE_SYMBOL_NUMBER);

	enum EstimateSymbol const *symbols_line = symbols_block;
	for (int j = 0; j < MB_HEIGHT; j++) {
		short result = fitLinePattern(symbols_line);
		if (result >= 0) {
			group_symbol_value = (short)(result + ESTIMATE_SYMBOL_LINE_PATTERN_OFFSET);
			*group_line_symbols++ = (short)group_symbol_value;
			group_symbol_size++;
		}
		else {
			group_symbol_value = symbols_line[0]
				+ symbols_line[1] * ESTIMATE_SYMBOL_NUMBER
				+ symbols_line[2] * squre_mult_factor
				+ symbols_line[3] * cube_mult_factor;
			*group_line_symbols++ = (short)group_symbol_value;

			group_symbol_value = symbols_line[4]
				+ symbols_line[5] * ESTIMATE_SYMBOL_NUMBER
				+ symbols_line[6] * squre_mult_factor
				+ symbols_line[7] * cube_mult_factor;
			*group_line_symbols++ = (short)group_symbol_value;

			group_symbol_value = symbols_line[8]
				+ symbols_line[9] * ESTIMATE_SYMBOL_NUMBER
				+ symbols_line[10] * squre_mult_factor
				+ symbols_line[11] * cube_mult_factor;
			*group_line_symbols++ = (short)group_symbol_value;

			group_symbol_value = symbols_line[12]
				+ symbols_line[13] * ESTIMATE_SYMBOL_NUMBER
				+ symbols_line[14] * squre_mult_factor
				+ symbols_line[15] * cube_mult_factor;
			*group_line_symbols++ = (short)group_symbol_value;

			group_symbol_size += 4;
		}

		symbols_line += MB_WIDTH;
	}

	*group_symbols_size = group_symbol_size;
	return 0;
}


double analyseTextCost(unsigned char *p_text_mb_y, unsigned char *p_text_mb_u, unsigned char *p_text_mb_v, int lamda, struct TextEncoderContext *text_encoder_context) {
	double t_cost, t_bits, t_sse = 0;
	double coeff[6] = { 4.6004,-0.0010, 3.2686, -0.001130, 7.7472, -0.0064 };
	struct TextEncodeObject *text_object = text_encoder_context->p_ktext_object;

	int ktext_index = text_object->t_cnt_rdo;
	pixel *p_ktext_mb_y = text_object->p_ktext_mb + ktext_index * 3 * MB_PIXEL_COUNT;
	pixel *p_ktext_mb_u = text_object->p_ktext_mb + (ktext_index * 3 + 1)*MB_PIXEL_COUNT;
	pixel *p_ktext_mb_v = text_object->p_ktext_mb + (ktext_index * 3 + 2)*MB_PIXEL_COUNT;
	memcpy(p_ktext_mb_y, p_text_mb_y, MB_PIXEL_COUNT * sizeof(pixel));
	memcpy(p_ktext_mb_u, p_text_mb_u, MB_PIXEL_COUNT * sizeof(pixel));
	memcpy(p_ktext_mb_v, p_text_mb_v, MB_PIXEL_COUNT * sizeof(pixel));

	int *histogram_y = text_object->p_ktext_mb_histogram + ktext_index * 3 * MAX_COLOR_COUNT_OF_ONE_COMPONENT;
	int *histogram_u = text_object->p_ktext_mb_histogram + (ktext_index * 3 + 1)*MAX_COLOR_COUNT_OF_ONE_COMPONENT;
	int *histogram_v = text_object->p_ktext_mb_histogram + (ktext_index * 3 + 2)*MAX_COLOR_COUNT_OF_ONE_COMPONENT;
	histogramGenerate_char(p_ktext_mb_y, MB_HEIGHT, MB_WIDTH, histogram_y);
	histogramGenerate_char(p_ktext_mb_u, MB_HEIGHT, MB_WIDTH, histogram_u);
	histogramGenerate_char(p_ktext_mb_v, MB_HEIGHT, MB_WIDTH, histogram_v);

	text_object->t_cnt_rdo ++;

	int escape_color_size = 0;
	int num_symbol_group = 0;
	int num_o_quantized_index = 0;
	int num_blocks = text_object->t_cnt_rdo - 1;
	short group_symbols[256] = { 0 };
	short o_symbol_group[256] = { 0 };
	enum EstimateSymbol estimate_symbol[256] = { 0 };
	short *escape_color = &text_encoder_context->p_escape_color[num_blocks];
	struct TextQuantizePara *text_quantize = text_encoder_context->text_quan_para;

	quantizeTextMB(num_blocks, text_object->p_ktext_mb, text_object->p_ktext_mb_histogram,
		&(text_encoder_context->quantize_factor), text_quantize, escape_color, &escape_color_size);

	unsigned char *original_mb = text_object->p_ktext_mb + num_blocks*MB_PIXEL_COUNT;
	unsigned char *quantized_mb = text_quantize->quantized_image + num_blocks*MB_PIXEL_COUNT;

	calculateMbSSE(original_mb, quantized_mb, &t_sse);

	QuantizedIndexPredictMB(num_blocks, estimate_symbol,
		text_quantize, &num_o_quantized_index, o_symbol_group);

	//predict group symbols
	hierarchyPatternCodeMB(estimate_symbol, num_blocks, group_symbols, &num_symbol_group);

	//rdo for text block
	t_bits = coeff[0] * escape_color_size + coeff[1] * escape_color_size*escape_color_size
		+ coeff[2] * num_symbol_group + coeff[3] * num_symbol_group*num_symbol_group
		+ coeff[4] * num_o_quantized_index + coeff[5] * num_o_quantized_index*num_o_quantized_index;
	t_cost = 0 + lamda * t_bits;
	/*FILE *text_info = NULL;
	fopen_s(&text_info, "./text_info.txt", "a+");
	fprintf(text_info, "escape_color_size: %d, num_symbol_group: %d, num_o_quantized_index: %d\n", escape_color_size, num_symbol_group, num_o_quantized_index);
	fclose(text_info);*/
	return t_cost;
}
