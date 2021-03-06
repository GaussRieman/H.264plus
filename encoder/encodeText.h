//
// Created by yanz on 2017/9/12.
//

#ifndef X264_0_148_R2795_1_ENCODETEXT_H
#define X264_0_148_R2795_1_ENCODETEXT_H

#include "huffman.h"

#define GLOBAL_COLOR_FORMAT                            kYUV444

#define BASE_COLOR_NUMBER                               4
#define BASE_COLOR_NUMBER2                              2
#define PRESET_DELTA1                                   4
#define PRESET_DELTA2                                   16

// Recognize Constants Constant.h
#define MB_WIDTH                                        16
#define MB_WIDTH2                                       8
#define MB_HEIGHT                                       16
#define MB_HEIGHT2                                      8
#define BIT_SHIFT_MB_WIDTH                              4
#define BIT_SHIFT_MB_HEIGHT                             4
#define MB_PIXEL_COUNT                                  (MB_WIDTH*MB_HEIGHT)
#define MB_PIXEL_COUNT2                                 (MB_WIDTH2*MB_HEIGHT2)
#define BLOCK_WIDTH                                     8
#define BLOCK_HEIGHT                                    8
#define BLOCK_PIXEL_COUNT                               64

//运行参数
#define PIXEL_GROUP_COUNT                               4  //分层编码中作为一组的图像数目
#define QUANTIZE_BASE_COLOR_DELTA1                      4 //基本色量化参数delta1
#define QUANTIZE_ESCAPE_COLOR_DELTA2                    16 //逃逸色量化参数delta2
#define ESTIMATE_SYMBOL_NUMBER                          3  //L U O
#define ESTIMATE_SYMBOL_LINE_PATTERN_OFFSET             81

#define ESCAPE_COLOR_INDEX                              BASE_COLOR_NUMBER  //量化索引图中逃逸色的索引编号
#define ESCAPE_COLOR_INDEX2                             BASE_COLOR_NUMBER2  //量化索引图中逃逸色的索引编号
#define COLOR_RANGE                                     256  //输入图像单一分量像素值取值范围
#define SAME_WITH_REF_PIXEL_FILL_VALUE                  COLOR_RANGE  //当像素与ref相同时填充值
#define O_SYMBOL_HUFF_LEAF_LENGTH                       7  //O-symbol huffman码表叶子长度
#define O_SYMBOL_HUFF_SYMBOL_NUMBER                     125  //O-symbol huffman码表符号个数
#define GROUP_SYMBOL_HUFF_LEAF_LENGTH                   8  //group symbol huffman码表叶子长度
#define GROUP_SYMBOL_HUFF_SYMBOL_NUMBER                 84   //group symbol huffman码表符号个数

#define XOR_STRIDE                                      (MB_WIDTH*MB_HEIGHT*3)
#define XOR_STRIDE2                                     ((MB_WIDTH*MB_HEIGHT) + (MB_WIDTH2*MB_HEIGHT2*2))
#define XOR_COMP_STRIDE                                 (MB_WIDTH*MB_HEIGHT)
#define MAX_COLOR_COUNT_OF_ONE_COMPONENT                256  //一个分量的最大可能性
#define NGH_HEIGHT                                      (MB_HEIGHT+2)
#define NGH_LINE                                        (MB_WIDTH+2)
#define NGH_STRIDE                                      ((MB_WIDTH+2)*(MB_HEIGHT+2))
#define TEXT_MB_HISTOGRAM_COMP_STRIDE                   256
#define TEXT_MB_HISTOGRAM_BLOCK_STRIDE                  3*TEXT_MB_HISTOGRAM_COMP_STRIDE

