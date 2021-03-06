//
// Created by xiangyy on 2017/9/2.
//

#ifndef VPIXEL_CORE_HRECOGNIZE_H
#define VPIXEL_CORE_HRECOGNIZE_H

# ifdef __cplusplus
extern "C" {
# endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>

// Recognize Constants Constant.h
#define MB_WIDTH                                                                                      16
#define MB_WIDTH2                                                                                      8
#define MB_HEIGHT                                                                                     16
#define MB_HEIGHT2                                                                                     8
#define BIT_SHIFT_MB_WIDTH                                                                            4
#define BIT_SHIFT_MB_HEIGHT                                                                           4
#define MB_PIXEL_COUNT                                                                              (MB_WIDTH*MB_HEIGHT)
#define MB_PIXEL_COUNT2                                                                             (MB_WIDTH2*MB_HEIGHT2)
#define BLOCK_WIDTH                                                                                    8
#define BLOCK_HEIGHT                                                                                   8
#define BLOCK_PIXEL_COUNT                                                                             64

//Tile的Padding宽度
#define IMAGE_PADDING_SIZE                                                                            16
#define IMAGE_PADDING_SIZE2                                                                           8
//宏块模式的Padding宽度
#define MB_MODE_PADDING_WIDTH                                                                          1

//color_space.h
#define GLOBAL_COLOR_FORMAT                                                                        kYUV444
#define IS_YUV420P (kYUV420p == GLOBAL_COLOR_FORMAT)

//Frame.h
#define MAX_FRAME_ID                                                                                  1023  //65535
#define MAX_FRAME_WIDTH                                                                               1920
#define MAX_FRAME_HEIGHT                                                                              1200
#define MAX_FRAME_MB_COUNT (((MAX_FRAME_WIDTH-1)>>BIT_SHIFT_MB_WIDTH+1)*((MAX_FRAME_HEIGHT-1)>>BIT_SHIFT_MB_HEIGHT+1))

//EncodeObject.h
#define XOR_STRIDE                                                                                  (MB_WIDTH*MB_HEIGHT*3)
#define XOR_STRIDE2                                                                                 ((MB_WIDTH*MB_HEIGHT) + (MB_WIDTH2*MB_HEIGHT2*2))
#define XOR_COMP_STRIDE                                                                             (MB_WIDTH*MB_HEIGHT)
#define MAX_COLOR_COUNT_OF_ONE_COMPONENT                                                                256  //一个分量的最大可能性
#define NGH_HEIGHT                                                                                  (MB_HEIGHT+2)
#define NGH_LINE                                                                                    (MB_WIDTH+2)
#define NGH_STRIDE                                                                                  ((MB_WIDTH+2)*(MB_HEIGHT+2))
#define TEXT_MB_HISTOGRAM_COMP_STRIDE                                                               256
#define TEXT_MB_HISTOGRAM_BLOCK_STRIDE                                                              3*TEXT_MB_HISTOGRAM_COMP_STRIDE

//Recognize.h
#define HASH_TABLE_SIZE                                                                             256256  // 平均值最大为255，8步长异或最大255，尺寸256256，兼容crc16模式的65535
//特征值的条目，个数为MAX_FEATURE_NUMBER_IN_TILE，为了快速计算，第0个不用
#define MAX_FEATURE_NUMBER_IN_TILE                                                                   8192   //每帧中特征的最大个数
#define FEATURE_LENGTH                                                                               32   //特征的长度，32个像素的Y分量
#define DATA1_STRIDE                                                                                1952

//RecognizeCommon.h
#define MOTIONLESS_MB_SAME_PIXEL_COUNT                                                               691//(90%)
#define I_INTERVAL                                                                                    30
#define MIN_REORDER_MB_COUNT                                                                          25
#define MIN_WAVELET_MB_COUNT                                                                         256

//高梯度值的门限
#define NGH_DIFF                                                                                      18
//如果一种颜色与主颜色值的差小于该值，认为这种颜色也是主颜色
#define NBC_DIFF                                                                                       4
#define NGH_TH                                                                                         1
#define NBC_TH0                                                                                      179
#define NBC_TH1                                                                                      160//原来是179

//Y分量的比特数
#define Y_BITS                                                				                           8
//出现频次不大于该值的mv，不做与最高频次mv的占比计算
#define GLOBAL_MIX_MV_THRESHOLD_TO_MATCHED_FEATURES                                                    5
//出现最高频次的mv占比超过该值，认为该mv是全局mv
#define GLOBAL_MV_RATIO_THRESHOLD_TO_MATCHED_FEATURES                                               0.70
//出现最高频次的mv的频次超过该值乘以宏块总个数，认为该mv是全局mv
//#define GLOBAL_MV_RATIO_THRESHOLD_TO_ALL_FEATURES                                                   0.10
//出现最高频次的匹配特征数量超过该值，认为该mv是全局mv
#define GLOBAL_MV_THRESHOLD_TO_MATCHED_FEATURES_NUM                                                   50
//如果unchanged宏块和predict宏块数量之和大于整Frame宏块的95%，认为是MOTIONLESS模式的Tile，不需要找MV
#define MOTIONLESS_RATIO_THRESHOLD                                                                  0.95
//如果Inter帧中kMvMatched、kUnchanged这两种模式所占的比例很低，
//就可以判定发生了场景切换(Scene cut)
#define SCENE_CUT_RATIO_THRESHOLD                                                                   0.05
//使用CRC函数
#define USE_CRC_FUNCTION                                                                               0
//使用梯度计算的核心函数
#define USE_NHG_CORE_FUNCTION                                                                          0
//记录xorMBPixel函数的计算结果
#define RECORD_XOR_FUNCTION_RESULT                                                                     1

#define NOISE_SAME(a,b,c) ((abs((a)-(b)))<=(c))
#define THRESHOLD_NONOISE 0.001  //差值为1的点占比不超过0.1%认为无噪
#define SAME_PIXEL_THRESHOLD																			1	//相同像素百分比阈值


//简化计算用局部变量 core_global_config.h
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
#define safe_free(p) do{if(p!=NULL){free(p);p=NULL;}}while(0)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
unsigned char clip_sh_0_255(int in);
/* clip to [0..255] */
short clip_short_0_255(short n1);
int clip_int_0_255(int n1);


enum MBPosition
{
    kUndefined,			///< Unknown or undefined MB position
    kUpLeft, 		    ///< the MB with location in the Up Left
    kUp, 				///< the MB with location in the Up
    kUpRight,			///< the MB with location in the Up Right
    kLeft          		///< the MB with location in the Left
};

//记录匹配特征值信息的条目，个数为MAX_FEATURE_NUMBER_IN_TILE，为了快速计算，第0个不用
typedef struct MatchedFeatureItem_
{
    int mv_x;
    int mv_y;
}MatchedFeatureItem;

//定义一个Tile中存在的所有的匹配特征的存放表(只需要一份，相当于临时变量)
typedef struct MatchedFeatureTable_
{
    MatchedFeatureItem matched_feature_item[MAX_FEATURE_NUMBER_IN_TILE];
}MatchedFeatureTable;

#define MAX_FOUND_MV_NUMBER_IN_TILE   MAX_FEATURE_NUMBER_IN_TILE   //每个Tile中找到的MV的最大个数
//记录找到的MV信息的条目，个数MAX_FOUND_MV_NUMBER_IN_TILE，为了快速计算，第0个不用
typedef struct FoundMvItem_
{
    int mv_x;
    int mv_y;
    int count;
    int dummy;
}FoundMvItem;

//定义一个Tile中存在的所有的匹配特征的存放表(只需要一份，相当于临时变量)
typedef struct FoundMvTable_
{
    FoundMvItem found_mv_item[MAX_FOUND_MV_NUMBER_IN_TILE];
}FoundMvTable;
/*
typedef enum ColorSpaceFormat {
    kYUV420p, 
    kYUV444,
    kBGRA,
    kNV12
} ColorSpaceFormat;
*/
//Pixel.h
typedef struct YUVPixel {
    int Y;
    int U;
    int V;
} YUVPixel;

typedef struct RGBPixel {
    unsigned char R;
    unsigned char G;
    unsigned char B;
} RGBPixel;

typedef struct Pixel {
    int component1;
    int component2;
    int component3;
} Pixel;

//EncodeObject.h
//全局mv的信息
struct GlobalMvMeta {
    int x;
    int y;
    int valid_flag; //如果是1, 该mv是有效地全局运动向量
};

//Tile.h
typedef struct Tile {
    int frame_id;
    int tile_id;
    //enum TileMode mode; //处理的模式
    int height; //有效像素高度
    int width;  //有效像素宽度
    int pos_x; //在Frame中的x位置。
    int pos_y; //在Frame中的y位置。
    int stride;
    int offset; //图像指针相对于延拓后图像的偏移量
    int component_memory_size; //单分量延拓后的内存数据大小
    unsigned char *data_component1;  //分量1指针
    unsigned char *data_component2;  //分量2指针
    unsigned char *data_component3;  //分量3指针
} Tile;

/***
 * 每个Tile的尺寸和位置
 */
struct TileSizeAndPosition {
    int tile_id; //tile的ID，从0开始
    int width;
    //每个Tile的宽度
    int height;
    //每个Tile的高度
    int pos_x;
    //每个Tile在Frame中的x位置
    int pos_y;
    //每个Tile在Frame中的y位置
    int stride; //整帧的宽度
    int tile_in_row; //一帧的宽度包含有多少个Tile
    int tile_in_column; //一帧的高度包含有多少个Tile
    int frame_width;
    int frame_height;
    int tile_width; //配置的Tile的宽度,右方的Tile有可能实际宽度width不是tile_width
    int tile_height; //配置的Tile的宽度,下方的Tile有可能实际高度width不是tile_height
    int mb_count_in_tile_width; //配置的Tile的宽度包含多少个宏块
    int mb_count_in_tile_height; //配置的Tile的宽度包含多少个宏块
    int tile_index_x; //在x方向该Tile是第几个
    int tile_index_y; //在y方向该Tile是第几个
};
/*
//Frame.h
struct Frame {
    // 通用属性
    int c_frame_id;   //采集的原始帧号
    int frame_id;
    int64_t pts;
    enum ColorSpaceFormat frame_format;
    int width;
    int height;
    int stride;
    int stride2;
    int offset; //图像指针相对于延拓后图像的偏移量
    int offset2;
    int frame_rate; //每秒的帧率

    // 数据
    int component_memory_size; //单分量延拓后的内存数据大小
    unsigned char *data_component1;  //分量1指针
    unsigned char *data_component2;  //分量2指针
    unsigned char *data_component3;  //分量3指针
};
*/

//Recognize.h
//Hash值对应的FeatureItem数组中得索引，它的个数为HASH_TABLE_SIZE个
// 考虑到有时会发生Hash冲突的情况，设置最大为4个Hash值冲突的特征出现，
// 如果大于4个，就忽略该特征，实验表明4个就可以
typedef struct FeatureIndexItem_ {
    unsigned short feature_index;
} FeatureIndexItem;

//定义一个Tile中存在的所有的特征的对应的索引表，便于特征值快速匹配
typedef struct FeatureIndexTable_ {
    FeatureIndexItem feature_index_item[HASH_TABLE_SIZE];
} FeatureIndexTable;

typedef struct FeatureItem_ {
    unsigned char feature[FEATURE_LENGTH];
    int x;
    int y;
    unsigned short hash_value;
    unsigned short collision_flag; //发生HASH碰撞的标记，如果为1，表明该特征与本帧的某个特征发生了碰撞，特征匹配时略去该特征
    unsigned char padding[20];
} FeatureItem;

//定义一个Tile中存在的所有的特征的存放表
typedef struct FeatureTable_ {
    FeatureItem feature_item[MAX_FEATURE_NUMBER_IN_TILE];
} FeatureTable;

typedef struct FeatureMeta_ {
    FeatureTable feature_table;
    FeatureIndexTable index_table;
} FeatureMeta;

/**
 * 定义像素值对比时，差值在指定范围内抖动认为相等
 */
enum RecognizeNoiseThreshold {
    kNoNoiseThreshold, // 像素值完全按照原值对比
    kOneBitThreshold,  // 像素值差值小于等于1时认为相同
    kTwoBitThreshold,  // 像素值差值小于等于2时认为相同
    kAutoDetermined,
};

/**
 * 定义识别模块的多线程模式
 */
enum RecognizePthreadMode {
    kSingleThreadMode = 0, //0，单线程完成
    kMultiThreadMode, //1，每个Tile的编码对象由一个独立的线程完成
};

enum MBType{
    DEFAULT,
    LBC,
    UBC,
    MVMATCHED,
    TEXT,
};

//定义识别的Context结构
struct RecognizeContext {
    int frame_width; //帧的宽度
    int frame_height; //帧的高度
    int tile_width; //Tile的宽度
    int tile_height; //Tile的高度
    int tile_count;  //Tile的数量
    struct TileSizeAndPosition *tile_info_table; //存放Tile的划分信息

    struct Frame *current_frame; //当前Frame的指针
    struct Frame *refer_frame; //参考Frame的指针
    struct Tile *tile_array_of_current_frame; //当前Frame划分的Tile数组

    ///与参考帧比较，各类宏块的数量
    int mb_in_row; //帧的一行包含的宏块数目
    int mb_in_column; //帧的一列包含的宏块数目
    int mb_count; //一帧的包含的宏块数目
    int mb_stride; //帧的一行的宏块跨度
    int mb_offset; //第一个有效宏块相对于起始地址的偏移，释放内存使用

    //做特征匹配需要的变量，根据当参考帧的概念，其中的特征索引表只需要两帧就可以
    int frame_id_of_one_frame; //其中一个帧的帧ID
    FeatureMeta *feature_meta_in_one_frame; //其中一个帧的特征索引表
    int frame_id_of_another_frame; //另一个帧的帧ID
    FeatureMeta *feature_meta_in_another_frame; //另一个帧的特征索引表
    struct GlobalMvMeta global_mv;

    //MBtype
    enum MBType *mbtype;
    int mvmatched_count;
    int text_count;
    int lbc_count;
    int ubc_count;

    // 像素值抖动屏蔽
    enum RecognizeNoiseThreshold noise_threshold; // 由外部指定的屏蔽像素值抖动范围

    FeatureMeta * feature_meta;
};

/**
 * 初始化识别模块的上下文
 *
 * 参数
 *     frame_width - INPUT，一个Frame的宽度
 *     frame_height - INPUT，一个Frame的高度
 *     tile_width - INPUT, 一个Tile的宽度
 *     tile_height - INPUT，一个Tile的高度
 *     strategy - INPUT，识别模块的对于视频的识别策略
 *     thread_mode - INPUT, 识别模块的线程执行方式
 *     noise_threshold - INPUT, 由外部指定的屏蔽像素值抖动范围
 *     context - INPUT/OUTPUT，识别模块的上下文，在内部分配
 *
 * 返回：
 *     0  ---初始化上下文成功
 *     -1 ---初始化上下文失败
 *     其他 ---其他错误码
 */
int initialRecognizeContext(const int frame_width, const int frame_height, \
                            const int tile_width, const int tile_height, \
                            enum RecognizeNoiseThreshold noise_threshold, \
                            struct RecognizeContext **context);

/**
 * 释放识别模块的上下文
 *
 * 参数
 *     context - INPUT/OUTPUT，识别模块的上下文
 *
 * 返回：
 *     无
 */
void freeRecognizeContext(struct RecognizeContext *context);


/**
 * 检测当前帧相对于参考帧的运动信息
 *
 * 参数
 *     context - INPUT/OUTPUT，识别模块的上下文，在内部计算时修改
 *
 * 返回：
 *     0  ---检测运动成功
 *     -1 ---检测运动失败
 *     其他 ---其他错误码
 */

/*
 * 从YUV文件中读取特定的一帧内容到frame中，其中frame的大小已经指定好，
 * 内存已经提前申请，内存大小是按照存放Padding后的图像
 *
 * 参数
 *     filename - 记录文件
 *     frame - 帧的指针
 *     frame_no - 读取的帧号
 *
 * 返回：
 *     无
 */
int readFrameFromYUVFile(const char *filename, struct Frame *frame, int frame_no);

/*
 * 设置Frame的数据结构并分配内存
 *
 * 参数
 *     frame_width - 帧的宽度
 *     frame_height - 帧的高度
 *     frame_id - 帧的FRAME_ID
 *     pp_frame - 帧的指针，在内部分配
 *
 * 返回：
 *     无
 */
void allocAndSetFrame(int frame_width, int frame_height, int frame_id, struct Frame **pp_frame);

/*
 * 记录Frame到YUV文件中
 *
 * 参数
 *     filename - 记录文件
 *     frame - 帧的指针
 *
 * 返回：
 *     无
 */
void recordFrameIntoYUVFile(char *filename, struct Frame *frame);


/***
 * 释放Frame的内存数据
 */
void freeFrame(struct Frame *frame, bool need_free);

/*
 * 在当前帧和参考帧中查找特征信息，并且进行全局MV的查找
 *
 * 参数
 *     context - INPUT/OUTPUT，识别模块的上下文，计算时修改其相关内容
 *
 * 返回：
 *     0  ---初始化成功
 *     -1 ---初始化失败
 */
int findFeatureInfoAndGlobalMv(struct RecognizeContext* const context);

/*
 * 在存在全局MV的情况下，重新标记宏块的模式，增加kMvMatched模式
 *
 * 参数
 *     context - INPUT/OUTPUT，识别模块的上下文，在内部计算时修改其相关内容
 *
 * 返回：
 *     0  ---初始化成功
 *     -1 ---初始化失败
 */
int markMvMatchedMb(struct RecognizeContext *const context);

/**
 * 检测当前帧相对于参考帧的运动信息
 *
 * 参数
 *     context - INPUT/OUTPUT，识别模块的上下文，在内部计算时修改
 *
 * 返回：
 *     0  ---检测运动成功
 *     -1 ---检测运动失败
 *     其他 ---其他错误码
 */
int generateMotionDetect(struct Frame * current_frame, struct Frame *refer_frame, struct RecognizeContext *const context);

/*
 * 检测一帧图像中的IBC宏块
 * 参数
 *     cur_frm - INPUT，帧
 *     context - INPUT，识别上下文
 */
void detectIntraCopyBlock(struct Frame *cur_frm, struct RecognizeContext *const context);

/*
 * 检测一帧图像中的文字宏块
 * 参数
 *     cur_frm - INPUT，帧
 *     context - INPUT，识别上下文
 * 仅hvgctest测试时使用
 */
void detectTextMB(struct Frame *cur_frm, struct RecognizeContext *const context);

# ifdef __cplusplus
}
# endif

#endif //VPIXEL_CORE_HRECOGNIZE_H