//文字标准模式专用参数
#define TEXT_NORMAL_BASE_COLOR_CODE_LENGTH              8 //文字模式基本色单一分量编码长度
#define TEXT_NORMAL_BASE_COLOR_RANGE                    256  //文字标准模式基本色单一分量取值范围 0-255
#define TEXT_NORMAL_BASE_COLOR_HUFF_LEAF_LENGTH         8   //文字标准模式基本色huffman码表叶子长度
#define TEXT_NORMAL_ESCAPE_COLOR_HUFF_LEAF_LENGTH       4   //文字标准模式逃逸色huffman码表叶子长度
#define TEXT_NORMAL_ESCAPE_COLOR_HUFF_SYMBOL_NUMBER     16 //文字标准模式逃逸色huffman码表符号个数

//文字预测和文字编码编码符号最多用257和256个，在统一分配内存时按照257个符号申请空间
#define HUFFMAN_CODE_SYMBOL_NUM 257

#define COMPARE_MAX(a,b) a>b ? a:b
#define cube_value(val) (val*val*val)
#define square_value(val) (val*val)
#define NULL_POINT_CHECK(p) \
if(p == NULL) \
{ \
printf("Allocated memory fail!!"); \
return; \
} \

typedef enum ColorSpaceFormat {
	kYUV420p, /*I420*/
	kYUV444,
} ColorSpaceFormat;

typedef struct ImageMacroBlock {
    pixel data_component1[MB_PIXEL_COUNT];
    pixel data_component2[MB_PIXEL_COUNT];
    pixel data_component3[MB_PIXEL_COUNT];
} ImageMacroBlock;

typedef struct ResidualBlock {
    unsigned int length;
    int mb_count; //宏块的个数
    struct MbInfo *p_mb_info; //宏块的信息指针
    short *data_component1;
    short *data_component2;
    short *data_component3;
} ResidualBlock;


struct MbNeighbourIndex {
    int up_mb_index;
    int left_mb_index;
};

struct MbPositionInfo {
    int mb_x;
    int mb_y;
};

struct MbSamePixel {
    int same_y_pixel_num;
    int same_u_pixel_num;
    int same_v_pixel_num;
    int dummy;
};

struct MbInfo {
    struct MbNeighbourIndex mb_neighbour_index;
    //临近宏块的索引信息指针，如果不是同类型的宏块，它的索引为-1，
    struct MbPositionInfo mb_position;
    //kText宏块的位置信息数组指针
};

//宏块复杂度信息，记录高梯度像素个数和主要基本颜色个数
struct MbComplex{
    int nhg_block_count;    //块内的高梯度像素个数
    int nbc_block_count;    //块内的基本色像素个数
};

//kText模式编码的宏块集合的编码对象
struct TextEncodeObject {
	int t_cnt_rdo;  //text count for rdo only
    int ktext_mb_count; //kText宏块的个数
    //p_ktext_mb的申请的总个数为ktext_mb_count个
    pixel *p_ktext_mb; //kText宏块图像，每个宏块光栅扫描顺序
    int *p_ktext_mb_histogram; //kText宏块的0~255像素值的直方图大小，三个分量每个256个元素
};

struct BaseColor444{
    short component1[BASE_COLOR_NUMBER];
    short component2[BASE_COLOR_NUMBER];
    short component3[BASE_COLOR_NUMBER];
};

struct BaseColor420{
    short component1[BASE_COLOR_NUMBER];
    short component2[BASE_COLOR_NUMBER2];
    short component3[BASE_COLOR_NUMBER2];
};

/**
 * 定义文字编码器量化参数
 *
 * delta1  基本色量化参数
 * delta2  逃逸色量化参数
 */
struct QuantizeFactor {
    int delta1;
    int delta2;
};

/**
 * Index of quantized image
 */
struct QuantizeIndex {
    int component1;
    int component2;
    int component3;
};

/**
 * 2D-estimate symbol  L/U/O
if //SMAIN_PIXEL_G */
enum EstimateSymbol {
    Left, Up, Other
};



struct TextQuantizePara{
    //struct Pixel *quantized_image;  //量化后的图像
    pixel *quantized_image;
    struct QuantizeIndex *quantized_index_map;   //量化后的index索引图
    struct QuantizeIndex *o_quantized_index_map; //符号o的index索引图
    //struct BaseColor *base_color;   //base color
    enum ColorSpaceFormat color_space_formate;   //彩色空间
    union{
        struct BaseColor444 *base_color444;
        struct BaseColor420 *base_color420;
    }bc_type;
    //文字预测3 Basecolor
    short *Tree_base_color;//YUV共9个基本色

};

struct TextEncoderContext {
    //量化、预测后的变量
    struct TextQuantizePara *text_quan_para;
    //哈夫曼编码时的变量
    struct HuffmanEncodeContext *huffman_encode_context;
    //作为中间变量的码流
    short *p_escape_color;         //量化的escape color
    int escape_color_num;
    short *p_group_symbols;      //每4个符号一个组
    int group_symbols_num;       //符号组的个数
    short *p_o_symbol_group;       //o symbol的三个分量组成的符号
    enum EstimateSymbol *estimate_symbols;   //预测的符号L、U、O
    //量化参数
    struct QuantizeFactor quantize_factor;
    struct TextEncodeObject *p_ktext_object;

};

double analyseTextCost(unsigned char *p_text_mb_y, unsigned char *p_text_mb_u, unsigned char *p_text_mb_v, int lamda, struct TextEncoderContext *text_encoder_context);

void releaseMemoryPreEncode(struct TextQuantizePara *text_quantize);

void initialTextEncoderContext(const int max_block_num, struct TextEncoderContext **text_encoder_context);

void releaseTextEncoderContext(struct TextEncoderContext *text_encoder_context);

void allocateMemoryForTextObject(int text_mb_count, struct TextEncodeObject *p_text_object);

int calcEscapeColor(struct TextEncoderContext *const text_encoder_context, unsigned char *p_text_mb_y, unsigned char *p_text_mb_u, unsigned char *p_text_mb_v, unsigned int stride);

int preProcessTextMB(struct TextEncoderContext *text_encoder_context, pixel **p_text_mb_y,pixel **p_text_mb_u,
                     pixel **p_text_mb_v);

int encodeText(bs_t *bitstream,struct TextEncoderContext *const text_encoder_context);

void codeTextBitStream(const unsigned int num_blocks,
                       struct TextQuantizePara *const text_quantize,
                       struct HuffmanEncodeContext *const text_huff_context,
                       const struct QuantizeFactor *quantize_factor,
                       short *const escape_color,
                       int escape_color_num,
                       short *const group_symbols,
                       int group_symbols_num,
                       short *const o_symbol_group,
                       int o_symbols_num,
                       bs_t *bitstream);

void hierarchyPatternCode(const enum EstimateSymbol *const estimate_symbols,
                          const int num_blocks, short *const group_symbols,
                          int *group_symbols_size);

void QuantizedIndexPredict(const unsigned int num_blocks,
                           enum EstimateSymbol *const estimated_symbols,
                           struct TextQuantizePara *const text_quantize,
                           int *num_o_quantized_index,
                           short *const o_symbol_group);

void  textQuantize444(const unsigned int num_blocks, const pixel *image_data, const int *image_histogram,
                      const struct QuantizeFactor *quantize_factor, struct TextQuantizePara *const text_quantize,
                      short *const escape_color, int *escape_color_num, ResidualBlock *residual_block, int residual_flag);

void  textQuantize420(const unsigned int num_blocks, const pixel *image_data, const int *image_histogram,
                      const struct QuantizeFactor *quantize_factor, struct TextQuantizePara *const text_quantize,
                      short *const escape_color, int *escape_color_num, ResidualBlock *residual_block, int residual_flag);

void allocateMemoryForTextObject(int text_mb_count, struct TextEncodeObject *p_text_object);

int generateKTextMBData(int mb_x, int mb_y, int ktext_index, struct Frame *cur, struct TextEncodeObject *p_object);

void generateTextData(struct TextEncoderContext *const text_encoder_context, struct RecognizeContext *context, struct Frame *cur, struct TextEncodeObject *p_object);

void generateWholeTextData(struct RecognizeContext *context, struct Frame *cur, struct TextEncodeObject *p_object);

void allocMemoryPreEncode(int block_num, struct TextQuantizePara *text_quantize_para);

#define DEF_COLOR_FORMAT_LOCAL_VAR(stride1) \
    int local_data1_stride = stride1; \
    int local_data2_stride; \
    int local_data3_stride; \
    int local_data1_MB_WIDTH = MB_WIDTH; \
    int local_data1_MB_HEIGHT = MB_HEIGHT; \
    int local_data2_MB_WIDTH; \
    int local_data2_MB_HEIGHT; \
    int local_data3_MB_WIDTH; \
    int local_data3_MB_HEIGHT; \
    int local_data1_MB_PIXEL_COUNT; \
    int local_data2_MB_PIXEL_COUNT; \
    int local_data3_MB_PIXEL_COUNT; \
    int local_XOR_STRIDE; \
    if (kYUV420p == GLOBAL_COLOR_FORMAT) { \
        local_data2_stride = ((local_data1_stride + 1) / 2); \
        local_data3_stride = local_data2_stride; \
        local_data2_MB_WIDTH = MB_WIDTH2; \
        local_data2_MB_HEIGHT = MB_HEIGHT2; \
        local_data3_MB_WIDTH = MB_WIDTH2; \
        local_data3_MB_HEIGHT = MB_HEIGHT2; \
    } else { \
        local_data2_stride = local_data1_stride; \
        local_data3_stride = local_data1_stride; \
        local_data2_MB_WIDTH = MB_WIDTH; \
        local_data2_MB_HEIGHT = MB_HEIGHT; \
        local_data3_MB_WIDTH = MB_WIDTH; \
        local_data3_MB_HEIGHT = MB_HEIGHT; \
    } \
    local_data1_MB_PIXEL_COUNT = (local_data1_MB_WIDTH * local_data1_MB_HEIGHT); \
    local_data2_MB_PIXEL_COUNT = (local_data2_MB_WIDTH * local_data2_MB_HEIGHT); \
    local_data3_MB_PIXEL_COUNT = (local_data3_MB_WIDTH * local_data3_MB_HEIGHT); \
    local_XOR_STRIDE = (local_data1_MB_PIXEL_COUNT +  local_data2_MB_PIXEL_COUNT + local_data3_MB_PIXEL_COUNT); \

//宏块索引映射出起始数据，与DEF_COLOR_FORMAT_LOCAL_VAR配套使用
#define MAP_MB_DATA(num_,data_component_,mb_y_,mb_x_) \
    (data_component_ + mb_y_ * local_data##num_##_MB_HEIGHT * local_data##num_##_stride + mb_x_ * local_data##num_##_MB_WIDTH)

#define IS_YUV420P (kYUV420p == GLOBAL_COLOR_FORMAT)
/*
enum MBType {
	DEFAULT,
	LBC,
	UBC,
	MVMATCHED,
	TEXT,
};
*/

struct Frame {
	// 通用属性
	int c_frame_id;   //采集的原始帧号
	int frame_id;
	int64_t pts;

	// 视频参数
	enum ColorSpaceFormat frame_format;
	int width;
	int height;
	int stride;
	int stride2;
	int offset; //图像指针相对于延拓后图像的偏移量
	int offset2;
	int frame_rate; //每秒的帧率

					// 音频参数
	int sample_rate;
	int sample_fmt;
	int channel;

	// 数据
	int component_memory_size; //单分量延拓后的内存数据大小
	pixel *data_component1;  //分量1指针
	pixel *data_component2;  //分量2指针
	pixel *data_component3;  //分量3指针
};
#endif //X264_0_148_R2795_1_ENCODETEXT_H
